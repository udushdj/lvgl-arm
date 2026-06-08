/**
 * @file stats.h
 * @brief 统计数据模块 —— 叫号统计、等待时间、就诊记录
 *
 * 所有统计数据由服务器 stats.json 提供，板端通过HTTP GET接收。
 * 数据在 sync_manager 中调用 stats_update_from_json() 更新。
 * Screen4 显示这些数据，通过 refresh_screen4_stats() 刷新。
 */

#ifndef STATS_H
#define STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#define HOURLY_SLOTS 8               /* 时段流量柱状图槽位数 (近8小时) */

/* ===== 统计指标 ===== */
extern int today_called;             /* 今日已叫号总数 */
extern int hourly_calls[HOURLY_SLOTS]; /* 近8小时每小时叫号数 */
extern int avg_wait_min;             /* 平均等待分钟数 */
extern int completed_count;          /* 已完成就诊数 */

/* ===== 就诊记录 ===== */
#define MAX_RECENT_EVENTS 50         /* 最多保存的就诊记录条数 */

typedef struct {
    long Patient_ID;                 /* 患者编号 */
    char name[20];                   /* 患者姓名 */
    char start_time[6];              /* 开始时间 "HH:MM" */
    char end_time[6];                /* 结束时间 "HH:MM" */
    char date[11];                   /* 日期 "YYYY-MM-DD" */
    int duration_min;                /* 就诊时长 (分钟) */
} recent_event_t;

extern recent_event_t recent_events[MAX_RECENT_EVENTS];
extern int recent_event_count;       /* 有效就诊记录条数 */

/* ===== API ===== */

/** @brief 初始化统计变量 (全部清零) */
void stats_init(void);

/**
 * @brief 解析服务器 stats.json 并更新全局统计变量
 * @param json_body JSON字符串 (HTTP响应体)
 * @return 0=成功, -1=JSON解析失败
 */
int stats_update_from_json(const char *json_body);

#ifdef __cplusplus
}
#endif

#endif
