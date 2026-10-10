package fr.ponera.jnkwatch.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import fr.ponera.jnkwatch.model.ActionButton
import fr.ponera.jnkwatch.model.WatchSettings
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow

enum class ConnState { DISCONNECTED, SCANNING, CONNECTING, CONNECTED }

/**
 * Minimal, self-contained BLE client for the JNKWatch. Callback-based Android
 * GATT wrapped into StateFlows, with a one-at-a-time operation queue (Android
 * only allows a single outstanding GATT operation).
 *
 * Permission checks (BLUETOOTH_SCAN/CONNECT) are done by the UI before calling
 * connect(); methods here are annotated to suppress the lint accordingly.
 */
@SuppressLint("MissingPermission")
class WatchBleClient(private val context: Context) {

    private val TAG = "WatchBle"

    private val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val adapter get() = manager.adapter
    private var gatt: BluetoothGatt? = null

    private val _state = MutableStateFlow(ConnState.DISCONNECTED)
    val state: StateFlow<ConnState> = _state.asStateFlow()

    private val _settings = MutableStateFlow<WatchSettings?>(null)
    val settings: StateFlow<WatchSettings?> = _settings.asStateFlow()

    private val _battery = MutableStateFlow<Int?>(null)
    val battery: StateFlow<Int?> = _battery.asStateFlow()

    private val _lastAction = MutableStateFlow<Int?>(null)
    val lastAction: StateFlow<Int?> = _lastAction.asStateFlow()

    // Every watch button tap (StateFlow above dedupes; this fires each time so
    // the executor POSTs on every tap, including the same button twice).
    private val _actionEvents = MutableSharedFlow<Int>(extraBufferCapacity = 16)
    val actionEvents: SharedFlow<Int> = _actionEvents.asSharedFlow()

    private val _log = MutableStateFlow("Prêt")
    val log: StateFlow<String> = _log.asStateFlow()

    private fun logMsg(m: String) { Log.i(TAG, m); _log.value = m }

    // --- Operation queue (one GATT op in flight at a time) --------------------
    private val ops = ArrayDeque<() -> Unit>()
    private var opInFlight = false
    private val lock = Any()

    private fun enqueue(op: () -> Unit) {
        synchronized(lock) {
            ops.addLast(op)
            if (!opInFlight) { opInFlight = true; dispatch() }
        }
    }

    private fun dispatch() {
        val op = synchronized(lock) { ops.removeFirstOrNull() }
        if (op == null) { synchronized(lock) { opInFlight = false }; return }
        op()
    }

    private fun opComplete() = dispatch()

    // --- Scan + connect -------------------------------------------------------
    private val scanner get() = adapter?.bluetoothLeScanner

    fun connect() {
        if (adapter?.isEnabled != true) { logMsg("Bluetooth désactivé"); return }
        if (_state.value == ConnState.CONNECTED) return
        // Restart cleanly even if a previous scan/attempt got stuck.
        try { scanner?.stopScan(scanCb) } catch (_: Exception) {}
        try { gatt?.close() } catch (_: Exception) {}
        gatt = null
        _state.value = ConnState.SCANNING
        logMsg("Recherche de JNKWatch…")
        // Scan without a filter and match by name (the service UUID sits in the
        // scan response, which Android filters unreliably); the name is in the
        // primary advertisement.
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        scanner?.startScan(null, settings, scanCb)
        // Give up after 15 s so a failed scan doesn't hang (auto-reconnect retries).
        handler.postDelayed(scanTimeout, 15_000)
    }

    private val handler: Handler = Handler(Looper.getMainLooper())
    private val scanTimeout: Runnable = Runnable {
        if (_state.value == ConnState.SCANNING) {
            try { scanner?.stopScan(scanCb) } catch (_: Exception) {}
            logMsg("Montre non trouvée")
            _state.value = ConnState.DISCONNECTED
        }
    }

