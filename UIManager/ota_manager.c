/**
 * @file ota_manager.c
 * @brief OTA自动更新状态机 —— 非阻塞HTTP版本检查 + 流式下载
 *
 * 两阶段流程:
 *   阶段1: GET /ota/version → 解析 VERSION\nMD5\nSIZE → 比较本地版本
 *   阶段2: GET /ota/lvgl_fb → 流式写入 /wz/lvgl_fb.new → MD5校验 → rename+exit
 *
 * 下载阶段不缓存完整响应，而是边接收边写文件。HTTP头通过 \r\n\r\n 识别后，
 * 剩余字节直接写入临时文件直到达到预期的 content-length。
 */

#include "ota_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

/* ==================== 配置 ==================== */

#define OTA_SERVER_HOST    "192.168.171.73"
#define OTA_SERVER_PORT    8080
#define OTA_VERSION_PATH   "/ota/version"
#define OTA_BINARY_PATH    "/ota/lvgl_fb"
#define OTA_LOCAL_VERSION  "/wz/version"
#define OTA_LOCAL_TEMP     "/wz/lvgl_fb.new"
#define OTA_LOCAL_BINARY   "/wz/lvgl_fb"
#define OTA_BUF_SIZE       8192       /* HTTP头接收缓冲 */
#define OTA_FILE_BUF_SIZE  65536      /* 下载写文件缓冲 */
#define OTA_CONNECT_TIMEOUT 5
#define OTA_RECV_TIMEOUT    30        /* 下载大文件需要更长超时 */

/* ==================== 状态机 ==================== */

enum {
    OTA_IDLE,
    OTA_CHECK_CONNECT,  /* 版本检查连接中 */
    OTA_CHECK_RECV,     /* 接收版本信息 */
    OTA_DL_CONNECT,     /* 下载连接中 */
    OTA_DL_RECV_HDR,    /* 接收下载HTTP头 */
    OTA_DL_RECV_BODY,   /* 流式接收文件体 */
    OTA_VERIFY,         /* MD5校验 */
    OTA_INSTALL         /* 安装并重启 */
};

static int   ota_state = OTA_IDLE;
static int   ota_sock = -1;
static int   ota_out_fd = -1;
static char  ota_hdr_buf[OTA_BUF_SIZE];
static char  ota_file_buf[OTA_FILE_BUF_SIZE];
static int   ota_hdr_total;
static time_t ota_deadline;
static char  ota_remote_ver[32];
static char  ota_remote_md5[64];
static off_t ota_expected_size;
static off_t ota_written;
static int   ota_content_len;       /* HTTP Content-Length */
static int   ota_hdr_done;           /* 头部接收完毕标志 */
static int   ota_percent;            /* 下载进度百分比 */

/* ==================== 简易MD5 (RFC 1321) ==================== */

typedef struct {
    uint32_t state[4]; uint32_t count[2]; uint8_t buffer[64];
} MD5_CTX;

static void md5_transform(uint32_t s[4], const uint8_t b[64]);

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x,y,z) ((x&y)|(~x&y))
#define G(x,y,z) ((x&z)|(y&~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y^(x|~z))
#define RL(x,n) ((x<<n)|(x>>(32-n)))
#define FF(a,b,c,d,x,s,ac) { a+=F(b,c,d)+x+(uint32_t)ac; a=RL(a,s); a+=b; }
#define GG(a,b,c,d,x,s,ac) { a+=G(b,c,d)+x+(uint32_t)ac; a=RL(a,s); a+=b; }
#define HH(a,b,c,d,x,s,ac) { a+=H(b,c,d)+x+(uint32_t)ac; a=RL(a,s); a+=b; }
#define II(a,b,c,d,x,s,ac) { a+=I(b,c,d)+x+(uint32_t)ac; a=RL(a,s); a+=b; }

