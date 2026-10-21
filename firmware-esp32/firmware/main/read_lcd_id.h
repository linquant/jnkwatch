#pragma once
#include <stdint.h>
/* Bit-bang read of the AMOLED controller ID (0x86 = SH8601, else CO5300), with
 * the long reset sequence the panel needs. Ported from the Waveshare demo. */
uint8_t read_lcd_id(void);
