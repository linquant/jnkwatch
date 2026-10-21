#pragma once
#include <stdint.h>

/* Device-side OTA engine. Owns the esp_ota_* flash writes on a dedicated task so
 * the NimBLE host task and the LVGL task are never blocked on flash. The BLE
 * layer (ble_server.c) feeds it raw OTA_CTRL / OTA_DATA writes; the pure framing
 * and SHA verification live in core/ota_proto.c. See docs/ble-protocol.md. */

/* Create the OTA queue + task. Call once at boot (after config_store_init). */
void ota_update_init(void);

/* Hand off one OTA_CTRL write (START/ABORT/COMMIT). Safe to call from the NimBLE
 * host task — it only enqueues; all flash work happens on the OTA task. */
void ota_update_on_ctrl(const uint8_t *buf, int len);

/* Hand off one OTA_DATA write ([seq u16][payload]). Enqueue-only, never blocks. */
void ota_update_on_data(const uint8_t *buf, int len);

/* Abort any transfer in progress (called on BLE disconnect). Enqueue-only. */
void ota_update_abort(void);