    private val scanCb: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            if (_state.value != ConnState.SCANNING) return
            val rec = result.scanRecord
            val name = rec?.deviceName ?: result.device.name
            val hasSvc = rec?.serviceUuids?.any { it.uuid == BleUuids.SERVICE } == true
            if (name != BleUuids.DEVICE_NAME && !hasSvc) return
            handler.removeCallbacks(scanTimeout)
            scanner?.stopScan(this)
            logMsg("Trouvée: ${name ?: result.device.address}")
            _state.value = ConnState.CONNECTING
            gatt = result.device.connectGatt(context, false, gattCb, BluetoothDevice.TRANSPORT_LE)
        }

        override fun onScanFailed(errorCode: Int) {
            logMsg("Scan échoué ($errorCode)")
            _state.value = ConnState.DISCONNECTED
        }
    }

    fun disconnect() {
        gatt?.disconnect()
    }

    private fun cleanup() {
        try { gatt?.close() } catch (_: Exception) {}
        gatt = null
        synchronized(lock) { ops.clear(); opInFlight = false }
        _settings.value = null
        _battery.value = null
        _state.value = ConnState.DISCONNECTED
    }

    // --- GATT callback --------------------------------------------------------
    private val gattCb = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                logMsg("Connecté, MTU + découverte…")
                g.requestMtu(247)
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                logMsg("Déconnecté")
                cleanup()
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            g.discoverServices()
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val svc = g.getService(BleUuids.SERVICE)
            if (svc == null) { logMsg("Service JNKWatch introuvable"); g.disconnect(); return }
            _state.value = ConnState.CONNECTED
            logMsg("Connecté")
            // On connect: subscribe to Settings + Action events, read current
            // settings, sync the clock.
            subscribe(BleUuids.SETTINGS)
            subscribe(BleUuids.ACTION_EVENT)
            readChar(BleUuids.SETTINGS)
            readChar(BleUuids.STATUS)
            syncTime()
            // Then let the link idle cheaply: some phones otherwise keep the
            // 7.5 ms connection interval used for discovery forever, and the
            // watch's BLE controller can never sleep (battery log 2026-09-29:
            // "conn 7.5ms lat0" -> 0 % light-sleep -> ~11 h autonomy). Delayed
            // so the setup reads/writes above run at full speed first.
            handler.postDelayed({
                gatt?.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_LOW_POWER)
                logMsg("Liaison basse consommation demandée")
            }, 3000)
        }

        override fun onCharacteristicRead(
            g: BluetoothGatt, ch: BluetoothGattCharacteristic, value: ByteArray, status: Int,
        ) {
            handleValue(ch.uuid, value)
            opComplete()
        }

        @Deprecated("Kept for API < 33")
        override fun onCharacteristicRead(
            g: BluetoothGatt, ch: BluetoothGattCharacteristic, status: Int,
        ) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
                handleValue(ch.uuid, ch.value ?: ByteArray(0)); opComplete()
            }
        }

        override fun onCharacteristicChanged(
            g: BluetoothGatt, ch: BluetoothGattCharacteristic, value: ByteArray,
        ) { handleValue(ch.uuid, value) }

        @Deprecated("Kept for API < 33")
        override fun onCharacteristicChanged(g: BluetoothGatt, ch: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
                handleValue(ch.uuid, ch.value ?: ByteArray(0))
        }

        override fun onCharacteristicWrite(
            g: BluetoothGatt, ch: BluetoothGattCharacteristic, status: Int,
        ) { opComplete() }

        override fun onDescriptorWrite(
            g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int,
        ) { opComplete() }
    }

    private fun handleValue(uuid: java.util.UUID, value: ByteArray) {
        when (uuid) {
            BleUuids.SETTINGS -> WatchSettings.fromBytes(value)?.let { _settings.value = it }
            BleUuids.STATUS -> if (value.isNotEmpty()) {
                val pct = value[0].toInt() and 0xFF
                _battery.value = if (pct <= 100) pct else null  // 0xFF = unknown (charging since boot)
            }
            BleUuids.ACTION_EVENT -> if (value.size >= 4) {
                val id = (value[0].toInt() and 0xFF) or ((value[1].toInt() and 0xFF) shl 8) or
                    ((value[2].toInt() and 0xFF) shl 16) or ((value[3].toInt() and 0xFF) shl 24)
                _lastAction.value = id
                _actionEvents.tryEmit(id)
                logMsg("Bouton montre #$id")
            }
        }
    }

    // --- Public operations ----------------------------------------------------
    fun writeSettings(s: WatchSettings) {
        _settings.value = s
        writeChar(BleUuids.SETTINGS, s.toBytes())
    }

    fun syncTime() {
        val epoch = System.currentTimeMillis() / 1000L
        val b = ByteArray(8) { i -> ((epoch shr (8 * i)) and 0xFF).toByte() }
        writeChar(BleUuids.TIME, b)
        logMsg("Heure synchronisée")
    }

    /** Push current weather: [temp int16 LE][cond u8][location NUL]. */
    fun pushWeather(tempC: Int, cond: Int, location: String) {
        if (_state.value != ConnState.CONNECTED) return
        val out = java.io.ByteArrayOutputStream()
        val t = tempC.coerceIn(-32768, 32767)
        out.write(t and 0xFF)
        out.write((t shr 8) and 0xFF)
        out.write(cond and 0xFF)
        out.write(location.take(23).toByteArray(Charsets.UTF_8))
        out.write(0)
        writeChar(BleUuids.WEATHER, out.toByteArray())
        logMsg("Météo → $tempC°C $location")
    }

    /**
     * Push the 6 button definitions to the watch: ActionCfg clear (op 0x00),
     * then one set (op 0x01, 20 bytes) per button:
     * [op=1][index][id u32 LE][icon][enabled][label 12B NUL-padded].
     * The watch echoes [id] in ActionEvent when a button is tapped.
     */
    fun pushActionConfig(buttons: List<ActionButton>) {
        if (_state.value != ConnState.CONNECTED) return
        writeChar(BleUuids.ACTION_CFG, byteArrayOf(0))   // clear
        buttons.forEachIndexed { i, b ->
            val out = java.io.ByteArrayOutputStream()
            out.write(1)               // op: set
            out.write(i and 0xFF)      // index
            out.write(b.id and 0xFF)   // id u32 LE
            out.write((b.id shr 8) and 0xFF)
            out.write((b.id shr 16) and 0xFF)
            out.write((b.id shr 24) and 0xFF)
            out.write(b.icon and 0xFF)
            out.write(if (b.enabled) 1 else 0)
            var lab = b.label.toByteArray(Charsets.UTF_8)
            if (lab.size > 11) lab = lab.copyOf(11)      // leave room for NUL
            out.write(lab)
            repeat(12 - lab.size) { out.write(0) }       // pad label to 12 bytes
            writeChar(BleUuids.ACTION_CFG, out.toByteArray())
        }
        logMsg("Boutons → montre (${buttons.size})")
    }

    /**
     * Push one server-probe reading to the watch's "Sondes" face:
     * [slot u8][name\0][value\0]. [slot] (0-based) is the probe's fixed row; the
     * watch keeps the latest value per slot and stamps it with its own clock.
     */
    fun pushProbe(slot: Int, name: String, value: String) {
        if (_state.value != ConnState.CONNECTED) return
        val out = java.io.ByteArrayOutputStream()
        out.write(slot and 0xFF)
        out.write(name.take(23).toByteArray(Charsets.UTF_8)); out.write(0)
        out.write(value.take(47).toByteArray(Charsets.UTF_8)); out.write(0)
        writeChar(BleUuids.PROBE, out.toByteArray())
        logMsg("Sonde[$slot] → $name : $value")
    }

    /** Push a phone notification: "app\0title\0body" (the watch truncates). */
    fun pushNotification(app: String, title: String, body: String) {
        if (_state.value != ConnState.CONNECTED) return
        val out = java.io.ByteArrayOutputStream()
        fun field(s: String, max: Int) {
            out.write(s.take(max).toByteArray(Charsets.UTF_8)); out.write(0)
        }
        field(app, 20); field(title, 50); field(body, 140)
        writeChar(BleUuids.NOTIF_PUSH, out.toByteArray())
        logMsg("Notif → $app : $title")
    }

    // --- Low-level GATT helpers (queued) --------------------------------------
    private fun charOf(uuid: java.util.UUID): BluetoothGattCharacteristic? =
        gatt?.getService(BleUuids.SERVICE)?.getCharacteristic(uuid)

    private fun readChar(uuid: java.util.UUID) = enqueue {
        val ch = charOf(uuid); if (ch == null) { opComplete(); return@enqueue }
        gatt?.readCharacteristic(ch)
    }

    private fun writeChar(uuid: java.util.UUID, value: ByteArray) = enqueue {
        val ch = charOf(uuid); val g = gatt
        if (ch == null || g == null) { opComplete(); return@enqueue }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeCharacteristic(ch, value, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            ch.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            @Suppress("DEPRECATION") run { ch.value = value; g.writeCharacteristic(ch) }
        }
    }

    private fun subscribe(uuid: java.util.UUID) = enqueue {
        val ch = charOf(uuid); val g = gatt
        if (ch == null || g == null) { opComplete(); return@enqueue }
        g.setCharacteristicNotification(ch, true)
        val cccd = ch.getDescriptor(BleUuids.CCCD)
        if (cccd == null) { opComplete(); return@enqueue }
        val enable = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeDescriptor(cccd, enable)
        } else {
            @Suppress("DEPRECATION") run { cccd.value = enable; g.writeDescriptor(cccd) }
        }
    }
}