static void md5_init(MD5_CTX *c) {
    c->count[0]=c->count[1]=0;
    c->state[0]=0x67452301;c->state[1]=0xefcdab89;
    c->state[2]=0x98badcfe;c->state[3]=0x10325476;
}

static void md5_update(MD5_CTX *c, const uint8_t *in, uint32_t len) {
    uint32_t i, idx = (c->count[0]>>3)&0x3F;
    c->count[0] += len<<3;
    if(c->count[0] < (len<<3)) c->count[1]++;
    c->count[1] += len>>29;
    uint32_t p = 64-idx;
    if(len>=p){ memcpy(&c->buffer[idx],in,p); md5_transform(c->state,c->buffer);
        for(i=p;i+63<len;i+=64) md5_transform(c->state,&in[i]); idx=0; }
    else i=0;
    memcpy(&c->buffer[idx],&in[i],len-i);
}

static void md5_final(uint8_t dg[16], MD5_CTX *c) {
    uint8_t bits[8]; uint32_t i, idx;
    for(i=0;i<8;i++) bits[i]=(uint8_t)(c->count[i>>2]>>((i&3)*8));
    idx = (c->count[0]>>3)&0x3f;
    uint8_t pad = idx<56 ? 56-idx : 120-idx;
    static const uint8_t PAD[64]={0x80};
    md5_update(c, PAD, pad); md5_update(c, bits, 8);
    for(i=0;i<4;i++){
        dg[i*4]=(uint8_t)(c->state[i]&0xff); dg[i*4+1]=(uint8_t)((c->state[i]>>8)&0xff);
        dg[i*4+2]=(uint8_t)((c->state[i]>>16)&0xff); dg[i*4+3]=(uint8_t)((c->state[i]>>24)&0xff);
    }
}
static void md5_transform(uint32_t s[4], const uint8_t b[64]) {
    uint32_t a=s[0],bb=s[1],c=s[2],d=s[3],x[16];
    for(int i=0;i<16;i++) x[i]=(uint32_t)b[i*4]|((uint32_t)b[i*4+1]<<8)|((uint32_t)b[i*4+2]<<16)|((uint32_t)b[i*4+3]<<24);
    FF(a,bb,c,d,x[0],S11,0xd76aa478);FF(d,a,bb,c,x[1],S12,0xe8c7b756);FF(c,d,a,bb,x[2],S13,0x242070db);FF(bb,c,d,a,x[3],S14,0xc1bdceee);
    FF(a,bb,c,d,x[4],S11,0xf57c0faf);FF(d,a,bb,c,x[5],S12,0x4787c62a);FF(c,d,a,bb,x[6],S13,0xa8304613);FF(bb,c,d,a,x[7],S14,0xfd469501);
    FF(a,bb,c,d,x[8],S11,0x698098d8);FF(d,a,bb,c,x[9],S12,0x8b44f7af);FF(c,d,a,bb,x[10],S13,0xffff5bb1);FF(bb,c,d,a,x[11],S14,0x895cd7be);
    FF(a,bb,c,d,x[12],S11,0x6b901122);FF(d,a,bb,c,x[13],S12,0xfd987193);FF(c,d,a,bb,x[14],S13,0xa679438e);FF(bb,c,d,a,x[15],S14,0x49b40821);
    GG(a,bb,c,d,x[1],S21,0xf61e2562);GG(d,a,bb,c,x[6],S22,0xc040b340);GG(c,d,a,bb,x[11],S23,0x265e5a51);GG(bb,c,d,a,x[0],S24,0xe9b6c7aa);
    GG(a,bb,c,d,x[5],S21,0xd62f105d);GG(d,a,bb,c,x[10],S22,0x02441453);GG(c,d,a,bb,x[15],S23,0xd8a1e681);GG(bb,c,d,a,x[4],S24,0xe7d3fbc8);
    GG(a,bb,c,d,x[9],S21,0x21e1cde6);GG(d,a,bb,c,x[14],S22,0xc33707d6);GG(c,d,a,bb,x[3],S23,0xf4d50d87);GG(bb,c,d,a,x[8],S24,0x455a14ed);
    GG(a,bb,c,d,x[13],S21,0xa9e3e905);GG(d,a,bb,c,x[2],S22,0xfcefa3f8);GG(c,d,a,bb,x[7],S23,0x676f02d9);GG(bb,c,d,a,x[12],S24,0x8d2a4c8a);
    HH(a,bb,c,d,x[5],S31,0xfffa3942);HH(d,a,bb,c,x[8],S32,0x8771f681);HH(c,d,a,bb,x[11],S33,0x6d9d6122);HH(bb,c,d,a,x[14],S34,0xfde5380c);
    HH(a,bb,c,d,x[1],S31,0xa4beea44);HH(d,a,bb,c,x[4],S32,0x4bdecfa9);HH(c,d,a,bb,x[7],S33,0xf6bb4b60);HH(bb,c,d,a,x[10],S34,0xbebfbc70);
    HH(a,bb,c,d,x[13],S31,0x289b7ec6);HH(d,a,bb,c,x[0],S32,0xeaa127fa);HH(c,d,a,bb,x[3],S33,0xd4ef3085);HH(bb,c,d,a,x[6],S34,0x04881d05);
    HH(a,bb,c,d,x[9],S31,0xd9d4d039);HH(d,a,bb,c,x[12],S32,0xe6db99e5);HH(c,d,a,bb,x[15],S33,0x1fa27cf8);HH(bb,c,d,a,x[2],S34,0xc4ac5665);
    II(a,bb,c,d,x[0],S41,0xf4292244);II(d,a,bb,c,x[7],S42,0x432aff97);II(c,d,a,bb,x[14],S43,0xab9423a7);II(bb,c,d,a,x[5],S44,0xfc93a039);
    II(a,bb,c,d,x[12],S41,0x655b59c3);II(d,a,bb,c,x[3],S42,0x8f0ccc92);II(c,d,a,bb,x[10],S43,0xffeff47d);II(bb,c,d,a,x[1],S44,0x85845dd1);
    II(a,bb,c,d,x[8],S41,0x6fa87e4f);II(d,a,bb,c,x[15],S42,0xfe2ce6e0);II(c,d,a,bb,x[6],S43,0xa3014314);II(bb,c,d,a,x[13],S44,0x4e0811a1);
    II(a,bb,c,d,x[4],S41,0xf7537e82);II(d,a,bb,c,x[11],S42,0xbd3af235);II(c,d,a,bb,x[2],S43,0x2ad7d2bb);II(bb,c,d,a,x[9],S44,0xeb86d391);
    s[0]+=a;s[1]+=bb;s[2]+=c;s[3]+=d;
}

