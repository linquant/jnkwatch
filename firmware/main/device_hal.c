/* Device implementation of the watch HAL (platform/display.h) for the
 * ESP32-S3. The PC platform/display.c is NOT built on device; ui/ calls these.
 * M1: minimal/stubbed — display comes up in main.c; sensors/BLE land in M2/M3. */
#include "display.h"
#include "imu_qmi8658.h"
#include "esp_timer.h"
#include <time.h>

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
    return 76; /* TODO(M2): ADC1 ch3 + Li-ion curve */
}

void watch_read_accel(double *ax, double *ay, double *az)
{
    if (imu_qmi8658_read_accel(ax, ay, az)) return;
    *ax = 0.0; *ay = 0.0; *az = 1.0;   /* resting fallback if IMU unavailable */
}

int watch_poll_notification(wf_notif_t *out)
{
    (void)out;
    return 0; /* TODO(M3): BLE ANCS/GATT */
}

void watch_send_action(unsigned action_id)
{
    (void)action_id; /* TODO(M3): BLE GATT */
}

void watch_set_backlight(int pct)
{
    (void)pct; /* TODO(M2): SH8601 DCS 0x51 brightness */
}

void watch_set_wrist_wake(bool on)
{
    (void)on; /* TODO(M3): QMI8658 wake-on-motion */
}
