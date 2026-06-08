#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

void refresh_waiting_queue(lv_event_t * e);
void refresh_current_patient(lv_event_t * e);
void refresh_waiting_count(lv_event_t * e);
void refresh_next_patient(lv_event_t * e);
void call_next_patient(lv_event_t * e);

void refresh_patient_list(lv_event_t * e);
void search_patient(lv_event_t * e);
void sort_patient_list(lv_event_t * e);

void refresh_event_list(lv_event_t * e);

void refresh_screen4_stats(void);

void on_screen3_send(lv_event_t *e);
void on_screen3_delete(lv_event_t *e);

#ifdef __cplusplus
}
#endif

#endif
