package fr.ponera.jnkwatch.ui

import android.app.Application
import android.content.Context
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
    private val prefs = app.getSharedPreferences("firmware", Context.MODE_PRIVATE)

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

    /** Firmware channel, persisted so the choice survives app restarts. */
    private val _channel = MutableStateFlow(
        runCatching { FirmwareRepo.Channel.valueOf(prefs.getString("channel", "")!!) }
            .getOrDefault(FirmwareRepo.Channel.STABLE)
    )
    val channel = _channel.asStateFlow()

    fun setChannel(c: FirmwareRepo.Channel) {
        if (c == _channel.value) return
        _channel.value = c
        prefs.edit().putString("channel", c.name).apply()
        _update.value = null      // the UI's LaunchedEffect(channel) re-checks on the new channel
    }

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

    /** Fetch the channel's manifest and flag an update if it is newer than the watch's version.
     *  On the dev channel the build is always offered, even at the same version, so a
     *  dev image can be flashed over the installed release. */
    fun checkFirmwareUpdate() {
        if (_checking.value) return
        viewModelScope.launch {
            _checking.value = true
            try {
                val ch = _channel.value
                val m = FirmwareRepo.fetchManifest(ch.manifestUrl)
                _update.value = FirmwareRepo.offerFor(ch, m, fwVersion.value)
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
