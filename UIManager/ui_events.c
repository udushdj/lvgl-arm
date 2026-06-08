/**
 * @file ui_events.c
 * @brief 全部业务逻辑实现: 界面刷新、数据排序搜索、闪烁动画、AI对话
 * @author [wz]
 * @date 2024-03-27
 *
 * 模块划分:
 *   1. 按钮样式辅助函数 (style_wide_btn / style_pin_btn / style_queue_btn)
 *   2. 闪烁提示功能 (blink_timer_cb + start_blink)
 *   3. Screen1: 主叫号界面业务 (5个刷新函数 + call_next_patient)
 *   4. Screen2: 患者管理业务 (refresh/search/sort/pin + 就诊记录)
 *   5. Screen4: 统计刷新
 *   6. Screen3: AI对话 (pthread + SSL + 聊天气泡)
 *
 * 重要约定:
 *   - 所有 refresh_* 函数接收 lv_event_t* 参数 (可为NULL表示直接调用)
 *   - 刷新函数内部防御性重建控件 (if NULL then create)
 *   - 非UI线程操作 (AI http) 使用 volatile flag + LVGL timer 轮询结果
 */

#include "ui.h"
#include "PatientManager/inc/linked_list.h"
#include "PatientManager/inc/stats.h"
#include "ai_chat.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* 字体测试完毕，恢复原字体 */

extern lv_font_t my_font_cn;        /* 16px 中文仿宋字体 */
extern lv_font_t my_font_cn_48;     /* 48px 中文仿宋字体 (大号叫号显示) */

/* ================================================================ */
/*                   1. 按钮样式辅助函数                               */
/* ================================================================ */

/**
 * @brief 宽按钮通用样式: 白色底 + 青灰边框 + 阴影 + 水平渐变
 *
 * 用于Screen2患者列表中的每个患者按钮 (672px宽)。
 */
static void style_wide_btn(lv_obj_t *btn)
{
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xccfbf1), 0);
    lv_obj_set_style_shadow_width(btn, 4, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0xf0fdfa), 0);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, 0);
}

/**
 * @brief "置顶"小按钮样式: 深青色底 + 白色文字
 */
static void style_pin_btn(lv_obj_t *btn)
{
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(btn, 0, 0);
}

/**
 * @brief Screen1等待队列按钮样式
 * @param is_wide 宽模式有水平渐变背景，窄模式仅白色底
 */
static void style_queue_btn(lv_obj_t *btn, bool is_wide)
{
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xccfbf1), 0);
    lv_obj_set_style_shadow_width(btn, 4, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
    if (is_wide) {
        lv_obj_set_style_bg_grad_color(btn, lv_color_hex(0xf0fdfa), 0);
        lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, 0);
    }
}

/* 前向声明 */
static void on_pin_patient(lv_event_t * e);

/* ================================================================ */
/*                   2. 闪烁提示功能                                  */
/*  当患者更新时，当前患者标签闪烁3次 (6个半周期 × 300ms = 1.8s)      */
/* ================================================================ */

static lv_timer_t * blink_timer = NULL;  /* 闪烁定时器句柄 */
static int blink_count = 0;              /* 当前闪烁次数 (0~5) */
#define BLINK_TOTAL_COUNT 6              /* 总闪烁半周期数 (3次完整闪烁) */
#define BLINK_INTERVAL_MS 300            /* 闪烁间隔 (毫秒) */

/**
 * @brief 闪烁定时器回调: 交替切换当前患者标签的透明度
 *
 * 偶数次 → 透明 (LV_OPA_30), 奇数次 → 不透明 (LV_OPA_COVER)
 * 完成 BLINK_TOTAL_COUNT 次后自动停止并恢复不透明。
 */
static void blink_timer_cb(lv_timer_t * timer)
{
    /* 防御: 标签可能已被外部删除 */
    if (ui_CurrentPatientLabel == NULL) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        return;
    }

    if (blink_count >= BLINK_TOTAL_COUNT) {
        lv_obj_set_style_opa(ui_CurrentPatientLabel, LV_OPA_COVER, 0);
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        return;
    }

    if (blink_count % 2 == 0) {
        lv_obj_set_style_opa(ui_CurrentPatientLabel, LV_OPA_30, 0);
    } else {
        lv_obj_set_style_opa(ui_CurrentPatientLabel, LV_OPA_COVER, 0);
    }
    blink_count++;
}