static void md5_file(const char *path, char hex[33]) {
    uint8_t dg[16]; MD5_CTX ctx; md5_init(&ctx);
    int fd = open(path, O_RDONLY);
    if(fd<0){ hex[0]='\0'; return; }
    uint8_t b[4096]; ssize_t n;
    while((n=read(fd,b,sizeof(b)))>0) md5_update(&ctx,b,n);
    close(fd); md5_final(dg,&ctx);
    for(int i=0;i<16;i++) snprintf(hex+i*2,3,"%02x",dg[i]);
    hex[32]='\0';
}

/* ==================== 工具 ==================== */

static int set_nb(int fd) {
    int f = fcntl(fd, F_GETFL, 0);
    return f<0 ? -1 : fcntl(fd, F_SETFL, f | O_NONBLOCK);
}

static void ota_reset(void) {
    if(ota_sock>=0){ close(ota_sock); ota_sock=-1; }
    if(ota_out_fd>=0){ close(ota_out_fd); ota_out_fd=-1; }
    ota_state = OTA_IDLE; ota_hdr_total=0; ota_written=0; ota_hdr_done=0;
}

static int get_local_ver(char *v, size_t len) {
    FILE *f = fopen(OTA_LOCAL_VERSION, "r");
    if(!f){ v[0]='\0'; return -1; }
    if(!fgets(v,len,f)){ fclose(f); v[0]='\0'; return -1; }
    fclose(f); size_t l=strlen(v);
    while(l>0&&(v[l-1]=='\n'||v[l-1]=='\r')) v[--l]='\0';
    return 0;
}

