# ESP32-S3-Touch-AMOLED-1.43 — hardware reference

Reference notes for porting this firmware to the Waveshare
**ESP32-S3-Touch-AMOLED-1.43** board. Pin/address values below are taken from the
**official demo sources (Demo-V3)** — the authoritative source — cross-checked with the
schematic; where the Waveshare wiki page disagrees it is noted (the wiki table was wrong).

Compiled 2026-09-21 from docs.waveshare.com and the demo package.

## Board at a glance
- **MCU**: ESP32-S3R8 (Xtensa LX7 dual-core @240 MHz), 16 MB flash, 8 MB PSRAM.
- **Display**: 1.43" round **AMOLED 466×466**, QSPI, driver **SH8601** (a.k.a. SH8601/CO5300), 16.7 M colors, 16 bpp (RGB565).
- **Touch**: **FT3168** capacitive, I2C.
- **IMU**: **QMI8658** (accel + gyro), I2C.
- **RTC**: **PCF85063**, I2C (+ SH1.0 RTC-battery header).
- **Power**: 3.7 V Li-ion (MX1.25 header) + charger; battery sense on ADC. No audio.
- **Other**: TF/SD slot, USB-C, BOOT + RST buttons, UART/I2C headers.

## Verified pinout (from Demo-V3 `lcd_config.h` / `i2c_bsp.c` / `touch_bsp.c` / `adc_bsp.c`)

### Display — QSPI (SH8601)
| Signal | GPIO |
|---|---|
| CS   | 9  |
| PCLK | 10 |
| DATA0 | 11 |
| DATA1 | 12 |
| DATA2 | 13 |
| DATA3 | 14 |
| RST  | 21 |
| Backlight | none (−1) — brightness is a display command |

- 16 bits/pixel (RGB565). Init: `0x11` (SLPOUT, 120 ms) → `0x53`=`0x20` (brightness ctrl on) → `0x51`=level → `0x29` (DISPON).
- **Brightness / backlight** = DCS command **`0x51`**, 1 byte `0x00..0xFF`. So `watch_set_backlight(pct)` sends `0x51` with `pct*255/100` over QSPI — NOT an LEDC PWM.
- ⚠️ The wiki "Pinout" table listed CS=10/CLK=11/D0=12…/RST=38/EN=43 — **incorrect**; use the values above.

### I2C bus (shared) — SDA=GPIO47, SCL=GPIO48
| Device | 7-bit addr | INT |
|---|---|---|
| FT3168 touch | `0x38` | (see schematic) |
| QMI8658 IMU  | `0x6B` | GPIO? (schematic: IMU_INT) |
| PCF85063 RTC | `0x51` | (schematic: RTC_INT) |

- Bus speed in demo: 300–400 kHz, internal pull-ups enabled.
- FT3168: point count at reg `0x02`, X/Y at reg `0x03` (4 bytes, 12-bit each).

### Battery / buttons / SD
- **Battery ADC**: ADC1, **channel 3** (ESP32-S3 ADC1_CH3 = **GPIO4**), atten 12 dB. (Wiki said GPIO5 = CH4 — trust the demo: CH3.)
- **BOOT** button: GPIO0. **RST**: chip EN.
- **SD/TF**: SPI (selectable SDMMC in demo) — see `sd_card_bsp` in the demo for pins.

## Software (from the demos)
- **ESP-IDF**: requires **v5.3.1+** (screenshots use 5.5.2). Display via an `esp_lcd_sh8601` component; LVGL integrated (`lv_conf.h`, 16-bit color). Examples 01_ADC … 09_FactoryProgram.
- **Arduino**: **Arduino_GFX** ("GFX_Library_for_Arduino") + **Arduino_DriveBus** + **LVGL v8.4.0** (offline install). Display has no HW rotation — rotate in software.
- The display driver source `esp_lcd_sh8601.{c,h}` and `lcd_bsp.c` in the demo are the reference for `platform/display.c`.

## Mapping to our port (milestone 2 — see README)
- `platform/display.c` flush → QSPI DMA to SH8601 using the pins above; `LV_DISPLAY_RENDER_MODE_PARTIAL` + PSRAM buffers; RGB565.
- `watch_set_backlight()` → DCS `0x51`.
- `watch_now()` seam → PCF85063 at `0x51` on the shared I2C bus.
- Touch indev → FT3168 at `0x38` (regs `0x02`/`0x03`).
- `watch_read_accel()` → QMI8658 at `0x6B`.
- `watch_battery_pct()` → ADC1 CH3 (GPIO4), 12 dB, with a Li-ion voltage→% curve.

## Resource links
Demo / hardware:
- Demo code (Demo-V3, ~115 MB): https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.43/ESP32-S3-AMOLED-1.43-Demo-V3.zip
- Schematic (PDF): https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.43/ESP32-S3-Touch-AMOLED-1.43-Schematic.pdf
- 3D/STEP (zip): https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.43/ESP32-S3-Touch-AMOLED-1.43-3DFile.zip

Datasheets:
- SH8601 display: https://files.waveshare.com/wiki/common/SH8601Z0_V0.0.pdf
- FT3168 touch: https://files.waveshare.com/wiki/common/DATA_SHEET_FT3168.pdf
- QMI8658 IMU: https://files.waveshare.com/wiki/common/QMI8658C_datasheet_rev_0.9.pdf
- PCF85063 RTC: https://files.waveshare.com/wiki/common/Pcf85063atl1118-NdPQpTGE-loeW7GbZ7.pdf
- ESP32-S3 datasheet: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf
- ESP32-S3 TRM: https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf

Wiki pages:
- Main: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43
- Resources: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43/Resources-And-Documents
- ESP-IDF: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43/ESP-IDF
- Arduino: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43/Arduino
- FAQ: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43/FAQ

Community projects:
- X-Wing game (thelastoutpostworkshop): https://github.com/thelastoutpostworkshop/waveshare_esp32s3_1.43_amoled_xwing_game
- Boost gauge (kytos22): https://github.com/kytos22/esp32-s3-civic-boost-gauge
- Numrow touchpad (fholmer): https://github.com/fholmer/numrow-touchpad

> The datasheets, schematic and demo zip were downloaded to `docs/hardware/vendor/`
> (git-ignored — vendor binaries are kept local, not committed). Re-fetch them from the
> links above.
