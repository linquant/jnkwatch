#pragma once
#include "lvgl.h"

/* Countdown timer face: MM:SS, depleting ring, touch controls
 * (start/pause/resume/reset and +/- one minute). */
void face_timer_create(lv_obj_t *parent);
void face_timer_tick(void);
void face_timer_apply_accent(void);
