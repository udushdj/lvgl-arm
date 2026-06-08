/**
 * @file ai_chat.c
 * @brief DeepSeek AI对话 —— HTTPS客户端 (OpenSSL阻塞调用，在pthread中运行)
 *
 * 架构:
 *   主线程: on_screen3_send() → pthread_create() → ai_poll_cb 轮询结果
 *   AI线程: ai_thread_fn() → ai_chat_send() → SSL_connect/SSL_write/SSL_read
 *
 * 注意: ai_chat_send() 是阻塞函数，必须在独立pthread中调用！
 *       内部使用 select() 超时控制连接，SO_RCVTIMEO 控制读取超时。
 */

#include "ai_chat.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* ===== DeepSeek API 配置 ===== */
#define AI_HOST     "api.deepseek.com"       /* API服务器地址 */
#define AI_PORT    "443"                      /* HTTPS端口 */
#define AI_PATH    "/chat/completions"        /* API端点 */
#define AI_MODEL   "deepseek-chat"            /* 模型名称 */
#define AI_TIMEOUT 15                         /* 连接/读超时 (秒) */
#define AI_KEY    "sk-8a95983675cc43499fd0464ae9adbc4d"

static char ai_error[256] = "";              /* 最后一次错误描述 */
static SSL_CTX *ai_ctx = NULL;               /* OpenSSL上下文 (全局复用) */

/* ===== 非阻塞TCP连接 (带超时) ===== */

static int create_socket_nb(const char *host, const char *port, int timeout_sec)
{
    struct addrinfo hints, *result, *p;
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    AI_LOG("DNS resolving %s:%s ...", host, port);
    int gai_ret = getaddrinfo(host, port, &hints, &result);
    if (gai_ret != 0) {
        AI_LOG("DNS FAILED: %s", gai_strerror(gai_ret));
        snprintf(ai_error, sizeof(ai_error), "DNS resolve failed: %s", gai_strerror(gai_ret));
        return -1;
    }
    AI_LOG("DNS OK");

    for (p = result; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            AI_LOG("socket() failed: %s", strerror(errno));
            continue;
        }
        AI_LOG("socket() OK, fd=%d", sockfd);

        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        int ret = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (ret == 0) {
            fcntl(sockfd, F_SETFL, flags);
            AI_LOG("connect() immediate OK");
            break;
        }
        if (errno != EINPROGRESS) {
            AI_LOG("connect() failed: %s", strerror(errno));
            close(sockfd);
            sockfd = -1;
            continue;
        }

        AI_LOG("connect() EINPROGRESS, waiting %ds...", timeout_sec);
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sockfd, &wfds);
        struct timeval tv = { timeout_sec, 0 };
        ret = select(sockfd + 1, NULL, &wfds, NULL, &tv);
        if (ret <= 0) {
            AI_LOG("select() timeout or error: ret=%d, err=%s", ret, strerror(errno));
            close(sockfd);
            sockfd = -1;
            continue;
        }

        int so_err = 0;
        socklen_t len = sizeof(so_err);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
        if (so_err != 0) {
            AI_LOG("connect SO_ERROR: %s", strerror(so_err));
            close(sockfd);
            sockfd = -1;
            continue;
        }

        fcntl(sockfd, F_SETFL, flags);
        AI_LOG("connect() OK");
        break;
    }

    freeaddrinfo(result);
    if (sockfd < 0) {
        snprintf(ai_error, sizeof(ai_error), "connect failed");
    }
    return sockfd;
}

/* ===== HTTP响应解析: 提取body + chunked解码 ===== */

static char *extract_body(char *response)
{
    char *body = strstr(response, "\r\n\r\n");
    if (!body) return response;
    body += 4;

    char *cl = strstr(response, "\r\nContent-Length:");
    if (!cl) cl = strstr(response, "\r\ncontent-length:");
    if (!cl) {
        char *te = strstr(response, "\r\nTransfer-Encoding:");
        if (!te) te = strstr(response, "\r\ntransfer-encoding:");
        if (te && strstr(te + 18, "chunked")) {
            char *src = body;
            char *dst = body;
            while (1) {
                char *endptr;
                long chunk_sz = strtol(src, &endptr, 16);
                if (chunk_sz <= 0) break;
                char *data = strstr(src, "\r\n");
                if (!data) break;
                data += 2;
                memmove(dst, data, chunk_sz);
                dst += chunk_sz;
                src = data + chunk_sz;
                if (src[0] == '\r' && src[1] == '\n') src += 2;
            }
            *dst = '\0';
            AI_LOG("chunked decoded, body_len=%d", (int)(dst - body));
        }
    }

    return body;
}

/* ===== 公开API ===== */

/** @brief 初始化OpenSSL库和全局SSL_CTX (在main()启动时调用一次) */
int ai_chat_init(void)
{
    AI_LOG("ai_chat_init start");
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_client_method();
    ai_ctx = SSL_CTX_new(method);
    if (!ai_ctx) {
        AI_LOG("SSL_CTX_new FAILED");
        snprintf(ai_error, sizeof(ai_error), "SSL_CTX_new failed");
        return -1;
    }
    SSL_CTX_set_verify(ai_ctx, SSL_VERIFY_NONE, NULL);
    AI_LOG("ai_chat_init OK");
    return 0;
}

