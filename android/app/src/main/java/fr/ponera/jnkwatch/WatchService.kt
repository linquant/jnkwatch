package fr.ponera.jnkwatch

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.telephony.TelephonyManager
import androidx.core.app.NotificationCompat
import fr.ponera.jnkwatch.actions.ActionRunner
import fr.ponera.jnkwatch.actions.ActionsPrefs
import fr.ponera.jnkwatch.ble.ConnState
import fr.ponera.jnkwatch.ble.WatchConnection
import fr.ponera.jnkwatch.notif.CallReceiver
import fr.ponera.jnkwatch.weather.WeatherRepo
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
        // Weather: push shortly after each connect, then refresh every 2 hours.
        scope.launch {
            var lastPush = 0L
            while (isActive) {
                val connected = WatchConnection.client.state.value == ConnState.CONNECTED
                if (connected && System.currentTimeMillis() - lastPush > WEATHER_INTERVAL_MS) {
                    WeatherRepo.fetch(applicationContext)?.let {
                        WatchConnection.client.pushWeather(it.tempC, it.cond, it.location)
                        lastPush = System.currentTimeMillis()
                    }
                }
                delay(15_000)
            }
        }
        // Action buttons: on each watch tap, POST to the button's configured URL.
        scope.launch {
            WatchConnection.client.actionEvents.collect { id ->
                val b = ActionsPrefs.buttonForId(applicationContext, id) ?: return@collect
                ActionRunner.post(b.url, b.id, b.label, b.body, b.contentType)
            }
        }
        // Re-push the button config on each (re)connect, so a watch reboot keeps
        // the labels/icons in sync with the phone.
        scope.launch {
            var wasConnected = false
            WatchConnection.client.state.collect { st ->
                val now = st == ConnState.CONNECTED
                if (now && !wasConnected) {
                    delay(1500)   // let settings/time sync finish first
                    WatchConnection.client.pushActionConfig(ActionsPrefs.load(applicationContext))
                }
                wasConnected = now
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
            unregisterReceiver(call)
            registered = false
        }
        WatchConnection.client.disconnect()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun registerReceivers() {
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
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                startForeground(1, notif, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
            } else {
                startForeground(1, notif)
            }
        } catch (e: Exception) {
            // Never let a foreground-service quirk crash the app; the direct
            // connection still works, only background forwarding is degraded.
            runCatching { startForeground(1, notif) }
        }
    }

    companion object {
        private const val CHANNEL = "watch"
        private const val WEATHER_INTERVAL_MS = 2L * 60 * 60 * 1000 // 2 hours
    }
}
