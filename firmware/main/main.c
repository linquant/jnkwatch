/* ESP32-S3-Touch-AMOLED-1.43 — app entry.
 * Brings up the SH8601/CO5300 QSPI AMOLED and LVGL v9 manually (mirroring the
 * proven Waveshare demo esp_lcd path), then runs the shared ui_init()/ui_tick()
 * from the repo's ui/. M1: display + UI only. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_sh8601.h"
#include "lvgl.h"

#include <time.h>
#include <sys/time.h>

#include "board.h"
#include "read_lcd_id.h"
#include "touch_ft3168.h"
#include "rtc_pcf85063.h"
#include "imu_qmi8658.h"
#include "config_store.h"
#include "device_hal.h"
#include "ble_server.h"
#include "display.h"   /* WATCH_SIZE */
#include "ui.h"

static const char *TAG = "jnkwatch";

#define LCD_H WATCH_SIZE
#define LCD_V WATCH_SIZE
#define SH8601_ID 0x86  /* read_lcd_id() returns this for SH8601, else CO5300 */

static const sh8601_lcd_init_cmd_t sh8601_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 10},
    {0x51, (uint8_t []){0x00}, 1, 10},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};
static const sh8601_lcd_init_cmd_t co5300_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 80},
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 1},
    {0x63, (uint8_t []){0xFF}, 1, 1},
    {0x51, (uint8_t []){0x00}, 1, 1},
    {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};

static esp_lcd_panel_handle_t s_panel;
static lv_display_t *s_disp;

/* esp_lcd calls this when a colour transfer to the panel finishes. */
static bool on_color_done(esp_lcd_panel_io_handle_t io,
                          esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    (void)io; (void)e; (void)ctx;
    lv_display_flush_ready(s_disp);
    return false;
}

/* The SH8601 needs even-aligned windows: enlarge the invalidated area so LVGL
 * renders x1/y1 even and x2/y2 odd. */
static void even_rounder(lv_event_t *e)
{
    lv_area_t *a = (lv_area_t *)lv_event_get_param(e);
    a->x1 &= ~1;
    a->y1 &= ~1;
    a->x2 |= 1;
    a->y2 |= 1;
}

static void flush_cb(lv_display_t *d, const lv_area_t *area, uint8_t *px_map)
{
    (void)d;
    uint32_t px = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    lv_draw_sw_rgb565_swap(px_map, px);   /* SH8601 wants byte-swapped RGB565 */
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);
    /* lv_display_flush_ready() is called from on_color_done(). */
}

/* LVGL pulls one touch point each frame; report the last position on release
 * so drags (tileview swipes) track smoothly. */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static uint16_t lx, ly;
    uint16_t x, y;
    if (touch_ft3168_read(&x, &y)) {
        lx = x; ly = y;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = lx;
        data->point.y = ly;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* struct tm (UTC) -> epoch, without timegm() (absent in ESP-IDF newlib).
 * Howard Hinnant's days-from-civil algorithm. */
static time_t utc_epoch(const struct tm *t)
{
    int y = t->tm_year + 1900;
    int m = t->tm_mon + 1;
    int d = t->tm_mday;
    y -= (m <= 2);
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long days = (long)era * 146097 + (long)doe - 719468;
    return (time_t)days * 86400 + t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
}

/* Timezone + system clock from the PCF85063 RTC. The RTC holds UTC; TZ turns
 * it into local Paris time (with automatic DST). A build with
 * -DWATCH_SET_RTC_EPOCH=<utc-epoch> force-seeds the RTC once from that instant;
 * normal builds just read whatever the (battery-backed) RTC is keeping. */
static void clock_init(void)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);  /* Europe/Paris */
    tzset();

    struct tm rt;
    bool valid = rtc_pcf85063_get(&rt);
#if defined(WATCH_SET_RTC_EPOCH) && defined(WATCH_FORCE_RTC)
    valid = false;   /* one-shot: force a reseed from the build epoch over USB */
#endif
    if (!valid) {
        /* Fresh chip or power was lost: seed from the build epoch so the watch
         * still shows a sane recent time until it is set properly. */
#ifdef WATCH_SET_RTC_EPOCH
        if ((long)WATCH_SET_RTC_EPOCH > 0) {
            time_t seed = (time_t)WATCH_SET_RTC_EPOCH;
            struct tm g;
            gmtime_r(&seed, &g);
            rtc_pcf85063_set(&g);
            valid = rtc_pcf85063_get(&rt);
        }
#endif
    }
    if (valid) {
        struct timeval tv = { .tv_sec = utc_epoch(&rt) };  /* RTC is UTC */
        settimeofday(&tv, NULL);
        time_t now = tv.tv_sec; struct tm lt; localtime_r(&now, &lt);
        char b[40]; strftime(b, sizeof(b), "%a %Y-%m-%d %H:%M %Z", &lt);
        ESP_LOGI(TAG, "clock from RTC: %s", b);
    } else {
        ESP_LOGW(TAG, "RTC time invalid (never set / power lost)");
    }
}

