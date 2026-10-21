# platform/esp32 — device HAL (ESP-IDF only)

Hardware drivers for the **ESP32-S3-Touch-AMOLED-1.43**. These files depend on
ESP-IDF headers (`driver/i2c.h`, FreeRTOS, `esp_log.h`) and are **NOT compiled by
the PC/SDL emulator build** (`CMakeLists.txt` / `build.sh`). They are compiled by
the ESP-IDF firmware project (to be added), e.g. added to a `main` component's
`CMakeLists.txt` `SRCS`.

Pin/address values are the verified ones from `docs/hardware/ESP32-S3-Touch-AMOLED-1.43.md`.

| File | Issue | Status |
|---|---|---|
| `touch_ft3168.{c,h}` | #3 | FT3168 → LVGL v9 pointer indev (I2C 0x38, SDA47/SCL48). Untested on hardware. |

## Integrating touch_ft3168
```c
#include "touch_ft3168.h"
// after lv_init() and the display is created (issue #1):
lv_indev_t *touch = touch_ft3168_create(I2C_NUM_0);
```
Then tune `TP_SWAP_XY / TP_MIRROR_X / TP_MIRROR_Y` in `touch_ft3168.c` so a tap
lands under the finger for the chosen display rotation (issue #1).

The shared I2C bus (SDA47/SCL48) is also used by the IMU (#4) and RTC (#2);
`ft3168_i2c_bus_init()` tolerates an already-installed driver, but eventually
factor a single `bsp_i2c` init shared by all three.
