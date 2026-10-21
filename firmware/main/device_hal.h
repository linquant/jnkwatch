#pragma once
#include "display.h"   /* wf_config_msg_t */
#include "notify.h"    /* wf_notif_t */

/* Create the inbound queues. Call once early in app_main, before the BLE server
 * or the LVGL task start using them. */
void device_hal_init(void);

/* Queue a config message from the BLE task; drained by watch_poll_config() on
 * the LVGL thread (keeps GATT writes off LVGL). */
void device_config_enqueue(const wf_config_msg_t *m);

/* Display state for the battery log: screen on (not in veille) + brightness %. */
bool device_display_awake(void);
int  device_brightness_pct(void);

/* Queue a phone notification from the BLE task; drained by
 * watch_poll_notification() on the LVGL thread. */
void device_notif_enqueue(const wf_notif_t *n);
