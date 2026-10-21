package fr.ponera.jnkwatch.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val Scheme = darkColorScheme(
    primary = Color(0xFF37DCE0),
    onPrimary = Color(0xFF05070A),
    background = Color(0xFF05070A),
    surface = Color(0xFF0C1418),
    onBackground = Color(0xFFE7EEF2),
    onSurface = Color(0xFFE7EEF2),
)

@Composable
fun JnkTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = Scheme, content = content)
}
