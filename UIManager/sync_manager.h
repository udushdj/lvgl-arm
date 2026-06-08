/**
 * @file sync_manager.h
 * @brief 非阻塞HTTP同步模块 —— 从服务器拉取患者队列和统计数据
 *
 * 设计要点:
 *   - 全程非阻塞socket + poll超时，绝不阻塞主循环
 *   - 两阶段同步: 先拉 /patient_queue.json，成功后再拉 /stats.json
 *   - sync_trigger() 启动同步，sync_poll() 每帧推进状态机
 *   - 服务器地址: 192.168.171.73:8080 (tiny-httpd)
 */

#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * @brief 启动一次非阻塞同步 (患者队列 + 统计数据)
 * @return 0=成功启动, -1=连接失败
 * @note  仅在 SYNC_IDLE 时有效，避免重复触发
 */
int sync_trigger(void);

/**
 * @brief 推进同步状态机 (主循环每帧调用，绝不阻塞)
 * @note  使用 poll() 零超时检测socket可读/可写
 */
void sync_poll(void);

/**
 * @brief LVGL定时器回调接口 (预留，当前未使用)
 * @note  同步由主循环直接驱动，不依赖LVGL定时器
 */
void sync_timer_cb(lv_timer_t * timer);

#ifdef __cplusplus
}
#endif

#endif
