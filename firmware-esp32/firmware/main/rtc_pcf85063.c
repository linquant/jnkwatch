/* PCF85063 RTC driver (I2C). Register map (datasheet):
 *   0x00 Control_1 (bit5 STOP)     0x04 Seconds (bit7 OS = osc-stop / invalid)
 *   0x05 Minutes   0x06 Hours(24h) 0x07 Days  0x08 Weekdays
 *   0x09 Months    0x0A Years(00-99, base 2000).  All time fields are BCD.
 * The I2C bus is assumed already installed (touch_ft3168_init does it). */
#include "rtc_pcf85063.h"
#include "board.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

#define RTC_I2C_PORT I2C_NUM_0
#define RTC_TIMEOUT  pdMS_TO_TICKS(100)
#define REG_CTRL1    0x00
#define REG_SECONDS  0x04

static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0f); }
static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

bool rtc_pcf85063_get(struct tm *out)
{
    uint8_t reg = REG_SECONDS, b[7];
    if (i2c_master_write_read_device(RTC_I2C_PORT, BSP_I2C_ADDR_RTC,
                                     &reg, 1, b, sizeof(b), RTC_TIMEOUT) != ESP_OK)
        return false;
    if (b[0] & 0x80)          /* OS flag: oscillator stopped -> time invalid */
        return false;

    out->tm_sec  = bcd2dec(b[0] & 0x7f);
    out->tm_min  = bcd2dec(b[1] & 0x7f);
    out->tm_hour = bcd2dec(b[2] & 0x3f);
    out->tm_mday = bcd2dec(b[3] & 0x3f);
    out->tm_wday = b[4] & 0x07;
    out->tm_mon  = bcd2dec(b[5] & 0x1f) - 1;
    out->tm_year = bcd2dec(b[6]) + 100;   /* years since 1900 (base 2000) */
    out->tm_isdst = 0;
    return true;
}

void rtc_pcf85063_set(const struct tm *t)
{
    uint8_t stop[2] = { REG_CTRL1, 0x20 };  /* STOP=1 while writing */
    i2c_master_write_to_device(RTC_I2C_PORT, BSP_I2C_ADDR_RTC, stop, 2, RTC_TIMEOUT);

    uint8_t buf[8] = {
        REG_SECONDS,
        dec2bcd(t->tm_sec),          /* also clears the OS flag (bit7 = 0) */
        dec2bcd(t->tm_min),
        dec2bcd(t->tm_hour),
        dec2bcd(t->tm_mday),
        (uint8_t)(t->tm_wday & 0x07),
        dec2bcd(t->tm_mon + 1),
        dec2bcd((uint8_t)(t->tm_year - 100)),
    };
    i2c_master_write_to_device(RTC_I2C_PORT, BSP_I2C_ADDR_RTC, buf, sizeof(buf), RTC_TIMEOUT);

    uint8_t start[2] = { REG_CTRL1, 0x00 };  /* STOP=0 -> run */
    i2c_master_write_to_device(RTC_I2C_PORT, BSP_I2C_ADDR_RTC, start, 2, RTC_TIMEOUT);
}
