package fr.ponera.jnkwatch

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.provider.Telephony
import android.telephony.TelephonyManager
import androidx.core.app.NotificationCompat
import fr.ponera.jnkwatch.ble.ConnState
import fr.ponera.jnkwatch.ble.WatchConnection
import fr.ponera.jnkwatch.notif.CallReceiver
import fr.ponera.jnkwatch.notif.SmsReceiver
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Foreground service that keeps the BLE connection alive in the background and
 * hosts the SMS/call receivers, so incoming SMS and calls are forwarded to the
 * watch even when the app UI is closed. Auto-reconnects when the link drops
 * (watch reboot, out of range, …).
 */
class WatchService : Service() {

    private val sms = SmsReceiver()
    private val call = CallReceiver()
    private var registered = false
    private val scope = CoroutineScope(Dispatchers.Main + SupervisorJob())

    override fun onCreate() {
        super.onCreate()
        createChannel()
        goForeground()
        registerReceivers()
        // Auto-reconnect: whenever the link is down, retry after a short delay.
        scope.launch {
            WatchConnection.client.state.collect { st ->
                if (st == ConnState.DISCONNECTED) {
                    delay(3000)
                    if (isActive) WatchConnection.client.connect()
                }
            }
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        WatchConnection.client.connect()   // no-op if already connecting/connected
        return START_STICKY
    }

    override fun onDestroy() {
        scope.cancel()
        if (registered) {
            unregisterReceiver(sms)
            unregisterReceiver(call)
            registered = false
        }
        WatchConnection.client.disconnect()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun registerReceivers() {
        registerReceiver(sms, IntentFilter(Telephony.Sms.Intents.SMS_RECEIVED_ACTION))
        registerReceiver(call, IntentFilter(TelephonyManager.ACTION_PHONE_STATE_CHANGED))
        registered = true
    }

    private fun createChannel() {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(CHANNEL, "JNKWatch", NotificationManager.IMPORTANCE_LOW)
        )
    }

    private fun goForeground() {
        val notif = NotificationCompat.Builder(this, CHANNEL)
            .setContentTitle("JNKWatch")
            .setContentText("Active — SMS et appels transférés")
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .setOngoing(true)
            .build()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(1, notif, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
        } else {
            startForeground(1, notif)
        }
    }

    companion object {
        private const val CHANNEL = "watch"
    }
}
