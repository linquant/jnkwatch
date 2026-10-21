#pragma once
/* Portable watch UI. This code contains NO platform dependencies: the exact
 * same source compiles for the PC simulator and for the ESP32-S3 firmware. */
#include <stdbool.h>
#include <stdint.h>

/* Build the watchface on the active (default) LVGL display. */
void ui_init(void);

/* Update dynamic elements (hands, digital time). Call once per frame. */
void ui_tick(void);

/* Jump to a face by index
 * (0=actions, 1=notifications, 2=probes/sondes, 3=clock, 4=aura, 5=settings,
 *  6=debug, 7=apps, 8=level, 9=steps).
 * Physical strip, left to right: actions | sondes | notif | clock | ... The UI
 * boots on the clock (3); notifications, sondes then actions are swipes left.
 * Clock (3) and aura (4) share one horizontal slot — aura is a swipe DOWN from
 * the clock; the rest of the strip is horizontal. */
void ui_show_face(int idx);

/* Jump directly to a raw tileview cell (col,row) — used by the shot tool to
 * preview sub-screens that aren't a top-level face index. */
void ui_goto_tile(int col, int row);

/* Cumulative time the screen has been lit, total uptime (ms), and the on ratio
 * (%). Surfaced on the debug face. */
unsigned long ui_screen_on_ms(void);
unsigned long ui_uptime_ms(void);
int           ui_screen_on_pct(void);

/* Steps counted for the current day (accel-only pedometer). Surfaced on the
 * steps app and as a clock complication. */
uint32_t ui_steps_today(void);

/* Re-apply the current accent teinte to every face (call after changing it). */
void ui_apply_accent(void);

/* Apply display brightness (10..100%): drives the backlight HAL and, in the
 * simulator, a dim overlay so the change is visible. */
void ui_apply_brightness(int pct);

/* True while the screen is asleep (blanked after inactivity). Lets the firmware
 * loop slow its cadence to save power while the panel is off. */
bool ui_is_asleep(void);