/* 大小写不敏感的字符串搜索 */
static const char *stristr(const char *haystack, const char *needle) {
    size_t nlen = strlen(needle);
    for(; *haystack; haystack++) {
        size_t i;
        for(i=0; i<nlen; i++) {
            char h = haystack[i], n = needle[i];
            if(h>='A'&&h<='Z') h+='a'-'A';
            if(n>='A'&&n<='Z') n+='a'-'A';
            if(h!=n) break;
        }
        if(i==nlen) return haystack;
    }
    return NULL;
}

/* 从HTTP头解析Content-Length */
static int parse_cl(const char *hdr) {
    const char *p = stristr(hdr, "content-length:");
    if(!p) return -1;
    return atoi(p + 15);
}

/* 在缓冲中查找 \r\n\r\n，返回body起始位置，-1表示头部未完整 */
static int find_body(const char *buf, int len) {
    for(int i=0; i<=len-4; i++)
        if(buf[i]=='\r'&&buf[i+1]=='\n'&&buf[i+2]=='\r'&&buf[i+3]=='\n')
            return i+4;
    return -1;
}

/* ==================== 连接发起 ==================== */

static int ota_connect(void) {
    ota_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(ota_sock<0) return -1;
    set_nb(ota_sock);
    struct sockaddr_in a; memset(&a,0,sizeof(a));
    a.sin_family=AF_INET; a.sin_port=htons(OTA_SERVER_PORT);
    if(inet_aton(OTA_SERVER_HOST, &a.sin_addr)==0){ close(ota_sock);ota_sock=-1;return -1; }
    int r = connect(ota_sock, (struct sockaddr*)&a, sizeof(a));
    if(r==0) return 0;
    if(errno==EINPROGRESS) return 1;
    close(ota_sock); ota_sock=-1; return -1;
}

static void ota_send_get(const char *path) {
    char req[512];
    snprintf(req,sizeof(req),
        "GET %s HTTP/1.0\r\nHost: %s:%d\r\nConnection: close\r\n\r\n",
        path, OTA_SERVER_HOST, OTA_SERVER_PORT);
    printf("[OTA] GET %s\n", path);
    send(ota_sock, req, strlen(req), 0);
}

/* ==================== 公开API ==================== */

int ota_check_trigger(void) {
    if(ota_state != OTA_IDLE) return 0;
    int r = ota_connect();
    if(r<0){ printf("[OTA] check connect failed\n"); ota_reset(); return -1; }
    ota_deadline = time(NULL) + OTA_CONNECT_TIMEOUT;
    if(r==0){ ota_send_get(OTA_VERSION_PATH); ota_state=OTA_CHECK_RECV; ota_hdr_total=0;
              ota_deadline=time(NULL)+OTA_RECV_TIMEOUT; }
    else ota_state = OTA_CHECK_CONNECT;
    return 0;
}

