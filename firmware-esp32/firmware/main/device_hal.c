/* Device implementation of the watch HAL (platform/display.h) for the
 * ESP32-S3. The PC platform/display.c is NOT built on device; ui/ calls these.
 * M1: minimal/stubbed — display comes up in main.c; sensors/BLE land in M2/M3. */
#include "display.h"
#include "device_hal.h"
#include "imu_qmi8658.h"
#include "config_store.h"
#include "ble_server.h"
#include "panel_ctrl.h"
#include "battery.h"
#include "touch_ft3168.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <time.h>

/* Inbound queues from the BLE task, drained on the LVGL thread. */
static QueueHandle_t s_config_q;
static QueueHandle_t s_notif_q;

void device_hal_init(void)
{
    s_config_q = xQueueCreate(8, sizeof(wf_config_msg_t));
    s_notif_q  = xQueueCreate(8, sizeof(wf_notif_t));
}

void device_config_enqueue(const wf_config_msg_t *m)
{
    if (s_config_q) xQueueSend(s_config_q, m, 0);
}

int watch_poll_config(wf_config_msg_t *out)
{
    return (s_config_q && xQueueReceive(s_config_q, out, 0) == pdTRUE) ? 1 : 0;
}

void device_notif_enqueue(const wf_notif_t *n)
{
    if (s_notif_q) xQueueSend(s_notif_q, n, 0);
}

void watch_settings_changed(int brightness, int sleep_ms, bool wrist_wake,
                            int wrist_sens, bool stay_awake_charging,
                            bool power_saver, int accent)
{
    config_store_set_settings(brightness, sleep_ms, wrist_wake, wrist_sens,
                              stay_awake_charging, power_saver, accent);
    ble_settings_notify(brightness, sleep_ms, wrist_wake, wrist_sens,
                        stay_awake_charging, power_saver, accent);
}

uint32_t watch_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

time_t watch_time(void)
{
    /* TODO(M2): read the PCF85063 RTC. For now, the system clock. */
    return time(NULL);
}

void watch_now(struct tm *out)
{
    time_t t = watch_time();
    localtime_r(&t, out);
}

int watch_battery_pct(void)
{
    return battery_pct();
}

int watch_battery_mv(void)
{
    return battery_mv();
}

bool watch_is_charging(void)
{
    /* External power seen on VCC (any charger), or a USB host as a fallback
     * should the ADC be unavailable. */
    return battery_ext_power() || usb_serial_jtag_is_connected();
}

void watch_read_accel(double *ax, double *ay, double *az)
{
    if (imu_qmi8658_read_accel(ax, ay, az)) return;
    *ax = 0.0; *ay = 0.0; *az = 1.0;   /* resting fallback if IMU unavailable */
}

uint32_t watch_steps_load(int *yday)
{
    return config_store_get_steps(yday);
}

void watch_steps_save(uint32_t steps, int yday)
{
    config_store_set_steps(steps, yday);
}

int watch_poll_notification(wf_notif_t *out)
{
    return (s_notif_q && xQueueReceive(s_notif_q, out, 0) == pdTRUE) ? 1 : 0;
}

void watch_send_action(unsigned action_id)
{
    ble_action_notify(action_id);  /* notify the phone over GATT */
}

/* Last brightness the UI asked for (10..100), so we can restore it on wake and
 * avoid writing 0x51 while the panel is off. */
static int s_backlight = 80;
static bool s_display_asleep = false;

void watch_set_backlight(int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    s_backlight = pct;
    /* Don't fight the sleep state: while asleep the panel is off; brightness is
     * restored by watch_display_sleep(false) on wake. */
    if (!s_display_asleep) board_panel_brightness((uint8_t)(pct * 255 / 100));
}

bool device_display_awake(void) { return !s_display_asleep; }
int  device_brightness_pct(void) { return s_backlight; }

void watch_display_sleep(bool sleeping)
{
    s_display_asleep = sleeping;
    if (sleeping) {
        board_panel_on(false);              /* AMOLED sleep-in (0x28 + 0x10) */
        touch_ft3168_set_monitor(true);     /* touch 1.5 mA -> ~30 uA, still wakes */
        /* Classic sleep for both modes for now: AMOLED off + loop throttle, CPU
         * kept running (light-sleep) so a wrist raise / tap wakes the watch and
         * BLE stays connected. The "watch only" power_saver deep-sleep path (real
         * SoC deep sleep + QMI8658 wake-on-motion, BLE dropped) is deferred — the
         * WoM/deep-sleep code lives in imu_qmi8658_arm_wom() for when we resume. */
        wf_stored_config_t c;
        config_store_get(&c);
        ESP_LOGI("power", "SLEEP: screen off (AMOLED sleep-in 0x10), loop throttled%s",
                 c.power_saver ? " [watch-only]" : "");
    } else {
        touch_ft3168_set_monitor(false);    /* full-rate tracking while in use */
        board_panel_on(true);               /* AMOLED on (0x11 + 0x29) */
        board_panel_brightness((uint8_t)(s_backlight * 255 / 100)); /* restore */
        ESP_LOGI("power", "WAKE: screen on (AMOLED 0x11+0x29), brightness=%d%%", s_backlight);
    }
}

void watch_set_wrist_wake(bool on)
{
    (void)on; /* TODO(M3): QMI8658 wake-on-motion */
}
