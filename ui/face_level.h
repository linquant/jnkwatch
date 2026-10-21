#pragma once
#include "lvgl.h"

/* Bubble-level app: a round spirit level driven by the accelerometer
 * (core/level.c). Draws an outer ring, a centre "level" target, and a ball that
 * rolls to the low side; turns green and shows 0.0deg when flat. */
void face_level_create(lv_obj_t *parent);

/* Sample the accelerometer and update the ball + readout. Call when visible. */
void face_level_tick(void);

/* Re-tint the accent-coloured widgets. */
void face_level_apply_accent(void);
