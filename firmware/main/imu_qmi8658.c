/* QMI8658 accelerometer (I2C, polled). Minimal bring-up derived from the
 * Waveshare demo (03_I2C_QMI8658): check WHO_AM_I, enable address
 * auto-increment, configure the accel (±4g, 125 Hz), enable it. The I2C bus is
 * assumed already installed (touch_ft3168_init does it). */
#include "imu_qmi8658.h"
#include "board.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define IMU_I2C_PORT I2C_NUM_0
#define IMU_TIMEOUT  pdMS_TO_TICKS(100)

#define REG_WHOAMI   0x00
#define REG_CTRL1    0x02
#define REG_CTRL2    0x03
#define REG_CTRL3    0x04
#define REG_CTRL8    0x09
#define REG_CTRL9    0x0A
#define REG_CTRL7    0x08
#define REG_STATUS0  0x2E
#define REG_AX_L     0x35
#define REG_RESET    0x60

#define WHOAMI_VAL   0x05
#define REG_CTRL5    0x06
#define CTRL1_ADDR_AI 0x40          /* auto-increment reg addr, little-endian */
#define CTRL2_ACC    (0x20 | 0x05)  /* ±8g | 250 Hz (matches the demo) */
#define CTRL3_GYR    (0x60 | 0x05)  /* ±1024 dps | 250 Hz (matches the demo) */
#define CTRL7_EN     0x03           /* enable accel + gyro (like the demo) */
#define ACC_LSB_PER_G 4096.0        /* ±8g full scale (measured: ~4096 at 1g) */

static const char *TAG = "qmi8658";
static bool s_ready;

static esp_err_t wr(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_write_to_device(IMU_I2C_PORT, BSP_I2C_ADDR_IMU, b, 2, IMU_TIMEOUT);
}

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_write_read_device(IMU_I2C_PORT, BSP_I2C_ADDR_IMU,
                                        &reg, 1, buf, n, IMU_TIMEOUT);
}

bool imu_qmi8658_init(void)
{
    /* Soft reset: the chip keeps its state across ESP32 resets, and a bad prior
     * config can leave the output registers frozen at one sample. */
    wr(REG_RESET, 0xB0);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t id = 0;
    for (int i = 0; i < 5 && id != WHOAMI_VAL; i++) {
        rd(REG_WHOAMI, &id, 1);
        if (id != WHOAMI_VAL) vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (id != WHOAMI_VAL) {
        ESP_LOGW(TAG, "WHO_AM_I=0x%02x (expected 0x05); IMU disabled", id);
        return false;
    }
    /* On-demand calibration, as the demo runs before configuring. This CTRL9
     * command (with its long settle) appears to start the sensor pipeline. */
    wr(REG_CTRL9, 0xA2);
    vTaskDelay(pdMS_TO_TICKS(2300));
    wr(REG_CTRL9, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));

    wr(REG_CTRL1, CTRL1_ADDR_AI);
    wr(REG_CTRL8, 0xC0);       /* data-valid handshake, like the demo */
    wr(REG_CTRL7, 0x00);       /* disable while configuring */
    wr(REG_CTRL2, CTRL2_ACC);
    wr(REG_CTRL3, CTRL3_GYR);
    wr(REG_CTRL5, 0x00);       /* no low-pass filter */
    wr(REG_CTRL7, CTRL7_EN);   /* enable accel + gyro */
    vTaskDelay(pdMS_TO_TICKS(10));
    s_ready = true;
    uint8_t c2 = 0, c7 = 0;   /* TEMP: confirm config applied post-reset */
    rd(REG_CTRL2, &c2, 1); rd(REG_CTRL7, &c7, 1);
    ESP_LOGI(TAG, "QMI8658 ready; CTRL2=0x%02x CTRL7=0x%02x", c2, c7);
    return true;
}

int imu_qmi8658_status0(void)
{
    uint8_t s = 0;
    return rd(REG_STATUS0, &s, 1) == ESP_OK ? s : -1;
}

bool imu_qmi8658_read_accel(double *ax, double *ay, double *az)
{
    if (!s_ready) return false;
    uint8_t st;
    rd(REG_STATUS0, &st, 1);   /* read status first (releases the data lock) */
    uint8_t b[6];
    if (rd(REG_AX_L, b, sizeof(b)) != ESP_OK) return false;
    int16_t rx = (int16_t)((b[1] << 8) | b[0]);
    int16_t ry = (int16_t)((b[3] << 8) | b[2]);
    int16_t rz = (int16_t)((b[5] << 8) | b[4]);
    /* The chip's Z points away from the screen; negate so az = +1g with the
     * screen up, which is what the lift-to-wake detector expects. */
    *ax = rx / ACC_LSB_PER_G;
    *ay = ry / ACC_LSB_PER_G;
    *az = -rz / ACC_LSB_PER_G;
    return true;
}
