package fr.ponera.jnkwatch

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import fr.ponera.jnkwatch.ui.JnkTheme
import fr.ponera.jnkwatch.ui.WatchScreen
import fr.ponera.jnkwatch.ui.WatchViewModel

class MainActivity : ComponentActivity() {

    private val vm: WatchViewModel by viewModels()

    private val permLauncher =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { JnkTheme { WatchScreen(vm, onNeedPermissions = ::ensurePermissions) } }
        ensurePermissions()
    }

    private fun ensurePermissions() {
        val perms = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            perms += Manifest.permission.BLUETOOTH_SCAN
            perms += Manifest.permission.BLUETOOTH_CONNECT
        } else {
            perms += Manifest.permission.ACCESS_FINE_LOCATION
        }
        // SMS + call forwarding
        perms += Manifest.permission.RECEIVE_SMS
        perms += Manifest.permission.READ_PHONE_STATE
        perms += Manifest.permission.READ_CALL_LOG
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
            perms += Manifest.permission.POST_NOTIFICATIONS

        val missing = perms.filter {
            checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isNotEmpty()) permLauncher.launch(missing.toTypedArray())
    }
}
