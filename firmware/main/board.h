#pragma once
/* ESP32-S3-Touch-AMOLED-1.43 board pins (verified from the Waveshare demo /
 * docs/hardware/ESP32-S3-Touch-AMOLED-1.43.md). */

/* Display — SH8601 QSPI */
#define BSP_LCD_SPI_HOST   SPI2_HOST
#define BSP_LCD_CS         9
#define BSP_LCD_PCLK       10
#define BSP_LCD_DATA0      11
#define BSP_LCD_DATA1      12
#define BSP_LCD_DATA2      13
#define BSP_LCD_DATA3      14
#define BSP_LCD_RST        21
#define BSP_LCD_EN         42   /* OLED_EN: panel power enable (drive HIGH) */
#define BSP_LCD_BITS_PP    16   /* RGB565 */

/* Shared I2C bus (touch / IMU / RTC) */
#define BSP_I2C_SDA        47
#define BSP_I2C_SCL        48
#define BSP_I2C_ADDR_TOUCH 0x38 /* FT3168   */
#define BSP_I2C_ADDR_IMU   0x6B /* QMI8658  */
#define BSP_I2C_ADDR_RTC   0x51 /* PCF85063 */

/* Battery sense: ADC1 channel 3 (= GPIO4) */
#define BSP_BAT_ADC_CH     3

#define BSP_BOOT_BTN       0
