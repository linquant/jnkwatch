package fr.ponera.jnkwatch.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
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
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import fr.ponera.jnkwatch.model.GetProbe
import fr.ponera.jnkwatch.probes.GetProbesPrefs
import fr.ponera.jnkwatch.probes.ProbeRunner
import kotlinx.coroutines.launch

@Composable
fun ProbesScreen(onBack: () -> Unit) {
    val ctx = LocalContext.current
    val probes = remember { mutableStateListOf<GetProbe>().apply { addAll(GetProbesPrefs.load(ctx)) } }
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
            Text("Sondes serveur (GET)", fontSize = 20.sp,
                fontWeight = FontWeight.Bold, color = Color.White)
        }
        Text(
            "L'app interroge chaque URL à intervalle régulier et affiche la valeur " +
                "extraite sur l'écran notifications de la montre.",
            color = Color(0xFF9AA7AE), fontSize = 14.sp,
        )

        probes.forEachIndexed { i, p ->
            ProbeCard(
                probe = p,
                onChange = { probes[i] = it; saved = false },
                onDelete = { probes.removeAt(i); saved = false },
            )
        }

        if (probes.size < GetProbe.MAX) {
            OutlinedButton(
                onClick = {
                    probes.add(GetProbe(id = GetProbesPrefs.nextId(probes), enabled = true))
                    saved = false
                },
                modifier = Modifier.fillMaxWidth(),
            ) { Text("+ Ajouter une sonde") }
        }

        Button(
            onClick = { GetProbesPrefs.save(ctx, probes.toList()); saved = true },
            modifier = Modifier.fillMaxWidth(),
        ) { Text("Enregistrer") }

        if (saved) {
            Text("Enregistré. Relève active en arrière-plan.",
                color = Color(0xFF6FCF8F), fontSize = 13.sp)
        }
    }
}

@Composable
private fun ProbeCard(
    probe: GetProbe,
    onChange: (GetProbe) -> Unit,
    onDelete: () -> Unit,
) {
    val scope = rememberCoroutineScope()
    var testResult by remember { mutableStateOf<String?>(null) }
    var testing by remember { mutableStateOf(false) }

    Card(Modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(probe.name.ifBlank { "Sonde" }, color = Color.White, fontWeight = FontWeight.Bold)
                Switch(checked = probe.enabled, onCheckedChange = { onChange(probe.copy(enabled = it)) })
            }
            OutlinedTextField(
                value = probe.name,
                onValueChange = { onChange(probe.copy(name = it.take(20))) },
                label = { Text("Nom (titre montre)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = probe.url,
                onValueChange = { onChange(probe.copy(url = it.trim())) },
                label = { Text("URL GET (https://…)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = probe.jsonField,
                onValueChange = { onChange(probe.copy(jsonField = it.trim())) },
                label = { Text("Champ JSON (ex. data.warning · vide = brut)") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = probe.headers,
                onValueChange = { onChange(probe.copy(headers = it)) },
                label = { Text("En-têtes (Clé: Valeur, une par ligne)") },
                placeholder = { Text("Authorization: Bearer …") },
                minLines = 1,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = probe.intervalMin.toString(),
                onValueChange = {
                    val n = it.filter { c -> c.isDigit() }.toIntOrNull() ?: 0
                    onChange(probe.copy(intervalMin = n))
                },
                label = { Text("Intervalle (min)") },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                modifier = Modifier.width(180.dp),
            )
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                OutlinedButton(
                    enabled = !testing && probe.url.isNotBlank(),
                    onClick = {
                        testing = true; testResult = null
                        scope.launch {
                            val out = ProbeRunner.fetch(probe)
                            testResult = out.value?.let { "→ $it" } ?: "Erreur : ${out.error}"
                            testing = false
                        }
                    },
                ) { Text(if (testing) "…" else "Tester") }
                TextButton(onClick = onDelete) { Text("Supprimer", color = Color(0xFFD9776B)) }
            }
            testResult?.let {
                Text(it, color = Color(0xFFCFE0E6), fontSize = 13.sp)
            }
        }
    }
}
