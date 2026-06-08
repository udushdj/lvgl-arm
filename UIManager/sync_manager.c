/**
 * @file sync_manager.c
 * @brief 非阻塞HTTP同步状态机 —— 从服务器拉取患者队列和统计数据
 * @author [wz]
 * @date 2024-03-27
 *
 * 设计要点:
 *   1. 全程使用非阻塞socket + poll超时检测，绝不阻塞主循环
 *   2. 先拉 /patient_queue.json，成功后再拉 /stats.json，两次共用同一个状态机
 *   3. 每次 sync_trigger() 启动一轮同步，sync_poll() 每帧推进状态
 *   4. 超时设置: connect 2秒, recv 首次2秒/每次收到数据重置为1秒
 *   5. 响应体通过 \r\n\r\n 分隔头部和JSON体，不解析HTTP状态行
 *
 * 状态流转:
 *   SYNC_IDLE → SYNC_CONNECT → SYNC_SEND → SYNC_RECV → SYNC_PARSE
 *                                                          ↓
 *                                            (如果有stats) SYNC_STATS_CONNECT → ...
 *                                                          ↓
 *                                                     SYNC_DONE → SYNC_IDLE
 */

#include "sync_manager.h"
#include "ui.h"
#include "ui_events.h"
#include "PatientManager/inc/linked_list.h"
#include "PatientManager/inc/stats.h"
#include "cJSON.h"

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

/* ==================== 服务器配置 ==================== */

#define SERVER_HOST  "192.168.171.73"   /* 服务器IP地址 */
#define SERVER_PORT  8080                /* 服务器端口 */
#define QUEUE_PATH   "/patient_queue.json" /* 患者队列接口路径 */
#define STATS_PATH   "/stats.json"         /* 统计数据接口路径 */
#define RECV_BUF_SIZE     32768            /* 接收缓冲区大小 (32KB) */
#define CONNECT_TIMEOUT_SEC 2              /* 连接超时 (秒) */
#define RECV_TIMEOUT_SEC    2              /* 首次接收超时 (秒) */
#define RECV_IDLE_SEC       1              /* 每次收到数据后重置的超时 (秒) */

extern P_List patient_queue;

/* ==================== 非阻塞状态机 ==================== */

/** 同步状态枚举 */
enum {
    SYNC_IDLE,          /* 空闲，等待触发 */
    SYNC_CONNECT,       /* 正在TCP连接 (非阻塞) */
    SYNC_SEND,          /* 正在发送HTTP请求 */
    SYNC_RECV,          /* 正在接收HTTP响应 */
    SYNC_PARSE,         /* 正在解析JSON (瞬时状态) */
    SYNC_STATS_CONNECT, /* 正在连接stats接口 */
    SYNC_STATS_SEND,    /* 正在发送stats请求 */
    SYNC_STATS_RECV,    /* 正在接收stats响应 */
    SYNC_STATS_PARSE,   /* 正在解析stats JSON */
    SYNC_DONE           /* 本轮同步完成 */
};

static int sync_state = SYNC_IDLE;  /* 当前状态 */
static int sync_sock = -1;          /* 当前socket fd，-1表示无连接 */
static char sync_buf[RECV_BUF_SIZE]; /* 接收缓冲区 */
static int  sync_total;              /* 已接收字节数 */
static time_t sync_deadline;         /* 超时截止时间 */
static int sync_count = 0;           /* 同步成功次数计数器 */
static const char *sync_current_path; /* 当前请求的路径 */
static int sync_is_stats;            /* 是否正在拉取stats (0=queue, 1=stats) */

/* ==================== JSON解析 ==================== */

/**
 * @brief 解析JSON患者队列并替换全局 patient_queue
 * @param json_body 指向HTTP响应体的字符串 (需以\0结尾)
 * @return 0=成功, -2=JSON格式错误或解析失败
 *
 * 注意: 成功时会将旧的 patient_queue 释放并替换为新链表。
 *       失败时原 patient_queue 保持不变。
 */
