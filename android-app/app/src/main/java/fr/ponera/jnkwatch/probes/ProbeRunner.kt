package fr.ponera.jnkwatch.probes

import android.util.Log
import fr.ponera.jnkwatch.model.GetProbe
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import org.json.JSONTokener
import java.net.HttpURLConnection
import java.net.URL

/** Performs a probe's GET and extracts the field to display. */
object ProbeRunner {
    private const val TAG = "ProbeRunner"

    /** value = extracted text (null if missing/empty); error = short reason or null. */
    data class Outcome(val value: String?, val error: String?)

    suspend fun fetch(p: GetProbe): Outcome = withContext(Dispatchers.IO) {
        var conn: HttpURLConnection? = null
        try {
            conn = (URL(p.url).openConnection() as HttpURLConnection).apply {
                requestMethod = "GET"
                connectTimeout = 8000
                readTimeout = 8000
                p.headers.lineSequence().forEach { line ->
                    val i = line.indexOf(':')
                    if (i > 0) setRequestProperty(line.substring(0, i).trim(), line.substring(i + 1).trim())
                }
            }
            val code = conn.responseCode
            val stream = if (code in 200..299) conn.inputStream else conn.errorStream
            val text = stream?.bufferedReader()?.use { it.readText() } ?: ""
            if (code !in 200..299) return@withContext Outcome(null, "HTTP $code")
            val v = extract(text, p.jsonField)
            Log.i(TAG, "GET ${p.url} → ${v ?: "(vide)"}")
            Outcome(v, if (v == null) "champ introuvable" else null)
        } catch (e: Exception) {
            Log.w(TAG, "GET ${p.url} échoué: ${e.message}")
            Outcome(null, e.message ?: "erreur réseau")
        } finally {
            conn?.disconnect()
        }
    }

    /** Walks a dot-path (e.g. "data.warning", "alerts.0.msg") into the JSON body.
     * Blank field returns the trimmed raw body. Returns null if absent/empty. */
    fun extract(body: String, field: String): String? {
        val f = field.trim()
        if (f.isEmpty()) return body.trim().ifEmpty { null }
        return try {
            var cur: Any? = JSONTokener(body).nextValue()
            for (seg in f.split('.')) {
                cur = when (val c = cur) {
                    is JSONObject -> if (c.isNull(seg)) null else c.opt(seg)
                    is JSONArray -> seg.toIntOrNull()
                        ?.let { if (it in 0 until c.length() && !c.isNull(it)) c.opt(it) else null }
                    else -> null
                }
                if (cur == null) return null
            }
            cur?.toString()?.trim()?.ifEmpty { null }
        } catch (_: Exception) {
            null
        }
    }
}
