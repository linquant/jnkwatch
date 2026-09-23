package fr.ponera.jnkwatch.actions

import android.content.Context
import fr.ponera.jnkwatch.model.ActionButton
import org.json.JSONArray
import org.json.JSONObject

/** Local persistence for the 6 action buttons (label/icon/enabled + POST URL). */
object ActionsPrefs {
    private const val PREFS = "actions"
    private const val KEY = "buttons"

    private fun prefs(ctx: Context) =
        ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    fun load(ctx: Context): List<ActionButton> {
        val json = prefs(ctx).getString(KEY, null) ?: return ActionButton.defaults()
        return try {
            val arr = JSONArray(json)
            val list = (0 until arr.length()).map { i ->
                val o = arr.getJSONObject(i)
                ActionButton(
                    id = o.getInt("id"),
                    label = o.optString("label"),
                    icon = o.optInt("icon", 8),
                    enabled = o.optBoolean("enabled"),
                    url = o.optString("url"),
                )
            }
            if (list.size == ActionButton.COUNT) list else ActionButton.defaults()
        } catch (_: Exception) {
            ActionButton.defaults()
        }
    }

    fun save(ctx: Context, buttons: List<ActionButton>) {
        val arr = JSONArray()
        buttons.forEach {
            arr.put(
                JSONObject()
                    .put("id", it.id)
                    .put("label", it.label)
                    .put("icon", it.icon)
                    .put("enabled", it.enabled)
                    .put("url", it.url)
            )
        }
        prefs(ctx).edit().putString(KEY, arr.toString()).apply()
    }

    /** The button (if any) mapped to a watch ActionEvent [id], enabled with a URL. */
    fun buttonForId(ctx: Context, id: Int): ActionButton? =
        load(ctx).firstOrNull { it.id == id && it.enabled && it.url.isNotBlank() }
}
