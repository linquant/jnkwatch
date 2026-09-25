#pragma once
#include <stdbool.h>

/* Start the NimBLE host + GATT server (advertises as "JNKWatch"). Requires NVS
 * to be initialised first (config_store_init) for bonding key storage. */
void ble_server_start(void);

/* Notify the phone that the settings changed on-device (BLE Settings char).
 * No-op when no central is connected/subscribed. */
void ble_settings_notify(int brightness, int sleep_ms, bool wrist_wake,
                         bool stay_awake_charging, bool power_saver, int accent);

/* Notify the phone that an action button was tapped (BLE ActionEvent char). */
void ble_action_notify(unsigned action_id);
