package fr.ponera.jnkwatch.model

/** A configurable GET poller: the app fetches [url] every [intervalMin] minutes,
 * extracts [jsonField] (dot-path; blank = raw body) and, when non-empty, shows the
 * value on the watch. [onFace] true routes it to the dedicated "Sondes" watchface
 * (a live row that refreshes at the polling frequency); false pushes it as a
 * one-off notification (the original behaviour). [headers] holds optional
 * "Key: Value" lines (e.g. Authorization) for authenticated APIs. */
data class GetProbe(
    val id: Int,
    val name: String = "",
    val url: String = "",
    val jsonField: String = "",
    val headers: String = "",
    val intervalMin: Int = 15,
    val enabled: Boolean = false,
    val onFace: Boolean = true,
) {
    companion object {
        const val MAX = 10
    }
}
