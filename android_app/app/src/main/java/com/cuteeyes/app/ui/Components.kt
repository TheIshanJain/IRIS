package com.cuteeyes.app.ui

import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.keyframes
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/** The IRIS face: round, black, two blinking eyes (same as the device). */
@Composable
fun EyesLogo(size: Dp = 96.dp, color: Color = LocalAppTheme.current.accent) {
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
        drawCircle(color.copy(alpha = 0.22f), radius = r - 2f, style = Stroke(width = r * 0.18f))
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

/** A themed card. */
@Composable
fun IrisCard(
    modifier: Modifier = Modifier,
    onClick: (() -> Unit)? = null,
    highlighted: Boolean = false,
    padding: Dp = 16.dp,
    content: @Composable () -> Unit,
) {
    val th = LocalAppTheme.current
    val shape = RoundedCornerShape(th.radius)
    var m = modifier.fillMaxWidth().clip(shape).background(th.card)
    if (th.outlined || highlighted) {
        m = m.border(BorderStroke(if (highlighted) 2.dp else 1.dp, if (highlighted) th.accent else th.accent.copy(alpha = 0.25f)), shape)
    }
    if (onClick != null) m = m.clickable { onClick() }
    Column(m.padding(padding)) { content() }
}

@Composable
fun SectionTitle(text: String) {
    val th = LocalAppTheme.current
    Text(
        text.uppercase(),
        color = th.dim,
        fontSize = 12.sp,
        fontWeight = FontWeight.Bold,
        letterSpacing = 1.2.sp,
        modifier = Modifier.padding(start = 4.dp, top = 14.dp, bottom = 2.dp),
    )
}

@Composable
fun InfoRow(label: String, value: String) {
    val th = LocalAppTheme.current
    Row(Modifier.fillMaxWidth().padding(vertical = 5.dp), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = th.dim, fontSize = 14.sp)
        Spacer(Modifier.width(12.dp))
        Text(value, fontSize = 14.sp, textAlign = TextAlign.End)
    }
}

/** Emoji in a soft rounded square, used as row icons. */
@Composable
fun EmojiBadge(emoji: String, size: Dp = 40.dp, tint: Color? = null) {
    val th = LocalAppTheme.current
    Box(
        Modifier.size(size).clip(RoundedCornerShape(size / 3)).background(tint ?: th.card2),
        contentAlignment = Alignment.Center,
    ) { Text(emoji, fontSize = (size.value * 0.48f).sp) }
}

/** The main call-to-action button: accent gradient, rounded. */
@Composable
fun PrimaryButton(text: String, modifier: Modifier = Modifier, enabled: Boolean = true, onClick: () -> Unit) {
    val th = LocalAppTheme.current
    val shape = RoundedCornerShape(th.radius.coerceAtMost(18.dp))
    Box(
        modifier
            .fillMaxWidth()
            .height(52.dp)
            .clip(shape)
            .background(
                if (enabled) Brush.horizontalGradient(listOf(th.accent, th.accent2))
                else Brush.horizontalGradient(listOf(th.card2, th.card2))
            )
            .clickable(enabled = enabled) { onClick() },
        contentAlignment = Alignment.Center,
    ) {
        Text(text, color = if (enabled) th.onAccent else th.dim, fontWeight = FontWeight.Bold, fontSize = 16.sp)
    }
}

