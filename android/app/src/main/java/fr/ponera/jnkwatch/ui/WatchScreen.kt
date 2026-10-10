package fr.ponera.jnkwatch.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import fr.ponera.jnkwatch.notif.NotifAccess
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import fr.ponera.jnkwatch.ble.ConnState
import fr.ponera.jnkwatch.model.WatchSettings

@Composable
fun WatchScreen(
    vm: WatchViewModel,
    onNeedPermissions: () -> Unit,
    onOpenFilter: () -> Unit,
    onOpenActions: () -> Unit,
    onOpenProbes: () -> Unit,
) {
    val state by vm.state.collectAsStateWithLifecycle()
    val settings by vm.settings.collectAsStateWithLifecycle()
    val battery by vm.battery.collectAsStateWithLifecycle()
    val lastAction by vm.lastAction.collectAsStateWithLifecycle()
    val log by vm.log.collectAsStateWithLifecycle()

    Column(
        Modifier
            .fillMaxSize()
            .background(Color(0xFF05070A))
            .verticalScroll(rememberScrollState())
            .padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
    ) {
        Text("JNKWatch", fontSize = 30.sp, fontWeight = FontWeight.Bold, color = Color.White)

        // Connection status + battery
        Row(verticalAlignment = Alignment.CenterVertically) {
            val label = when (state) {
                ConnState.DISCONNECTED -> "Déconnecté"
                ConnState.SCANNING -> "Recherche…"
                ConnState.CONNECTING -> "Connexion…"
                ConnState.CONNECTED -> "Connecté"
            }
            Text(label, color = Color(0xFF9AA7AE))
            battery?.let { Text("   🔋 $it %", color = Color(0xFF9AA7AE)) }
        }

        Button(
            onClick = {
                onNeedPermissions()
                if (state == ConnState.DISCONNECTED) vm.connect() else vm.disconnect()
            },
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text(if (state == ConnState.DISCONNECTED) "Connecter" else "Déconnecter")
        }

        androidx.compose.material3.OutlinedButton(
            onClick = onOpenFilter,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Notifications à transférer")
        }

        androidx.compose.material3.OutlinedButton(
            onClick = onOpenActions,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Boutons d'action")
        }

        androidx.compose.material3.OutlinedButton(
            onClick = onOpenProbes,
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Sondes serveur (GET)")
        }

        // Notification access — needed to forward SMS/RCS/app messages.
        val context = LocalContext.current
        var notifAccess by remember { mutableStateOf(NotifAccess.isGranted(context)) }
        val lifecycleOwner = LocalLifecycleOwner.current
        DisposableEffect(lifecycleOwner) {
            val obs = LifecycleEventObserver { _, e ->
                if (e == Lifecycle.Event.ON_RESUME) notifAccess = NotifAccess.isGranted(context)
            }
            lifecycleOwner.lifecycle.addObserver(obs)
            onDispose { lifecycleOwner.lifecycle.removeObserver(obs) }
        }
        if (!notifAccess) {
            Card(Modifier.fillMaxWidth()) {
                Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    Text("Accès aux notifications requis pour transférer les SMS et messages.",
                        color = Color(0xFFE7EEF2))
                    Button(onClick = { NotifAccess.openSettings(context) },
                        modifier = Modifier.fillMaxWidth()) {
                        Text("Autoriser l'accès aux notifications")
                    }
                }
            }
        }

        val s = settings
        if (state == ConnState.CONNECTED && s != null) {
            SettingsCard(s, onChange = vm::update)
            Button(onClick = vm::syncTime, modifier = Modifier.fillMaxWidth()) {
                Text("Synchroniser l'heure")
            }
            lastAction?.let {
                Text("Dernier bouton montre : #$it", color = Color(0xFF9AA7AE))
            }
        }

        Spacer(Modifier.height(4.dp))
        Text(log, color = Color(0xFF5A666C), fontSize = 13.sp)
    }
}

@Composable
private fun SettingsCard(s: WatchSettings, onChange: (WatchSettings) -> Unit) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(18.dp), verticalArrangement = Arrangement.spacedBy(18.dp)) {

            // Brightness — track locally while dragging, write once on release.
            var pos by remember(s.brightness) { mutableFloatStateOf(s.brightness.toFloat()) }
            Text("Luminosité : ${pos.toInt()} %", color = Color.White)
            Slider(
                value = pos,
                onValueChange = { pos = it },
                onValueChangeFinished = { onChange(s.copy(brightness = pos.toInt().coerceIn(10, 100))) },
                valueRange = 10f..100f,
            )

            // Sleep
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("Veille :", color = Color.White)
                FilterChip(selected = !s.sleep10s, onClick = { onChange(s.copy(sleep10s = false)) },
                    label = { Text("5 s") })
                FilterChip(selected = s.sleep10s, onClick = { onChange(s.copy(sleep10s = true)) },
                    label = { Text("10 s") })
            }

            // Accent
            Text("Couleur", color = Color.White)
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                WatchSettings.ACCENT_ARGB.forEachIndexed { i, argb ->
                    val selected = i == s.accent
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Spacer(
                            Modifier
                                .size(38.dp)
                                .background(Color(argb), CircleShape)
                                .then(
                                    if (selected) Modifier.border(3.dp, Color.White, CircleShape)
                                    else Modifier
                                )
                                .clickable { onChange(s.copy(accent = i)) },
                        )
                    }
                }
            }

            // Wrist wake
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("Réveil au poignet", color = Color.White)
                Switch(checked = s.wristWake, onCheckedChange = { onChange(s.copy(wristWake = it)) })
            }

            // Wrist-wake sensitivity (only relevant when the wake gesture is on)
            if (s.wristWake) {
                Row(
                    Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    Text("Sensibilité :", color = Color.White)
                    WatchSettings.WRIST_SENS.forEachIndexed { i, label ->
                        FilterChip(
                            selected = s.wristSens == i,
                            onClick = { onChange(s.copy(wristSens = i)) },
                            label = { Text(label) },
                        )
                    }
                }
            }

            // Keep screen on while charging
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("Garder allumé sur secteur", color = Color.White)
                Switch(
                    checked = s.stayAwakeCharging,
                    onCheckedChange = { onChange(s.copy(stayAwakeCharging = it)) },
                )
            }

            // Power-saver "watch only" mode (deep sleep, BLE off, clock only)
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column(Modifier.weight(1f)) {
                    Text("Mode montre seule", color = Color.White)
                    Text(
                        "Écran éteint ; lever le poignet affiche l'heure. Bluetooth coupé.",
                        color = Color.Gray,
                        fontSize = 12.sp,
                    )
                }
                Switch(
                    checked = s.powerSaver,
                    onCheckedChange = { onChange(s.copy(powerSaver = it)) },
                )
            }
        }
    }
}
