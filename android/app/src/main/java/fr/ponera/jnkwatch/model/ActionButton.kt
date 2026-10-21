package fr.ponera.jnkwatch.model

/** One configurable watch button: shown on the watch (label/icon/enabled) and,
 * when tapped, the phone POSTs to [url]. [id] is stable and echoed by the watch
 * in the ActionEvent so the phone maps it back to this button. */
data class ActionButton(
    val id: Int,
    val label: String = "",
    val icon: Int = 8,        // WF_ICON_GENERIC
    val enabled: Boolean = false,
    val url: String = "",
    /** POST body. Blank = default `{"button":id,"label":label}`. The tokens
     * {id} and {label} are substituted at send time. */
    val body: String = "",
    val contentType: String = "application/json",
) {
    companion object {
        const val COUNT = 6   // one 2x3 page on the watch

        /** Icon names indexed by the watch's wf_icon_t (0..8). */
        val ICONS = listOf(
            "Aucune", "Téléphone", "Caméra", "Musique", "Maison",
            "Cloche", "Volume", "Power", "Générique",
        )

        fun defaults(): List<ActionButton> =
            (1..COUNT).map { ActionButton(id = it, label = "Btn $it", icon = 8) }
    }
}
