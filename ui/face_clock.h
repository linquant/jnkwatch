#pragma once
#include "lvgl.h"

/* Dual Display clock face (Casio-style): LCD time stacked on the left
 * (HH / MM / :SS) and a status column on the right (date, battery, weather,
 * page dots), split by a hairline. Six teintes swap only the accent hue —
 * see theme.h (theme_set_accent / accent_t). */
void face_clock_create(lv_obj_t *parent);
void face_clock_tick(void);

/* Re-apply the current accent teinte to this face. */
void face_clock_apply_accent(void);
