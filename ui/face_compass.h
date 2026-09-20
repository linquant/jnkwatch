#pragma once
#include "lvgl.h"

/* Compass face: rotating graduated rose, cardinal marks, heading + bearing.
 * Heading comes from the watch_heading_deg() HAL. */
void face_compass_create(lv_obj_t *parent);
void face_compass_tick(void);
void face_compass_apply_accent(void);
