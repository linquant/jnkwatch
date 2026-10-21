package fr.ponera.jnkwatch.notif

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.provider.Telephony
import fr.ponera.jnkwatch.ble.WatchConnection

/** Forwards incoming SMS to the watch as an "SMS" notification. */
class SmsReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != Telephony.Sms.Intents.SMS_RECEIVED_ACTION) return
        val msgs = Telephony.Sms.Intents.getMessagesFromIntent(intent) ?: return
        if (msgs.isEmpty()) return
        val sender = msgs[0].displayOriginatingAddress ?: "SMS"
        val body = msgs.joinToString(separator = "") { it.messageBody ?: "" }
        WatchConnection.client.pushNotification("SMS", sender, body)
    }
}
