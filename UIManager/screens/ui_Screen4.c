/**
 * @file ui_Screen4.c
 * @brief Screen4界面创建 —— 数据统计看板
 *
 * 布局:
 *   - 顶栏 "数据统计"
 *   - 3个统计卡片: 已就诊 / 平均等待 / 当前排队
 *   - 近8小时流量柱状图 (8根柱子 + 数值标签 + 时间标签)
 *   - 返回主界面按钮
 *
 * 配色: 青绿色主题 (#0f766e / #115e59)，与整体UI一致
 */

#include "../ui.h"
#include "PatientManager/inc/linked_list.h"
#include "PatientManager/inc/stats.h"
#include <stdio.h>
#include <time.h>

extern lv_font_t my_font_cn;
extern P_List patient_queue;
LV_IMG_DECLARE(bg_screen2);

/* ===== 静态控件变量 (仅在Screen4内部使用) ===== */
static lv_obj_t * ui_Screen4StatCard1;   /* 已就诊卡片 */
static lv_obj_t * ui_Screen4StatCard2;   /* 平均等待卡片 */
static lv_obj_t * ui_Screen4StatCard3;   /* 当前排队卡片 */

/* 全局变量 (ui.h中extern声明) */
lv_obj_t * ui_Screen4StatNum1;           /* 已就诊人数数值 */
lv_obj_t * ui_Screen4StatNum2;           /* 平均等待时间数值 */
lv_obj_t * ui_Screen4StatNum3;           /* 当前排队人数数值 */
lv_obj_t * ui_Screen4FlowPanel;          /* 柱状图面板 */

/* 柱状图控件数组 */
static lv_obj_t * bars[8];               /* 8根柱子 */
static lv_obj_t * bar_labels[8];         /* 底部时间标签 */
static lv_obj_t * bar_value_labels[8];   /* 柱体上方数值标签 */

/* ===== 绘制近8小时流量柱状图 ===== */
/**
 * @brief 根据 hourly_calls[8] 数据绘制/更新柱状图
 *
 * 每次调用会先删除旧的8组控件再重建。
 * 横轴标签为从当前小时往前7小时的数字 (0-23)。
 * 柱高 = 当前值 / 最大值 × 80px (最小2px)
 */
void screen4_draw_flow_bars(void)
{
    /* 计算最大值 (至少为1，避免除零) */
    int max_val = 1;
    for (int i = 0; i < HOURLY_SLOTS; i++) {
        if (hourly_calls[i] > max_val) max_val = hourly_calls[i];
    }

    /* 删除旧的柱状图控件 */
    for (int i = 0; i < 8; i++) {
        if (bars[i]) lv_obj_del(bars[i]);
        bars[i] = NULL;
        if (bar_labels[i]) {
            lv_obj_del(bar_labels[i]);
            bar_labels[i] = NULL;
        }
        if (bar_value_labels[i]) {
            lv_obj_del(bar_value_labels[i]);
            bar_value_labels[i] = NULL;
        }
    }

    /* 计算起始小时 (当前小时往前7小时) */
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    int cur_hour = tm_info->tm_hour;
    int start_hour = (cur_hour - 7 + 24) % 24;

    /* 绘制8组: 柱子 + 数值标签 + 时间标签 */
    for (int i = 0; i < 8; i++) {
        int val = hourly_calls[i];
        int bar_h = (max_val > 0) ? (val * 80 / max_val) : 2;
        if (bar_h < 2) bar_h = 2;

        /* 柱子 */
        bars[i] = lv_obj_create(ui_Screen4FlowPanel);
        lv_obj_set_width(bars[i], 50);
        lv_obj_set_height(bars[i], bar_h);
        lv_obj_set_x(bars[i], -180 + i * 72);
        lv_obj_set_y(bars[i], -20);
        lv_obj_set_align(bars[i], LV_ALIGN_BOTTOM_MID);
        lv_obj_set_style_bg_color(bars[i], lv_color_hex(0x0f766e), 0);
        lv_obj_set_style_radius(bars[i], 4, 0);
        lv_obj_set_style_border_width(bars[i], 0, 0);
        lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_SCROLLABLE);

        /* 柱体上方数值 */
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", val);
        bar_value_labels[i] = lv_label_create(ui_Screen4FlowPanel);
        lv_label_set_text(bar_value_labels[i], buf);
        lv_obj_set_x(bar_value_labels[i], -180 + i * 72);
        lv_obj_set_y(bar_value_labels[i], -25 - bar_h);
        lv_obj_set_align(bar_value_labels[i], LV_ALIGN_BOTTOM_MID);
        lv_obj_set_style_text_color(bar_value_labels[i], lv_color_hex(0x6b7170), 0);

        /* 底部时间标签 */
        snprintf(buf, sizeof(buf), "%d", (start_hour + i) % 24);
        bar_labels[i] = lv_label_create(ui_Screen4FlowPanel);
        lv_label_set_text(bar_labels[i], buf);
        lv_obj_set_x(bar_labels[i], -180 + i * 72);
        lv_obj_set_y(bar_labels[i], -5);
        lv_obj_set_align(bar_labels[i], LV_ALIGN_BOTTOM_MID);
        lv_obj_set_style_text_color(bar_labels[i], lv_color_hex(0x9da3a0), 0);
    }
}

