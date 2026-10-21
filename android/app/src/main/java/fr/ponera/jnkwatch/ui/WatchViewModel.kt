package fr.ponera.jnkwatch.ui

import android.app.Application
import android.content.Intent
import androidx.core.content.ContextCompat
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import fr.ponera.jnkwatch.WatchService
import fr.ponera.jnkwatch.ble.WatchConnection
import fr.ponera.jnkwatch.firmware.FirmwareRepo
import fr.ponera.jnkwatch.model.WatchSettings
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class WatchViewModel(app: Application) : AndroidViewModel(app) {
    private val client get() = WatchConnection.client

    val state = client.state
    val settings = client.settings
    val battery = client.battery
    val lastAction = client.lastAction
    val log = client.log
    val fwVersion = client.fwVersion
    val otaProgress = client.otaProgress

    /** A firmware update newer than the connected watch, or null. Set by checkFirmwareUpdate(). */
    private val _update = MutableStateFlow<FirmwareRepo.Manifest?>(null)
    val update = _update.asStateFlow()
    private val _checking = MutableStateFlow(false)
    val checking = _checking.asStateFlow()

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

    /** Fetch the manifest and flag an update if it is newer than the watch's version. */
    fun checkFirmwareUpdate() {
        if (_checking.value) return
        viewModelScope.launch {
            _checking.value = true
            try {
                val m = FirmwareRepo.fetchManifest()
                val dev = fwVersion.value
                _update.value = if (m != null && dev != null && m.isNewerThan(dev.first, dev.second)) m else null
            } finally {
                _checking.value = false
            }
        }
    }

    /** Download the flagged image (verifying size+SHA) and stream it to the watch. */
    fun updateFirmware() {
        val m = _update.value ?: return
        viewModelScope.launch {
            val bin = FirmwareRepo.download(m)
            if (bin == null) { client.reportOtaError("Téléchargement du firmware échoué"); return@launch }
            client.performOta(bin, m.sha256)
        }
    }
}