static int parse_and_replace(const char *json_body)
{
    cJSON *root = cJSON_Parse(json_body);
    if (!root) return -2;

    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return -2;
    }

    /* 构建新链表 */
    P_List new_list = InitList();
    if (!new_list) {
        cJSON_Delete(root);
        return -2;
    }

    int count = cJSON_GetArraySize(root);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        cJSON *f_id    = cJSON_GetObjectItem(item, "Patient_ID");
        cJSON *f_name  = cJSON_GetObjectItem(item, "name");
        cJSON *f_idnum = cJSON_GetObjectItem(item, "ID_number");
        cJSON *f_age   = cJSON_GetObjectItem(item, "age");

        /* 必填字段校验 */
        if (!f_name || !cJSON_IsString(f_name)) continue;
        if (!f_idnum || !cJSON_IsString(f_idnum)) continue;
        if (!f_id || (!cJSON_IsNumber(f_id) && !cJSON_IsString(f_id))) continue;
        if (!f_age || (!cJSON_IsNumber(f_age) && !cJSON_IsString(f_age))) continue;

        P_Node node = (P_Node)calloc(1, sizeof(patient_t));
        if (!node) continue;

        /* Patient_ID 支持数字和字符串两种格式 */
        if (cJSON_IsNumber(f_id))
            node->Patient_ID = (long)f_id->valuedouble;
        else
            node->Patient_ID = atol(f_id->valuestring);

        snprintf(node->name, sizeof(node->name), "%s", f_name->valuestring);
        snprintf(node->ID_number, sizeof(node->ID_number), "%s", f_idnum->valuestring);

        if (cJSON_IsNumber(f_age))
            node->age = f_age->valueint;
        else
            node->age = atoi(f_age->valuestring);
        node->next = NULL;

        list_push_back(new_list, node);
    }

    cJSON_Delete(root);

    /* JSON数组有内容但一个有效患者都没解析出来 → 失败 */
    if (new_list->size == 0 && count > 0) {
        kill_List(new_list);
        return -2;
    }

    /* 原子替换: 先释放旧链表，再挂载新链表 */
    if (patient_queue) kill_List(patient_queue);
    patient_queue = new_list;
    return 0;
}

/**
 * @brief 从HTTP响应中提取Body部分
 * @param response 完整的HTTP响应字符串
 * @return 指向Body起始位置的指针 (\r\n\r\n 或 \n\n 之后)
 *
 * HTTP响应格式: Status-Line\r\nHeaders\r\n\r\nBody
 * 查找到第一个空行后返回其后内容。
 */
static char *extract_body(char *response)
{
    char *body = strstr(response, "\r\n\r\n");
    if (body) return body + 4;
    body = strstr(response, "\n\n");
    if (body) return body + 2;
    return response; /* 找不到分隔符，整包当body处理 */
}

/* ==================== 连接管理 ==================== */

/**
 * @brief 清理当前同步状态 (关闭socket，回到IDLE)
 */
static void sync_cleanup(void)
{
    if (sync_sock >= 0) { close(sync_sock); sync_sock = -1; }
    sync_state = SYNC_IDLE;
}

/**
 * @brief 设置socket为非阻塞模式
 * @return 旧的文件状态标志，失败返回-1
 */
static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return flags;
}

/**
 * @brief 发起非阻塞TCP连接到服务器
 * @return  0=立即连接成功, 1=正在连接中, -1=失败
 *
 * socket始终保持在非阻塞模式，不再切回阻塞模式。
 */
static int sync_start_connect(const char *host, int port)
{
    sync_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sync_sock < 0) { sync_cleanup(); return -1; }

    /* 全程非阻塞 —— 避免阻塞主循环 */
    set_nonblock(sync_sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_aton(host, &addr.sin_addr) == 0) { sync_cleanup(); return -1; }

    int ret = connect(sync_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
        /* 本地或同网段可能立即连接成功 */
        return 0;
    }
    if (errno != EINPROGRESS) { sync_cleanup(); return -1; }

    /* 正在异步连接中 */
    sync_deadline = time(NULL) + CONNECT_TIMEOUT_SEC;
    return 1;
}

/**
 * @brief 构造并发送HTTP GET请求 (非阻塞send)
 *
 * 请求使用 HTTP/1.0 + Connection: close 确保服务器发送完响应后关闭连接。
 * 如果send返回EAGAIN则保留状态，下次poll时重试。
 */
