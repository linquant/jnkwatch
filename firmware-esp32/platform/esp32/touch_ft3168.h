#pragma once
/* FT3168 capacitive-touch driver → LVGL v9 pointer input device.
 *
 * DEVICE-ONLY (ESP-IDF). This file is NOT part of the PC/SDL build — it is
 * compiled by the ESP32-S3 firmware project. It depends on ESP-IDF headers.
 *
 * Board: ESP32-S3-Touch-AMOLED-1.43. FT3168 sits on the shared I2C bus
 * (SDA=GPIO47, SCL=GPIO48) at 7-bit address 0x38. See
 * docs/hardware/ESP32-S3-Touch-AMOLED-1.43.md. Ref: GitLab issue #3. */
#include "lvgl.h"
#include "driver/i2c.h"

/* Initialize the shared I2C master bus (idempotent-ish: install once).
 * ticket #2/#4 also use this bus — later, factor this into one bsp_i2c. */
esp_err_t ft3168_i2c_bus_init(i2c_port_t port);

/* Create the LVGL pointer indev backed by the FT3168 on `port`.
 * Returns the indev, or NULL on failure. Call after lv_init() + a display. */
lv_indev_t *touch_ft3168_create(i2c_port_t port);
