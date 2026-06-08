/**
 * @file ui_Screen3.c
 * @brief Screen3界面创建 —— AI消息发送 (DeepSeek对话)
 *
 * 布局: 顶栏(AI助手) + 聊天记录面板(780×310 flex列) + 输入框 + 发送/返回按钮 + 软键盘
 * 配色: 青绿色主题 (#0f766e / #115e59)
 */

#include "../ui.h"

extern lv_font_t my_font_cn;
LV_IMG_DECLARE(bg_screen3);

void ui_Screen3_screen_init(void)
{
    /* ===== 根容器 ===== */
    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_img_src(ui_Screen3, &bg_screen3, 0);

    /* ===== 全屏面板 (透明容器) ===== */
    ui_Screen3Panel = lv_obj_create(ui_Screen3);
    lv_obj_set_width(ui_Screen3Panel, 800);
    lv_obj_set_height(ui_Screen3Panel, 480);
    lv_obj_set_align(ui_Screen3Panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Screen3Panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui_Screen3Panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_Screen3Panel, 0, 0);
    lv_obj_set_style_pad_all(ui_Screen3Panel, 0, 0);

    /* ===== 顶栏 "AI助手" ===== */
    ui_Screen3TopBar = lv_obj_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3TopBar, 800);
    lv_obj_set_height(ui_Screen3TopBar, 40);
    lv_obj_set_align(ui_Screen3TopBar, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Screen3TopBar, LV_OBJ_FLAG_SCROLLABLE);

    ui_Screen3TopLabel = lv_label_create(ui_Screen3TopBar);
    lv_obj_set_width(ui_Screen3TopLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen3TopLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen3TopLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen3TopLabel, "AI助手");

    /* ===== 聊天记录面板 (flex列, 可滚动) ===== */
    ui_Screen3MiddlePanel = lv_obj_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3MiddlePanel, 780);
    lv_obj_set_height(ui_Screen3MiddlePanel, 310);
    lv_obj_set_align(ui_Screen3MiddlePanel, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_Screen3MiddlePanel, 44);
    lv_obj_set_flex_flow(ui_Screen3MiddlePanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_Screen3MiddlePanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(ui_Screen3MiddlePanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(ui_Screen3MiddlePanel, LV_DIR_VER);
    lv_obj_set_style_pad_all(ui_Screen3MiddlePanel, 8, 0);
    lv_obj_set_style_pad_row(ui_Screen3MiddlePanel, 6, 0);

    /* ===== 消息输入框 ===== */
    ui_Screen3Input = lv_textarea_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3Input, 570);
    lv_obj_set_height(ui_Screen3Input, 40);
    lv_obj_set_align(ui_Screen3Input, LV_ALIGN_TOP_MID);
    lv_obj_set_x(ui_Screen3Input, -5);
    lv_obj_set_y(ui_Screen3Input, 370);
    lv_textarea_set_placeholder_text(ui_Screen3Input, "请输入内容...");

    /* ===== 发送按钮 ===== */
    ui_Screen3SendBtn = lv_btn_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3SendBtn, 100);
    lv_obj_set_height(ui_Screen3SendBtn, 40);
    lv_obj_set_align(ui_Screen3SendBtn, LV_ALIGN_TOP_MID);
    lv_obj_set_x(ui_Screen3SendBtn, 335);
    lv_obj_set_y(ui_Screen3SendBtn, 370);
    lv_obj_add_flag(ui_Screen3SendBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_Screen3SendBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_Screen3SendBtnLabel = lv_label_create(ui_Screen3SendBtn);
    lv_obj_set_width(ui_Screen3SendBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen3SendBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen3SendBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen3SendBtnLabel, "发送");

    /* ===== 返回按钮 → Screen1 ===== */
    ui_Screen3BackBtn = lv_btn_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3BackBtn, 100);
    lv_obj_set_height(ui_Screen3BackBtn, 40);
    lv_obj_set_align(ui_Screen3BackBtn, LV_ALIGN_TOP_MID);
    lv_obj_set_x(ui_Screen3BackBtn, -345);
    lv_obj_set_y(ui_Screen3BackBtn, 370);
    lv_obj_add_flag(ui_Screen3BackBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_Screen3BackBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_Screen3BackBtnLabel = lv_label_create(ui_Screen3BackBtn);
    lv_obj_set_width(ui_Screen3BackBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen3BackBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen3BackBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen3BackBtnLabel, "返回");

    /* ===== 软键盘 (默认隐藏) ===== */
    ui_Screen3Keyboard = lv_keyboard_create(ui_Screen3Panel);
    lv_obj_set_width(ui_Screen3Keyboard, 771);
    lv_obj_set_height(ui_Screen3Keyboard, 200);
    lv_obj_set_align(ui_Screen3Keyboard, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(ui_Screen3Keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_Screen3Keyboard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_opa(ui_Screen3, LV_OPA_TRANSP, 0);

    lv_obj_set_style_bg_color(ui_Screen3TopBar, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen3TopBar, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen3TopBar, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen3TopBar, 0, 0);
    lv_obj_set_style_border_width(ui_Screen3TopBar, 0, 0);
    lv_obj_set_style_pad_all(ui_Screen3TopBar, 0, 0);
    lv_obj_set_style_text_color(ui_Screen3TopLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui_Screen3TopLabel, &my_font_cn, 0);

    lv_obj_set_style_radius(ui_Screen3Input, 12, 0);
    lv_obj_set_style_border_width(ui_Screen3Input, 1, 0);
    lv_obj_set_style_border_color(ui_Screen3Input, lv_color_hex(0xe0e3e1), 0);
    lv_obj_set_style_bg_color(ui_Screen3Input, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui_Screen3Input, &my_font_cn, LV_PART_MAIN);

    lv_obj_set_style_bg_color(ui_Screen3MiddlePanel, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_Screen3MiddlePanel, 20, 0);
    lv_obj_set_style_border_width(ui_Screen3MiddlePanel, 1, 0);
    lv_obj_set_style_border_color(ui_Screen3MiddlePanel, lv_color_hex(0xe0e3e1), 0);
    lv_obj_set_style_shadow_width(ui_Screen3MiddlePanel, 10, 0);

    lv_obj_set_style_bg_color(ui_Screen3SendBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen3SendBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen3SendBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen3SendBtn, 12, 0);
    lv_obj_set_style_text_color(ui_Screen3SendBtnLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui_Screen3SendBtnLabel, &my_font_cn, 0);

    lv_obj_set_style_bg_color(ui_Screen3BackBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen3BackBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen3BackBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen3BackBtn, 12, 0);
    lv_obj_set_style_text_color(ui_Screen3BackBtnLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui_Screen3BackBtnLabel, &my_font_cn, 0);

    lv_obj_add_event_cb(ui_Screen3Input, ui_event_Screen3Input, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Screen3SendBtn, ui_event_Screen3SendBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Screen3BackBtn, ui_event_Screen3BackBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Screen3, on_screen3_delete, LV_EVENT_DELETE, NULL);
}
