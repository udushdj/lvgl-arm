/**
 * @file ui.h
 * @brief 全部UI控件全局变量声明 + 屏幕初始化函数 + 事件回调声明
 *
 * 本头文件汇总了4个界面（Screen1~4）的所有 lv_obj_t* 全局变量、
 * 屏幕初始化函数声明和事件回调函数声明。
 * 变量定义在 ui.c 中，界面创建代码在 screens/ui_ScreenX.c 中。
 */

#ifndef _UI1_UI_H
#define _UI1_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "ui_helpers.h"
#include "ui_events.h"

/* =========================== Screen1：主叫号界面 =========================== */

void ui_MainScreen_screen_init(void);

/* 控件变量 */
extern lv_obj_t * ui_MainScreen;            /* 主屏幕容器 */
extern lv_obj_t * ui_DeptPanel;             /* 顶栏科室信息面板 */
extern lv_obj_t * ui_DeptLabel;             /* 科室名称标签 "当前科室：B12-131" */
extern lv_obj_t * ui_WaitingQueuePanel;     /* 等待队列滚动面板 (flex列布局) */
extern lv_obj_t * ui_CurrentPatientCard;    /* 当前患者信息卡片 (居中带阴影) */
extern lv_obj_t * ui_CurrentPatientLabel;   /* 当前患者大号标签 (48px字体) */
extern lv_obj_t * ui_WaitingCountLabel;     /* 等待人数标签 */
extern lv_obj_t * ui_NextPatientLabel;      /* 下一位患者标签 */
extern lv_obj_t * ui_CallNextBtn;           /* "同步"按钮 (手动触发服务器同步) */
extern lv_obj_t * ui_CallNextBtnLabel;
extern lv_obj_t * ui_PatientDetailBtn;      /* "患者详情"按钮 → Screen2 */
extern lv_obj_t * ui_PatientDetailBtnLabel;
extern lv_obj_t * ui_Screen3NavBtn;         /* "消息发送"按钮 → Screen3 */
extern lv_obj_t * ui_Screen3NavBtnLabel;
extern lv_obj_t * ui_ReservedBtn;           /* "数据统计"按钮 → Screen4 */
extern lv_obj_t * ui_ReservedBtnLabel;

/* 事件回调 */
void ui_event_WaitingQueuePanel(lv_event_t * e);
void ui_event_CurrentPatientLabel(lv_event_t * e);
void ui_event_WaitingCountLabel(lv_event_t * e);
void ui_event_NextPatientLabel(lv_event_t * e);
void ui_event_CallNextBtn(lv_event_t * e);
void ui_event_PatientDetailBtn(lv_event_t * e);
void ui_event_Screen3NavBtn(lv_event_t * e);
void ui_event_ReservedBtn(lv_event_t * e);
void ui_event_Screen1Loaded(lv_event_t * e);  /* 动画完成后刷新 */

/* =========================== Screen2：患者管理界面 =========================== */

void ui_PatientListScreen_screen_init(void);

extern lv_obj_t * ui_PatientListScreen;     /* Screen2根容器 */
extern lv_obj_t * ui_PatientListPanel;      /* 患者列表滚动面板 (flex列布局) */
extern lv_obj_t * ui_SearchInput;           /* 搜索输入框 (TextArea) */
extern lv_obj_t * ui_SearchBtn;             /* 搜索按钮 */
extern lv_obj_t * ui_SearchBtnLabel;
extern lv_obj_t * ui_SortDropdown;          /* 排序下拉框 (正序/倒序) */
extern lv_obj_t * ui_SortBtn;               /* 排序按钮 */
extern lv_obj_t * ui_SortBtnLabel;
extern lv_obj_t * ui_DeptPanel2;            /* Screen2顶栏面板 */
extern lv_obj_t * ui_DeptLabel2;            /* Screen2科室标签 */
extern lv_obj_t * ui_ScreenKeyboard;        /* 软键盘 (默认隐藏) */
extern lv_obj_t * ui_BackBtn;               /* 返回Screen1按钮 */
extern lv_obj_t * ui_BackBtnLabel;
extern lv_obj_t * ui_EventListBtn;          /* 就诊记录按钮 */
extern lv_obj_t * ui_EventListBtnLabel;

void ui_event_PatientListPanel(lv_event_t * e);
void ui_event_SearchInput(lv_event_t * e);
void ui_event_SearchBtn(lv_event_t * e);
void ui_event_SortBtn(lv_event_t * e);
void ui_event_BackBtn(lv_event_t * e);
void ui_event_EventListBtn(lv_event_t * e);
void ui_event_Screen2Loaded(lv_event_t * e);  /* 动画完成后刷新患者列表 */

/* =========================== Screen3：AI消息发送界面 =========================== */

void ui_Screen3_screen_init(void);

extern lv_obj_t * ui_Screen3;               /* Screen3根容器 */
extern lv_obj_t * ui_Screen3Panel;          /* 全屏容器面板 */
extern lv_obj_t * ui_Screen3Keyboard;       /* 软键盘 (默认隐藏) */
extern lv_obj_t * ui_Screen3TopBar;         /* 顶栏 */
extern lv_obj_t * ui_Screen3TopLabel;       /* 顶栏标题 "AI助手" */
extern lv_obj_t * ui_Screen3Input;          /* 消息输入框 (TextArea) */
extern lv_obj_t * ui_Screen3MiddlePanel;    /* 聊天记录面板 */
extern lv_obj_t * ui_Screen3SendBtn;        /* 发送按钮 */
extern lv_obj_t * ui_Screen3SendBtnLabel;
extern lv_obj_t * ui_Screen3BackBtn;        /* 返回Screen1按钮 */
extern lv_obj_t * ui_Screen3BackBtnLabel;

void ui_event_Screen3Input(lv_event_t * e);
void ui_event_Screen3SendBtn(lv_event_t * e);
void ui_event_Screen3BackBtn(lv_event_t * e);

/* =========================== Screen4：统计看板界面 =========================== */

void ui_Screen4_screen_init(void);

extern lv_obj_t * ui_Screen4;               /* Screen4根容器 */
extern lv_obj_t * ui_Screen4TopBar;         /* 顶栏 */
extern lv_obj_t * ui_Screen4TopLabel;       /* 顶栏标题 "数据统计" */
extern lv_obj_t * ui_Screen4BackBtn;        /* 返回Screen1按钮 */
extern lv_obj_t * ui_Screen4BackBtnLabel;
extern lv_obj_t * ui_Screen4StatNum1;       /* 已就诊人数数值 */
extern lv_obj_t * ui_Screen4StatNum2;       /* 平均等待时间数值 */
extern lv_obj_t * ui_Screen4StatNum3;       /* 当前排队人数数值 */
extern lv_obj_t * ui_Screen4FlowPanel;      /* 近8小时流量柱状图面板 */

void screen4_draw_flow_bars(void);           /* 绘制流量柱状图 */
void ui_event_Screen4BackBtn(lv_event_t * e);

/* =========================== 全局初始化 =========================== */

void ui_init(void);                         /* 创建全部4个界面并加载主屏幕 */

#ifdef __cplusplus
}
#endif

#endif
