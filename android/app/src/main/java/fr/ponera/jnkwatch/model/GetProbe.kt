package fr.ponera.jnkwatch.model

/** A configurable GET poller: the app fetches [url] every [intervalMin] minutes,
 * extracts [jsonField] (dot-path; blank = raw body) and, when non-empty, shows it
 * on the watch's notification screen. [headers] holds optional "Key: Value" lines
 * (e.g. Authorization) for authenticated APIs. */
data class GetProbe(
    val id: Int,
    val name: String = "",
    val url: String = "",
    val jsonField: String = "",
    val headers: String = "",
    val intervalMin: Int = 15,
    val enabled: Boolean = false,
) {
    companion object {
        const val MAX = 10
    }
}
