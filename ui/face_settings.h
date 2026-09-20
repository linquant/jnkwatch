#pragma once
#include "lvgl.h"

#include <stdbool.h>

/* Settings face: brightness slider + lift-to-wake switch. */
void face_settings_create(lv_obj_t *parent);

/* Current lift-to-wake preference (read by the power manager). */
bool face_settings_wrist_wake(void);
void face_settings_apply_accent(void);
