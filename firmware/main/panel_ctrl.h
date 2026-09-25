#pragma once
/* Runtime control of the AMOLED panel from the HAL (device_hal.c). The panel
 * and its IO handle are owned by main.c (the boot code that brings the QSPI
 * display up); these thin accessors expose just power + brightness so the
 * shared power_tick() sleep logic can turn the screen off and set brightness
 * without device_hal.c needing the esp_lcd handles. */
#include <stdbool.h>
#include <stdint.h>

/* Turn the display on (DCS 0x29) or off (DCS 0x28). Off cuts the AMOLED to its
 * lowest draw and is the main battery saver during screen sleep. */
void board_panel_on(bool on);

/* Set AMOLED brightness via DCS 0x51 (WRDISBV), value 0..255. */
void board_panel_brightness(uint8_t value);
