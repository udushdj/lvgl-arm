/**
 * @file ui.c
 * @brief UI事件回调函数 & 全局对象定义 & 全局初始化
 * @author SquareLine Studio + 手动修改
 * @date 2024-03-27
 *
 * 架构说明:
 *   1. 全局 lv_obj_t* 变量由 ui.h 声明，ui.c 定义
 *   2. 每个可交互的控件都有一个对应的 ui_event_XXX 回调函数
 *   3. 回调函数在 SquareLine Studio 生成的代码基础上扩展了业务逻辑
 *   4. ui_init() 预创建所有4个屏幕，主屏立即加载
 *
 * 事件回调约定:
 *   - 所有回调绑定 LV_EVENT_ALL，在函数内部通过 event_code 筛选
 *   - 当前只响应 LV_EVENT_CLICKED (触摸点击)
 *   - 某些控件额外处理 LV_EVENT_FOCUSED / LV_EVENT_DEFOCUSED (文本框键盘弹出)
 */

#include <stdio.h>
#include "ui.h"
#include "ui_helpers.h"
#include "sync_manager.h"

/* ================================================================ */
/*                    Screen1：主叫号界面                              */
/*          变量声明 + 事件回调 + 其他三个界面的公共声明               */
/* ================================================================ */

/* ---------- Screen1 初始化函数声明 ---------- */
void ui_MainScreen_screen_init(void);

/* ---------- Screen1 全局控件变量 ---------- */
lv_obj_t * ui_MainScreen;            /* 主屏幕容器 */
lv_obj_t * ui_DeptPanel;             /* 顶部科室信息面板 */
lv_obj_t * ui_DeptLabel;             /* 科室名称标签 "当前科室：B12-131" */
lv_obj_t * ui_WaitingQueuePanel;     /* 等待队列滚动容器 (左侧flex列布局) */
lv_obj_t * ui_CurrentPatientCard;    /* 当前患者卡片 (居中、带阴影) */
lv_obj_t * ui_CurrentPatientLabel;   /* 当前患者大号信息 (48px字体) */
lv_obj_t * ui_WaitingCountLabel;     /* 等待人数标签 */
lv_obj_t * ui_NextPatientLabel;      /* 下一位患者标签 */
lv_obj_t * ui_CallNextBtn;           /* "同步"按钮 (手动触发服务器同步) */
lv_obj_t * ui_CallNextBtnLabel;
lv_obj_t * ui_PatientDetailBtn;      /* "患者详情"按钮 (切换到Screen2) */
lv_obj_t * ui_PatientDetailBtnLabel;
lv_obj_t * ui_Screen3NavBtn;         /* "消息发送"按钮 (切换到Screen3) */
lv_obj_t * ui_Screen3NavBtnLabel;
lv_obj_t * ui_ReservedBtn;           /* "数据统计"按钮 (切换到Screen4) */
lv_obj_t * ui_ReservedBtnLabel;

/* ---------- Screen1 事件回调 ---------- */

/** @brief 等待队列面板点击 → 刷新等待队列 */
void ui_event_WaitingQueuePanel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_waiting_queue(e);
    }
}

/** @brief 当前患者标签点击 → 刷新当前患者信息 */
void ui_event_CurrentPatientLabel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_current_patient(e);
    }
}

void ui_event_WaitingCountLabel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_waiting_count(e);
    }
}

void ui_event_NextPatientLabel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_next_patient(e);
    }
}

/**
 * @brief "同步"按钮点击 → 手动触发服务器同步
 *
 * 调用 sync_trigger() 启动非阻塞HTTP同步状态机，
 * 由主循环的 sync_poll() 推进，不会阻塞UI。
 */
void ui_event_CallNextBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        sync_trigger();
    }
}

/**
 * @brief Screen2加载完成事件 → 刷新患者列表
 *
 * 通过 LV_EVENT_SCREEN_LOADED 触发，该事件在屏幕切换动画完成后才发送。
 * 在动画期间创建大量widget会导致LVGL渲染器卡死。
 */
void ui_event_Screen2Loaded(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_SCREEN_LOADED) {
        printf("[UI] Screen2 LOADED, refreshing patient list\n");
        refresh_patient_list(NULL);
    }
}

