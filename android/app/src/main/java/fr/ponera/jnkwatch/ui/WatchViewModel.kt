package fr.ponera.jnkwatch.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import fr.ponera.jnkwatch.ble.WatchBleClient
import fr.ponera.jnkwatch.model.WatchSettings

class WatchViewModel(app: Application) : AndroidViewModel(app) {
    private val client = WatchBleClient(app.applicationContext)

    val state = client.state
    val settings = client.settings
    val battery = client.battery
    val lastAction = client.lastAction
    val log = client.log

    fun connect() = client.connect()
    fun disconnect() = client.disconnect()
    fun update(s: WatchSettings) = client.writeSettings(s)
    fun syncTime() = client.syncTime()
}
