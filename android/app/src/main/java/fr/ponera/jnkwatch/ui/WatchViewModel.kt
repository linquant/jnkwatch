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

    /** Connecting starts the foreground service, which owns the connection and
     *  the SMS/call forwarding. */
    fun connect() {
        ContextCompat.startForegroundService(
            getApplication(), Intent(getApplication(), WatchService::class.java)
        )
    }

    fun disconnect() {
        getApplication<Application>().stopService(
            Intent(getApplication(), WatchService::class.java)
        )
    }

    fun update(s: WatchSettings) = client.writeSettings(s)
    fun syncTime() = client.syncTime()
}
