package fr.ponera.jnkwatch.notif

import android.content.Context
import android.provider.Telephony

/**
 * Which notification sources are forwarded to the watch. Persisted in
 * SharedPreferences. Defaults on first run: Calls ON + the default SMS app ON;
 * other apps are OFF until the user enables them on the filter screen. Apps are
 * "discovered" (marked seen) as they post notifications so they can be toggled.
 */
object NotifPrefs {
    private const val PREFS = "notif_filter"
    private const val KEY_CALLS = "calls"
    private const val KEY_ENABLED = "enabled"
    private const val KEY_SEEN = "seen"
    private const val KEY_INIT = "init"

    private fun p(ctx: Context) = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    private fun ensureInit(ctx: Context) {
        val prefs = p(ctx)
        if (prefs.getBoolean(KEY_INIT, false)) return
        val sms = try { Telephony.Sms.getDefaultSmsPackage(ctx) } catch (_: Exception) { null }
        val defaults = if (sms != null) setOf(sms) else emptySet()
        prefs.edit()
            .putBoolean(KEY_INIT, true)
            .putBoolean(KEY_CALLS, true)
            .putStringSet(KEY_ENABLED, defaults)
            .putStringSet(KEY_SEEN, defaults)
            .apply()
    }

    fun callsEnabled(ctx: Context): Boolean {
        ensureInit(ctx); return p(ctx).getBoolean(KEY_CALLS, true)
    }

    fun setCallsEnabled(ctx: Context, on: Boolean) {
        ensureInit(ctx); p(ctx).edit().putBoolean(KEY_CALLS, on).apply()
    }

    fun enabled(ctx: Context): Set<String> {
        ensureInit(ctx); return p(ctx).getStringSet(KEY_ENABLED, emptySet())!!.toSet()
    }

    fun seen(ctx: Context): Set<String> {
        ensureInit(ctx); return p(ctx).getStringSet(KEY_SEEN, emptySet())!!.toSet()
    }

    fun isEnabled(ctx: Context, pkg: String): Boolean = enabled(ctx).contains(pkg)

    fun setEnabled(ctx: Context, pkg: String, on: Boolean) {
        ensureInit(ctx)
        val s = enabled(ctx).toHashSet()
        if (on) s.add(pkg) else s.remove(pkg)
        p(ctx).edit().putStringSet(KEY_ENABLED, s).apply()
    }

    /** Record that an app posted a notification, so it appears on the filter list. */
    fun markSeen(ctx: Context, pkg: String) {
        ensureInit(ctx)
        val s = seen(ctx)
        if (!s.contains(pkg)) {
            val ns = s.toHashSet(); ns.add(pkg)
            p(ctx).edit().putStringSet(KEY_SEEN, ns).apply()
        }
    }
}
