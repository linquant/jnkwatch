package fr.ponera.jnkwatch.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
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
import fr.ponera.jnkwatch.actions.ActionsPrefs
import fr.ponera.jnkwatch.ble.ConnState
import fr.ponera.jnkwatch.ble.WatchConnection
import fr.ponera.jnkwatch.model.ActionButton

@Composable
fun ActionsScreen(onBack: () -> Unit) {
    val ctx = LocalContext.current
    val buttons = remember { mutableStateListOf<ActionButton>().apply { addAll(ActionsPrefs.load(ctx)) } }
    var saved by remember { mutableStateOf(false) }

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
            Text("Boutons d'action", fontSize = 20.sp,
                fontWeight = FontWeight.Bold, color = Color.White)
        }
        Text(
            "Chaque bouton de la montre déclenche une requête POST vers l'URL indiquée.",
            color = Color(0xFF9AA7AE), fontSize = 14.sp,
        )

        buttons.forEachIndexed { i, b ->
            ButtonCard(
                index = i,
                button = b,
                onChange = { buttons[i] = it; saved = false },
            )
        }

        Button(
            onClick = {
                ActionsPrefs.save(ctx, buttons.toList())
                WatchConnection.client.pushActionConfig(buttons.toList())
                saved = true
            },
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Enregistrer et envoyer à la montre")
        }

        val connected = WatchConnection.client.state.value == ConnState.CONNECTED
        if (saved && !connected) {
            Text("Enregistré. Boutons envoyés à la prochaine connexion de la montre.",
                color = Color(0xFFE0A050), fontSize = 13.sp)
        } else if (saved) {
            Text("Enregistré et envoyé.", color = Color(0xFF6FCF8F), fontSize = 13.sp)
        }
    }
}

@Composable
private fun ButtonCard(index: Int, button: ActionButton, onChange: (ActionButton) -> Unit) {
    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text("Bouton ${index + 1}", color = Color.White, fontWeight = FontWeight.Bold)
                Switch(checked = button.enabled,
                    onCheckedChange = { onChange(button.copy(enabled = it)) })
            }
            OutlinedTextField(
                value = button.label,
                onValueChange = { onChange(button.copy(label = it.take(11))) },
                label = { Text("Nom (montre)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            IconPicker(button.icon) { onChange(button.copy(icon = it)) }
            OutlinedTextField(
                value = button.url,
                onValueChange = { onChange(button.copy(url = it.trim())) },
                label = { Text("URL POST (https://…)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
}

@Composable
private fun IconPicker(selected: Int, onSelect: (Int) -> Unit) {
    var open by remember { mutableStateOf(false) }
    Box {
        OutlinedButton(onClick = { open = true }, modifier = Modifier.fillMaxWidth()) {
            Text("Icône : ${ActionButton.ICONS.getOrElse(selected) { "?" }}")
        }
        DropdownMenu(expanded = open, onDismissRequest = { open = false }) {
            ActionButton.ICONS.forEachIndexed { i, name ->
                DropdownMenuItem(
                    text = { Text(name) },
                    onClick = { onSelect(i); open = false },
                )
            }
        }
    }
}
