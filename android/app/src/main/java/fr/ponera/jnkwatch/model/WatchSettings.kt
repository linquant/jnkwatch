package fr.ponera.jnkwatch.model

/** Mirror of the 4-byte Settings characteristic. */
data class WatchSettings(
    val brightness: Int = 80,     // 10..100
    val sleep10s: Boolean = true, // true = 10 s, false = 5 s
    val wristWake: Boolean = true,
    val accent: Int = 2,          // 0..5 (Cyan default)
) {
    fun toBytes(): ByteArray = byteArrayOf(
        brightness.coerceIn(10, 100).toByte(),
        if (sleep10s) 1 else 0,
        if (wristWake) 1 else 0,
        (accent % ACCENTS.size).toByte(),
    )

    companion object {
        val ACCENTS = listOf("Rose", "Vert", "Cyan", "Violet", "Or", "Bleu")
        val ACCENT_ARGB = listOf(
            0xFFFF3D9A, 0xFF54E08A, 0xFF37DCE0, 0xFFB57BFF, 0xFFE6C34C, 0xFF5B8CF5,
        )

        fun fromBytes(b: ByteArray): WatchSettings? {
            if (b.size < 4) return null
            return WatchSettings(
                brightness = b[0].toInt() and 0xFF,
                sleep10s = b[1].toInt() != 0,
                wristWake = b[2].toInt() != 0,
                accent = (b[3].toInt() and 0xFF).coerceIn(0, ACCENTS.size - 1),
            )
        }
    }
}
