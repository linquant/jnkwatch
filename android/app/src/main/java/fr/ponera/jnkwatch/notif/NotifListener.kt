package fr.ponera.jnkwatch.notif

import android.app.Notification
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import fr.ponera.jnkwatch.ble.WatchConnection

/**
 * Captures posted notifications (SMS, RCS, WhatsApp, e-mail, …) and forwards
 * them to the watch. This is techno-agnostic — unlike the SMS_RECEIVED
 * broadcast, it also catches RCS/"chat" messages. Requires the user to grant
 * "notification access" (see NotifAccess).
 */
class NotifListener : NotificationListenerService() {

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val n = sbn.notification ?: return

        // Skip noise: incoming calls (handled by CallReceiver), our own service
        // notification, ongoing (media/progress) and group summaries.
        if (n.category == Notification.CATEGORY_CALL) return
        if (sbn.packageName == packageName) return
        if (n.flags and Notification.FLAG_ONGOING_EVENT != 0) return
        if (n.flags and Notification.FLAG_GROUP_SUMMARY != 0) return

        val extras = n.extras
        val title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString().orEmpty()
        val text = (extras.getCharSequence(Notification.EXTRA_TEXT)
            ?: extras.getCharSequence(Notification.EXTRA_BIG_TEXT))?.toString().orEmpty()
        if (title.isBlank() && text.isBlank()) return

        // Discover the app (so it can be toggled) and honour the user's filter.
        NotifPrefs.markSeen(this, sbn.packageName)
        if (!NotifPrefs.isEnabled(this, sbn.packageName)) return

        WatchConnection.client.pushNotification(appLabel(sbn.packageName), title, text)
    }

    private fun appLabel(pkg: String): String = try {
        val pm = packageManager
        pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)).toString()
    } catch (_: Exception) {
        pkg.substringAfterLast('.')
    }
}
