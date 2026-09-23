package fr.ponera.jnkwatch.probes

import android.content.Context
import fr.ponera.jnkwatch.model.GetProbe
import org.json.JSONArray
import org.json.JSONObject

/** Local persistence for the GET probes (dynamic list, up to GetProbe.MAX). */
object GetProbesPrefs {
    private const val PREFS = "probes"
    private const val KEY = "list"

    private fun prefs(ctx: Context) =
        ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    fun load(ctx: Context): List<GetProbe> {
        val json = prefs(ctx).getString(KEY, null) ?: return emptyList()
        return try {
            val arr = JSONArray(json)
            (0 until arr.length()).map { i ->
                val o = arr.getJSONObject(i)
                GetProbe(
                    id = o.getInt("id"),
                    name = o.optString("name"),
                    url = o.optString("url"),
                    jsonField = o.optString("jsonField"),
                    headers = o.optString("headers"),
                    intervalMin = o.optInt("intervalMin", 15),
                    enabled = o.optBoolean("enabled"),
                )
            }
        } catch (_: Exception) {
            emptyList()
        }
    }

    fun save(ctx: Context, probes: List<GetProbe>) {
        val arr = JSONArray()
        probes.forEach {
            arr.put(
                JSONObject()
                    .put("id", it.id)
                    .put("name", it.name)
                    .put("url", it.url)
                    .put("jsonField", it.jsonField)
                    .put("headers", it.headers)
                    .put("intervalMin", it.intervalMin)
                    .put("enabled", it.enabled)
            )
        }
        prefs(ctx).edit().putString(KEY, arr.toString()).apply()
    }

    fun nextId(probes: List<GetProbe>): Int = (probes.maxOfOrNull { it.id } ?: 0) + 1
}