static int sync_send_request(void)
{
    char req[512];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.0\r\n"
             "Host: %s:%d\r\n"
             "Connection: close\r\n"
             "\r\n",
             sync_current_path, SERVER_HOST, SERVER_PORT);
    printf("[SYNC] GET %s\n", sync_current_path);

    ssize_t n = send(sync_sock, req, strlen(req), 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* 发送缓冲区满，保留状态，下次poll时重试 */
            return 1;
        }
        printf("[SYNC] send error: %s\n", strerror(errno));
        sync_cleanup();
        return -1;
    }
    /* 发送成功 (可能只发送了部分，但对于小请求通常一次完成) */
    sync_total = 0;
    sync_deadline = time(NULL) + RECV_TIMEOUT_SEC;
    return 0;
}

/* ==================== 状态机入口 ==================== */

/**
 * @brief 触发一轮同步 (拉取患者队列 + 统计数据)
 * @return 0=成功启动, -1=连接失败
 *
 * 仅在 SYNC_IDLE 状态下有效，避免重复触发。
 * 同步分两步: 先拉queue，成功后再拉stats。
 */
int sync_trigger(void)
{
    if (sync_state != SYNC_IDLE) return 0;
    sync_is_stats = 0;
    sync_current_path = QUEUE_PATH;
    int r = sync_start_connect(SERVER_HOST, SERVER_PORT);
    if (r < 0) {
        printf("[SYNC] #%d connect failed\n", sync_count + 1);
        return -1;
    }
    if (r == 0) {
        /* 立即连接成功 → 直接发送请求 */
        sync_state = SYNC_SEND;
        sync_send_request();
        sync_state = SYNC_RECV;
    } else {
        sync_state = SYNC_CONNECT;
    }
    return 0;
}

/**
 * @brief 推进同步状态机 (每帧调用一次，绝不阻塞)
 *
 * 使用poll()代替select()检查socket可读/可写状态，
 * 所有I/O操作均为非阻塞，EAGAIN时安全退出等待下次poll。
 */