/**
 * @brief Screen1加载完成事件 → 刷新主界面数据
 *
 * 与 ui_event_Screen2Loaded 同理，在返回主界面的动画完成后刷新。
 */
void ui_event_Screen1Loaded(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_SCREEN_LOADED) {
        printf("[UI] Screen1 LOADED, refreshing main screen\n");
        refresh_current_patient(NULL);
        refresh_waiting_count(NULL);
        refresh_next_patient(NULL);
        refresh_waiting_queue(NULL);
    }
}

/**
 * @brief "患者详情"按钮点击 → 切换到Screen2
 *
 * 屏幕切换通过 lv_scr_load_anim 左滑动画完成。
 * 患者列表在动画完成后的 LV_EVENT_SCREEN_LOADED 事件中刷新，
 * 不在此时同步创建widget，避免与动画渲染冲突导致卡死。
 */
void ui_event_PatientDetailBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        printf("[UI] PatientDetailBtn CLICKED\n");
        _ui_screen_change(&ui_PatientListScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_PatientListScreen_screen_init);
    }
}

/** @brief "消息发送"按钮 → 切换到Screen3 AI对话界面 */
void ui_event_Screen3NavBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_Screen3_screen_init);
    }
}

/**
 * @brief "数据统计"按钮 → 切换到Screen4并刷新统计数据
 *
 * 切换到统计看板后立即调用 refresh_screen4_stats() 填充最新数据。
 */
void ui_event_ReservedBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_Screen4_screen_init);
        refresh_screen4_stats();
    }
}

/* ================================================================ */
/*                    Screen2：患者管理界面                            */
/* ================================================================ */

void ui_PatientListScreen_screen_init(void);
lv_obj_t * ui_PatientListScreen;     /* Screen2根容器 */
lv_obj_t * ui_PatientListPanel;      /* 患者列表滚动面板 */
lv_obj_t * ui_SearchInput;           /* 搜索输入框 (TextArea) */
lv_obj_t * ui_SearchBtn;             /* 搜索按钮 */
lv_obj_t * ui_SearchBtnLabel;
lv_obj_t * ui_SortDropdown;          /* 排序下拉框 (正序/倒序) */
lv_obj_t * ui_SortBtn;               /* 排序按钮 */
lv_obj_t * ui_SortBtnLabel;
lv_obj_t * ui_DeptPanel2;            /* Screen2顶栏面板 */
lv_obj_t * ui_DeptLabel2;            /* Screen2科室标签 */
lv_obj_t * ui_ScreenKeyboard;        /* 软键盘 (LVGL内置) */
lv_obj_t * ui_BackBtn;               /* 返回Screen1按钮 */
lv_obj_t * ui_BackBtnLabel;
lv_obj_t * ui_EventListBtn;          /* 就诊记录按钮 */
lv_obj_t * ui_EventListBtnLabel;

/* ---------- Screen2 事件回调 ---------- */

void ui_event_PatientListPanel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_patient_list(e);
    }
}

/**
 * @brief 搜索框交互:
 *   - CLICKED: 将键盘绑定到此搜索框
 *   - FOCUSED: 显示键盘
 *   - DEFOCUSED: 隐藏键盘
 */
void ui_event_SearchInput(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_keyboard_set_target(ui_ScreenKeyboard, ui_SearchInput);
    }
    if (event_code == LV_EVENT_FOCUSED) {
        _ui_flag_modify(ui_ScreenKeyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
    }
    if (event_code == LV_EVENT_DEFOCUSED) {
        _ui_flag_modify(ui_ScreenKeyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    }
}

/** @brief 搜索按钮 → 按姓名或ID查找患者 */
void ui_event_SearchBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        search_patient(e);
    }
}

/** @brief 排序按钮 → 按Patient_ID对列表排序 */
void ui_event_SortBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        sort_patient_list(e);
    }
}

/**
 * @brief 返回按钮 → 右滑动画回到Screen1主界面
 *
 * 刷新由 Screen1 的 LV_EVENT_SCREEN_LOADED 事件触发 (动画完成后)。
 */
void ui_event_BackBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_MainScreen_screen_init);
    }
}

/** @brief 就诊记录按钮 → 显示最近的叫号记录列表 */
void ui_event_EventListBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        refresh_event_list(e);
    }
}

