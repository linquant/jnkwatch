/* Battery monitor — see battery.h for the VCC/BAT_ADC hardware quirk. */
#include "battery.h"
#include "board.h"
#include "config_store.h"
#include "batlog.h"
#include "device_hal.h"   /* device_display_awake / brightness */
#include "ble_server.h"   /* ble_is_connected */
#include "driver/usb_serial_jtag.h"
#include "esp_pm.h"
#include <string.h>
#include <time.h>
#include "watchface_logic.h"   /* wf_battery_pct_from_mv */
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "battery";

#define DIVIDER        3      /* R12 200K / R17 100K                       */
#define SAMPLES        16     /* oneshot reads averaged per measurement    */
#define PERIOD_MS      1000
#define EXT_ON_MV      4400   /* VCC above this => USB_5V via D2 present   */
#define EXT_OFF_MV     4300   /* hysteresis on the way down                */
#define SETTLE_MS      5000   /* ignore readings right after unplugging    */
#define RISE_RESYNC    3      /* % above the shown value that forces a resync */
#define LOG_PERIOD_S   60     /* one battery-log record per minute          */
/* Calibration: a full cell (charger done, voltage flat for 1 h+) reads a steady
 * ~4120 mV here vs the 4200 mV of a full Li-ion (battery log 2026-09-29), so
 * the % is computed on the reading + this offset. The log keeps the raw mV. */
#define CAL_OFFSET_MV  80

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;   /* NULL if calibration is unavailable */

static volatile int  s_mv;          /* filtered VCC (mV)            */
static volatile int  s_pct = -1;    /* shown charge, -1 = unknown   */
static volatile bool s_ext;

static int read_mv(void)
{
    int sum = 0, n = 0;
    for (int i = 0; i < SAMPLES; i++) {
        int raw, mv;
        if (adc_oneshot_read(s_adc, (adc_channel_t)BSP_BAT_ADC_CH, &raw) != ESP_OK) continue;
        if (!s_cali || adc_cali_raw_to_voltage(s_cali, raw, &mv) != ESP_OK)
            mv = raw * 3300 / 4095;  /* uncalibrated fallback */
        sum += mv;
        n++;
    }
    return n ? (sum / n) * DIVIDER : 0;
}

/* One measurement; returns the BATLOG_F_PLUG/UNPLUG event it detected, if any. */
static uint8_t sample(void)
{
    static int64_t settle_until_us;
    static int filt;                 /* 0 = reseed on next battery reading */

    int mv = read_mv();
    if (mv <= 0) return 0;
    uint8_t event = 0;
    int64_t now = esp_timer_get_time();

    bool ext = s_ext ? (mv >= EXT_OFF_MV) : (mv >= EXT_ON_MV);
    if (ext != s_ext) {
        s_ext = ext;
        event = ext ? BATLOG_F_PLUG : BATLOG_F_UNPLUG;
        ESP_LOGI(TAG, "%s (VCC %d mV, batt %d%%)",
                 ext ? "external power plugged" : "on battery", mv, s_pct);
        if (ext) {
            if (s_pct >= 0) config_store_set_battery_pct(s_pct);
        } else {
            /* Unplugged: the cell reads high for a few seconds (surface charge),
             * then we resync from scratch — it may have charged a lot. */
            settle_until_us = now + (int64_t)SETTLE_MS * 1000;
            filt = 0;
        }
    }
    if (ext) { s_mv = mv; return event; }
    if (now < settle_until_us) return event;

    /* Slow EMA (~8 s) over the averaged reads: rejects load steps (screen
     * on/off, BLE bursts) without lagging the real discharge. */
    filt = filt ? filt + (mv - filt) / 8 : mv;
    s_mv = filt;

    int p = wf_battery_pct_from_mv(filt + CAL_OFFSET_MV);
    /* On battery the charge only goes down; a jump up means a resync (first
     * reading, unplugged after charging, heavy-load dip recovered). */
    if (s_pct < 0 || p < s_pct || p >= s_pct + RISE_RESYNC) s_pct = p;
    return event;
}

