package fr.ponera.jnkwatch

import android.app.Application
import fr.ponera.jnkwatch.ble.WatchConnection

class JnkApp : Application() {
    override fun onCreate() {
        super.onCreate()
        WatchConnection.init(this)
    }
}
