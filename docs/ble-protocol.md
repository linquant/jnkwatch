# JNKWatch — BLE protocol (companion app ⇄ watch)

Single source of truth for the GATT contract shared by the **watch firmware**
(`firmware-esp32/firmware/main/ble_server.c`, NimBLE GATT server) and the **Android app**
(`android-app/`). Bump the version and keep both sides in lock-step when changing it.

**Protocol version: 3**

## Link
- **Role**: the watch is the GATT **peripheral**; the phone is the **central**.
- **Advertised name**: `JNKWatch`. The 128-bit service UUID (below) is included in
  the advertisement so the app can scan-filter on it.
- **Security**: currently **none** — characteristics are unencrypted and no
  pairing/bonding is required, to work around a NimBLE↔Android re-pairing loop.
  Re-enabling encryption/bonding is tracked in
  `docs/tickets/reenable-ble-encryption.md` (label: important).
- **MTU**: the app requests an ATT MTU of **247**. NotifPush needs a payload of
  190 bytes, so an MTU ≥ 195 is required for a single write; otherwise the write
  is split (NimBLE reassembles long writes).
- **Endianness**: all multi-byte integers are **little-endian**.
- **Strings**: fixed-size, UTF-8, NUL-padded; the watch truncates on overflow
  (same rule as `wf_notif_push` / `wf_actions_*`).

## Service & characteristics
Base UUID family `6a4e01NN-9b7a-4f2c-8d61-2a1b3c4d5e6f` ("6a4e" = 'j','N').

| Name | UUID (`…-9b7a-4f2c-8d61-2a1b3c4d5e6f`) | Props | Len |
|------|----------------------------------------|-------|-----|
| **Service** | `6a4e0100-` | — | — |
| Settings | `6a4e0101-` | Read, Write, Notify | 7 |
| Time | `6a4e0102-` | Write | 8 |
| NotifPush | `6a4e0103-` | Write | var |
| NotifClear | `6a4e0104-` | Write | 1 |
| ActionCfg | `6a4e0105-` | Write | 1 or 20 |
| ActionEvent | `6a4e0106-` | Notify | 4 |
| Status | `6a4e0107-` | Read, Notify | 3 |
| Weather | `6a4e0108-` | Write | var |
| Probe | `6a4e0109-` | Write | var |
| OtaCtrl | `6a4e010A-` | Write, Notify | var |
| OtaData | `6a4e010B-` | Write, WriteNR | var |

### Settings — `6a4e0101` (R / W / Notify), 7 bytes
Mirrors `wf_settings_t` + accent. Write applies **and persists** (NVS); read
returns the live state; the watch **notifies** when the user changes it on-device.

| Offset | Field | Type | Range / meaning |
|-------:|-------|------|-----------------|
| 0 | brightness | u8 | 10–100 (`WF_BRIGHT_MIN..MAX`; clamped) |
| 1 | sleep | u8 | 0 = 5 s, 1 = 10 s (`WF_SLEEP_5S/10S`) |
| 2 | wrist_wake | u8 | 0/1 |
| 3 | accent | u8 | 0–5 = Rose, Vert, Cyan, Violet, Or, Bleu (`accent_t`) |
| 4 | stay_awake_charging | u8 | 0/1 — keep screen on while on USB/mains (default 1) |
| 5 | power_saver | u8 | 0/1 — "watch only" ultra-low-power mode: deep sleep, BLE off, always-on clock (default 0) |
| 6 | wrist_sens | u8 | 0–2 = Basse, Moyenne, Haute — lift-to-wake sensitivity (default 1). Higher wakes at a larger tilt (smaller raise triggers it). |

Bytes 4–6 were appended after the app's original 4-byte payload. A shorter write
is still accepted (older app): missing `stay_awake_charging` defaults to **1**,
missing `power_saver` to **0**, missing `wrist_sens` to **1** (Moyenne).

### Time — `6a4e0102` (W), 8 bytes
`u64` Unix epoch **seconds, UTC**. Watch sets the PCF85063 RTC + `settimeofday`.
The watch applies its own timezone (`CET-1CEST,…`), so send UTC only.

### NotifPush — `6a4e0103` (W), variable
Three UTF-8 strings separated by NUL bytes: `app\0title\0body` (a trailing NUL
is fine). Pushed into the on-device store (keeps newest 8; the watch assigns
the id and timestamps it with its own clock). Each field is truncated to the
`wf_notif_t` limits (`firmware-esp32/core/notify.h`): app ≤17, title ≤39, body ≤127 bytes.
Example bytes: `4D 65 73 73 00 48 69 00 59 6F 21 00` = "Mess\0Hi\0Yo!\0".

### NotifClear — `6a4e0104` (W), 1 byte
Any value clears the notification store (`wf_notif_clear`).

### ActionCfg — `6a4e0105` (W), resync the 8 action buttons
Phone owns the button set; it sends **clear** then one **set** per button.

- **Clear** (1 byte): `op = 0x00` → `wf_actions_clear`.
- **Set** (20 bytes):

| Offset | Field | Type | Notes |
|-------:|-------|------|-------|
| 0 | op | u8 | 0x01 = set |
| 1 | index | u8 | 0–7 |
| 2 | id | u32 | stable id echoed back in ActionEvent |
| 6 | icon | u8 | 0–8 (`wf_icon_t`: NONE,PHONE,CAMERA,MUSIC,HOME,BELL,VOLUME,POWER,GENERIC) |
| 7 | enabled | u8 | 0/1 |
| 8 | label | char[12] | ≤11 chars (`WF_ACTION_LABEL_LEN`) |

