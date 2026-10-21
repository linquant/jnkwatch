/* FT3168 capacitive touch → LVGL v9 pointer indev (ESP32-S3-Touch-AMOLED-1.43).
 *
 * DEVICE-ONLY: compiled by the ESP-IDF firmware, not the PC/SDL build.
 * Verified against the Waveshare Demo-V3 touch_bsp.c:
 *   - I2C: SDA=GPIO47, SCL=GPIO48, addr 0x38
 *   - reg 0x00 = mode (write 0x00 → normal)
 *   - reg 0x02 = number of touch points
 *   - reg 0x03 = P1 coords: X=((b0&0x0F)<<8)|b1, Y=((b2&0x0F)<<8)|b3
 * Panel is 466×466. Coordinates come in panel pixels; if the display is
 * rotated (the demo uses EXAMPLE_Rotate_90), enable the mapping macros below
 * so touch matches what's drawn. Ref: GitLab issue #3.
 */
#include "touch_ft3168.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FT3168_ADDR        0x38
#define TP_PIN_SDA         47
#define TP_PIN_SCL         48
#define TP_I2C_HZ          400000
#define TP_W               466
#define TP_H               466
#define TP_TIMEOUT         pdMS_TO_TICKS(50)

/* Orientation mapping — tune to match the display rotation (issue #1).
 * Start with all 0; flip until a touch lands under your finger. */
#define TP_SWAP_XY   0
#define TP_MIRROR_X  0
#define TP_MIRROR_Y  0

static const char *TAG = "ft3168";
static i2c_port_t s_port = I2C_NUM_0;

static esp_err_t ft_read(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(s_port, FT3168_ADDR, &reg, 1,
                                        buf, len, TP_TIMEOUT);
}

static esp_err_t ft_write(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_write_to_device(s_port, FT3168_ADDR, b, 2, TP_TIMEOUT);
}

esp_err_t ft3168_i2c_bus_init(i2c_port_t port)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TP_PIN_SDA,
        .scl_io_num = TP_PIN_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TP_I2C_HZ,
    };
    esp_err_t err = i2c_param_config(port, &cfg);
    if (err != ESP_OK) return err;
    err = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    /* Already installed by another peripheral (IMU/RTC) is fine. */
    if (err == ESP_ERR_INVALID_STATE) err = ESP_OK;
    return err;
}

/* Read one touch point; returns true if a finger is down. */
static bool ft_read_point(int *x, int *y)
{
    uint8_t n = 0;
    if (ft_read(0x02, &n, 1) != ESP_OK) return false;
    if ((n & 0x0F) == 0) return false;

    uint8_t b[4];
    if (ft_read(0x03, b, 4) != ESP_OK) return false;
    int rx = (int)(((uint16_t)(b[0] & 0x0F) << 8) | b[1]);
    int ry = (int)(((uint16_t)(b[2] & 0x0F) << 8) | b[3]);

#if TP_SWAP_XY
    int t = rx; rx = ry; ry = t;
#endif
#if TP_MIRROR_X
    rx = (TP_W - 1) - rx;
#endif
#if TP_MIRROR_Y
    ry = (TP_H - 1) - ry;
#endif
    if (rx < 0) rx = 0; else if (rx >= TP_W) rx = TP_W - 1;
    if (ry < 0) ry = 0; else if (ry >= TP_H) ry = TP_H - 1;
    *x = rx; *y = ry;
    return true;
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static int last_x = 0, last_y = 0;
    int x, y;
    if (ft_read_point(&x, &y)) {
        last_x = x; last_y = y;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = last_x;      /* keep last point on release */
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_indev_t *touch_ft3168_create(i2c_port_t port)
{
    s_port = port;
    if (ft3168_i2c_bus_init(port) != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed");
        return NULL;
    }
    (void)ft_write(0x00, 0x00);      /* switch to normal mode */

    /* Probe: reading the point count should succeed. */
    uint8_t n;
    if (ft_read(0x02, &n, 1) != ESP_OK)
        ESP_LOGW(TAG, "FT3168 not responding at 0x%02X", FT3168_ADDR);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    ESP_LOGI(TAG, "FT3168 touch indev ready (SDA%d/SCL%d @0x%02X)",
             TP_PIN_SDA, TP_PIN_SCL, FT3168_ADDR);
    return indev;
}
