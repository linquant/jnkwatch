#pragma once
#include "lvgl.h"

/* Living watchface: a particle field + colour that react to motion energy
 * measured from the IMU (calm cyan at rest -> vivid magenta in motion). */
void face_aura_create(lv_obj_t *parent);
void face_aura_tick(void);
