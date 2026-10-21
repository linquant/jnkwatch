package fr.ponera.jnkwatch.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Card
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import fr.ponera.jnkwatch.notif.NotifPrefs

@Composable
fun NotifFilterScreen(onBack: () -> Unit) {
    val ctx = LocalContext.current
    val pm = ctx.packageManager

    var calls by remember { mutableStateOf(NotifPrefs.callsEnabled(ctx)) }

    data class AppRow(val pkg: String, val label: String)
    val apps = remember {
        (NotifPrefs.seen(ctx) + NotifPrefs.enabled(ctx)).map { pkg ->
            val label = try {
                pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)).toString()
            } catch (_: Exception) { pkg }
            AppRow(pkg, label)
        }.sortedBy { it.label.lowercase() }
    }
    val enabledState = remember {
        mutableStateMapOf<String, Boolean>().apply {
            apps.forEach { put(it.pkg, NotifPrefs.isEnabled(ctx, it.pkg)) }
        }
    }

    Column(
        Modifier
            .fillMaxSize()
            .background(Color(0xFF05070A))
            .verticalScroll(rememberScrollState())
            .padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onBack) { Text("← Retour") }
            Text("Notifications à transférer", fontSize = 20.sp,
                fontWeight = FontWeight.Bold, color = Color.White)
        }

        Card(Modifier.fillMaxWidth()) {
            Column(Modifier.padding(horizontal = 16.dp)) {
                ToggleRow("Appels", calls) {
                    calls = it; NotifPrefs.setCallsEnabled(ctx, it)
                }
            }
        }

        Text("Applications", color = Color(0xFF9AA7AE))

        Card(Modifier.fillMaxWidth()) {
            Column(Modifier.padding(horizontal = 16.dp)) {
                if (apps.isEmpty()) {
                    Text(
                        "Aucune appli détectée pour l'instant. Elles apparaissent ici dès " +
                            "qu'elles envoient une notification.",
                        color = Color(0xFF9AA7AE),
                        modifier = Modifier.padding(vertical = 14.dp),
                    )
                }
                apps.forEachIndexed { i, app ->
                    if (i > 0) HorizontalDivider(color = Color(0xFF1A2228))
                    ToggleRow(app.label, enabledState[app.pkg] ?: false) {
                        enabledState[app.pkg] = it
                        NotifPrefs.setEnabled(ctx, app.pkg, it)
                    }
                }
            }
        }
    }
}

@Composable
private fun ToggleRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        Modifier
            .fillMaxWidth()
            .padding(vertical = 6.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, color = Color.White)
        Switch(checked = checked, onCheckedChange = onChange)
    }
}