void ota_poll(void) {
    if(ota_state==OTA_IDLE) return;
    struct pollfd pfd; int r;

    /* ---- 连接阶段 ---- */
    if(ota_state==OTA_CHECK_CONNECT || ota_state==OTA_DL_CONNECT) {
        if(time(NULL) > ota_deadline){ printf("[OTA] connect timeout\n"); ota_reset(); return; }
        pfd.fd=ota_sock; pfd.events=POLLOUT;
        if(poll(&pfd,1,0)<=0) return;
        int e=0; socklen_t l=sizeof(e);
        getsockopt(ota_sock,SOL_SOCKET,SO_ERROR,&e,&l);
        if(e!=0){ printf("[OTA] connect err: %s\n",strerror(e)); ota_reset(); return; }

        if(ota_state==OTA_CHECK_CONNECT) {
            ota_send_get(OTA_VERSION_PATH);
            ota_state=OTA_CHECK_RECV; ota_hdr_total=0;
            ota_deadline=time(NULL)+OTA_RECV_TIMEOUT;
        } else {
            ota_send_get(OTA_BINARY_PATH);
            ota_state=OTA_DL_RECV_HDR; ota_hdr_total=0; ota_hdr_done=0;
            ota_deadline=time(NULL)+OTA_RECV_TIMEOUT;
        }
        return;
    }

    /* ---- 版本检查接收 ---- */
    if(ota_state==OTA_CHECK_RECV) {
        if(time(NULL)>ota_deadline){ printf("[OTA] version recv timeout\n"); ota_reset(); return; }
        pfd.fd=ota_sock; pfd.events=POLLIN;
        if(poll(&pfd,1,0)<=0) return;
        int n=recv(ota_sock, ota_hdr_buf+ota_hdr_total, OTA_BUF_SIZE-1-ota_hdr_total, 0);
        if(n>0){ ota_hdr_total+=n; ota_deadline=time(NULL)+OTA_RECV_TIMEOUT; return; }
        if(n==0){
            ota_hdr_buf[ota_hdr_total]='\0';
            close(ota_sock); ota_sock=-1;
            int bi=find_body(ota_hdr_buf, ota_hdr_total);
            if(bi<0){ printf("[OTA] version: no header end\n"); ota_reset(); return; }
            char *body=ota_hdr_buf+bi;
            printf("[OTA] version body: [%.100s]\n", body);
            char *l1=strtok(body,"\n\r"), *l2=strtok(NULL,"\n\r"), *l3=strtok(NULL,"\n\r");
            printf("[OTA] parsed: l1=%s l2=%s l3=%s\n", l1?l1:"NULL", l2?l2:"NULL", l3?l3:"NULL");
            if(!l1||!l2||!l3||!l1[0]||!l2[0]||!l3[0]){
                printf("[OTA] version format err\n"); ota_reset(); return; }
            strncpy(ota_remote_ver,l1,sizeof(ota_remote_ver)-1);
            strncpy(ota_remote_md5,l2,sizeof(ota_remote_md5)-1);
            ota_expected_size=atol(l3);
            char lv[32]; get_local_ver(lv,sizeof(lv));
            printf("[OTA] local=%s remote=%s size=%ld\n", lv[0]?lv:"(none)", ota_remote_ver,(long)ota_expected_size);
            if(strcmp(lv,ota_remote_ver)==0&&lv[0]){ printf("[OTA] up to date\n"); ota_reset(); return; }
            /* 需要更新 → 启动下载 */
            printf("[OTA] need update, starting download\n"); ota_reset();
            r=ota_connect();
            if(r<0){ printf("[OTA] dl connect failed\n"); ota_reset(); return; }
            ota_deadline=time(NULL)+OTA_CONNECT_TIMEOUT;
            if(r==0){ ota_send_get(OTA_BINARY_PATH); ota_state=OTA_DL_RECV_HDR;
                      ota_hdr_total=0; ota_hdr_done=0; ota_deadline=time(NULL)+OTA_RECV_TIMEOUT; }
            else ota_state=OTA_DL_CONNECT;
            return;
        }
        if(errno!=EAGAIN){ printf("[OTA] version recv err\n"); ota_reset(); }
        return;
    }

    /* ---- 下载HTTP头部接收 ---- */
    if(ota_state==OTA_DL_RECV_HDR) {
        if(time(NULL)>ota_deadline){ printf("[OTA] dl hdr timeout\n"); ota_reset(); return; }
        pfd.fd=ota_sock; pfd.events=POLLIN;
        if(poll(&pfd,1,0)<=0) return;
        int n=recv(ota_sock, ota_hdr_buf+ota_hdr_total, OTA_BUF_SIZE-1-ota_hdr_total, 0);
        if(n>0){ ota_hdr_total+=n; ota_deadline=time(NULL)+OTA_RECV_TIMEOUT;
            int bi=find_body(ota_hdr_buf, ota_hdr_total);
            if(bi>=0){
                ota_content_len=parse_cl(ota_hdr_buf);
                printf("[OTA] Content-Length=%d\n", ota_content_len);
                /* 头部之后的body数据直接写入文件 */
                int extra=ota_hdr_total-bi;
                if(extra>0){
                    ota_out_fd=open(OTA_LOCAL_TEMP, O_WRONLY|O_CREAT|O_TRUNC, 0755);
                    if(ota_out_fd<0){ printf("[OTA] open temp failed\n"); ota_reset(); return; }
                    write(ota_out_fd, ota_hdr_buf+bi, extra);
                    ota_written=extra;
                }
                ota_state=OTA_DL_RECV_BODY;
            }
            return;
        }
        if(n==0){ printf("[OTA] dl hdr: conn closed early\n"); ota_reset(); return; }
        if(errno!=EAGAIN){ printf("[OTA] dl hdr recv err\n"); ota_reset(); }
        return;
    }

    /* ---- 流式接收文件体 ---- */
    if(ota_state==OTA_DL_RECV_BODY) {
        if(time(NULL)>ota_deadline){ printf("[OTA] dl body timeout (%ld/%ld)\n",(long)ota_written,(long)ota_expected_size); ota_reset(); return; }
        pfd.fd=ota_sock; pfd.events=POLLIN;
        if(poll(&pfd,1,0)<=0) return;
        int n=recv(ota_sock, ota_file_buf, OTA_FILE_BUF_SIZE, 0);
        if(n>0){
            write(ota_out_fd, ota_file_buf, n);
            ota_written+=n; ota_deadline=time(NULL)+OTA_RECV_TIMEOUT;
            int pct=(int)(ota_written*100/ota_expected_size);
            if(pct!=ota_percent){ ota_percent=pct; printf("[OTA] download %d%% (%ld/%ld)\n",pct,(long)ota_written,(long)ota_expected_size); }
            return;
        }
        if(n==0){
            close(ota_sock); ota_sock=-1;
            close(ota_out_fd); ota_out_fd=-1;
            printf("[OTA] download complete: %ld bytes\n", (long)ota_written);
            ota_state=OTA_VERIFY;
        } else if(errno!=EAGAIN){ printf("[OTA] dl body recv err\n"); ota_reset(); }
        return;
    }

    /* ---- MD5校验 + 安装 ---- */
    if(ota_state==OTA_VERIFY) {
        char md5[33]; md5_file(OTA_LOCAL_TEMP, md5);
        printf("[OTA] expected MD5=%s\n[OTA] actual   MD5=%s\n", ota_remote_md5, md5);
        if(strcmp(md5, ota_remote_md5)!=0 && ota_remote_md5[0]){
            printf("[OTA] MD5 MISMATCH, deleting temp\n");
            unlink(OTA_LOCAL_TEMP); ota_reset(); return;
        }
        ota_state=OTA_INSTALL;
    }

    if(ota_state==OTA_INSTALL) {
        printf("[OTA] md5 ok, installing...\n");
        rename(OTA_LOCAL_TEMP, OTA_LOCAL_BINARY);
        FILE *vf=fopen(OTA_LOCAL_VERSION,"w");
        if(vf){ fprintf(vf,"%s\n",ota_remote_ver); fclose(vf); }
        printf("[OTA] install done, restarting...\n");
        sync(); sleep(1); exit(0);
    }
}
