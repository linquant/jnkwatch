/* FT3168 capacitive touch (I2C, polled) for the ESP32-S3-Touch-AMOLED-1.43.
 * Logic ported from the Waveshare demo (07_LVGL_Test/FT3168.cpp): switch the
 * controller to normal mode, then poll the touch-count register and read one
 * point. Uses the legacy driver/i2c.h API to match the proven demo. */
#include "touch_ft3168.h"
#include "board.h"
#include "display.h"   /* WATCH_SIZE */
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#define TOUCH_I2C_PORT I2C_NUM_0
#define TOUCH_TIMEOUT  pdMS_TO_TICKS(100)

static esp_err_t rd(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_master_write_read_device(TOUCH_I2C_PORT, BSP_I2C_ADDR_TOUCH,
                                        &reg, 1, buf, len, TOUCH_TIMEOUT);
}

void touch_ft3168_init(void)
{
    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 300 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_I2C_PORT, conf.mode, 0, 0, 0));

    uint8_t cmd[2] = { 0x00, 0x00 };  /* reg 0x00 <- 0x00: normal mode */
    i2c_master_write_to_device(TOUCH_I2C_PORT, BSP_I2C_ADDR_TOUCH,
                               cmd, sizeof(cmd), TOUCH_TIMEOUT);
}

bool touch_ft3168_read(uint16_t *x, uint16_t *y)
{
    uint8_t touches = 0;
    if (rd(0x02, &touches, 1) != ESP_OK || touches == 0)
        return false;

    uint8_t b[4];
    if (rd(0x03, b, sizeof(b)) != ESP_OK)
        return false;

    uint16_t px = (((uint16_t)b[0] & 0x0f) << 8) | b[1];
    uint16_t py = (((uint16_t)b[2] & 0x0f) << 8) | b[3];
    if (px >= WATCH_SIZE) px = WATCH_SIZE - 1;
    if (py >= WATCH_SIZE) py = WATCH_SIZE - 1;
    *x = px;
    *y = py;
    return true;
}
