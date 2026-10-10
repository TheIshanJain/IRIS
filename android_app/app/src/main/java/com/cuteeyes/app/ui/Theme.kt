package com.cuteeyes.app.ui

import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.keyframes
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

val Bg = Color(0xFF0D1020)
val CardColor = Color(0xFF171A2E)
val Card2 = Color(0xFF1F2340)
val Cyan = Color(0xFF46E1FF)
val Pink = Color(0xFFFF6FAE)
val Purple = Color(0xFFA98BFF)
val TextMain = Color(0xFFEEF1FF)
val Dim = Color(0xFF8D93B5)
val Good = Color(0xFF4DFFA0)
val Warn = Color(0xFFFFC24D)
val Bad = Color(0xFFFF6B81)

@Composable
fun CuteEyesTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Cyan,
            onPrimary = Color(0xFF0B1020),
            secondary = Pink,
            onSecondary = Color(0xFF0B1020),
            tertiary = Purple,
            background = Bg,
            onBackground = TextMain,
            surface = CardColor,
            onSurface = TextMain,
            surfaceVariant = Card2,
            onSurfaceVariant = Dim,
            error = Bad,
        ),
        shapes = Shapes(
            small = RoundedCornerShape(12.dp),
            medium = RoundedCornerShape(18.dp),
            large = RoundedCornerShape(24.dp),
        ),
        content = content,
    )
}

/** The little round face with two blinking eyes (same as the device). */
@Composable
fun EyesLogo(size: Dp = 96.dp, color: Color = Cyan) {
    val t = rememberInfiniteTransition(label = "blink")
    val open by t.animateFloat(
        initialValue = 1f,
        targetValue = 1f,
        animationSpec = infiniteRepeatable(
            animation = keyframes {
                durationMillis = 3800
                1f at 3400
                0.1f at 3550
                1f at 3700
            },
            repeatMode = RepeatMode.Restart,
        ),
        label = "open",
    )
    Canvas(Modifier.size(size)) {
        val r = this.size.minDimension / 2f
        drawCircle(Color.Black, radius = r)
        drawCircle(color.copy(alpha = 0.25f), radius = r - 2f, style = Stroke(width = r * 0.18f))
        drawCircle(color, radius = r - 2f, style = Stroke(width = r * 0.05f))
        val w = r * 0.30f
        val h = r * 0.62f * open
        val gap = r * 0.36f
        listOf(center.x - gap, center.x + gap).forEach { cx ->
            drawRoundRect(
                color = color,
                topLeft = Offset(cx - w / 2f, center.y - h / 2f),
                size = Size(w, h),
                cornerRadius = CornerRadius(w / 2f, w / 2f),
            )
        }
    }
}

@Composable
fun SoftCard(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    Card(
        modifier = modifier.fillMaxWidth(),
        shape = RoundedCornerShape(22.dp),
        colors = CardDefaults.cardColors(containerColor = CardColor),
    ) {
        Column(Modifier.padding(16.dp)) { content() }
    }
}

@Composable
fun SectionTitle(text: String) {
    Text(
        text.uppercase(),
        color = Dim,
        fontSize = 12.sp,
        fontWeight = FontWeight.Bold,
        letterSpacing = 1.sp,
        modifier = Modifier.padding(start = 4.dp, top = 12.dp, bottom = 2.dp),
    )
}

@Composable
fun InfoRow(label: String, value: String) {
    Row(Modifier.fillMaxWidth().padding(vertical = 5.dp), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = Dim, fontSize = 14.sp)
        Spacer(Modifier.width(12.dp))
        Text(value, fontSize = 14.sp)
    }
}

/** Emoji in a soft rounded square, used as list icons. */
@Composable
fun EmojiBadge(emoji: String) {
    Box(
        Modifier.size(40.dp),
        contentAlignment = Alignment.Center,
    ) {
        Canvas(Modifier.size(40.dp)) {
            drawRoundRect(Card2, cornerRadius = CornerRadius(24f, 24f))
        }
        Text(emoji, fontSize = 19.sp)
    }
}

fun signalBars(rssi: Int): String = when {
    rssi >= -55 -> "▂▄▆█"
    rssi >= -67 -> "▂▄▆ "
    rssi >= -78 -> "▂▄  "
    else -> "▂   "
}
