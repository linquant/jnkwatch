#pragma once
#include "lvgl.h"

/* Step-counter app: a progress ring toward a daily goal with the day's step
 * total (ui_steps_today()) in the centre. The ring turns green at goal. */
void face_steps_create(lv_obj_t *parent);

/* Refresh the number + ring from the current step total. Call when visible. */
void face_steps_tick(void);

/* Re-tint the accent-coloured ring. */
void face_steps_apply_accent(void);
