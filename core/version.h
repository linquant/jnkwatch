#pragma once
/* Firmware version — single source of truth, shared by the BLE Status
 * characteristic (firmware/main/ble_server.c) and the debug face (ui/face_debug.c).
 * Bump on every release; the OTA update check compares this to the manifest. */
#define WF_FW_MAJOR 1
#define WF_FW_MINOR 3
