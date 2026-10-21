# JNKWatch — companion Android app

Kotlin + Jetpack Compose app that configures the watch over BLE. Talks to the
firmware GATT server (`firmware/main/ble_server.c`) using the contract in
[`../docs/ble-protocol.md`](../docs/ble-protocol.md).

## Status
- **Phase 3 (this):** scan / connect / bond to `JNKWatch`, read + write the
  Settings characteristic (brightness, sleep, accent, wrist-wake), and sync the
  clock on connect. Receives Action-button events and battery/status.
- Phase 4 (notification forwarding) and Phase 5 (action-button mapping) to come.

## Build & run
1. Open the `android/` folder in **Android Studio** (Ladybug / 2024.2+). It will
   create the Gradle wrapper and sync (JDK 17, Android SDK 34).
   - Or from the command line, once a wrapper exists: `./gradlew installDebug`.
2. On first launch, grant the **Nearby devices / Bluetooth** permission.
3. Make sure the watch is powered and advertising, then tap **Connecter**.
   - First connection prompts a **pairing/bonding** dialog — accept it (the
     config characteristics are encrypted).
4. Move the sliders / chips → the watch updates live and persists the change.

## Layout
- `ble/BleUuids.kt` — the GATT UUIDs (keep in sync with the firmware).
- `ble/WatchBleClient.kt` — scan/connect + a serialized GATT operation queue.
- `model/WatchSettings.kt` — the 4-byte Settings payload.
- `ui/` — `WatchViewModel`, `WatchScreen` (Compose), theme.

## Requirements
- Android 8.0 (API 26)+; BLE hardware.
- Android 12+ uses the `BLUETOOTH_SCAN`/`BLUETOOTH_CONNECT` runtime permissions;
  older versions use location for the BLE scan.
