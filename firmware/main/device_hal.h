#pragma once
#include "display.h"   /* wf_config_msg_t */

/* Create the inbound-config queue. Call once early in app_main, before the BLE
 * server or the LVGL task start using it. */
void device_hal_init(void);

/* Queue a config message from the BLE task; drained by watch_poll_config() on
 * the LVGL thread (keeps GATT writes off LVGL). */
void device_config_enqueue(const wf_config_msg_t *m);
