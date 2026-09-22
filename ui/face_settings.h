#pragma once
#include "lvgl.h"

#include <stdbool.h>

/* Settings face: brightness slider + lift-to-wake switch. */
void face_settings_create(lv_obj_t *parent);

/* Current lift-to-wake preference (read by the power manager). */
bool face_settings_wrist_wake(void);

/* Selected screen-sleep timeout in ms (WF_SLEEP_5S / WF_SLEEP_10S). */
int face_settings_sleep_ms(void);

void face_settings_apply_accent(void);