/* Light-sleep accounting, fed from the IDLE task by esp_pm's exit callback. */
static volatile int64_t s_slept_us;
static volatile uint32_t s_sleeps;

static esp_err_t on_ls_exit(int64_t sleep_time_us, void *arg)
{
    (void)arg;
    s_slept_us += sleep_time_us;
    s_sleeps++;
    return ESP_OK;
}

#if CONFIG_PM_PROFILING
/* TEMP diagnostic — light-sleep is never entered on battery (log 2026-09-29):
 * find which PM lock is held. Each minute, parse esp_pm_dump_locks() and put
 * the lock held longest during the period (rtos* excluded: they are held
 * whenever a task runs) into the record: rsv = name1[4] pct1, then the BLE
 * connection parameters in force: rsv[5..6] interval (1.25 ms units),
 * rsv[7..8] slave latency, rsv[9] = 0xC1 marker (0xFF: not connected).
 * Every 10 min the full dump also goes to NVS ("pmdump") for full names. */
#define PM_MAXL 24
static char s_pm_buf[2048];

static void pm_top_locks(uint8_t *rsv, int period_s)
{
    static struct { char name[16]; long long us; } prev[PM_MAXL];
    static int nprev, calls;
    FILE *f = fmemopen(s_pm_buf, sizeof(s_pm_buf) - 1, "w");
    if (!f) return;
    esp_pm_dump_locks(f);
    long len = ftell(f);
    fclose(f);
    s_pm_buf[len < 0 ? 0 : len] = 0;

    char best[2][16] = { "", "" };
    long long bus[2] = { 0, 0 };
    for (char *line = strtok(s_pm_buf, "\n"); line; line = strtok(NULL, "\n")) {
        char name[16], type[16];
        int arg, active, total;
        long long us;
        if (sscanf(line, "%15s %15s %d %d %d %lld", name, type, &arg, &active, &total, &us) != 6)
            continue;
        long long d = us;
        int i;
        for (i = 0; i < nprev && strcmp(prev[i].name, name) != 0; i++) {}
        if (i < nprev) { d = us - prev[i].us; prev[i].us = us; }
        else if (nprev < PM_MAXL) { strcpy(prev[nprev].name, name); prev[nprev++].us = us; }
        if (strncmp(name, "rtos", 4) == 0) continue;
        if (d > bus[0]) { bus[1] = bus[0]; strcpy(best[1], best[0]); bus[0] = d; strcpy(best[0], name); }
        else if (d > bus[1]) { bus[1] = d; strcpy(best[1], name); }
    }
    long long span = period_s > 0 ? (long long)period_s * 1000000 : 0;
    memset(rsv, 0, 4);
    strncpy((char *)rsv, best[0], 4);
    long long p = span ? bus[0] * 100 / span : 0;
    rsv[4] = (uint8_t)(p > 100 ? 100 : p);
    uint16_t itvl, lat;
    if (ble_conn_params(&itvl, &lat)) {
        memcpy(rsv + 5, &itvl, 2);
        memcpy(rsv + 7, &lat, 2);
        rsv[9] = 0xC1;
    }

    /* Full dump to NVS every 10 min (on battery, where it matters). */
    if (++calls % 10 == 0 && !usb_serial_jtag_is_connected()) {
        FILE *g = fmemopen(s_pm_buf, sizeof(s_pm_buf) - 1, "w");
        if (g) {
            esp_pm_dump_locks(g);
            long n = ftell(g);
            fclose(g);
            s_pm_buf[n < 0 ? 0 : n] = 0;
            config_store_set_debug_text("pmdump", s_pm_buf);
        }
    }
}
#else
static void pm_top_locks(uint8_t *rsv, int period_s) { (void)rsv; (void)period_s; }
#endif

