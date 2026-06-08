/**
 * @file main.c
 * @brief 医用排队叫号系统 —— ARM嵌入式Linux主入口
 * @author [wz]
 * @date 2024-03-27
 *
 * 系统架构概要:
 *   - LVGL (v8.3.11) 作为GUI框架，驱framebuffer显示 + evdev触摸输入
 *   - UIManager 管理4个界面: 主叫号屏、患者管理、消息发送、统计看板
 *   - PatientManager 管理患者队列 (带头结点的单向链表)
 *   - sync_manager 通过非阻塞HTTP轮询服务器同步患者数据
 *   - ai_chat 通过HTTPS调用DeepSeek API实现AI对话
 *
 * 主循环节奏:
 *   while(1) {
 *       lv_timer_handler();   // 驱动LVGL (处理动画、定时器、事件)
 *       usleep(5000);          // 5ms休眠，降低CPU占用
 *       sync_poll();           // 推进HTTP同步状态机
 *       每5秒 sync_trigger();  // 触发新一轮同步
 *   }
 *
 * 硬件平台: ARM Cortex-A (800x480 LCD, 触摸屏)
 * 编译器:   arm-linux-gcc (交叉编译)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "UIManager/ui.h"
#include "UIManager/ui_events.h"
#include "UIManager/sync_manager.h"
#include "PatientManager/inc/linked_list.h"
#include "PatientManager/inc/stats.h"
#include "UIManager/ai_chat.h"
#include "UIManager/ota_manager.h"

#define DISP_BUF_SIZE (800*480)    /* 全屏单缓冲: 800×480像素 */
#define SYNC_INTERVAL_SEC 5        /* 自动同步间隔 (秒) */
#define OTA_CHECK_INTERVAL_SEC 60  /* OTA版本检查间隔 (秒) */

P_List patient_queue = NULL;       /* 全局患者队列 (带头结点单向链表) */
extern const lv_font_t my_font_cn_48;

/* ==================== LVGL 底层初始化 ==================== */

/**
 * @brief 初始化LVGL显示和输入驱动
 *
 * 显示: Linux framebuffer (/dev/fb0)
 * 输入: evdev触摸设备 (/dev/input/event*)
 */
static void lvgl_init(void)
{
    lv_init();

    /* 显示驱动: framebuffer */
    fbdev_init();

    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf  = &disp_buf;
    disp_drv.flush_cb  = fbdev_flush;
    disp_drv.hor_res   = 800;
    disp_drv.ver_res   = 480;
    lv_disp_drv_register(&disp_drv);

    /* 输入驱动: evdev触摸 */
    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_drv_register(&indev_drv);
}

/**
 * @brief 自定义系统tick函数 (毫秒级)
 *
 * LVGL需要知道自启动以来经过的毫秒数来驱动动画和定时器。
 * 使用 gettimeofday() 实现，精度足够且不依赖系统 tick。
 */
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (uint64_t)(tv_start.tv_sec) * 1000 + tv_start.tv_usec / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms = (uint64_t)(tv_now.tv_sec) * 1000 + tv_now.tv_usec / 1000;

    return (uint32_t)(now_ms - start_ms);
}

/* ==================== 主函数 ==================== */

int main(void)
{
    /* 1. 初始化硬件抽象层 (显示 + 触摸) */
    lvgl_init();

    /* 2. 创建空的全局患者队列 */
    patient_queue = InitList();
    if (patient_queue == NULL) {
        fprintf(stderr, "FATAL: InitList() failed\n");
        return -1;
    }

    /* 3. 初始化统计模块 (清零所有计数器) */
    stats_init();

    /* 4. 初始化AI对话模块 (加载OpenSSL) */
    ai_chat_init();

    /* 5. 创建所有4个UI界面并加载主屏幕 */
    ui_init();

    /* 6. 初始化刷新所有界面数据显示 (此时队列为空，显示占位提示) */
    refresh_current_patient(NULL);
    refresh_waiting_count(NULL);
    refresh_next_patient(NULL);
    refresh_waiting_queue(NULL);
    refresh_patient_list(NULL);

    /* 7. 启动后立即检查一次OTA更新 */
    printf("[OTA] initial version check\n");
    ota_check_trigger();

    /* 8. 主事件循环 */
    time_t last_sync = time(NULL);
    time_t last_ota_check = time(NULL);
    while (1) {
        lv_timer_handler();     /* 驱动LVGL: 处理定时器、动画、输入事件 */
        usleep(5000);            /* 5ms休眠，100~200fps的实际刷新率 */
        sync_poll();             /* 推进HTTP同步状态机 (非阻塞) */
        ota_poll();              /* 推进OTA状态机 (非阻塞) */

        time_t now = time(NULL);

        /* 每SYNC_INTERVAL_SEC秒触发一次服务器同步 */
        if (now - last_sync >= SYNC_INTERVAL_SEC) {
            sync_trigger();
            last_sync = now;
        }

        /* 每OTA_CHECK_INTERVAL_SEC秒检查一次版本更新 */
        if (now - last_ota_check >= OTA_CHECK_INTERVAL_SEC) {
            ota_check_trigger();
            last_ota_check = now;
        }
    }

    return 0;
}
