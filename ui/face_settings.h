#pragma once
#include "lvgl.h"

#include <stdbool.h>

/* Settings face, split into four vertical sub-screens — one parent tile each:
 * brightness, sleep timeout, accent colours, wrist wake. */
void face_settings_create(lv_obj_t *bright, lv_obj_t *sleep,
                          lv_obj_t *color, lv_obj_t *wrist);

/* Current lift-to-wake preference (read by the power manager). */
bool face_settings_wrist_wake(void);

/* Selected screen-sleep timeout in ms (WF_SLEEP_5S / WF_SLEEP_10S). */
int face_settings_sleep_ms(void);

/* Current backlight brightness, 10..100 (read by the BLE Settings char). */
int face_settings_brightness(void);

/* Apply settings pushed from the phone (BLE) or restored from NVS at boot.
 * Updates the widgets and hardware without firing the user callbacks. */
void face_settings_apply_config(int brightness, int sleep_ms, bool wrist_wake);

void face_settings_apply_accent(void);
