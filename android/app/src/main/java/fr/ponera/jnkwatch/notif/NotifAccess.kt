package fr.ponera.jnkwatch.notif

import android.content.Context
import android.content.Intent
import android.provider.Settings

/** Helpers for the special "notification access" permission (a system toggle). */
object NotifAccess {

    fun isGranted(context: Context): Boolean {
        val enabled = Settings.Secure.getString(
            context.contentResolver, "enabled_notification_listeners"
        ) ?: return false
        return enabled.split(':').any { it.contains(context.packageName) }
    }

    fun openSettings(context: Context) {
        val intent = Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }
}
