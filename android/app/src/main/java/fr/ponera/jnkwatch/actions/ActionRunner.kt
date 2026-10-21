package fr.ponera.jnkwatch.actions

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

/** Fires the HTTP POST bound to a watch button. */
object ActionRunner {
    private const val TAG = "ActionRunner"

    /**
     * POST to [url]. If [body] is blank, sends `{"button":id,"label":label}`;
     * otherwise sends [body] with `{id}`/`{label}` tokens substituted, tagged with
     * [contentType]. Best-effort: failures are logged, never thrown (a tap should
     * never crash the service).
     */
    suspend fun post(
        url: String,
        id: Int,
        label: String,
        body: String = "",
        contentType: String = "application/json",
    ): Boolean = withContext(Dispatchers.IO) {
        var conn: HttpURLConnection? = null
        try {
            val ct = contentType.ifBlank { "application/json" }
            conn = (URL(url).openConnection() as HttpURLConnection).apply {
                requestMethod = "POST"
                doOutput = true
                connectTimeout = 8000
                readTimeout = 8000
                setRequestProperty("Content-Type", "$ct; charset=utf-8")
            }
            val payload = if (body.isBlank()) {
                JSONObject().put("button", id).put("label", label).toString()
            } else {
                body.replace("{id}", id.toString()).replace("{label}", label)
            }
            conn.outputStream.use { it.write(payload.toByteArray(Charsets.UTF_8)) }
            val code = conn.responseCode
            Log.i(TAG, "POST $url → $code")
            code in 200..299
        } catch (e: Exception) {
            Log.w(TAG, "POST $url échoué: ${e.message}")
            false
        } finally {
            conn?.disconnect()
        }
    }
}
