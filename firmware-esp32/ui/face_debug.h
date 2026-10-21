#pragma once
#include "lvgl.h"

/* Debug face (after settings): screen-on time (AMOLED usage), uptime, on-ratio
 * and a few live stats. */
void face_debug_create(lv_obj_t *parent);
void face_debug_tick(void);
void face_debug_apply_accent(void);