/* ================================================================ */
/*                    Screen3：AI消息发送界面                          */
/* ================================================================ */

void ui_Screen3_screen_init(void);
lv_obj_t * ui_Screen3;               /* Screen3根容器 */
lv_obj_t * ui_Screen3Panel;          /* Screen3主面板 */
lv_obj_t * ui_Screen3Keyboard;       /* Screen3软键盘 */
lv_obj_t * ui_Screen3TopBar;         /* Screen3顶栏 */
lv_obj_t * ui_Screen3TopLabel;       /* Screen3顶栏标题 */
lv_obj_t * ui_Screen3Input;          /* Screen3消息输入框 */
lv_obj_t * ui_Screen3MiddlePanel;    /* Screen3聊天记录面板 */
lv_obj_t * ui_Screen3SendBtn;        /* Screen3发送按钮 */
lv_obj_t * ui_Screen3SendBtnLabel;
lv_obj_t * ui_Screen3BackBtn;        /* Screen3返回按钮 */
lv_obj_t * ui_Screen3BackBtnLabel;

/* ---------- Screen3 事件回调 ---------- */

/**
 * @brief 输入框交互 (同Screen2搜索框逻辑)
 *   CLICKED/FOCUSED → 显示键盘并上移层级
 *   DEFOCUSED → 隐藏键盘
 */
void ui_event_Screen3Input(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_keyboard_set_target(ui_Screen3Keyboard, ui_Screen3Input);
        _ui_flag_modify(ui_Screen3Keyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
        lv_obj_move_foreground(ui_Screen3Keyboard);
    }
    if (event_code == LV_EVENT_FOCUSED) {
        _ui_flag_modify(ui_Screen3Keyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
        lv_obj_move_foreground(ui_Screen3Keyboard);
    }
    if (event_code == LV_EVENT_DEFOCUSED) {
        _ui_flag_modify(ui_Screen3Keyboard, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    }
}

/**
 * @brief 发送按钮 → 调用DeepSeek AI接口
 *
 * 触发 on_screen3_send() 创建pthread异步发送请求，
 * 避免阻塞主线程。同时显示"等待中..."提示。
 */
void ui_event_Screen3SendBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        on_screen3_send(e);
    }
}

void ui_event_Screen3BackBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_MainScreen_screen_init);
    }
}

/* ================================================================ */
/*                    Screen4：统计数据看板界面                         */
/* ================================================================ */

void ui_Screen4_screen_init(void);
lv_obj_t * ui_Screen4;               /* Screen4根容器 */
lv_obj_t * ui_Screen4TopBar;         /* Screen4顶栏 */
lv_obj_t * ui_Screen4TopLabel;       /* Screen4顶栏标题 */
lv_obj_t * ui_Screen4BackBtn;        /* Screen4返回按钮 */
lv_obj_t * ui_Screen4BackBtnLabel;
/* 以下变量在 ui_Screen4.c 中定义，ui.h 中 extern 声明 */
/* ui_Screen4StatNum1/2/3, ui_Screen4Bar1~8, ui_Screen4FlowPanel */

void ui_event_Screen4BackBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_MainScreen_screen_init);
    }
}

/* ================================================================ */
/*                         全局初始化                                 */
/* ================================================================ */

/**
 * @brief 全局UI初始化 —— 创建所有4个屏幕并加载主屏幕
 *
 * 策略: 所有屏幕在启动时预创建，屏幕切换仅通过 lv_scr_load_anim 实现，
 *       避免反复销毁/重建造成性能抖动和内存碎片。
 *
 * 主题: teal色系主题，默认字体为 LV_FONT_DEFAULT (14px monospace)，
 *       中文显示通过 my_font_cn (16px) 和 my_font_cn_48 (48px) 自定义字体。
 */
void ui_init(void)
{
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_TEAL), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    /* 按顺序创建4个界面 (每个 *_screen_init 函数由SquareLine生成) */
    ui_MainScreen_screen_init();        /* Screen1: 主叫号界面 */
    ui_PatientListScreen_screen_init(); /* Screen2: 患者管理 */
    ui_Screen3_screen_init();           /* Screen3: AI消息发送 */
    ui_Screen4_screen_init();           /* Screen4: 数据统计看板 */

    /* 立即加载主屏幕 */
    lv_disp_load_scr(ui_MainScreen);
}
