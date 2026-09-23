package fr.ponera.jnkwatch.notif

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.telephony.TelephonyManager
import fr.ponera.jnkwatch.ble.WatchConnection

/** Forwards an incoming call (RINGING) to the watch, debounced per call. */
class CallReceiver : BroadcastReceiver() {
    private var lastKey: String? = null

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != TelephonyManager.ACTION_PHONE_STATE_CHANGED) return
        if (!NotifPrefs.callsEnabled(context)) return
        val state = intent.getStringExtra(TelephonyManager.EXTRA_STATE)
        if (state == TelephonyManager.EXTRA_STATE_RINGING) {
            val num = intent.getStringExtra(TelephonyManager.EXTRA_INCOMING_NUMBER) ?: "Inconnu"
            val key = "R:$num"
            if (key != lastKey) {
                lastKey = key
                WatchConnection.client.pushNotification("Appel", "Appel entrant", num)
            }
        } else {
            lastKey = null   // reset when the call ends / is answered
        }
    }
}
