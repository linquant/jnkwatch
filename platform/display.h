#pragma once
/* Portable display/platform layer shared by both the PC simulator and the
 * (future) ESP32-S3 firmware. On the PC we render into an in-memory RGB565
 * framebuffer; on the device the same LVGL display would flush to the GC9A01. */
#include "lvgl.h"
#include "notify.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define WATCH_SIZE 466 /* ESP32-S3-Touch-AMOLED-1.43 is 466x466 (round) */

/* Create the 240x240 LVGL display backed by an internal RGB565 framebuffer. */
lv_display_t *watch_display_init(void);

/* Access the current frame (WATCH_SIZE*WATCH_SIZE RGB565 pixels). */
const uint16_t *watch_framebuffer(void);

/* Monotonic milliseconds, used as the LVGL tick source. */
uint32_t watch_millis(void);

/* Battery charge, 0..100%. Mocked on PC; reads the ADC on real hardware. */
int watch_battery_pct(void);

/* True while the watch is on USB power (charging). On device: the native USB
 * Serial/JTAG is connected to a host (a dumb charger without data won't count).
 * Used to keep the screen awake while plugged in. PC: $WATCH_CHARGING. */
bool watch_is_charging(void);

/* Set display backlight, 10..100%. No-op on PC (the sim dims via an overlay);
 * on hardware, drive the backlight GPIO with LEDC PWM. */
void watch_set_backlight(int pct);

/* Enable/disable lift-to-wake. No-op on PC; on hardware, arm the QMI8658
 * wake-on-motion / tilt detection and gate the display power. */
void watch_set_wrist_wake(bool on);

/* Read the accelerometer in g. Mocked on PC (simulates a periodic wrist
 * raise); on hardware, read the QMI8658. */
void watch_read_accel(double *ax, double *ay, double *az);

/* Simulated phone notification source (BLE stand-in). Returns 1 and fills
 * *out when a new notification has arrived since the last poll, else 0. On PC
 * a small scripted feed drips notifications in so the screen has content; on
 * hardware this is driven by the BLE (ANCS/GATT) notification callback. */
int watch_poll_notification(wf_notif_t *out);

/* Fire a customizable action button. No-op on PC (the sim just flashes the
 * button as feedback); on hardware, send action_id to the phone over BLE
 * (a GATT characteristic the companion app subscribes to). */
void watch_send_action(unsigned action_id);

/* --- Inbound configuration from the phone (BLE) ------------------------------
 * The companion app writes settings / action-button config over GATT (see
 * docs/ble-protocol.md). The BLE task queues those as wf_config_msg_t; the UI
 * drains them each tick (ui_tick) and applies them to the running faces — this
 * indirection keeps the write-side off the LVGL thread. On PC there is no phone,
 * so watch_poll_config() returns 0 (mock feed possible for tests). */
typedef enum {
    WF_CFG_SETTINGS = 1,   /* brightness / sleep_ms / wrist_wake / accent */
    WF_CFG_ACTION_CLEAR,   /* wipe the action-button set                  */
    WF_CFG_ACTION_SET,     /* set one action button (index/id/icon/...)   */
    WF_CFG_NOTIF_CLEAR,    /* clear the notification store                */
} wf_config_kind_t;

typedef struct {
    wf_config_kind_t kind;
    /* WF_CFG_SETTINGS */
    int  brightness;       /* 10..100                     */
    int  sleep_ms;         /* WF_SLEEP_5S / WF_SLEEP_10S   */
    bool wrist_wake;
    int  accent;           /* 0..ACCENT_COUNT-1           */
    /* WF_CFG_ACTION_SET */
    int      index;        /* 0..WF_ACTION_MAX-1          */
    unsigned action_id;
    int      icon;         /* 0..WF_ICON_COUNT-1          */
    bool     enabled;
    char     label[16];
} wf_config_msg_t;

/* Dequeue one pending config message. Returns 1 and fills *out, else 0. */
int watch_poll_config(wf_config_msg_t *out);

/* Report a settings change the USER made on the watch, so the phone can be
 * notified (BLE Settings notify) and the value persisted. No-op on PC. */
void watch_settings_changed(int brightness, int sleep_ms, bool wrist_wake, int accent);

/* Current wall-clock time as an epoch. On PC it is the host clock, but can be
 * pinned via the $WATCH_EPOCH env var (seconds) for deterministic screenshots.
 * On hardware, read the PCF85063 RTC. The UI must use this instead of libc
 * time() so it stays platform-independent and testable. */
time_t watch_time(void);

/* Broken-down LOCAL time, filled from watch_time(). */
void watch_now(struct tm *out);
