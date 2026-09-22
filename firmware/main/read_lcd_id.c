/* Ported verbatim from the Waveshare ESP32-S3-AMOLED-1.43 demo
 * (07_LVGL_Test/components/read_lcd_id_bsp). Bit-bangs a manual reset + reads
 * the controller ID so the correct init sequence can be chosen. */
#include "read_lcd_id.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define bit_mask (uint64_t)0x01

#define PIN_CS    9
#define PIN_PCLK  10
#define PIN_D0    11
#define PIN_D1    12
#define PIN_D2    13
#define PIN_D3    14
#define PIN_RST   21

#define lcd_cs_1  gpio_set_level(PIN_CS, 1)
#define lcd_cs_0  gpio_set_level(PIN_CS, 0)
#define lcd_clk_1 gpio_set_level(PIN_PCLK, 1)
#define lcd_clk_0 gpio_set_level(PIN_PCLK, 0)
#define lcd_d0_1  gpio_set_level(PIN_D0, 1)
#define lcd_d0_0  gpio_set_level(PIN_D0, 0)
#define lcd_rst_1 gpio_set_level(PIN_RST, 1)
#define lcd_rst_0 gpio_set_level(PIN_RST, 0)
#define read_d0   gpio_get_level(PIN_D0)

static void lcd_gpio_init(void)
{
    gpio_config_t c = {0};
    c.intr_type = GPIO_INTR_DISABLE;
    c.mode = GPIO_MODE_OUTPUT;
    c.pin_bit_mask = (bit_mask << PIN_CS) | (bit_mask << PIN_PCLK) |
                     (bit_mask << PIN_D0) | (bit_mask << PIN_D1) |
                     (bit_mask << PIN_D2) | (bit_mask << PIN_D3) |
                     (bit_mask << PIN_RST);
    c.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&c));
}

static void sda_read_mode(void)
{
    gpio_config_t c = {0};
    c.mode = GPIO_MODE_INPUT;
    c.pin_bit_mask = (bit_mask << PIN_D0);
    c.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&c));
}

static void sda_write_mode(void)
{
    gpio_config_t c = {0};
    c.mode = GPIO_MODE_OUTPUT;
    c.pin_bit_mask = (bit_mask << PIN_D0);
    c.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&c));
}

static void SPI_1L_SendData(uint8_t dat)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (dat & 0x80) lcd_d0_1; else lcd_d0_0;
        dat <<= 1;
        lcd_clk_0;
        lcd_clk_1;
    }
}

static void SPI_ReadComm(uint8_t regval)
{
    SPI_1L_SendData(0x03);
    SPI_1L_SendData(0x00);
    SPI_1L_SendData(regval);
    SPI_1L_SendData(0x00);
}

static uint8_t SPI_ReadData_Continue(void)
{
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++) {
        lcd_clk_0;
        sda_read_mode();
        esp_rom_delay_us(1);
        dat = (dat << 1) | read_d0;
        sda_write_mode();
        lcd_clk_1;
        esp_rom_delay_us(1);
    }
    return dat;
}

uint8_t read_lcd_id(void)
{
    lcd_gpio_init();
    lcd_rst_1;
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_rst_0;
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_rst_1;
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cs_0;
    SPI_ReadComm(0xDA);
    uint8_t ret = SPI_ReadData_Continue();
    lcd_cs_1;
    ESP_LOGI("lcd_id", "0x%02x", ret);
    return ret;
}
