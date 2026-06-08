#include "../ui.h"

extern lv_font_t my_font_cn;
LV_IMG_DECLARE(bg_screen2);

void ui_PatientListScreen_screen_init(void)
{
    ui_PatientListScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PatientListScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_img_src(ui_PatientListScreen, &bg_screen2, 0);

    /* 患者列表容器面板 */
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

    /* 搜索输入框 */
    ui_SearchInput = lv_textarea_create(ui_PatientListScreen);
    lv_obj_set_width(ui_SearchInput, 330);
    lv_obj_set_height(ui_SearchInput, 43);
    lv_obj_set_x(ui_SearchInput, 99);
    lv_obj_set_y(ui_SearchInput, -169);
    lv_obj_set_align(ui_SearchInput, LV_ALIGN_CENTER);
    lv_textarea_set_placeholder_text(ui_SearchInput, "请输入ID/姓名");
    lv_obj_set_style_text_font(ui_SearchInput, &my_font_cn, LV_PART_MAIN);

    /* 搜索按钮 */
    ui_SearchBtn = lv_btn_create(ui_PatientListScreen);
    lv_obj_set_width(ui_SearchBtn, 80);
    lv_obj_set_height(ui_SearchBtn, 36);
    lv_obj_set_x(ui_SearchBtn, 319);
    lv_obj_set_y(ui_SearchBtn, -169);
    lv_obj_set_align(ui_SearchBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_SearchBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SearchBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_SearchBtnLabel = lv_label_create(ui_SearchBtn);
    lv_label_set_text(ui_SearchBtnLabel, "搜索");
    lv_obj_set_style_text_font(ui_SearchBtnLabel, &my_font_cn, 0);

    /* 排序下拉框 */
    ui_SortDropdown = lv_dropdown_create(ui_PatientListScreen);
    lv_dropdown_set_options(ui_SortDropdown, "正序\n倒序");
    lv_obj_set_width(ui_SortDropdown, 150);
    lv_obj_set_x(ui_SortDropdown, -155);
    lv_obj_set_y(ui_SortDropdown, -168);
    lv_obj_set_align(ui_SortDropdown, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(ui_SortDropdown, LV_FONT_DEFAULT, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(ui_SortDropdown, &my_font_cn, LV_PART_MAIN);
    lv_obj_t* list = lv_dropdown_get_list(ui_SortDropdown);
    lv_obj_set_style_text_font(list, &my_font_cn, LV_PART_MAIN);

    /* 排序按钮 */
    ui_SortBtn = lv_btn_create(ui_PatientListScreen);
    lv_obj_set_width(ui_SortBtn, 100);
    lv_obj_set_height(ui_SortBtn, 40);
    lv_obj_set_x(ui_SortBtn, -292);
    lv_obj_set_y(ui_SortBtn, -169);
    lv_obj_set_align(ui_SortBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_SortBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SortBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_SortBtnLabel = lv_label_create(ui_SortBtn);
    lv_label_set_text(ui_SortBtnLabel, "排序");
    lv_obj_set_style_text_font(ui_SortBtnLabel, &my_font_cn, 0);

    /* 顶栏科室信息 */
    ui_DeptPanel2 = lv_obj_create(ui_PatientListScreen);
    lv_obj_set_width(ui_DeptPanel2, 816);
    lv_obj_set_height(ui_DeptPanel2, 38);
    lv_obj_set_x(ui_DeptPanel2, 2);
    lv_obj_set_y(ui_DeptPanel2, -223);
    lv_obj_set_align(ui_DeptPanel2, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_DeptPanel2, LV_OBJ_FLAG_SCROLLABLE);

    ui_DeptLabel2 = lv_label_create(ui_DeptPanel2);
    lv_label_set_text(ui_DeptLabel2, "当前科室：B12-131");
    lv_obj_set_style_text_font(ui_DeptLabel2, &my_font_cn, 0);
    lv_obj_center(ui_DeptLabel2);

    /* 软键盘 */
    ui_ScreenKeyboard = lv_keyboard_create(ui_PatientListScreen);
    lv_obj_set_height(ui_ScreenKeyboard, 231);
    lv_obj_set_width(ui_ScreenKeyboard, lv_pct(100));
    lv_obj_set_x(ui_ScreenKeyboard, 0);
    lv_obj_set_y(ui_ScreenKeyboard, 77);
    lv_obj_set_align(ui_ScreenKeyboard, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_ScreenKeyboard, LV_OBJ_FLAG_HIDDEN);

    /* 就诊记录按钮（灰色） */
    ui_EventListBtn = lv_btn_create(ui_PatientListScreen);
    lv_obj_set_width(ui_EventListBtn, 100);
    lv_obj_set_height(ui_EventListBtn, 30);
    lv_obj_set_x(ui_EventListBtn, -110);
    lv_obj_set_y(ui_EventListBtn, 217);
    lv_obj_set_align(ui_EventListBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_EventListBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_EventListBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_EventListBtnLabel = lv_label_create(ui_EventListBtn);
    lv_label_set_text(ui_EventListBtnLabel, "就诊记录");
    lv_obj_set_style_text_font(ui_EventListBtnLabel, &my_font_cn, 0);

    /* 返回按钮 */
    ui_BackBtn = lv_btn_create(ui_PatientListScreen);
    lv_obj_set_width(ui_BackBtn, 100);
    lv_obj_set_height(ui_BackBtn, 30);
    lv_obj_set_x(ui_BackBtn, 110);
    lv_obj_set_y(ui_BackBtn, 217);
    lv_obj_set_align(ui_BackBtn, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_BackBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_BackBtn, LV_OBJ_FLAG_SCROLLABLE);

    ui_BackBtnLabel = lv_label_create(ui_BackBtn);
    lv_label_set_text(ui_BackBtnLabel, "返回");
    lv_obj_set_style_text_font(ui_BackBtnLabel, &my_font_cn, 0);

    /* ========== 样式美化 ========== */

    /* 屏幕背景 */
    lv_obj_set_style_bg_opa(ui_PatientListScreen, LV_OPA_TRANSP, 0);

    /* 顶栏：紫色渐变 */
    lv_obj_set_style_bg_color(ui_DeptPanel2, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_DeptPanel2, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_DeptPanel2, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_DeptPanel2, 0, 0);
    lv_obj_set_style_text_color(ui_DeptLabel2, lv_color_white(), 0);

    /* 患者列表面板：白色圆角 */
    lv_obj_set_style_bg_color(ui_PatientListPanel, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_PatientListPanel, 20, 0);
    lv_obj_set_style_shadow_width(ui_PatientListPanel, 10, 0);
    lv_obj_set_style_border_width(ui_PatientListPanel, 1, 0);
    lv_obj_set_style_border_color(ui_PatientListPanel, lv_color_hex(0xe0e3e1), 0);
    lv_obj_set_style_pad_all(ui_PatientListPanel, 15, 0);
    lv_obj_set_style_pad_row(ui_PatientListPanel, 10, 0);

    /* 搜索框 */
    lv_obj_set_style_radius(ui_SearchInput, 12, 0);
    lv_obj_set_style_border_width(ui_SearchInput, 1, 0);
    lv_obj_set_style_border_color(ui_SearchInput, lv_color_hex(0xe0e3e1), 0);
    lv_obj_set_style_bg_color(ui_SearchInput, lv_color_white(), 0);

    /* 搜索按钮：紫色 */
    lv_obj_set_style_bg_color(ui_SearchBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_SearchBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_SearchBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_SearchBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_SearchBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_SearchBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_SearchBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_SearchBtnLabel, lv_color_white(), 0);

    /* 排序按钮：浅紫色 */
    lv_obj_set_style_bg_color(ui_SortBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_SortBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_SortBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_SortBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_SortBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_SortBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_SortBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_SortBtnLabel, lv_color_white(), 0);

    /* 下拉框 */
    lv_obj_set_style_bg_color(ui_SortDropdown, lv_color_white(), 0);
    lv_obj_set_style_radius(ui_SortDropdown, 12, 0);
    lv_obj_set_style_border_width(ui_SortDropdown, 1, 0);
    lv_obj_set_style_border_color(ui_SortDropdown, lv_color_hex(0xe0e3e1), 0);

    /* 就诊记录按钮：灰色 */
    lv_obj_set_style_bg_color(ui_EventListBtn, lv_color_hex(0x6b7170), 0);
    lv_obj_set_style_bg_grad_color(ui_EventListBtn, lv_color_hex(0x171a19), 0);
    lv_obj_set_style_bg_grad_dir(ui_EventListBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_EventListBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_EventListBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_EventListBtn, lv_color_hex(0x171a19), 0);
    lv_obj_set_style_shadow_opa(ui_EventListBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_EventListBtnLabel, lv_color_white(), 0);

    /* 返回按钮：蓝色（回到Screen1） */
    lv_obj_set_style_bg_color(ui_BackBtn, lv_color_hex(0x0f766e), 0);
    lv_obj_set_style_bg_grad_color(ui_BackBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_bg_grad_dir(ui_BackBtn, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_radius(ui_BackBtn, 12, 0);
    lv_obj_set_style_shadow_width(ui_BackBtn, 8, 0);
    lv_obj_set_style_shadow_color(ui_BackBtn, lv_color_hex(0x115e59), 0);
    lv_obj_set_style_shadow_opa(ui_BackBtn, LV_OPA_40, 0);
    lv_obj_set_style_text_color(ui_BackBtnLabel, lv_color_white(), 0);

    /* 事件回调绑定 */
    lv_obj_add_event_cb(ui_PatientListPanel, ui_event_PatientListPanel, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SearchInput,      ui_event_SearchInput,      LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SearchBtn,        ui_event_SearchBtn,        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SortBtn,          ui_event_SortBtn,          LV_EVENT_ALL, NULL);
    lv_keyboard_set_textarea(ui_ScreenKeyboard, ui_SearchInput);
    lv_obj_add_event_cb(ui_BackBtn,          ui_event_BackBtn,          LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_EventListBtn,     ui_event_EventListBtn,     LV_EVENT_ALL, NULL);

    /* 动画完成后刷新患者列表，避免动画渲染期间创建大量widget导致卡死 */
    lv_obj_add_event_cb(ui_PatientListScreen, ui_event_Screen2Loaded, LV_EVENT_SCREEN_LOADED, NULL);
}