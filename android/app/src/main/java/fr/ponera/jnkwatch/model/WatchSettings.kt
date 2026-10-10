package fr.ponera.jnkwatch.model

/** Mirror of the 7-byte Settings characteristic (bytes 4, 5 & 6 appended after v1). */
data class WatchSettings(
    val brightness: Int = 80,     // 10..100
    val sleep10s: Boolean = true, // true = 10 s, false = 5 s
    val wristWake: Boolean = true,
    val accent: Int = 2,          // 0..5 (Cyan default)
    val stayAwakeCharging: Boolean = true, // keep screen on while on USB/mains
    val powerSaver: Boolean = false, // "watch only" ultra-low-power (deep sleep)
    val wristSens: Int = 1,       // lift-to-wake sensitivity 0..2 (Moyenne default)
) {
    fun toBytes(): ByteArray = byteArrayOf(
        brightness.coerceIn(10, 100).toByte(),
        if (sleep10s) 1 else 0,
        if (wristWake) 1 else 0,
        (accent % ACCENTS.size).toByte(),
        if (stayAwakeCharging) 1 else 0,
        if (powerSaver) 1 else 0,
        wristSens.coerceIn(0, WRIST_SENS.size - 1).toByte(),
    )

    companion object {
        val ACCENTS = listOf("Rose", "Vert", "Cyan", "Violet", "Or", "Bleu")
        val ACCENT_ARGB = listOf(
            0xFFFF3D9A, 0xFF54E08A, 0xFF37DCE0, 0xFFB57BFF, 0xFFE6C34C, 0xFF5B8CF5,
        )
        // Lift-to-wake sensitivity levels, index = wire value (0..2).
        val WRIST_SENS = listOf("Basse", "Moyenne", "Haute")

        fun fromBytes(b: ByteArray): WatchSettings? {
            if (b.size < 4) return null
            return WatchSettings(
                brightness = b[0].toInt() and 0xFF,
                sleep10s = b[1].toInt() != 0,
                wristWake = b[2].toInt() != 0,
                accent = (b[3].toInt() and 0xFF).coerceIn(0, ACCENTS.size - 1),
                // byte 4 optional: older firmware sends 4 bytes → default true.
                stayAwakeCharging = if (b.size >= 5) b[4].toInt() != 0 else true,
                // byte 5 optional: older firmware omits it → power-saver off.
                powerSaver = if (b.size >= 6) b[5].toInt() != 0 else false,
                // byte 6 optional: older firmware omits it → Moyenne (1).
                wristSens = if (b.size >= 7)
                    (b[6].toInt() and 0xFF).coerceIn(0, WRIST_SENS.size - 1) else 1,
            )
        }
    }
}