/** A quieter button. */
@Composable
fun SoftButton(text: String, modifier: Modifier = Modifier, danger: Boolean = false, enabled: Boolean = true, onClick: () -> Unit) {
    val th = LocalAppTheme.current
    val shape = RoundedCornerShape(th.radius.coerceAtMost(16.dp))
    Box(
        modifier
            .height(46.dp)
            .clip(shape)
            .background(if (danger) th.bad.copy(alpha = 0.14f) else th.card2)
            .clickable(enabled = enabled) { onClick() }
            .padding(horizontal = 16.dp),
        contentAlignment = Alignment.Center,
    ) {
        Text(text, color = if (danger) th.bad else if (enabled) th.text else th.dim, fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
    }
}

@Composable
fun IrisSwitch(checked: Boolean, onChange: (Boolean) -> Unit) {
    val th = LocalAppTheme.current
    Switch(
        checked = checked,
        onCheckedChange = onChange,
        colors = SwitchDefaults.colors(
            checkedThumbColor = th.onAccent,
            checkedTrackColor = th.accent,
            uncheckedThumbColor = th.dim,
            uncheckedTrackColor = th.card2,
            uncheckedBorderColor = th.dim.copy(alpha = 0.4f),
        ),
    )
}

/** Title + subtitle on the left, a switch on the right. */
@Composable
fun ToggleRow(title: String, subtitle: String? = null, emoji: String? = null, checked: Boolean, onChange: (Boolean) -> Unit) {
    val th = LocalAppTheme.current
    Row(Modifier.fillMaxWidth().padding(vertical = 6.dp), verticalAlignment = Alignment.CenterVertically) {
        if (emoji != null) {
            EmojiBadge(emoji)
            Spacer(Modifier.width(12.dp))
        }
        Column(Modifier.weight(1f)) {
            Text(title, fontWeight = FontWeight.SemiBold)
            if (subtitle != null) Text(subtitle, color = th.dim, fontSize = 12.sp)
        }
        IrisSwitch(checked, onChange)
    }
}

/** Title on the left, a dropdown on the right. options = (value, label). */
@Composable
fun <T> SelectRow(title: String, subtitle: String? = null, options: List<Pair<T, String>>, selected: T, onSelect: (T) -> Unit) {
    val th = LocalAppTheme.current
    var open by remember { mutableStateOf(false) }
    Row(Modifier.fillMaxWidth().padding(vertical = 6.dp), verticalAlignment = Alignment.CenterVertically) {
        Column(Modifier.weight(1f)) {
            Text(title, fontWeight = FontWeight.SemiBold)
            if (subtitle != null) Text(subtitle, color = th.dim, fontSize = 12.sp)
        }
        Box {
            Row(
                Modifier
                    .clip(RoundedCornerShape(th.radius.coerceAtMost(14.dp)))
                    .background(th.card2)
                    .clickable { open = true }
                    .padding(horizontal = 14.dp, vertical = 10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(options.firstOrNull { it.first == selected }?.second ?: "-", fontSize = 14.sp)
                Spacer(Modifier.width(6.dp))
                Text("▾", color = th.dim, fontSize = 14.sp)
            }
            DropdownMenu(expanded = open, onDismissRequest = { open = false }) {
                options.forEach { (value, label) ->
                    DropdownMenuItem(
                        text = { Text(label, color = if (value == selected) th.accent else th.text) },
                        onClick = {
                            open = false
                            onSelect(value)
                        },
                    )
                }
            }
        }
    }
}

/** Slider that reports the final value when the finger lifts. */
@Composable
fun SliderRow(title: String, value: Int, range: IntRange, label: (Int) -> String, onDone: (Int) -> Unit) {
    val th = LocalAppTheme.current
    var v by remember { mutableFloatStateOf(value.toFloat()) }
    LaunchedEffect(value) { v = value.toFloat() }
    Column(Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
        Row(Modifier.fillMaxWidth()) {
            Text(title, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
            Text(label(v.toInt()), color = th.dim, fontSize = 13.sp)
        }
        Slider(
            value = v,
            onValueChange = { v = it },
            onValueChangeFinished = { onDone(v.toInt()) },
            valueRange = range.first.toFloat()..range.last.toFloat(),
            colors = SliderDefaults.colors(
                thumbColor = th.accent,
                activeTrackColor = th.accent,
                inactiveTrackColor = th.card2,
            ),
        )
    }
}

/** Little rounded label. */
@Composable
fun Chip(text: String, selected: Boolean = false, onClick: (() -> Unit)? = null) {
    val th = LocalAppTheme.current
    var m = Modifier
        .clip(RoundedCornerShape(50))
        .background(if (selected) th.accent.copy(alpha = 0.18f) else th.card2)
    if (selected) m = m.border(1.dp, th.accent, RoundedCornerShape(50))
    if (onClick != null) m = m.clickable { onClick() }
    Box(m.padding(horizontal = 12.dp, vertical = 7.dp)) {
        Text(text, fontSize = 13.sp, color = if (selected) th.accent else th.text)
    }
}

fun signalBars(rssi: Int): String = when {
    rssi >= -55 -> "▂▄▆█"
    rssi >= -67 -> "▂▄▆ "
    rssi >= -78 -> "▂▄  "
    else -> "▂   "
}

fun weatherEmoji(code: Int, day: Boolean): String = when {
    code < 0 -> "🌡️"
    code <= 1 -> if (day) "☀️" else "🌙"
    code == 2 -> if (day) "⛅" else "☁️"
    code == 3 -> "☁️"
    code == 45 || code == 48 -> "🌫️"
    code >= 95 -> "⛈️"
    code in 71..77 || code == 85 || code == 86 -> "❄️"
    code >= 51 -> "🌧️"
    else -> "🌡️"
}

fun batteryEmoji(state: String, pct: Int): String = when (state) {
    "charging" -> "⚡"
    "full" -> "🔋"
    "none" -> "🔌"
    else -> if (pct <= 15) "🪫" else "🔋"
}

fun batteryText(state: String, pct: Int): String = when (state) {
    "none" -> "USB power"
    "charging" -> "$pct% · Charging"
    "full" -> "Full"
    else -> "$pct%"
}

/** Emoji for each eye theme id (matches the firmware's theme numbers). */
fun eyeThemeEmoji(id: Int): String = when (id) {
    0 -> "🙂"
    1 -> "😜"
    2 -> "✨"
    3 -> "🤖"
    else -> "💬"
}
