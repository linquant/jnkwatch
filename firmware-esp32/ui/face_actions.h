#pragma once
#include "lvgl.h"

/* Action-buttons face: a 2x3 grid of customizable shortcut buttons (icon +
 * caption). Tapping one fires watch_send_action() (a BLE signal to the phone;
 * stubbed on PC) and flashes the button. The set comes from the tested
 * core/actions.c model, ready for later BLE customization. */
void face_actions_create(lv_obj_t *parent);
void face_actions_tick(void);
void face_actions_apply_accent(void);

/* Apply an action-button set pushed from the phone (BLE ActionCfg): the phone
 * clears then sets buttons in index order to resync the whole page. */
void face_actions_apply_clear(void);
void face_actions_apply_set(int index, unsigned id, const char *label,
                            int icon, bool enabled);
