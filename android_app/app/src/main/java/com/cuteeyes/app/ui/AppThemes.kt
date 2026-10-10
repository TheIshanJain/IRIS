package com.cuteeyes.app.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.Font
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.cuteeyes.app.R

/** Everything that makes one app theme look different. */
data class AppTheme(
    val id: String,
    val name: String,
    val emoji: String,
    val tagline: String,
    val dark: Boolean,
    val bg: Color,
    val card: Color,
    val card2: Color,
    val accent: Color,
    val accent2: Color,
    val text: Color,
    val dim: Color,
    val good: Color,
    val warn: Color,
    val bad: Color,
    val onAccent: Color,
    val display: FontFamily,   // headings
    val body: FontFamily,      // everything else
    val radius: Dp,            // card corner roundness
    val outlined: Boolean,     // thin accent border on cards
)

private val Poppins = FontFamily(
    Font(R.font.poppins_regular, FontWeight.Normal),
    Font(R.font.poppins_semibold, FontWeight.SemiBold),
    Font(R.font.poppins_bold, FontWeight.Bold),
    Font(R.font.poppins_bold, FontWeight.ExtraBold),
)
private val Varela = FontFamily(Font(R.font.varela_round, FontWeight.Normal))
private val DmSerif = FontFamily(Font(R.font.dm_serif_display, FontWeight.Normal))
private val SpaceMono = FontFamily(
    Font(R.font.space_mono_regular, FontWeight.Normal),
    Font(R.font.space_mono_bold, FontWeight.Bold),
)
private val Righteous = FontFamily(Font(R.font.righteous, FontWeight.Normal))

object AppThemes {
    val Neon = AppTheme(
        id = "neon", name = "Iris Neon", emoji = "🌌", tagline = "Glowing cyan on deep night",
        dark = true, bg = Color(0xFF0D1020), card = Color(0xFF171A2E), card2 = Color(0xFF22274A),
        accent = Color(0xFF46E1FF), accent2 = Color(0xFFA98BFF), text = Color(0xFFEEF1FF), dim = Color(0xFF8D93B5),
        good = Color(0xFF4DFFA0), warn = Color(0xFFFFC24D), bad = Color(0xFFFF6B81), onAccent = Color(0xFF0B1020),
        display = Poppins, body = Poppins, radius = 22.dp, outlined = false,
    )
    val Sakura = AppTheme(
        id = "sakura", name = "Sakura", emoji = "🌸", tagline = "Soft, light and lovely",
        dark = false, bg = Color(0xFFFFF4F7), card = Color(0xFFFFFFFF), card2 = Color(0xFFFFE3EC),
        accent = Color(0xFFFF5C9A), accent2 = Color(0xFFFF9CC2), text = Color(0xFF3A2230), dim = Color(0xFF9B7A88),
        good = Color(0xFF2EB87A), warn = Color(0xFFE59A1A), bad = Color(0xFFE5486A), onAccent = Color.White,
        display = Varela, body = Varela, radius = 28.dp, outlined = false,
    )
    val Gold = AppTheme(
        id = "gold", name = "Midnight Gold", emoji = "👑", tagline = "Black, gold and elegant",
        dark = true, bg = Color(0xFF0A0A0B), card = Color(0xFF151412), card2 = Color(0xFF221F1A),
        accent = Color(0xFFE8C27A), accent2 = Color(0xFFB8893B), text = Color(0xFFF5EFE3), dim = Color(0xFF9A917F),
        good = Color(0xFF9BD67A), warn = Color(0xFFE8C27A), bad = Color(0xFFE07A6A), onAccent = Color(0xFF1A1408),
        display = DmSerif, body = Poppins, radius = 14.dp, outlined = true,
    )
    val Terminal = AppTheme(
        id = "terminal", name = "Terminal", emoji = "💚", tagline = "Green on black, hacker style",
        dark = true, bg = Color(0xFF040704), card = Color(0xFF0A110A), card2 = Color(0xFF102010),
        accent = Color(0xFF39FF6A), accent2 = Color(0xFF1EB34A), text = Color(0xFFCFFFD9), dim = Color(0xFF5E8F68),
        good = Color(0xFF39FF6A), warn = Color(0xFFE5FF39), bad = Color(0xFFFF5A5A), onAccent = Color(0xFF021004),
        display = SpaceMono, body = SpaceMono, radius = 6.dp, outlined = true,
    )
    val Ocean = AppTheme(
        id = "ocean", name = "Ocean Breeze", emoji = "🌊", tagline = "Calm teal and aqua",
        dark = true, bg = Color(0xFF04161F), card = Color(0xFF0A2633), card2 = Color(0xFF0F3546),
        accent = Color(0xFF3DE0C8), accent2 = Color(0xFF2F8CFF), text = Color(0xFFE6FBFF), dim = Color(0xFF7FA9B5),
        good = Color(0xFF3DE0C8), warn = Color(0xFFFFC857), bad = Color(0xFFFF7A85), onAccent = Color(0xFF02141A),
        display = Righteous, body = Varela, radius = 24.dp, outlined = false,
    )

    val all = listOf(Neon, Sakura, Gold, Terminal, Ocean)

    fun byId(id: String) = all.firstOrNull { it.id == id } ?: Neon
}

val LocalAppTheme = staticCompositionLocalOf { AppThemes.Neon }

private fun typographyFor(t: AppTheme): Typography {
    val base = Typography()
    fun TextStyle.d() = copy(fontFamily = t.display)
    fun TextStyle.b() = copy(fontFamily = t.body)
    return base.copy(
        displayLarge = base.displayLarge.d(), displayMedium = base.displayMedium.d(), displaySmall = base.displaySmall.d(),
        headlineLarge = base.headlineLarge.d(), headlineMedium = base.headlineMedium.d(), headlineSmall = base.headlineSmall.d(),
        titleLarge = base.titleLarge.d(), titleMedium = base.titleMedium.b(), titleSmall = base.titleSmall.b(),
        bodyLarge = base.bodyLarge.b(), bodyMedium = base.bodyMedium.b(), bodySmall = base.bodySmall.b(),
        labelLarge = base.labelLarge.b(), labelMedium = base.labelMedium.b(), labelSmall = base.labelSmall.b(),
    )
}

@Composable
fun IrisTheme(theme: AppTheme, content: @Composable () -> Unit) {
    val scheme = if (theme.dark) {
        darkColorScheme(
            primary = theme.accent, onPrimary = theme.onAccent, secondary = theme.accent2,
            background = theme.bg, onBackground = theme.text, surface = theme.card, onSurface = theme.text,
            surfaceVariant = theme.card2, onSurfaceVariant = theme.dim, error = theme.bad,
            outline = theme.dim, primaryContainer = theme.card2, onPrimaryContainer = theme.text,
            secondaryContainer = theme.card2, onSecondaryContainer = theme.text,
        )
    } else {
        lightColorScheme(
            primary = theme.accent, onPrimary = theme.onAccent, secondary = theme.accent2,
            background = theme.bg, onBackground = theme.text, surface = theme.card, onSurface = theme.text,
            surfaceVariant = theme.card2, onSurfaceVariant = theme.dim, error = theme.bad,
            outline = theme.dim, primaryContainer = theme.card2, onPrimaryContainer = theme.text,
            secondaryContainer = theme.card2, onSecondaryContainer = theme.text,
        )
    }
    CompositionLocalProvider(LocalAppTheme provides theme) {
        MaterialTheme(colorScheme = scheme, typography = typographyFor(theme), content = content)
    }
}
