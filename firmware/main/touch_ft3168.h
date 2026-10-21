#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Bring up the FT3168 on the shared I2C bus (normal mode). */
void touch_ft3168_init(void);

/* Poll one touch point. Returns true with *x,*y (screen coords) when a finger
 * is down, false when the panel reports no touch. */
bool touch_ft3168_read(uint16_t *x, uint16_t *y);

/* Screen-off power mode. true = MONITOR (slow scan, ~30 uA vs 1.5 mA active,
 * datasheet): the controller only looks for a finger and switches itself back
 * to ACTIVE on touch, so touch-to-wake keeps working. false = ACTIVE. */
void touch_ft3168_set_monitor(bool monitor);