static void log_record(uint8_t event, int period_s, int screen_s)
{
    static int64_t last_us;
    int64_t now = esp_timer_get_time();
    int64_t span = last_us ? now - last_us : 0;
    last_us = now;
    int64_t slept = s_slept_us;   /* read-then-clear: a sleep ending in between */
    uint32_t n = s_sleeps;        /* just lands in the next record              */
    s_slept_us = 0;
    s_sleeps = 0;
    int pm = span > 0 ? (int)(slept * 1000 / span) : 0;

    batlog_rec_t r = {
        .time     = (uint32_t)time(NULL),
        .mv       = (uint16_t)s_mv,
        .pct      = (int8_t)s_pct,
        .flags    = (uint8_t)(event | (s_ext ? BATLOG_F_EXT : 0) |
                              (ble_is_connected() ? BATLOG_F_BLE : 0) |
                              (usb_serial_jtag_is_connected() ? BATLOG_F_USB : 0)),
        .screen_s = (uint8_t)screen_s,
        .bright   = (uint8_t)device_brightness_pct(),
        .period_s = (uint8_t)period_s,
        .sleep_pm = (uint16_t)(pm > 1000 ? 1000 : pm),
        .sleeps   = (uint16_t)(n > 0xFFFF ? 0xFFFF : n),
    };
    memset(r.rsv, 0xFF, sizeof(r.rsv));
    pm_top_locks(r.rsv, period_s);
    batlog_append(&r);
}

static void battery_task(void *arg)
{
    (void)arg;
    int secs = 0, screen = 0;
    log_record(BATLOG_F_BOOT, 0, 0);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
        uint8_t event = sample();
        secs++;
        if (device_display_awake()) screen++;
        /* Power events are logged at once (exact plug/unplug time), else one
         * record per minute; each covers the seconds since the previous one. */
        if (event || secs >= LOG_PERIOD_S) {
            log_record(event, secs, screen);
            secs = screen = 0;
        }
    }
}

void battery_init(void)
{
    adc_oneshot_unit_init_cfg_t unit = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&unit, &s_adc) != ESP_OK) {
        ESP_LOGE(TAG, "ADC1 init failed");
        return;
    }
    adc_oneshot_chan_cfg_t ch = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    adc_oneshot_config_channel(s_adc, (adc_channel_t)BSP_BAT_ADC_CH, &ch);

    adc_cali_curve_fitting_config_t cc = {
        .unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_curve_fitting(&cc, &s_cali) != ESP_OK) {
        ESP_LOGW(TAG, "no ADC calibration (eFuse) — using raw scale");
        s_cali = NULL;
    }

    /* First reading synchronously so the UI never starts on a placeholder. */
    int mv = read_mv();
    s_ext = mv >= EXT_ON_MV;
    s_mv = mv;
    if (s_ext) {
        s_pct = config_store_get_battery_pct();   /* last known, may be -1 */
    } else if (mv > 0) {
        s_pct = wf_battery_pct_from_mv(mv + CAL_OFFSET_MV);
    }
    ESP_LOGI(TAG, "VCC %d mV, %s, batt %d%%", mv, s_ext ? "external power" : "battery", s_pct);

    batlog_init();
#if CONFIG_PM_LIGHT_SLEEP_CALLBACKS
    esp_pm_sleep_cbs_register_config_t cbs = { .exit_cb = on_ls_exit };
    esp_err_t e = esp_pm_light_sleep_register_cbs(&cbs);
    if (e != ESP_OK) ESP_LOGW(TAG, "light-sleep callbacks: %s", esp_err_to_name(e));
#endif

    xTaskCreate(battery_task, "battery", 3072, NULL, 2, NULL);
}

int  battery_pct(void)       { return s_pct; }
bool battery_ext_power(void) { return s_ext; }
int  battery_mv(void)        { return s_mv; }
