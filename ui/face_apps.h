#pragma once
#include "lvgl.h"

/* Applications launcher: a simple menu of apps. For now two entries — the bubble
 * level (live) and a placeholder for the second app. Tapping an entry navigates
 * to that app's tile. */
void face_apps_create(lv_obj_t *parent);

/* Re-tint the accent-coloured widgets. */
void face_apps_apply_accent(void);
