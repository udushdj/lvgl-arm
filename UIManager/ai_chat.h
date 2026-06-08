/**
 * @file ai_chat.h
 * @brief DeepSeek AI对话模块 —— HTTPS客户端 (OpenSSL + pthread)
 *
 * 设计要点:
 *   - 独立pthread执行HTTPS请求，避免阻塞主循环
 *   - 主线程通过 volatile ai_busy 标志 + LVGL定时器轮询结果
 *   - 超时: SSL连接15秒 (SO_RCVTIMEO)
 *   - 响应最大2048字节 (AI_RESPONSE_MAX)
 */

#ifndef AI_CHAT_H
#define AI_CHAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define AI_DEBUG 1                        /* 调试日志开关 */

#if AI_DEBUG
#define AI_LOG(fmt, ...) printf("[AI] " fmt "\n", ##__VA_ARGS__)
#else
#define AI_LOG(fmt, ...) ((void)0)
#endif

#define AI_RESPONSE_MAX 2048              /* AI响应缓冲区最大字节数 */

/**
 * @brief 初始化AI模块 (加载OpenSSL, 创建SSL_CTX)
 * @return 0=成功, -1=失败
 */
int ai_chat_init(void);

/** @brief 清理AI模块 (释放SSL_CTX) */
void ai_chat_cleanup(void);

/**
 * @brief 发送消息到DeepSeek API并获取回复 (阻塞调用，请在pthread中使用)
 * @param user_msg  用户输入的消息文本
 * @param response  输出缓冲区 (存放AI回复)
 * @param resp_size 输出缓冲区大小
 * @return 0=成功, -1=失败 (错误信息通过 ai_chat_last_error() 获取)
 */
int ai_chat_send(const char *user_msg, char *response, int resp_size);

/** @brief 获取最后一次错误的描述字符串 */
const char *ai_chat_last_error(void);

#ifdef __cplusplus
}
#endif

#endif