/* ===== Screen4初始化 ===== */

void ui_Screen4_screen_init(void)
{
    /* ===== 根容器 ===== */
    ui_Screen4 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui_Screen4, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_img_src(ui_Screen4, &bg_screen2, 0);

    /* ===== 顶栏 "数据统计" ===== */
    ui_Screen4TopBar = lv_obj_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4TopBar, 825);
    lv_obj_set_height(ui_Screen4TopBar, 38);
    lv_obj_set_x(ui_Screen4TopBar, 0);
    lv_obj_set_y(ui_Screen4TopBar, -220);
    lv_obj_set_align(ui_Screen4TopBar, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen4TopBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4TopBar, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen4TopBar, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen4TopBar, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen4TopBar, 0, 0);

    ui_Screen4TopLabel = lv_label_create(ui_Screen4TopBar);
    lv_obj_set_width(ui_Screen4TopLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen4TopLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen4TopLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen4TopLabel, "数据统计");
    lv_obj_set_style_text_font(ui_Screen4TopLabel, &my_font_cn, 0);
    lv_obj_set_style_text_color(ui_Screen4TopLabel, lv_color_white(), 0);

    /* ===== 统计卡片1: 已就诊 ===== */
    ui_Screen4StatCard1 = lv_obj_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4StatCard1, 220);
    lv_obj_set_height(ui_Screen4StatCard1, 110);
    lv_obj_set_x(ui_Screen4StatCard1, -240);
    lv_obj_set_y(ui_Screen4StatCard1, -150);
    lv_obj_set_align(ui_Screen4StatCard1, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen4StatCard1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4StatCard1, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_Screen4StatCard1, 16, 0);
    lv_obj_set_style_shadow_width(ui_Screen4StatCard1, 8, 0);
    lv_obj_set_style_border_width(ui_Screen4StatCard1, 1, 0);
    lv_obj_set_style_border_color(ui_Screen4StatCard1, lv_color_hex(0xe0e3e1), 0);

    lv_obj_t *card1_title = lv_label_create(ui_Screen4StatCard1);
    lv_label_set_text(card1_title, "已就诊");
    lv_obj_set_align(card1_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(card1_title, 12);
    lv_obj_set_style_text_font(card1_title, &my_font_cn, 0);
    lv_obj_set_style_text_color(card1_title, lv_color_hex(0x6b7170), 0);

    ui_Screen4StatNum1 = lv_label_create(ui_Screen4StatCard1);
    lv_label_set_text(ui_Screen4StatNum1, "0 人");
    lv_obj_set_align(ui_Screen4StatNum1, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Screen4StatNum1, &my_font_cn, 0);
    lv_obj_set_style_text_color(ui_Screen4StatNum1, lv_color_hex(0x115e59), 0);

    /* ===== 统计卡片2: 平均等待 ===== */
    ui_Screen4StatCard2 = lv_obj_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4StatCard2, 220);
    lv_obj_set_height(ui_Screen4StatCard2, 110);
    lv_obj_set_x(ui_Screen4StatCard2, 0);
    lv_obj_set_y(ui_Screen4StatCard2, -150);
    lv_obj_set_align(ui_Screen4StatCard2, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen4StatCard2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4StatCard2, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_Screen4StatCard2, 16, 0);
    lv_obj_set_style_shadow_width(ui_Screen4StatCard2, 8, 0);
    lv_obj_set_style_border_width(ui_Screen4StatCard2, 1, 0);
    lv_obj_set_style_border_color(ui_Screen4StatCard2, lv_color_hex(0xe0e3e1), 0);

    lv_obj_t *card2_title = lv_label_create(ui_Screen4StatCard2);
    lv_label_set_text(card2_title, "平均等待");
    lv_obj_set_align(card2_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(card2_title, 12);
    lv_obj_set_style_text_font(card2_title, &my_font_cn, 0);
    lv_obj_set_style_text_color(card2_title, lv_color_hex(0x6b7170), 0);

    ui_Screen4StatNum2 = lv_label_create(ui_Screen4StatCard2);
    lv_label_set_text(ui_Screen4StatNum2, "0 分钟");
    lv_obj_set_align(ui_Screen4StatNum2, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Screen4StatNum2, &my_font_cn, 0);
    lv_obj_set_style_text_color(ui_Screen4StatNum2, lv_color_hex(0x115e59), 0);

    /* ===== 统计卡片3: 当前排队 ===== */
    ui_Screen4StatCard3 = lv_obj_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4StatCard3, 220);
    lv_obj_set_height(ui_Screen4StatCard3, 110);
    lv_obj_set_x(ui_Screen4StatCard3, 240);
    lv_obj_set_y(ui_Screen4StatCard3, -150);
    lv_obj_set_align(ui_Screen4StatCard3, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen4StatCard3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4StatCard3, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_Screen4StatCard3, 16, 0);
    lv_obj_set_style_shadow_width(ui_Screen4StatCard3, 8, 0);
    lv_obj_set_style_border_width(ui_Screen4StatCard3, 1, 0);
    lv_obj_set_style_border_color(ui_Screen4StatCard3, lv_color_hex(0xe0e3e1), 0);

    lv_obj_t *card3_title = lv_label_create(ui_Screen4StatCard3);
    lv_label_set_text(card3_title, "当前排队");
    lv_obj_set_align(card3_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(card3_title, 12);
    lv_obj_set_style_text_font(card3_title, &my_font_cn, 0);
    lv_obj_set_style_text_color(card3_title, lv_color_hex(0x6b7170), 0);

    ui_Screen4StatNum3 = lv_label_create(ui_Screen4StatCard3);
    lv_label_set_text(ui_Screen4StatNum3, "0 人");
    lv_obj_set_align(ui_Screen4StatNum3, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_Screen4StatNum3, &my_font_cn, 0);
    lv_obj_set_style_text_color(ui_Screen4StatNum3, lv_color_hex(0x115e59), 0);

    /* ===== 近8小时流量柱状图面板 ===== */
    ui_Screen4FlowPanel = lv_obj_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4FlowPanel, 740);
    lv_obj_set_height(ui_Screen4FlowPanel, 160);
    lv_obj_set_x(ui_Screen4FlowPanel, 0);
    lv_obj_set_y(ui_Screen4FlowPanel, 50);
    lv_obj_set_align(ui_Screen4FlowPanel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen4FlowPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4FlowPanel, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_Screen4FlowPanel, 16, 0);
    lv_obj_set_style_shadow_width(ui_Screen4FlowPanel, 8, 0);
    lv_obj_set_style_border_width(ui_Screen4FlowPanel, 1, 0);
    lv_obj_set_style_border_color(ui_Screen4FlowPanel, lv_color_hex(0xe0e3e1), 0);

    lv_obj_t *flow_title = lv_label_create(ui_Screen4FlowPanel);
    lv_label_set_text(flow_title, "近8小时流量");
    lv_obj_set_x(flow_title, -320);
    lv_obj_set_y(flow_title, -60);
    lv_obj_set_align(flow_title, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(flow_title, &my_font_cn, 0);
    lv_obj_set_style_text_color(flow_title, lv_color_hex(0x6b7170), 0);

    for (int i = 0; i < 8; i++) {
        bar_labels[i] = NULL;
    }

    screen4_draw_flow_bars();  /* 初始绘制空柱状图 */

    /* ===== 返回主界面按钮 ===== */
    ui_Screen4BackBtn = lv_btn_create(ui_Screen4);
    lv_obj_set_width(ui_Screen4BackBtn, 130);
    lv_obj_set_height(ui_Screen4BackBtn, 50);
    lv_obj_set_x(ui_Screen4BackBtn, 0);
    lv_obj_set_y(ui_Screen4BackBtn, 195);
    lv_obj_set_align(ui_Screen4BackBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Screen4BackBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_Screen4BackBtn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4BackBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen4BackBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen4BackBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen4BackBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_Screen4BackBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_Screen4BackBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_Screen4BackBtn, LV_OPA_40, 0);

    ui_Screen4BackBtnLabel = lv_label_create(ui_Screen4BackBtn);
    lv_obj_set_width(ui_Screen4BackBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen4BackBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen4BackBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen4BackBtnLabel, "返回主界面");
    lv_obj_set_style_text_font(ui_Screen4BackBtnLabel, &my_font_cn, 0);
    lv_obj_set_style_text_color(ui_Screen4BackBtnLabel, lv_color_white(), 0);

    /* ===== 事件绑定 ===== */
    lv_obj_add_event_cb(ui_Screen4BackBtn, ui_event_Screen4BackBtn, LV_EVENT_ALL, NULL);
}
