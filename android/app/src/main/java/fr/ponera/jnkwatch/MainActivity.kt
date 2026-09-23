package fr.ponera.jnkwatch

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import fr.ponera.jnkwatch.ui.JnkTheme
import fr.ponera.jnkwatch.ui.NotifFilterScreen
import fr.ponera.jnkwatch.ui.WatchScreen
import fr.ponera.jnkwatch.ui.WatchViewModel

class MainActivity : ComponentActivity() {

    private val vm: WatchViewModel by viewModels()

    private val permLauncher =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            JnkTheme {
                var showFilter by remember { mutableStateOf(false) }
                if (showFilter) {
                    NotifFilterScreen(onBack = { showFilter = false })
                } else {
                    WatchScreen(
                        vm,
                        onNeedPermissions = ::ensurePermissions,
                        onOpenFilter = { showFilter = true },
                    )
                }
            }
        }
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
        // Call forwarding
        perms += Manifest.permission.READ_PHONE_STATE
        perms += Manifest.permission.READ_CALL_LOG
        // Weather (phone location)
        perms += Manifest.permission.ACCESS_COARSE_LOCATION
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
            perms += Manifest.permission.POST_NOTIFICATIONS

        val missing = perms.filter {
            checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isNotEmpty()) permLauncher.launch(missing.toTypedArray())
    }
}
