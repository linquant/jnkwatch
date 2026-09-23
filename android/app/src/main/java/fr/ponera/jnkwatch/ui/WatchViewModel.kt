package fr.ponera.jnkwatch.ui

import android.app.Application
import android.content.Intent
import androidx.core.content.ContextCompat
import androidx.lifecycle.AndroidViewModel
import fr.ponera.jnkwatch.WatchService
import fr.ponera.jnkwatch.ble.WatchConnection
import fr.ponera.jnkwatch.model.WatchSettings

class WatchViewModel(app: Application) : AndroidViewModel(app) {
    private val client get() = WatchConnection.client

    val state = client.state
    val settings = client.settings
    val battery = client.battery
    val lastAction = client.lastAction
    val log = client.log

    /** Connect the shared client directly (proven path), and start the
     *  foreground service for background SMS/call forwarding (best-effort — a
     *  service failure must never block the connection itself). */
    fun connect() {
        client.connect()
        runCatching {
            ContextCompat.startForegroundService(
                getApplication(), Intent(getApplication(), WatchService::class.java)
            )
        }
    }

    fun disconnect() {
        runCatching {
            getApplication<Application>().stopService(
                Intent(getApplication(), WatchService::class.java)
            )
        }
        client.disconnect()
    }

    fun update(s: WatchSettings) = client.writeSettings(s)
    fun syncTime() = client.syncTime()
}
