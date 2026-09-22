#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Bring up the FT3168 on the shared I2C bus (normal mode). */
void touch_ft3168_init(void);

/* Poll one touch point. Returns true with *x,*y (screen coords) when a finger
 * is down, false when the panel reports no touch. */
bool touch_ft3168_read(uint16_t *x, uint16_t *y);
