#pragma once
/* Runtime control of the AMOLED panel from the HAL (device_hal.c). The panel
 * and its IO handle are owned by main.c (the boot code that brings the QSPI
 * display up); these thin accessors expose just power + brightness so the
 * shared power_tick() sleep logic can turn the screen off and set brightness
 * without device_hal.c needing the esp_lcd handles. */
#include <stdbool.h>
#include <stdint.h>

/* Turn the panel off into SLEEP-IN (DCS 0x28 display off, then 0x10) or back
 * on (0x11 sleep out, 120 ms, 0x29). Sleep-in also stops the controller's
 * oscillator and panel supplies — far below display-off alone. The FT3168
 * touch is a separate IC, so touch-to-wake keeps working. Blocks ~40 ms (off)
 * / ~120 ms (on), per the SH8601 power sequences. */
void board_panel_on(bool on);

/* Set AMOLED brightness via DCS 0x51 (WRDISBV), value 0..255. */
void board_panel_brightness(uint8_t value);