static uint32_t tick_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void lvgl_task(void *arg)
{
    (void)arg;
    for (;;) {
        lv_timer_handler();
        ui_tick();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot");

    /* NVS + inbound-config queue first (NVS is also needed by BLE bonding). */
    device_hal_init();
    config_store_init();

    /* Touch first, like the Waveshare demo: the FT3168 reset is tied to the
     * OLED_RESET line (GPIO21), so bringing touch up BEFORE the LCD lets the
     * panel's own reset pulse also reset the touch and leave it freshly
     * booted. Doing it after (our previous order) left touch mis-initialised:
     * it scanned but never asserted a touch (td_status stuck at 0). */
    touch_ft3168_init();

    /* Detect the controller (also runs the panel's long reset sequence). */
    uint8_t lcd_id = read_lcd_id();
    bool is_co5300 = (lcd_id != SH8601_ID);
    ESP_LOGI(TAG, "LCD id 0x%02x -> %s", lcd_id, is_co5300 ? "CO5300" : "SH8601");

    /* --- QSPI bus + panel ---------------------------------------------- */
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        BSP_LCD_PCLK, BSP_LCD_DATA0, BSP_LCD_DATA1, BSP_LCD_DATA2, BSP_LCD_DATA3,
        LCD_H * LCD_V * (BSP_LCD_BITS_PP / 8));
    ESP_ERROR_CHECK(spi_bus_initialize(BSP_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_panel_io_spi_config_t io_cfg =
        SH8601_PANEL_IO_QSPI_CONFIG(BSP_LCD_CS, on_color_done, NULL);
    sh8601_vendor_config_t vendor_cfg = {
        .init_cmds = is_co5300 ? co5300_init_cmds : sh8601_init_cmds,
        .init_cmds_size = is_co5300
            ? sizeof(co5300_init_cmds) / sizeof(co5300_init_cmds[0])
            : sizeof(sh8601_init_cmds) / sizeof(sh8601_init_cmds[0]),
        .flags = { .use_qspi_interface = 1 },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_HOST, &io_cfg, &io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PP,
        .vendor_config = &vendor_cfg,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    if (is_co5300) esp_lcd_panel_set_gap(s_panel, 6, 0);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    /* --- LVGL v9 (manual) ---------------------------------------------- */
    lv_init();
    lv_tick_set_cb(tick_ms);

    s_disp = lv_display_create(LCD_H, LCD_V);
    lv_display_set_flush_cb(s_disp, flush_cb);
    lv_display_add_event_cb(s_disp, even_rounder, LV_EVENT_INVALIDATE_AREA, NULL);

    const size_t buf_px = LCD_H * LCD_V / 10;  /* ~1/10 screen, ×2 */
    void *buf1 = heap_caps_malloc(buf_px * 2, MALLOC_CAP_DMA);
    void *buf2 = heap_caps_malloc(buf_px * 2, MALLOC_CAP_DMA);
    ESP_ERROR_CHECK((buf1 && buf2) ? ESP_OK : ESP_ERR_NO_MEM);
    lv_display_set_buffers(s_disp, buf1, buf2, buf_px * 2,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Timezone + clock from the RTC (I2C bus already up from touch init). */
    clock_init();

    /* IMU (accelerometer) for lift-to-wake + the aura activity level. */
    imu_qmi8658_init();

    /* --- Touch indev (FT3168 already inited + reset above) ------------- */
    vTaskDelay(pdMS_TO_TICKS(200));  /* let the controller finish booting */
    lv_indev_t *touch = lv_indev_create();
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, touch_read_cb);
    lv_indev_set_display(touch, s_disp);

    ui_init();

    /* Apply the persisted config on first UI tick (enqueue before the task runs). */
    {
        wf_stored_config_t c;
        config_store_get(&c);
        wf_config_msg_t m = {
            .kind = WF_CFG_SETTINGS, .brightness = c.brightness,
            .sleep_ms = c.sleep_ms, .wrist_wake = c.wrist_wake, .accent = c.accent,
        };
        device_config_enqueue(&m);
    }

    xTaskCreate(lvgl_task, "lvgl", 8 * 1024, NULL, 3, NULL);

    /* Companion-app BLE GATT server (advertises as JNKWatch). */
    ble_server_start();

    ESP_LOGI(TAG, "UI up (%dx%d), touch ready, BLE up", LCD_H, LCD_V);
}