void sync_poll(void)
{
    if (sync_state == SYNC_IDLE) return;

    struct pollfd pfd;
    int timeout_ms = 0;  /* 默认零超时，立即返回 */
    int ret;

    switch (sync_state) {

    /* ========== 患者队列: TCP连接阶段 ========== */
    case SYNC_CONNECT:
    case SYNC_STATS_CONNECT:
    {
        /* 检查连接超时 */
        if (time(NULL) > sync_deadline) {
            printf("[SYNC] connect timeout\n");
            sync_cleanup();
            return;
        }

        pfd.fd = sync_sock;
        pfd.events = POLLOUT;  /* 等待socket可写 = 连接完成 */
        ret = poll(&pfd, 1, timeout_ms);
        if (ret <= 0) return;  /* 还没连上或出错，等下次poll */

        /* 检查连接结果 */
        int so_err = 0;
        socklen_t len = sizeof(so_err);
        getsockopt(sync_sock, SOL_SOCKET, SO_ERROR, &so_err, &len);
        if (so_err != 0) {
            printf("[SYNC] connect error: %s\n", strerror(so_err));
            sync_cleanup();
            return;
        }

        /* 连接成功 → 发送HTTP请求 */
        if (sync_state == SYNC_CONNECT) {
            sync_state = SYNC_SEND;
        } else {
            sync_state = SYNC_STATS_SEND;
        }
        int sr = sync_send_request();
        if (sr < 0) return; /* send失败，已cleanup */
        if (sr == 0) {
            /* send已完成 */
            if (sync_state == SYNC_SEND) sync_state = SYNC_RECV;
            else sync_state = SYNC_STATS_RECV;
        }
        /* sr == 1: send EAGAIN, 保留当前SEND状态，等下次poll */
        return;
    }

    /* ========== 患者队列: 发送请求阶段 (处理EAGAIN重试) ========== */
    case SYNC_SEND:
    case SYNC_STATS_SEND:
    {
        /* 检查超时 */
        if (time(NULL) > sync_deadline) {
            printf("[SYNC] send timeout\n");
            sync_cleanup();
            return;
        }

        int sr = sync_send_request();
        if (sr < 0) return;
        if (sr == 0) {
            if (sync_state == SYNC_SEND) sync_state = SYNC_RECV;
            else sync_state = SYNC_STATS_RECV;
        }
        return;
    }

    /* ========== 接收响应阶段 (患者队列 & stats共用) ========== */
    case SYNC_RECV:
    case SYNC_STATS_RECV:
    {
        /* 检查接收超时 */
        if (time(NULL) > sync_deadline) {
            printf("[SYNC] recv timeout (%d bytes received)\n", sync_total);
            if (sync_total > 0) {
                /* 已收到部分数据 → 尝试解析 */
                sync_buf[sync_total] = '\0';
                goto do_parse;
            }
            sync_cleanup();
            return;
        }

        /* poll检查是否有数据可读 */
        pfd.fd = sync_sock;
        pfd.events = POLLIN;
        ret = poll(&pfd, 1, timeout_ms);
        if (ret <= 0) return; /* 暂无数据，等下次poll */

        /* 非阻塞recv: 读多少算多少 */
        ssize_t n = recv(sync_sock, sync_buf + sync_total,
                         RECV_BUF_SIZE - 1 - sync_total, 0);
        if (n > 0) {
            sync_total += n;
            /* 每次收到数据重置超时，防止慢速传输被误杀 */
            sync_deadline = time(NULL) + RECV_IDLE_SEC;
            return;
        }
        if (n == 0) {
            /* 服务器关闭连接 = 响应接收完毕 */
            sync_buf[sync_total] = '\0';
            close(sync_sock);
            sync_sock = -1;
            goto do_parse;
        }
        /* n < 0 */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return; /* 暂无更多数据，等下次poll */
        }
        printf("[SYNC] recv error: %s\n", strerror(errno));
        sync_cleanup();
        return;
    }

    /* ========== 解析响应体 ========== */
    do_parse:
    {
        printf("[SYNC] received %d bytes\n", sync_total);
        char *body = extract_body(sync_buf);

        if (sync_state == SYNC_STATS_RECV) {
            /* 解析stats.json */
            int rc = stats_update_from_json(body);
            if (rc == 0) {
                printf("[STATS] updated: called=%d wait=%dmin events=%d\n",
                       today_called, avg_wait_min, recent_event_count);
                refresh_screen4_stats();
            } else {
                printf("[STATS] JSON parse failed\n");
            }
            sync_count++;
            printf("[SYNC] #%d done: %d patients\n", sync_count, patient_queue->size);
            sync_cleanup();
            return;
        } else {
            /* 解析patient_queue.json */
            int rc = parse_and_replace(body);
            if (rc != 0) {
                printf("[SYNC] patient_queue JSON parse failed\n");
                sync_cleanup();
                return;
            }
            /* 更新Screen1的UI */
            refresh_current_patient(NULL);
            refresh_waiting_count(NULL);
            refresh_next_patient(NULL);
            refresh_waiting_queue(NULL);

            /* 患者队列拉取成功 → 继续拉取统计数据 */
            sync_is_stats = 1;
            sync_current_path = STATS_PATH;
            int r2 = sync_start_connect(SERVER_HOST, SERVER_PORT);
            if (r2 < 0) {
                printf("[STATS] connect failed\n");
                sync_count++;
                printf("[SYNC] #%d done: %d patients\n", sync_count,
                       patient_queue->size);
                sync_cleanup();
                return;
            }
            if (r2 == 0) {
                /* 立即连接成功 */
                sync_state = SYNC_STATS_SEND;
                int sr = sync_send_request();
                if (sr < 0) return;
                if (sr == 0) sync_state = SYNC_STATS_RECV;
            } else {
                sync_state = SYNC_STATS_CONNECT;
            }
            return;
        }
    }

    default:
        return;
    }
}

/**
 * @brief LVGL定时器回调 (保留接口，当前未使用)
 *
 * 同步由主循环的 sync_poll() 驱动，不依赖LVGL定时器。
 * 该回调作为预留接口，未来可改为定时器驱动。
 */
void sync_timer_cb(lv_timer_t * timer)
{
    (void)timer;
}
