package fr.ponera.jnkwatch.ble

import java.util.UUID

/** GATT contract — must match firmware/main/ble_server.c and docs/ble-protocol.md. */
object BleUuids {
    private fun u(nn: Int): UUID =
        UUID.fromString("6a4e01%02x-9b7a-4f2c-8d61-2a1b3c4d5e6f".format(nn))

    val SERVICE: UUID = u(0x00)
    val SETTINGS: UUID = u(0x01)
    val TIME: UUID = u(0x02)
    val NOTIF_PUSH: UUID = u(0x03)
    val NOTIF_CLEAR: UUID = u(0x04)
    val ACTION_CFG: UUID = u(0x05)
    val ACTION_EVENT: UUID = u(0x06)
    val STATUS: UUID = u(0x07)
    val WEATHER: UUID = u(0x08)
    val PROBE: UUID = u(0x09)
    val OTA_CTRL: UUID = u(0x0A)   // START/ABORT/COMMIT write + READY/CREDIT/DONE/ERROR notify
    val OTA_DATA: UUID = u(0x0B)   // [seq u16][payload] write-without-response

    /** Standard Client Characteristic Configuration Descriptor. */
    val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    const val DEVICE_NAME = "JNKWatch"
}
