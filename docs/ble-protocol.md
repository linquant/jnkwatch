# JNKWatch — BLE protocol (companion app ⇄ watch)

Single source of truth for the GATT contract shared by the **watch firmware**
(`firmware/main/ble_server.c`, NimBLE GATT server) and the **Android app**
(`android/`). Bump the version and keep both sides in lock-step when changing it.

**Protocol version: 1**

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
| Settings | `6a4e0101-` | Read, Write, Notify | 4 |
| Time | `6a4e0102-` | Write | 8 |
| NotifPush | `6a4e0103-` | Write | var |
| NotifClear | `6a4e0104-` | Write | 1 |
| ActionCfg | `6a4e0105-` | Write | 1 or 20 |
| ActionEvent | `6a4e0106-` | Notify | 4 |
| Status | `6a4e0107-` | Read, Notify | 3 |

### Settings — `6a4e0101` (R / W / Notify), 4 bytes
Mirrors `wf_settings_t` + accent. Write applies **and persists** (NVS); read
returns the live state; the watch **notifies** when the user changes it on-device.

| Offset | Field | Type | Range / meaning |
|-------:|-------|------|-----------------|
| 0 | brightness | u8 | 10–100 (`WF_BRIGHT_MIN..MAX`; clamped) |
| 1 | sleep | u8 | 0 = 5 s, 1 = 10 s (`WF_SLEEP_5S/10S`) |
| 2 | wrist_wake | u8 | 0/1 |
| 3 | accent | u8 | 0–5 = Rose, Vert, Cyan, Violet, Or, Bleu (`accent_t`) |

### Time — `6a4e0102` (W), 8 bytes
`u64` Unix epoch **seconds, UTC**. Watch sets the PCF85063 RTC + `settimeofday`.
The watch applies its own timezone (`CET-1CEST,…`), so send UTC only.

### NotifPush — `6a4e0103` (W), variable
Three UTF-8 strings separated by NUL bytes: `app\0title\0body` (a trailing NUL
is fine). Pushed into the on-device store (keeps newest 8; the watch assigns
the id and timestamps it with its own clock). Each field is truncated to the
`wf_notif_t` limits (`core/notify.h`): app ≤17, title ≤39, body ≤127 bytes.
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