### ActionEvent — `6a4e0106` (Notify), 4 bytes
`u32` = the `id` of the action button tapped on the watch (`watch_send_action`).
The phone maps it to a phone-side effect (Phase 5).

### Status — `6a4e0107` (R / Notify), 3 bytes

| Offset | Field | Type | Notes |
|-------:|-------|------|-------|
| 0 | battery | u8 | 0–100 (`watch_battery_pct`) |
| 1 | fw_major | u8 | firmware version |
| 2 | fw_minor | u8 | firmware version |

### Weather — `6a4e0108` (W), variable
Current weather, fetched by the phone (the watch never fetches — no WiFi) and
shown on the clock's weather row.

| Offset | Field | Type | Notes |
|-------:|-------|------|-------|
| 0 | temp | int16 LE | whole degrees Celsius (signed) |
| 2 | cond | u8 | 0 clear, 1 partly, 2 cloudy, 3 fog, 4 rain, 5 snow, 6 storm |
| 3 | location | UTF-8 | city name, NUL-terminated (truncated to 23 bytes) |

The phone maps the Open-Meteo WMO `weather_code` to `cond`: 0→clear; 1,2→partly;
3→cloudy; 45,48→fog; 51–67/80–82→rain; 71–77/85,86→snow; 95,96,99→storm.

### Probe — `6a4e0109` (W), variable
One server-probe reading for the **Sondes** watchface. The phone polls each
configured GET probe on its own interval (`GetProbe.intervalMin`) and, for probes
set to display on the face, writes the extracted value here. The watch keeps the
latest value per slot and stamps reception with its own clock, so the face shows
how fresh each value is; no notification is spawned.

| Offset | Field | Type | Notes |
|-------:|-------|------|-------|
| 0 | slot | u8 | 0..9 — fixed row per probe (`WF_PROBE_MAX`) |
| 1 | name | UTF-8 | probe title, NUL-terminated (≤23 bytes) |
| — | value | UTF-8 | extracted value, NUL-terminated (≤47 bytes) |

The phone assigns `slot` as the probe's position among the probes bound to the
face, so values keep a stable row. Probes left as notifications use NotifPush
(`6a4e0103`) instead.

### OTA firmware update — `6a4e010A` (OtaCtrl, W/Notify) + `6a4e010B` (OtaData, W/WriteNR)
Pushes a new firmware image to the watch over BLE. The watch writes it to the
**inactive** OTA app slot (`esp_ota_*`) and, on success, reboots into it; a boot
that never confirms healthy is rolled back automatically by the bootloader. The
pure framing/state-machine logic lives in `firmware-esp32/core/ota_proto.c` (unit-tested); the
device flash glue is `firmware-esp32/firmware/main/ota_update.c`. Requires the OTA partition
table (otadata + ota_0 + ota_1) — the migration to that table is a one-time USB
flash (a phone cannot deliver a partition table).

**Security (v1): the transfer is unauthenticated** — consistent with the
currently unencrypted link. Anyone in BLE range could push firmware; hardening
(signed image / Secure Boot v2) is tracked alongside
`docs/tickets/reenable-ble-encryption.md`.

All integers little-endian. The SHA-256 is over the whole image, big-endian digest.

**OtaCtrl — phone → watch (write-with-response):**

| op | name | layout | bytes |
|----|------|--------|-------|
| 0x01 | START | `[0x01][total_size u32][chunk_size u16][sha256 32B]` | 39 |
| 0x02 | ABORT | `[0x02]` | 1 |
| 0x03 | COMMIT | `[0x03]` | 1 |

**OtaCtrl — watch → phone (notify):**

| code | name | layout | meaning |
|------|------|--------|---------|
| 0x81 | READY | `[0x81][status u8][chunk_size u16][window u16]` | START ack; status 0=ok; `chunk_size` clamped/accepted; `window` = initial credits |
| 0x82 | CREDIT | `[0x82][next_seq u16][bytes_recvd u32][credits u16]` | more credits + progress |
| 0x83 | DONE | `[0x83][status u8]` | `esp_ota_end` result; 0=ok, then the watch reboots |
| 0x84 | ERROR | `[0x84][code u8]` | abort: 1 bad_size · 2 flash · 3 sha_mismatch · 4 low_battery · 5 out_of_order · 6 timeout · 7 begin_fail · 8 bad_state |

**OtaData — phone → watch (write-without-response):** `[seq u16][payload …]`.
`seq` = 0-based chunk index; payload length = negotiated `chunk_size` (last chunk
shorter). The watch checks `seq == expected`, else ERROR(5) and aborts.

Flow: phone sends START (gets READY with `window` credits), then streams DATA
chunks up to the outstanding credit, replenished by CREDIT notifications; finally
COMMIT (gets DONE). The phone raises the connection priority during the transfer
and restores low-power after. `chunk_size` ≤ MTU − 5 (≈242 B at MTU 247).

## Typical session
1. App scans for `JNKWatch`, connects, bonds, negotiates MTU 247.
2. App **reads Settings** and **Status**; subscribes to Settings / ActionEvent / Status.
3. App **writes Time** (UTC now) to sync the clock.
4. App **resyncs actions** (ActionCfg clear + sets) from its stored config.
5. Ongoing: NotificationListenerService → **NotifPush**; watch button tap →
   **ActionEvent** notify; on-device settings change → **Settings** notify.

## Notes
- The watch persists Settings + accent + the action set in NVS, so a reboot keeps
  the last configuration even before the phone reconnects. Notifications are not
  persisted.
- On the PC/emulator build these paths are mocked via `watch_poll_config`
  (`platform/display.c`); the wire format here is device-only.