void ai_chat_cleanup(void)
{
    if (ai_ctx) {
        SSL_CTX_free(ai_ctx);
        ai_ctx = NULL;
    }
    EVP_cleanup();
}

int ai_chat_send(const char *user_msg, char *response, int resp_size)
{
    AI_LOG("ai_chat_send: msg='%s'", user_msg ? user_msg : "(null)");

    if (!ai_ctx) {
        snprintf(ai_error, sizeof(ai_error), "not initialized");
        AI_LOG("ERROR: not initialized");
        return -1;
    }
    if (!user_msg || !response || resp_size <= 0) {
        snprintf(ai_error, sizeof(ai_error), "invalid params");
        AI_LOG("ERROR: invalid params");
        return -1;
    }

    response[0] = '\0';

    AI_LOG("step1: creating socket...");
    int sockfd = create_socket_nb(AI_HOST, AI_PORT, AI_TIMEOUT);
    if (sockfd < 0) {
        AI_LOG("socket creation FAILED: %s", ai_error);
        return -1;
    }

    struct timeval tv = { AI_TIMEOUT, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    AI_LOG("step2: SSL_connect...");
    SSL *ssl = SSL_new(ai_ctx);
    SSL_set_fd(ssl, sockfd);

    int ssl_ret = SSL_connect(ssl);
    if (ssl_ret <= 0) {
        int ssl_err = SSL_get_error(ssl, ssl_ret);
        AI_LOG("SSL_connect FAILED: ret=%d, ssl_err=%d", ssl_ret, ssl_err);
        ERR_print_errors_fp(stderr);
        snprintf(ai_error, sizeof(ai_error), "SSL_connect failed (err=%d)", ssl_err);
        SSL_free(ssl);
        close(sockfd);
        return -1;
    }
    AI_LOG("SSL_connect OK");

    AI_LOG("step3: building request...");
    cJSON *msg_arr = cJSON_CreateArray();
    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content",
        "你是一个医疗叫号系统的AI助手，帮助医护人员和患者回答医疗相关问题。请简洁回答。");
    cJSON_AddItemToArray(msg_arr, sys_msg);

    cJSON *user_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(user_obj, "role", "user");
    cJSON_AddStringToObject(user_obj, "content", user_msg);
    cJSON_AddItemToArray(msg_arr, user_obj);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", AI_MODEL);
    cJSON_AddItemToObject(root, "messages", msg_arr);
    cJSON_AddBoolToObject(root, "stream", 0);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    char request[4096];
    int req_len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        AI_PATH, AI_HOST, AI_KEY, strlen(post_data), post_data);

    AI_LOG("step4: SSL_write, req_len=%d...", req_len);
    int sent = SSL_write(ssl, request, req_len);
    free(post_data);

    if (sent <= 0) {
        int ssl_err = SSL_get_error(ssl, sent);
        AI_LOG("SSL_write FAILED: sent=%d, ssl_err=%d", sent, ssl_err);
        snprintf(ai_error, sizeof(ai_error), "SSL_write failed (err=%d)", ssl_err);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sockfd);
        return -1;
    }
    AI_LOG("SSL_write OK, sent=%d bytes", sent);

    AI_LOG("step5: SSL_read...");
    static char recv_buf[8192];
    int total = 0, n;
    while (total < (int)sizeof(recv_buf) - 1) {
        n = SSL_read(ssl, recv_buf + total, sizeof(recv_buf) - 1 - total);
        if (n > 0) {
            total += n;
        } else {
            break;
        }
    }
    recv_buf[total] = '\0';

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);

    AI_LOG("SSL_read total=%d bytes", total);
    if (total == 0) {
        snprintf(ai_error, sizeof(ai_error), "empty response");
        AI_LOG("ERROR: empty response");
        return -1;
    }

    AI_LOG("step6: parsing response...");
    char *body = extract_body(recv_buf);
    AI_LOG("response body (first 200 chars): %.200s", body);

    cJSON *resp_json = cJSON_Parse(body);
    if (!resp_json) {
        snprintf(ai_error, sizeof(ai_error), "JSON parse failed");
        AI_LOG("JSON parse FAILED");
        return -1;
    }

    cJSON *choices = cJSON_GetObjectItem(resp_json, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON_Delete(resp_json);
        snprintf(ai_error, sizeof(ai_error), "no choices in response");
        AI_LOG("no choices in response");
        return -1;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItem(first, "message");
    cJSON *content = cJSON_GetObjectItem(message, "content");

    if (cJSON_IsString(content)) {
        snprintf(response, resp_size, "%s", content->valuestring);
        AI_LOG("SUCCESS: '%s'", content->valuestring);
    } else {
        snprintf(ai_error, sizeof(ai_error), "no content in response");
        cJSON_Delete(resp_json);
        AI_LOG("no content field");
        return -1;
    }

    cJSON_Delete(resp_json);
    return 0;
}

const char *ai_chat_last_error(void)
{
    return ai_error;
}