/**
 * @brief 启动闪烁动画 (如果已有闪烁则先停止再重新开始)
 */
static void start_blink(void)
{
    if (blink_timer != NULL) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
    }

    if (ui_CurrentPatientLabel == NULL) return;

    blink_count = 0;
    blink_timer = lv_timer_create(blink_timer_cb, BLINK_INTERVAL_MS, NULL);
}

/* ================================================================ */
/*              3. Screen1：主叫号界面业务逻辑                         */
/* ================================================================ */

/**
 * @brief 刷新等待队列面板 —— 遍历 patient_queue 渲染按钮列表
 *
 * 实现细节:
 *   - 防御性重建: 如果 ui_WaitingQueuePanel 为NULL，从零创建面板
 *   - lv_obj_clean 清除旧按钮再重建，避免内存泄漏
 *   - 空队列时显示"暂无候诊患者"占位文字
 *   - 每个患者一个按钮，显示 "Patient_ID：姓名"
 */
void refresh_waiting_queue(lv_event_t * e)
{
    extern P_List patient_queue;

    /* 防御性重建面板 (正常情况下面板在 ui_Screen1.c 中已创建) */
    if (ui_WaitingQueuePanel == NULL) {
        ui_WaitingQueuePanel = lv_obj_create(ui_MainScreen);
        lv_obj_set_width(ui_WaitingQueuePanel, 209);
        lv_obj_set_height(ui_WaitingQueuePanel, 380);
        lv_obj_set_x(ui_WaitingQueuePanel, 231);
        lv_obj_set_y(ui_WaitingQueuePanel, 3);
        lv_obj_set_align(ui_WaitingQueuePanel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_WaitingQueuePanel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(ui_WaitingQueuePanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_scrollbar_mode(ui_WaitingQueuePanel, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(ui_WaitingQueuePanel, LV_DIR_VER);
    }

    /* 先清除面板内所有旧控件 */
    lv_obj_clean(ui_WaitingQueuePanel);

    /* 空队列: 显示占位提示 */
    if (patient_queue == NULL || patient_queue->size == 0) {
        lv_obj_t *label = lv_label_create(ui_WaitingQueuePanel);
        lv_label_set_text(label, "暂无候诊患者");
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        return;
    }

    /* 遍历链表 (跳过头结点) 创建按钮 */
    P_Node node = patient_queue->head->next;
    while (node != NULL) {
        lv_obj_t *btn = lv_btn_create(ui_WaitingQueuePanel);
        lv_obj_set_width(btn, 175);
        lv_obj_set_height(btn, 50);
        lv_obj_set_align(btn, LV_ALIGN_CENTER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        style_queue_btn(btn, false);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%ld：%s", node->Patient_ID, node->name);
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0x171a19), 0);
        node = node->next;
    }
}

/**
 * @brief 刷新当前患者大标签 —— 显示队列第一个患者 (48px大字体)
 *
 * 每次刷新后启动闪烁动画提示用户注意。
 * 队列为空时显示"无当前患者"。
 */
void refresh_current_patient(lv_event_t * e)
{
    extern P_List patient_queue;

    /* 防御性重建 */
    if (ui_CurrentPatientLabel == NULL) {
        ui_CurrentPatientLabel = lv_label_create(ui_CurrentPatientCard);
        lv_obj_set_width(ui_CurrentPatientLabel, lv_pct(45));
        lv_obj_set_height(ui_CurrentPatientLabel, lv_pct(50));
        lv_obj_set_x(ui_CurrentPatientLabel, 0);
        lv_obj_set_y(ui_CurrentPatientLabel, -13);
        lv_obj_set_align(ui_CurrentPatientLabel, LV_ALIGN_CENTER);
    }

    lv_obj_set_style_text_font(ui_CurrentPatientLabel,
                               &my_font_cn_48,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    if (patient_queue != NULL && patient_queue->size > 0) {
        P_Node first = patient_queue->head->next;
        lv_label_set_text_fmt(ui_CurrentPatientLabel,
                              "%ld\n%s",
                              first->Patient_ID,
                              first->name);
    } else {
        lv_label_set_text(ui_CurrentPatientLabel, "无当前患者");
    }

    start_blink();
}

/**
 * @brief 刷新等待人数标签
 */
void refresh_waiting_count(lv_event_t * e)
{
    extern P_List patient_queue;

    if (ui_WaitingCountLabel == NULL) {
        ui_WaitingCountLabel = lv_label_create(ui_CurrentPatientCard);
        lv_obj_set_width(ui_WaitingCountLabel, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_WaitingCountLabel, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_WaitingCountLabel, -112);
        lv_obj_set_y(ui_WaitingCountLabel, 80);
        lv_obj_set_align(ui_WaitingCountLabel, LV_ALIGN_CENTER);
        lv_obj_set_style_text_font(ui_WaitingCountLabel, &my_font_cn, 0);
    }

    if (patient_queue != NULL) {
        lv_label_set_text_fmt(ui_WaitingCountLabel,
                              "等待人数：%d", patient_queue->size);
    } else {
        lv_label_set_text(ui_WaitingCountLabel, "等待人数：0");
    }
}

/**
 * @brief 刷新下一位患者标签 —— 显示队列第二个患者
 *
 * 需要队列至少2个患者才显示，否则显示"无候诊患者"。
 */
void refresh_next_patient(lv_event_t * e)
{
    extern P_List patient_queue;

    if (ui_NextPatientLabel == NULL) {
        ui_NextPatientLabel = lv_label_create(ui_CurrentPatientCard);
        lv_obj_set_width(ui_NextPatientLabel, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_NextPatientLabel, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_NextPatientLabel, 104);
        lv_obj_set_y(ui_NextPatientLabel, 81);
        lv_obj_set_align(ui_NextPatientLabel, LV_ALIGN_CENTER);
        lv_obj_set_style_text_font(ui_NextPatientLabel, &my_font_cn, 0);
    }

    if (patient_queue != NULL && patient_queue->size > 1) {
        P_Node next_patient = patient_queue->head->next->next;
        lv_label_set_text_fmt(ui_NextPatientLabel,
                              "%ld\n%s",
                              next_patient->Patient_ID,
                              next_patient->name);
    } else {
        lv_label_set_text(ui_NextPatientLabel, "无候诊患者");
    }
}

/**
 * @brief 叫号操作: 弹出队列第一个患者并更新全部UI
 *
 * 执行顺序:
 *   1. list_pop_front 弹出并返回第一个患者节点
 *   2. free 释放该节点内存
 *   3. 更新当前患者、等待人数、下一位三个标签
 *   4. 触发闪烁动画
 *   5. 重建等待队列面板
 *
 * 注意: 此函数仅修改本地链表，不通知服务器。
 *       服务器同步由 sync_trigger / sync_poll 处理。
 */
void call_next_patient(lv_event_t * e)
{
    extern P_List patient_queue;

    P_Node called = list_pop_front(patient_queue);
    if (called != NULL) {
        free(called);
    }

    /* 更新当前患者标签 */
    if (ui_CurrentPatientLabel != NULL) {
        if (patient_queue != NULL && patient_queue->size > 0) {
            P_Node first = patient_queue->head->next;
            lv_label_set_text_fmt(ui_CurrentPatientLabel,
                                  "%ld\n%s",
                                  first->Patient_ID,
                                  first->name);
        } else {
            lv_label_set_text(ui_CurrentPatientLabel, "无当前患者");
        }
    }

    /* 更新等待人数 */
    if (ui_WaitingCountLabel != NULL) {
        if (patient_queue != NULL) {
            lv_label_set_text_fmt(ui_WaitingCountLabel,
                                  "等待人数：%d", patient_queue->size);
        } else {
            lv_label_set_text(ui_WaitingCountLabel, "等待人数：0");
        }
    }

    start_blink();

    /* 更新下一位患者 */
    if (ui_NextPatientLabel != NULL) {
        if (patient_queue != NULL && patient_queue->size > 1) {
            P_Node next_patient = patient_queue->head->next->next;
            lv_label_set_text_fmt(ui_NextPatientLabel,
                                  "%ld\n%s",
                                  next_patient->Patient_ID,
                                  next_patient->name);
        } else {
            lv_label_set_text(ui_NextPatientLabel, "无候诊患者");
        }
    }

    /* 重建等待队列面板按钮 */
    refresh_waiting_queue(NULL);
}

/* ================================================================ */
/*              4. Screen2：患者管理界面业务逻辑                       */
/* ================================================================ */

/**
 * @brief Screen2删除事件回调: 将面板指针置NULL
 *
 * 当Screen2被外部代码 lv_obj_del 时，通过 LV_EVENT_DELETE 事件通知，
 * 将全局指针置NULL防止悬空引用。
 */
static void on_screen2_delete(lv_event_t * e)
{
    ui_PatientListPanel = NULL;
}

/**
 * @brief 刷新患者列表 —— 遍历 patient_queue 在Screen2中渲染
 *
 * 每个患者行包含:
 *   - 宽按钮显示 "Patient_ID：姓名" (左对齐，flex-grow填充)
 *   - "置顶"小按钮 (右侧，点击将该患者移到队列第一位)
 *
 * 这是"患者详情"按钮点击后执行的关键函数。
 * 如果此函数卡住，说明 patient_queue 链表可能形成了环或者节点数据被破坏。
 */
void refresh_patient_list(lv_event_t * e)
{
    extern P_List patient_queue;

    printf("[UI] refresh_patient_list START, panel=%p, queue=%p, size=%d\n",
           (void*)ui_PatientListPanel, (void*)patient_queue,
           patient_queue ? patient_queue->size : -1);

    /* 防御性重建面板 */
    if (ui_PatientListPanel == NULL) {
        printf("[UI] creating PatientListPanel\n");
        ui_PatientListPanel = lv_obj_create(ui_PatientListScreen);
        lv_obj_set_width(ui_PatientListPanel, 717);
        lv_obj_set_height(ui_PatientListPanel, 290);
        lv_obj_set_x(ui_PatientListPanel, 4);
        lv_obj_set_y(ui_PatientListPanel, 50);
        lv_obj_set_align(ui_PatientListPanel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_PatientListPanel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(ui_PatientListPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_scrollbar_mode(ui_PatientListPanel, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(ui_PatientListPanel, LV_DIR_VER);
        lv_obj_add_event_cb(ui_PatientListScreen, on_screen2_delete, LV_EVENT_DELETE, NULL);
    }

    /* 清除旧内容 */
    lv_obj_clean(ui_PatientListPanel);

    if (patient_queue == NULL || patient_queue->size == 0) {
        lv_obj_t *label = lv_label_create(ui_PatientListPanel);
        lv_label_set_text(label, "暂无候诊患者");
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        printf("[UI] refresh_patient_list DONE (empty)\n");
        return;
    }

    /* 遍历链表创建按钮行 */
    P_Node node = patient_queue->head->next;
    while (node != NULL) {
        /* 外层按钮: 整行容器 (flex row布局) */
        lv_obj_t *btn = lv_btn_create(ui_PatientListPanel);
        lv_obj_set_width(btn, 672);
        lv_obj_set_height(btn, 50);
        lv_obj_set_align(btn, LV_ALIGN_CENTER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_left(btn, 16, 0);
        lv_obj_set_style_pad_right(btn, 4, 0);

        style_wide_btn(btn);

        /* 患者信息标签 (flex-grow=1 填充剩余空间) */
        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_width(label, LV_SIZE_CONTENT);
        lv_obj_set_height(label, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(label, 1);
        lv_label_set_text_fmt(label, "%ld：%s", node->Patient_ID, node->name);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0x171a19), 0);

        /* "置顶"按钮: 携带 Patient_ID 作为 user_data */
        lv_obj_t *pin_btn = lv_btn_create(btn);
        lv_obj_set_width(pin_btn, 56);
        lv_obj_set_height(pin_btn, 30);
        style_pin_btn(pin_btn);

        lv_obj_t *pin_label = lv_label_create(pin_btn);
        lv_label_set_text(pin_label, "置顶");
        lv_obj_center(pin_label);
        lv_obj_set_style_text_font(pin_label, &my_font_cn, 0);
        lv_obj_set_style_text_color(pin_label, lv_color_white(), 0);

        lv_obj_add_event_cb(pin_btn, on_pin_patient, LV_EVENT_CLICKED, (void*)(long)node->Patient_ID);

        node = node->next;
    }
    printf("[UI] refresh_patient_list DONE\n");
}

/**
 * @brief "置顶"按钮回调: 将指定Patient_ID的患者移至队列第一位
 *
 * 使用 list_move_to_top 原地修改链表，然后刷新列表显示。
 * 注意: user_data 被转型为 long 存储 Patient_ID，仅适用于64位平台或ID小于2^31。
 */
static void on_pin_patient(lv_event_t * e)
{
    extern P_List patient_queue;

    long patient_id = (long)lv_event_get_user_data(e);

    if (patient_queue == NULL) return;

    /* 查找目标患者 */
    P_Node node = patient_queue->head->next;
    while (node != NULL) {
        if (node->Patient_ID == patient_id) {
            list_move_to_top(patient_queue, node);
            break;
        }
        node = node->next;
    }

    refresh_patient_list(NULL);
}

/**
 * @brief 排序患者列表 —— 根据下拉框选择进行升序/降序
 *
 * 排序方式:
 *   - 选中索引0: 按Patient_ID升序 (cmp_by_ID)
 *   - 选中索引1: 按Patient_ID降序 (cmp_by_ID_desc)
 *
 * 排序后自动刷新列表显示。
 */
void sort_patient_list(lv_event_t * e)
{
    extern P_List patient_queue;
    if (patient_queue == NULL || patient_queue->size == 0 || ui_SortDropdown == NULL) return;

    uint16_t selected = lv_dropdown_get_selected(ui_SortDropdown);

    CompareFunc sorter = NULL;
    if (selected == 0) {
        sorter = cmp_by_ID;       /* 升序 */
    } else if (selected == 1) {
        sorter = cmp_by_ID_desc;  /* 降序 */
    }

    if (sorter != NULL) {
        list_sort(patient_queue, sorter);
        refresh_patient_list(NULL);
    }
}

/**
 * @brief 搜索患者 —— 按姓名或Patient_ID搜索
 *
 * 搜索策略 (三级匹配):
 *   1. 精确匹配: 姓名或ID完全等于搜索词 → 绿色高亮按钮
 *   2. 模糊匹配: 姓名或ID包含搜索词 → 普通白色按钮列表
 *   3. 无匹配: 显示"未找到匹配的患者"
 *
 * 搜索为空字符串时显示提示。
 * 模糊匹配时通过 malloc+memcpy 创建临时节点副本 (最后free释放)。
 */
void search_patient(lv_event_t * e)
{
    extern P_List patient_queue;
    const char * search_key = lv_textarea_get_text(ui_SearchInput);

    /* 空搜索 → 提示输入 */
    if (search_key == NULL || strlen(search_key) == 0) {
        lv_obj_clean(ui_PatientListPanel);
        lv_obj_t * tip_label = lv_label_create(ui_PatientListPanel);
        lv_label_set_text(tip_label, "请输入姓名或编号进行搜索");
        lv_obj_center(tip_label);
        lv_obj_set_style_text_font(tip_label, &my_font_cn, 0);
        return;
    }

    lv_obj_clean(ui_PatientListPanel);

    if (patient_queue == NULL) {
        lv_obj_t *empty_label = lv_label_create(ui_PatientListPanel);
        lv_label_set_text(empty_label, "暂无患者");
        lv_obj_center(empty_label);
        return;
    }

    /* 遍历链表，分别收集精确匹配和模糊匹配 */
    P_Node current = patient_queue->head->next;
    P_Node exact_match = NULL;       /* 第一个精确匹配节点 */
    P_Node partial_list = NULL;      /* 模糊匹配临时链表头 */
    P_Node partial_tail = NULL;      /* 模糊匹配临时链表尾 */

    while (current != NULL) {
        char patient_id_str[20];
        snprintf(patient_id_str, sizeof(patient_id_str), "%ld", current->Patient_ID);

        /* 精确匹配: 姓名或ID完全相同 */
        if (strcmp(current->name, search_key) == 0 || strcmp(patient_id_str, search_key) == 0) {
            exact_match = current;
            break; /* 找到精确匹配立即停止 */
        }

        /* 模糊匹配: 姓名或ID包含搜索词 */
        if (strstr(current->name, search_key) != NULL || strstr(patient_id_str, search_key) != NULL) {
            /* 创建临时副本 (因为原链表节点不能被free) */
            P_Node temp_node = (P_Node)malloc(sizeof(struct patient));
            memcpy(temp_node, current, sizeof(struct patient));
            temp_node->next = NULL;

            if (partial_list == NULL) {
                partial_list = temp_node;
                partial_tail = temp_node;
            } else {
                partial_tail->next = temp_node;
                partial_tail = temp_node;
            }
        }

        current = current->next;
    }

    /* 渲染结果: 精确匹配优先 */
    if (exact_match != NULL) {
        lv_obj_t * result_btn = lv_btn_create(ui_PatientListPanel);
        lv_obj_set_width(result_btn, 680);
        lv_obj_set_height(result_btn, 50);
        lv_obj_set_align(result_btn, LV_ALIGN_CENTER);
        lv_obj_add_flag(result_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(result_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(result_btn, 12, 0);
        lv_obj_set_style_bg_color(result_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_shadow_width(result_btn, 4, 0);
        lv_obj_set_style_text_color(result_btn, lv_color_white(), 0);

        lv_obj_t * result_label = lv_label_create(result_btn);
        lv_label_set_text_fmt(result_label,
                              "精确匹配|姓名：%s|编号：%ld|年龄：%d",
                              exact_match->name,
                              exact_match->Patient_ID,
                              exact_match->age);
        lv_obj_center(result_label);
        lv_obj_set_style_text_font(result_label, &my_font_cn, 0);
        lv_obj_set_style_text_color(result_label, lv_color_white(), 0);
    }
    else if (partial_list != NULL) {
        /* 渲染模糊匹配结果并释放临时副本 */
        P_Node temp = partial_list;
        while (temp != NULL) {
            lv_obj_t * result_btn = lv_btn_create(ui_PatientListPanel);
            lv_obj_set_width(result_btn, 680);
            lv_obj_set_height(result_btn, 50);
            lv_obj_set_align(result_btn, LV_ALIGN_CENTER);
            lv_obj_add_flag(result_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
            lv_obj_clear_flag(result_btn, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(result_btn, 12, 0);
            lv_obj_set_style_bg_color(result_btn, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_border_width(result_btn, 1, 0);
            lv_obj_set_style_border_color(result_btn, lv_color_hex(0xccfbf1), 0);

            lv_obj_t * result_label = lv_label_create(result_btn);
            lv_label_set_text_fmt(result_label,
                                  "姓名：%s|编号：%ld|年龄：%d",
                                  temp->name,
                                  temp->Patient_ID,
                                  temp->age);
            lv_obj_center(result_label);
            lv_obj_set_style_text_font(result_label, &my_font_cn, 0);
            lv_obj_set_style_text_color(result_label, lv_color_hex(0x171a19), 0);

            P_Node next_node = temp->next;
            free(temp);     /* 释放临时副本 */
            temp = next_node;
        }
    }
    else {
        /* 无匹配结果 */
        lv_obj_t * empty_label = lv_label_create(ui_PatientListPanel);
        lv_label_set_text(empty_label, "未找到匹配的患者");
        lv_obj_center(empty_label);
        lv_obj_set_style_text_font(empty_label, &my_font_cn, 0);
    }
}

/* ================================================================ */
/*              5. Screen2：就诊记录                                   */
/* ================================================================ */

/**
 * @brief 刷新就诊记录列表 —— 显示最近叫号事件
 *
 * 数据来源: stats模块的 recent_events[] 数组 (由sync从stats.json解析)
 * 每条记录显示: Patient_ID、姓名、日期、开始~结束时间
 */
void refresh_event_list(lv_event_t * e)
{
    extern P_List patient_queue;

    if (ui_PatientListPanel == NULL) return;

    lv_obj_clean(ui_PatientListPanel);

    if (recent_event_count == 0) {
        lv_obj_t *label = lv_label_create(ui_PatientListPanel);
        lv_label_set_text(label, "暂无就诊记录");
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        return;
    }

    for (int i = 0; i < recent_event_count; i++) {
        lv_obj_t *btn = lv_btn_create(ui_PatientListPanel);
        lv_obj_set_width(btn, 672);
        lv_obj_set_height(btn, 50);
        lv_obj_set_align(btn, LV_ALIGN_CENTER);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        style_wide_btn(btn);

        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_width(label, LV_SIZE_CONTENT);
        lv_obj_set_height(label, LV_SIZE_CONTENT);
        lv_label_set_text_fmt(label, "%ld：%s（%s %s~%s）",
                              recent_events[i].Patient_ID,
                              recent_events[i].name,
                              recent_events[i].date,
                              recent_events[i].start_time,
                              recent_events[i].end_time);
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &my_font_cn, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0x171a19), 0);
    }
}

/* ================================================================ */
/*              6. Screen4：统计看板刷新                               */
/* ================================================================ */

/**
 * @brief 刷新Screen4统计数据 —— 更新3个数字标签 + 重绘柱状图
 *
 * 数据来源:
 *   - today_called:  今日已叫号人数 (来自stats.json)
 *   - avg_wait_min:  平均等待时间(分钟) (来自stats.json)
 *   - patient_queue->size: 当前等待人数 (本地链表)
 */
void refresh_screen4_stats(void)
{
    if (!ui_Screen4StatNum1 || !ui_Screen4StatNum2 || !ui_Screen4StatNum3) return;

    char buf[32];

    snprintf(buf, sizeof(buf), "%d 人", today_called);
    lv_label_set_text(ui_Screen4StatNum1, buf);

    snprintf(buf, sizeof(buf), "%d 分钟", avg_wait_min);
    lv_label_set_text(ui_Screen4StatNum2, buf);

    extern P_List patient_queue;
    snprintf(buf, sizeof(buf), "%d 人", patient_queue ? patient_queue->size : 0);
    lv_label_set_text(ui_Screen4StatNum3, buf);

    screen4_draw_flow_bars(); /* 按 hourly_calls[8] 重绘柱状图 */
}

/* ================================================================ */
/*              7. Screen3：AI对话 (DeepSeek API)                     */
/* ================================================================ */

static pthread_t ai_thread;                    /* AI请求线程 */
static volatile int ai_busy = 0;               /* 忙标志 (非UI线程写入，主线程读取) */
static char ai_resp_buf[AI_RESPONSE_MAX];      /* AI响应缓冲区 */
static lv_timer_t *ai_poll_timer = NULL;       /* 轮询定时器 (每200ms检查线程完成) */
static lv_obj_t *thinking_label = NULL;         /* "等待中..." 标签 */

/**
 * @brief 在聊天面板中添加一个聊天气泡
 * @param text   消息文本内容
 * @param is_user 1=用户消息(右对齐/深青色), 0=AI回复(左对齐/浅灰色)
 *
 * 气泡宽度根据文字自动计算 (lv_txt_get_size)，最大540px，最小60px。
 * 文字使用 LV_LABEL_LONG_WRAP 模式自动换行。
 */
static void add_chat_bubble(const char *text, int is_user)
{
    AI_LOG("add_chat_bubble: panel=%p, text='%s', is_user=%d",
           (void*)ui_Screen3MiddlePanel, text ? text : "(null)", is_user);
    if (ui_Screen3MiddlePanel == NULL) return;

    /* 行容器 (flex row, 控制左右对齐) */
    lv_obj_t *row = lv_obj_create(ui_Screen3MiddlePanel);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(row, 2, 0);

    if (is_user) {
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    } else {
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }

    /* 气泡容器 */
    lv_obj_t *bubble = lv_obj_create(row);
    lv_obj_remove_style_all(bubble);
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(bubble, 10, 0);
    lv_obj_set_style_radius(bubble, 12, 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);

    if (is_user) {
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0x0f766e), 0);  /* 深青色 */
    } else {
        lv_obj_set_style_bg_color(bubble, lv_color_hex(0xf3f4f3), 0);  /* 浅灰色 */
    }

    /* 计算气泡宽度: 文字宽度 + 24px padding, 限制在60~540px */
    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, text, &my_font_cn, 0, 0, LV_COORD_MAX, 0);
    int bubble_w = txt_size.x + 24;
    if (bubble_w > 540) bubble_w = 540;
    if (bubble_w < 60) bubble_w = 60;
    lv_obj_set_width(bubble, bubble_w);
    AI_LOG("bubble size: w=%d, txt_w=%d", bubble_w, (int)txt_size.x);

    /* 文字标签 (自动换行) */
    lv_obj_t *label = lv_label_create(bubble);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &my_font_cn, 0);
    lv_obj_set_width(label, LV_PCT(100));

    if (is_user) {
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x171a19), 0);
    }
}

/**
 * @brief AI请求线程函数 —— 在独立pthread中执行HTTPS请求
 *
 * 执行流程:
 *   1. 调用 ai_chat_send() 通过OpenSSL发送请求 (阻塞，最多15秒超时)
 *   2. 结果写入全局 ai_resp_buf
 *   3. 设置 ai_busy = 0 通知主线程
 *   4. free 消息副本
 */
static void *ai_thread_fn(void *arg)
{
    char *msg = (char *)arg;
    AI_LOG("thread start: msg='%s'", msg);
    if (ai_chat_send(msg, ai_resp_buf, sizeof(ai_resp_buf)) != 0) {
        AI_LOG("ai_chat_send FAILED: %s", ai_chat_last_error());
        snprintf(ai_resp_buf, sizeof(ai_resp_buf), "请求出错");
    } else {
        AI_LOG("ai_chat_send OK: resp='%s'", ai_resp_buf);
    }
    ai_busy = 0;
    free(msg);
    return NULL;
}

/**
 * @brief AI轮询定时器回调 —— 每200ms检查AI线程是否完成
 *
 * 当 ai_busy == 0 时:
 *   1. 停止并销毁定时器
 *   2. 移除"等待中..."标签
 *   3. 添加AI回复气泡
 *   4. 滚动到底部
 */
static void ai_poll_cb(lv_timer_t *timer)
{
    if (ai_busy) return; /* AI线程还在运行，继续等待 */

    lv_timer_del(ai_poll_timer);
    ai_poll_timer = NULL;

    AI_LOG("poll: ai_busy=0, panel=%p", (void*)ui_Screen3MiddlePanel);

    if (ui_Screen3MiddlePanel == NULL) return;

    if (thinking_label) {
        lv_obj_del(thinking_label);
        thinking_label = NULL;
    }

    add_chat_bubble(ai_resp_buf, 0);
    lv_obj_scroll_to_y(ui_Screen3MiddlePanel, LV_COORD_MAX, LV_ANIM_ON);
}

/**
 * @brief Screen3发送按钮回调 —— 启动AI对话请求
 *
 * 完整流程:
 *   1. 检查 ai_busy 标志防止重复发送
 *   2. 获取输入框文本，添加到聊天面板 (用户气泡)
 *   3. 清空输入框，隐藏键盘
 *   4. 显示"等待中..."提示
 *   5. 创建pthread异步发送AI请求 (pthread_detach 自动回收)
 *   6. 创建200ms轮询定时器检查结果
 *
 * 线程安全说明:
 *   - ai_busy 使用 volatile 修饰
 *   - 主线程写入 (设为1), AI线程读取后写入 (设为0)
 *   - 简单标志位无需mutex，但在严格标准下应使用 atomic
 */
void on_screen3_send(lv_event_t *e)
{
    AI_LOG("on_screen3_send: ai_busy=%d, input=%p, panel=%p",
           ai_busy, (void*)ui_Screen3Input, (void*)ui_Screen3MiddlePanel);
    if (ai_busy) return;
    if (ui_Screen3Input == NULL || ui_Screen3MiddlePanel == NULL) return;

    const char *text = lv_textarea_get_text(ui_Screen3Input);
    if (!text || strlen(text) == 0) return;

    AI_LOG("sending: '%s'", text);

    char *msg_copy = strdup(text); /* 线程需要独立副本 */

    /* 添加用户消息气泡 */
    add_chat_bubble(text, 1);
    lv_textarea_set_text(ui_Screen3Input, "");
    lv_obj_add_flag(ui_Screen3Keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_scroll_to_y(ui_Screen3MiddlePanel, LV_COORD_MAX, LV_ANIM_ON);

    /* 显示等待提示 */
    thinking_label = lv_label_create(ui_Screen3MiddlePanel);
    lv_label_set_text(thinking_label, "等待中...");
    lv_obj_set_style_text_font(thinking_label, &my_font_cn, 0);
    lv_obj_set_style_text_color(thinking_label, lv_color_hex(0x9da3a0), 0);
    lv_obj_scroll_to_y(ui_Screen3MiddlePanel, LV_COORD_MAX, LV_ANIM_ON);

    ai_busy = 1;

    /* 创建独立线程执行HTTPS请求 */
    pthread_create(&ai_thread, NULL, ai_thread_fn, msg_copy);
    pthread_detach(ai_thread);

    /* 创建轮询定时器 (无限制重复次数，由 ai_poll_cb 主动停止) */
    ai_poll_timer = lv_timer_create(ai_poll_cb, 200, NULL);
    lv_timer_set_repeat_count(ai_poll_timer, -1);
}

/**
 * @brief Screen3删除事件回调: 清理AI轮询状态
 *
 * 当Screen3被卸载时，停止轮询定时器并将相关指针置NULL。
 */
void on_screen3_delete(lv_event_t *e)
{
    ui_Screen3MiddlePanel = NULL;
    if (ai_poll_timer) {
        lv_timer_del(ai_poll_timer);
        ai_poll_timer = NULL;
    }
    thinking_label = NULL;
}
