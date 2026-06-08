/**
 * @file stats.c
 * @brief 统计数据模块实现 —— 全局变量定义 + JSON解析
 *
 * 数据来源: 服务器 /stats.json，由 sync_manager HTTP GET 获取，
 * 通过 stats_update_from_json() 解析更新全局变量。
 * Screen4 通过 refresh_screen4_stats() 读取这些变量刷新UI。
 */

#include "PatientManager/inc/stats.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ===== 全局统计变量 (定义) ===== */

int today_called = 0;                        /* 今日已叫号总数 */
int hourly_calls[HOURLY_SLOTS] = {0};        /* 近8小时每小时叫号数 */
int avg_wait_min = 0;                        /* 平均等待分钟数 */
int completed_count = 0;                     /* 已完成就诊数 */

recent_event_t recent_events[MAX_RECENT_EVENTS] = {0};
int recent_event_count = 0;                  /* 有效就诊记录条数 */

/* ===== API ===== */

void stats_init(void)
{
    today_called = 0;
    memset(hourly_calls, 0, sizeof(hourly_calls));
    avg_wait_min = 0;
    completed_count = 0;
    recent_event_count = 0;
}

/**
 * @brief 解析 stats.json 并更新全部统计变量
 *
 * 预期JSON格式:
 * {
 *   "today_called": 23,
 *   "avg_wait_min": 15,
 *   "hourly_calls": [0, 2, 4, 5, 3, 1, 3, 2],
 *   "recent_events": [
 *     {"id": 1008, "name": "周八", "start_time": "09:30",
 *      "end_time": "09:42", "date": "2026-06-01", "duration_min": 12}
 *   ]
 * }
 *
 * @param json_body JSON字符串 (HTTP响应体)
 * @return 0=成功, -1=输入为空或JSON解析失败
 */
int stats_update_from_json(const char *json_body)
{
    if (!json_body) return -1;

    cJSON *root = cJSON_Parse(json_body);
    if (!root) return -1;

    /* 解析4个顶层字段 */
    cJSON *f_called  = cJSON_GetObjectItem(root, "today_called");
    cJSON *f_wait    = cJSON_GetObjectItem(root, "avg_wait_min");
    cJSON *f_hourly  = cJSON_GetObjectItem(root, "hourly_calls");
    cJSON *f_events  = cJSON_GetObjectItem(root, "recent_events");

    /* 今日叫号数 */
    if (cJSON_IsNumber(f_called)) {
        today_called = f_called->valueint;
        completed_count = today_called;
    }
    /* 平均等待时间 */
    if (cJSON_IsNumber(f_wait)) {
        avg_wait_min = f_wait->valueint;
    }
    /* 近8小时时段流量数组 (最多取 HOURLY_SLOTS 个) */
    if (cJSON_IsArray(f_hourly)) {
        int n = cJSON_GetArraySize(f_hourly);
        for (int i = 0; i < HOURLY_SLOTS && i < n; i++) {
            cJSON *v = cJSON_GetArrayItem(f_hourly, i);
            if (cJSON_IsNumber(v)) {
                hourly_calls[i] = v->valueint;
            }
        }
    }
    /* 最近就诊事件列表 (最多取 MAX_RECENT_EVENTS 条) */
    if (cJSON_IsArray(f_events)) {
        recent_event_count = 0;
        int n = cJSON_GetArraySize(f_events);
        for (int i = 0; i < n && i < MAX_RECENT_EVENTS; i++) {
            cJSON *ev = cJSON_GetArrayItem(f_events, i);
            if (!cJSON_IsObject(ev)) continue;

            cJSON *f_id  = cJSON_GetObjectItem(ev, "id");
            cJSON *f_name = cJSON_GetObjectItem(ev, "name");
            cJSON *f_st  = cJSON_GetObjectItem(ev, "start_time");
            cJSON *f_et  = cJSON_GetObjectItem(ev, "end_time");
            cJSON *f_dt  = cJSON_GetObjectItem(ev, "date");
            cJSON *f_dur = cJSON_GetObjectItem(ev, "duration_min");

            /* id和name为必填字段 */
            if (!cJSON_IsNumber(f_id) || !cJSON_IsString(f_name)) continue;

            recent_events[recent_event_count].Patient_ID = (long)f_id->valuedouble;
            snprintf(recent_events[recent_event_count].name,
                     sizeof(recent_events[0].name), "%s", f_name->valuestring);

            /* 以下为可选字段，不存在时置空/0 */
            if (cJSON_IsString(f_st)) {
                snprintf(recent_events[recent_event_count].start_time,
                         sizeof(recent_events[0].start_time), "%s", f_st->valuestring);
            } else {
                recent_events[recent_event_count].start_time[0] = '\0';
            }
            if (cJSON_IsString(f_et)) {
                snprintf(recent_events[recent_event_count].end_time,
                         sizeof(recent_events[0].end_time), "%s", f_et->valuestring);
            } else {
                recent_events[recent_event_count].end_time[0] = '\0';
            }
            if (cJSON_IsString(f_dt)) {
                snprintf(recent_events[recent_event_count].date,
                         sizeof(recent_events[0].date), "%s", f_dt->valuestring);
            } else {
                recent_events[recent_event_count].date[0] = '\0';
            }
            if (cJSON_IsNumber(f_dur)) {
                recent_events[recent_event_count].duration_min = f_dur->valueint;
            } else {
                recent_events[recent_event_count].duration_min = 0;
            }
            recent_event_count++;
        }
    }

    cJSON_Delete(root);
    return 0;
}
