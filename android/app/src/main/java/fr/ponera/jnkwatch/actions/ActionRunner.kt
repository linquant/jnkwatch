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
     * POST to [url] with a small JSON body `{"button":id,"label":label}` so the
     * remote server can tell which button fired. Best-effort: failures are logged,
     * never thrown (a tap should never crash the service).
     */
    suspend fun post(url: String, id: Int, label: String): Boolean = withContext(Dispatchers.IO) {
        var conn: HttpURLConnection? = null
        try {
            conn = (URL(url).openConnection() as HttpURLConnection).apply {
                requestMethod = "POST"
                doOutput = true
                connectTimeout = 8000
                readTimeout = 8000
                setRequestProperty("Content-Type", "application/json; charset=utf-8")
            }
            val body = JSONObject()
                .put("button", id)
                .put("label", label)
                .toString()
                .toByteArray(Charsets.UTF_8)
            conn.outputStream.use { it.write(body) }
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
