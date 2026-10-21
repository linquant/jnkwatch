package fr.ponera.jnkwatch.ble

import android.content.Context

/**
 * App-wide single BLE client, so the UI, the foreground service and the
 * SMS/call receivers all share one connection.
 */
object WatchConnection {
    @Volatile private var instance: WatchBleClient? = null

    fun init(context: Context) {
        if (instance == null) {
            synchronized(this) {
                if (instance == null) instance = WatchBleClient(context.applicationContext)
            }
        }
    }

    val client: WatchBleClient
        get() = instance ?: error("WatchConnection.init() not called")
}
