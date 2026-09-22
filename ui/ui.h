#pragma once
/* Portable watch UI. This code contains NO platform dependencies: the exact
 * same source compiles for the PC simulator and for the ESP32-S3 firmware. */

/* Build the watchface on the active (default) LVGL display. */
void ui_init(void);

/* Update dynamic elements (hands, digital time). Call once per frame. */
void ui_tick(void);

/* Jump to a face by index
 * (0=actions, 1=notifications, 2=clock, 3=aura, 4=timer, 5=settings).
 * The UI boots on the clock (2); notifications then actions are swipes left.
 * Clock (2) and aura (3) share one horizontal slot — aura is a swipe DOWN from
 * the clock; the rest of the strip is horizontal. */
void ui_show_face(int idx);

/* Re-apply the current accent teinte to every face (call after changing it). */
void ui_apply_accent(void);

/* Apply display brightness (10..100%): drives the backlight HAL and, in the
 * simulator, a dim overlay so the change is visible. */
void ui_apply_brightness(int pct);
