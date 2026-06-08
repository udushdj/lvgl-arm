#include "../ui.h"

extern lv_font_t my_font_cn;
extern lv_font_t my_font_cn_48;
LV_IMG_DECLARE(bg_screen1);

void ui_MainScreen_screen_init(void)
{
    ui_MainScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_MainScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_img_src(ui_MainScreen, &bg_screen1, 0);

    /* 顶栏：科室信息 */
    ui_DeptPanel = lv_obj_create(ui_MainScreen);
    lv_obj_set_width(ui_DeptPanel, 825);
    lv_obj_set_height(ui_DeptPanel, 50);
    lv_obj_set_x(ui_DeptPanel, 1);
    lv_obj_set_y(ui_DeptPanel, -223);
    lv_obj_set_align(ui_DeptPanel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_DeptPanel, LV_OBJ_FLAG_SCROLLABLE);

    ui_DeptLabel = lv_label_create(ui_DeptPanel);
    lv_obj_set_width(ui_DeptLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_DeptLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_DeptLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_DeptLabel, "当前科室：B12-131");
    lv_obj_set_style_text_font(ui_DeptLabel, &my_font_cn, 0);

    /* 左侧候诊队列面板 */
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

    /* 中央当前患者信息卡片 */
    ui_CurrentPatientCard = lv_obj_create(ui_MainScreen);
    lv_obj_set_width(ui_CurrentPatientCard, 417);
    lv_obj_set_height(ui_CurrentPatientCard, 284);
    lv_obj_set_x(ui_CurrentPatientCard, -104);
    lv_obj_set_y(ui_CurrentPatientCard, -39);
    lv_obj_set_align(ui_CurrentPatientCard, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_CurrentPatientCard, LV_OBJ_FLAG_SCROLLABLE);

    /* 当前患者ID/姓名（48号大字体，叫号后闪烁目标） */
    ui_CurrentPatientLabel = lv_label_create(ui_CurrentPatientCard);
    lv_obj_set_width(ui_CurrentPatientLabel, lv_pct(45));
    lv_obj_set_height(ui_CurrentPatientLabel, lv_pct(50));
    lv_obj_set_x(ui_CurrentPatientLabel, 0);
    lv_obj_set_y(ui_CurrentPatientLabel, -13);
    lv_obj_set_align(ui_CurrentPatientLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_CurrentPatientLabel, "暂无\n患者");
    lv_obj_set_style_text_font(ui_CurrentPatientLabel, &my_font_cn_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 等待人数标签 */
    ui_WaitingCountLabel = lv_label_create(ui_CurrentPatientCard);
    lv_obj_set_width(ui_WaitingCountLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_WaitingCountLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_WaitingCountLabel, -112);
    lv_obj_set_y(ui_WaitingCountLabel, 80);
    lv_obj_set_align(ui_WaitingCountLabel, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_WaitingCountLabel, &my_font_cn, 0);

    /* 下一位患者信息标签 */
    ui_NextPatientLabel = lv_label_create(ui_CurrentPatientCard);
    lv_obj_set_width(ui_NextPatientLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_NextPatientLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_NextPatientLabel, 104);
    lv_obj_set_y(ui_NextPatientLabel, 81);
    lv_obj_set_align(ui_NextPatientLabel, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_NextPatientLabel, &my_font_cn, 0);

    /* "下一个"叫号按钮 */
    ui_CallNextBtn = lv_btn_create(ui_MainScreen);
    lv_obj_set_width(ui_CallNextBtn, 131);
    lv_obj_set_height(ui_CallNextBtn, 50);
    lv_obj_set_x(ui_CallNextBtn, -216);
    lv_obj_set_y(ui_CallNextBtn, 141);
    lv_obj_set_align(ui_CallNextBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_CallNextBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_CallNextBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_CallNextBtnLabel = lv_label_create(ui_CallNextBtn);
    lv_obj_set_width(ui_CallNextBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_CallNextBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_CallNextBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_CallNextBtnLabel, "同步");
    lv_obj_set_style_text_font(ui_CallNextBtnLabel, &my_font_cn, 0);

    /* "患者详情"导航按钮 */
    ui_PatientDetailBtn = lv_btn_create(ui_MainScreen);
    lv_obj_set_width(ui_PatientDetailBtn, 130);
    lv_obj_set_height(ui_PatientDetailBtn, 50);
    lv_obj_set_x(ui_PatientDetailBtn, 10);
    lv_obj_set_y(ui_PatientDetailBtn, 141);
    lv_obj_set_align(ui_PatientDetailBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_PatientDetailBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_PatientDetailBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_PatientDetailBtnLabel = lv_label_create(ui_PatientDetailBtn);
    lv_obj_set_width(ui_PatientDetailBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_PatientDetailBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_PatientDetailBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PatientDetailBtnLabel, "患者详情");
    lv_obj_set_style_text_font(ui_PatientDetailBtnLabel, &my_font_cn, 0);

    ui_Screen3NavBtn = lv_btn_create(ui_MainScreen);
    lv_obj_set_width(ui_Screen3NavBtn, 130);
    lv_obj_set_height(ui_Screen3NavBtn, 50);
    lv_obj_set_x(ui_Screen3NavBtn, -216);
    lv_obj_set_y(ui_Screen3NavBtn, 201);
    lv_obj_set_align(ui_Screen3NavBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Screen3NavBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_Screen3NavBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_Screen3NavBtnLabel = lv_label_create(ui_Screen3NavBtn);
    lv_obj_set_width(ui_Screen3NavBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Screen3NavBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_Screen3NavBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Screen3NavBtnLabel, "消息发送");
    lv_obj_set_style_text_font(ui_Screen3NavBtnLabel, &my_font_cn, 0);

    ui_ReservedBtn = lv_btn_create(ui_MainScreen);
    lv_obj_set_width(ui_ReservedBtn, 130);
    lv_obj_set_height(ui_ReservedBtn, 50);
    lv_obj_set_x(ui_ReservedBtn, 10);
    lv_obj_set_y(ui_ReservedBtn, 201);
    lv_obj_set_align(ui_ReservedBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ReservedBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_ReservedBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_ReservedBtnLabel = lv_label_create(ui_ReservedBtn);
    lv_obj_set_width(ui_ReservedBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_ReservedBtnLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_ReservedBtnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ReservedBtnLabel, "数据统计");
    lv_obj_set_style_text_font(ui_ReservedBtnLabel, &my_font_cn, 0);

    /* ========== 样式美化 ========== */

    /* 屏幕背景 */
    lv_obj_set_style_bg_opa(ui_MainScreen, LV_OPA_TRANSP, 0);

    /* 顶栏：蓝色渐变 */
    lv_obj_set_style_bg_color(ui_DeptPanel, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_color(ui_DeptPanel, lv_color_hex(0x14b8a6), 0);
    lv_obj_set_style_bg_grad_dir(ui_DeptPanel, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_DeptPanel, 0, 0);
    lv_obj_set_style_text_color(ui_DeptLabel, lv_color_white(), 0);

    /* 患者卡片：白色圆角阴影 */
    lv_obj_set_style_bg_color(ui_CurrentPatientCard, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_CurrentPatientCard, 20, 0);
    lv_obj_set_style_shadow_width(ui_CurrentPatientCard, 15, 0);
    lv_obj_set_style_shadow_color(ui_CurrentPatientCard, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_shadow_opa(ui_CurrentPatientCard, LV_OPA_30, 0);
    lv_obj_set_style_border_width(ui_CurrentPatientCard, 1, 0);
    lv_obj_set_style_border_color(ui_CurrentPatientCard, lv_color_hex(0xe0e3e1), 0);

    /* 卡片顶部装饰条 */
    lv_obj_t * top_line = lv_obj_create(ui_CurrentPatientCard);
    lv_obj_set_width(top_line, lv_pct(100));
    lv_obj_set_height(top_line, 8);
    lv_obj_set_align(top_line, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(top_line, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_color(top_line, lv_color_hex(0x14b8a6), 0);
    lv_obj_set_style_bg_grad_dir(top_line, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(top_line, 0, 0);
    lv_obj_set_style_border_width(top_line, 0, 0);
    lv_obj_clear_flag(top_line, LV_OBJ_FLAG_SCROLLABLE);

    /* 当前患者大标签样式 */
    lv_obj_set_style_text_color(ui_CurrentPatientLabel, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_text_align(ui_CurrentPatientLabel, LV_TEXT_ALIGN_CENTER, 0);

    /* 等待人数 / 下一患者文本颜色 */
    lv_obj_set_style_text_color(ui_WaitingCountLabel, lv_color_hex(0x6b7170), 0);
    lv_obj_set_style_text_color(ui_NextPatientLabel, lv_color_hex(0x6b7170), 0);

    /* "下一个"按钮：蓝色渐变 */
    lv_obj_set_style_bg_color(ui_CallNextBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_CallNextBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_CallNextBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_CallNextBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_CallNextBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_CallNextBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_CallNextBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_CallNextBtnLabel, lv_color_white(), 0);

    /* "患者详情"按钮：青碧色渐变 */
    lv_obj_set_style_bg_color(ui_PatientDetailBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_PatientDetailBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_PatientDetailBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_PatientDetailBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_PatientDetailBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_PatientDetailBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_text_color(ui_PatientDetailBtnLabel, lv_color_white(), 0);

    /* "消息发送"按钮：绿色渐变 */
    lv_obj_set_style_bg_color(ui_Screen3NavBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_Screen3NavBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_Screen3NavBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_Screen3NavBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_Screen3NavBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_Screen3NavBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_Screen3NavBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_Screen3NavBtnLabel, lv_color_white(), 0);

    /* "数据统计"按钮：橙色渐变 */
    lv_obj_set_style_bg_color(ui_ReservedBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_ReservedBtn, lv_color_hex(0x14b8a6), 0);
    lv_obj_set_style_bg_grad_dir(ui_ReservedBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_ReservedBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_ReservedBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_ReservedBtn, lv_color_hex(0x14b8a6), 0);
    lv_obj_set_style_shadow_opa(ui_ReservedBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_ReservedBtnLabel, lv_color_white(), 0);

    /* 候诊队列面板：白色圆角 */
    lv_obj_set_style_bg_color(ui_WaitingQueuePanel, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_WaitingQueuePanel, 20, 0);
    lv_obj_set_style_shadow_width(ui_WaitingQueuePanel, 10, 0);
    lv_obj_set_style_border_width(ui_WaitingQueuePanel, 1, 0);
    lv_obj_set_style_border_color(ui_WaitingQueuePanel, lv_color_hex(0xe0e3e1), 0);
    lv_obj_set_style_pad_all(ui_WaitingQueuePanel, 10, 0);
    lv_obj_set_style_pad_row(ui_WaitingQueuePanel, 10, 0);

    /* 事件回调绑定 */
    lv_obj_add_event_cb(ui_WaitingQueuePanel,  ui_event_WaitingQueuePanel,  LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_CurrentPatientLabel, ui_event_CurrentPatientLabel, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WaitingCountLabel,   ui_event_WaitingCountLabel,   LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_NextPatientLabel,    ui_event_NextPatientLabel,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_CallNextBtn,         ui_event_CallNextBtn,         LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PatientDetailBtn,    ui_event_PatientDetailBtn,    LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Screen3NavBtn,       ui_event_Screen3NavBtn,       LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ReservedBtn,         ui_event_ReservedBtn,         LV_EVENT_ALL, NULL);

    /* 动画完成后刷新数据，避免动画渲染期间创建大量widget导致卡死 */
    lv_obj_add_event_cb(ui_MainScreen, ui_event_Screen1Loaded, LV_EVENT_SCREEN_LOADED, NULL);
}