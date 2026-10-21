#pragma once
#include <stdbool.h>

/* Battery / external-power monitor (ADC1 CH3 = GPIO4, BAT_ADC).
 *
 * Hardware note (schematic): BAT_ADC is VCC through a 200K/100K divider, and
 * VCC is the power-path output — USB_5V through Schottky D2 (~4.7 V) when a
 * cable/charger is plugged, else VBAT through P-MOS Q1. So:
 *   - VCC well above a Li-ion's 4.2 V  => external power present (charging),
 *     with ANY charger (a wall brick too, unlike USB-JTAG detection);
 *   - while plugged, the battery voltage itself is NOT measurable (the ETA6098
 *     STAT pin only drives LED2), so the last on-battery charge is kept.
 * A low-priority task samples once a second; the getters are lock-free. */

/* Start ADC + sampling task. Call after config_store_init() (uses NVS). */
void battery_init(void);

/* Battery charge 0..100 %, or -1 when unknown (booted on external power with
 * no reading ever saved). */
int battery_pct(void);

/* True while external power (USB/charger) is present. */
bool battery_ext_power(void);

/* Filtered raw reading in mV (before CAL_OFFSET_MV), 0 before the first read. */
int battery_mv(void);
