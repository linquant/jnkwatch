# JNKWatch — a hackable open-source smartwatch

A round smartwatch built in the open: a **platform-independent LVGL v9** interface that runs
on a desktop **PC emulator** and on a real **ESP32-S3 AMOLED** board, paired with an
**Android companion app**. The exact same UI source compiles for both targets — only a thin
driver layer changes — so the whole watchface can be developed, screenshotted and
unit-tested on a laptop before any hardware is involved.

Target board: [Waveshare **ESP32-S3-Touch-AMOLED-1.43**](https://www.waveshare.com/product/esp32-s3-touch-amoled-1.43.htm)
— ESP32-S3, round **AMOLED 466×466** (SH8601 QSPI), capacitive touch **FT3168**,
**QMI8658** IMU, **PCF85063** RTC, Bluetooth LE.

![Watch faces](docs/screenshots/faces.png)

> License: **Apache-2.0** · Firmware (C/LVGL + ESP-IDF) · Android companion (Kotlin/Compose)

---

## ⬇️ Downloads

Prebuilt binaries are published on the [**Releases**](https://gitlab.com/johann.nowacki/jnkwatch/-/releases) page.
The links below always point to the **latest** release:

| What | Download | Notes |
|------|----------|-------|
| 📱 **Android companion** | [**jnkwatch-debug.apk**](https://gitlab.com/johann.nowacki/jnkwatch/-/releases/permalink/latest/downloads/jnkwatch-debug.apk) | Install on Android 8.0+ (allow "install from unknown sources"). |
| ⌚ **Watch firmware** | [**jnkwatch.bin**](https://gitlab.com/johann.nowacki/jnkwatch/-/releases/permalink/latest/downloads/jnkwatch.bin) | Flashable image for the ESP32-S3-Touch-AMOLED-1.43 — see below. |

### Flash the watch firmware

The app image flashes to the `factory` partition at offset **`0x10000`**. With
[esptool](https://docs.espressif.com/projects/esptool/) installed and the watch connected over USB:

```sh
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 write_flash 0x10000 jnkwatch.bin
```

> This updates the app on a board that already has the bootloader + partition table (i.e. one
> that has been flashed from source at least once). For a **fresh** board, build from source and
> run `idf.py -C firmware flash` once (it writes the bootloader, partition table and app together).

### Install the Android app

Download the APK above and open it on the phone, or via ADB:

```sh
adb install jnkwatch-debug.apk
```

Then open the app, grant Bluetooth + notification access, and connect to the watch
(advertised as `JNKWatch`).

---

## Features

- **Swipeable faces & apps** in an `lv_tileview` — a horizontal strip with vertical pairs:
  `actions | notifications | [clock ⇅ aura] | settings | debug | [apps ⇅ level]`.
- **Dual-display clock** (Casio-style): big LCD time + a status column (date, battery,
  weather, activity/steps, unread bell).
- **Aura** living watchface: a tilt-gravity particle field driven by the IMU — particles pool
  with gravity and *shake-to-explode*.
- **Notifications**: a latest-notification card (tap to dismiss & advance) with an unread
  **bell** on the clock; fed by the phone (app notifications, SMS and calls).
- **Action buttons**: a customizable 2×3 shortcut grid; each press fires a BLE event the phone
  turns into an **HTTP POST webhook**.
- **Apps**: a launcher with a live **bubble level** (accelerometer), plus a **step counter**
  (progress ring to a daily goal) and **Probes** (live server values pushed from the phone).
- **6 accent "teintes"** applied consistently across every face (Rose, Vert, Cyan, Violet,
  Or, Bleu) — swap live in Settings.
- **Power-aware**: screen-off standby, `esp_pm` light-sleep, low-power BLE and an ultra-low
  "watch-only" mode that keeps the clock alive with the radio off.

## Android companion

A Kotlin / Jetpack Compose app that connects over BLE and feeds the watch everything it needs,
built around a documented GATT contract — see [docs/ble-protocol.md](docs/ble-protocol.md):

- Forwards **notifications, SMS and incoming calls** to the watch.
- Fetches **weather** (Open-Meteo) and pushes it to the clock face.
- Maps each **action button** to a configurable **HTTP POST webhook** (custom body + headers).
- Polls **server probes** (GET endpoints) and pushes the values to the Probes face.
- Keeps **time and settings** mirrored between phone and watch.

## Architecture

The UI is plain **LVGL v9** split from a thin hardware layer, so one codebase targets three outputs:

```
core/      pure LOGIC — unit-tested, NO LVGL/platform dependency
ui/        LVGL faces (turn core/ results into widgets) — shared by every target
platform/  display + tick + HAL (PC framebuffer AND ESP32-S3)
firmware/  ESP-IDF app (device drivers: SH8601 panel, FT3168 touch, QMI8658 IMU,
           PCF85063 RTC, battery/ADC, NimBLE GATT server, NVS config store)
shot/      headless target: renders one frame to a PNG (for CI / review)
sim/       interactive target: SDL2 window with mouse-as-touch
android/   Kotlin/Jetpack Compose companion app
tests/     Unity unit tests for core/
third_party/ stb_image_write.h (PNG) + Unity (submodule)
lvgl/      LVGL v9 (submodule)
```

## Build from source

### PC emulator

```sh
sudo apt install cmake build-essential libsdl2-dev   # prerequisites
git clone --recursive https://gitlab.com/johann.nowacki/jnkwatch.git
cd jnkwatch                      # already cloned? git submodule update --init --recursive
./build.sh                       # configures + builds all targets into ./build
./build/watch_sim                # interactive SDL2 window (mouse = touch)
./shot.sh frame.png <face> <teinte>   # headless: render one face to a PNG
./test.sh                        # run the Unity/CTest suite
```

`<face>`: 0=actions 1=notifications 2=clock 3=aura 4=settings 5=debug 6=apps 7=level.
`<teinte>`: 0..5 (Rose, Vert, Cyan, Violet, Or, Bleu).

`./pixels.sh --check` (also run in CI as the `amoled_budget` test) fails if any face lights too
many pixels — a guard against a bright background regressing AMOLED power.

### Device firmware (ESP-IDF ≥ 5.3)

```sh
idf.py -C firmware set-target esp32s3
idf.py -C firmware flash monitor
```

## Test-driven development

Logic lives in `core/` with **no LVGL/SDL dependency**, so it runs as fast host unit tests
(Unity + CTest). The UI only translates `core/` results into widgets:

1. **Red** — add a failing test in `tests/`.
2. **Green** — implement the pure logic in `core/`, run `./test.sh`.
3. **Refactor** the UI in `ui/`, confirm visually with `./shot.sh` (read the PNG).

## Hardware

Full pinout, driver notes and datasheet links:
[docs/hardware/ESP32-S3-Touch-AMOLED-1.43.md](docs/hardware/ESP32-S3-Touch-AMOLED-1.43.md).

### Bill of materials

The physical build targets the Waveshare board below; the rest are the battery,
connectors, charging and fasteners used for the prototype. Amazon links are
affiliate links (`tag=jnk00d-20`).

| Part | Description | Link |
|------|-------------|------|
| **Main board** | Waveshare ESP32-S3 1.43″ AMOLED round display (466×466, capacitive touch, ESP32-S3R8, Wi-Fi 2.4 GHz + BLE 5.0) | [Amazon](https://www.amazon.fr/dp/B0DV3KM88S?tag=jnk00d-20) |
| **Battery** | EEMB LiPo 3.7 V 500 mAh (562438), JST 2.0 connector | [Amazon](https://www.amazon.fr/dp/B09DPPMX14?tag=jnk00d-20) |
| **Connectors** | RUNCCI-YUN JST 1.25 mm 2-pin male/female, 15 cm leads (100 pcs) | [Amazon](https://www.amazon.fr/dp/B09TDCLZGB?tag=jnk00d-20) |
| **Charging cable** | Magnetic USB-C charging cable, 540° rotatable, 5 V/2 A (2-pack) | [Amazon](https://www.amazon.fr/dp/B0H6JB5NFQ?tag=jnk00d-20) |
| **Fasteners** | M2 countersunk hex-socket screw kit, grade 10.9, nuts + washers + Allen keys (410 pcs) | [Amazon](https://www.amazon.fr/dp/B0C1BLSBLW?tag=jnk00d-20) |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Licensed under the Apache License, Version 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
Third-party components (LVGL, Unity, stb, Montserrat) keep their own licenses.
