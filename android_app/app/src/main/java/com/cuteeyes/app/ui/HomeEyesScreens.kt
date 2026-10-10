package com.cuteeyes.app.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.cuteeyes.app.Iris
import kotlinx.coroutines.delay

// ================================================================ HOME

@Composable
fun HomeScreen() {
    val th = LocalAppTheme.current
    val s by Iris.status.collectAsState()
    val set by Iris.settings.collectAsState()
    val info by Iris.info.collectAsState()
    val themes by Iris.themes.collectAsState()
    var showWeather by remember { mutableStateOf(false) }

    // keep time/weather fresh while Home is visible
    LaunchedEffect(Unit) {
        while (true) {
            Iris.refreshInfo()
            delay(30_000)
            Iris.refreshStatus()
        }
    }

    val st = s
    val inf = info
    val name = set?.name.orEmpty()
    val greet = inf?.greet ?: "Hello"

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        // ---- hero
        item {
            Box(
                Modifier
                    .fillMaxWidth()
                    .clip(RoundedCornerShape(th.radius + 6.dp))
                    .background(Brush.verticalGradient(listOf(th.accent.copy(alpha = 0.22f), th.card)))
                    .padding(22.dp),
            ) {
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                    EyesLogo(118.dp)
                    Spacer(Modifier.height(14.dp))
                    Text(
                        buildAnnotatedString {
                            append(greet)
                            if (name.isNotEmpty()) {
                                append(", ")
                                withStyle(SpanStyle(color = th.accent2)) { append(name) }
                            }
                            append("!")
                        },
                        style = MaterialTheme.typography.headlineMedium,
                        textAlign = TextAlign.Center,
                    )
                    val themeName = themes.firstOrNull { it.id == st?.expressionTheme }?.name
                    Text(
                        if (st != null) "Feeling ${st.expression}" + (themeName?.let { " · $it" } ?: "") else "Saying hello...",
                        color = th.dim,
                        fontSize = 14.sp,
                    )
                    Spacer(Modifier.height(16.dp))
                    PrimaryButton("✨  Next expression") { Iris.next() }
                }
            }
        }

        // ---- time + weather tiles
        item {
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                IrisCard(Modifier.weight(1f)) {
                    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(if (inf?.timeValid == true) inf.time else "--:--", style = MaterialTheme.typography.headlineLarge, color = th.text)
                        Text(if (inf?.timeValid == true) inf.date else "Needs Wi-Fi", color = th.dim, fontSize = 13.sp)
                    }
                }
                IrisCard(Modifier.weight(1f), onClick = { showWeather = !showWeather }) {
                    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                        if (inf?.haveWeather == true) {
                            Text("${weatherEmoji(inf.code, inf.isDay)}  ${Math.round(inf.temp)}°", style = MaterialTheme.typography.headlineMedium)
                            Text(inf.text, color = th.dim, fontSize = 13.sp, maxLines = 1)
                        } else {
                            Text("🌡️", fontSize = 30.sp)
                            Text(if (st?.online == true) "Loading..." else "Needs Wi-Fi", color = th.dim, fontSize = 13.sp)
                        }
                    }
                }
            }
        }
        if (showWeather && inf?.haveWeather == true) {
            item {
                IrisCard {
                    Text(if (inf.haveLoc) "📍 ${inf.location}" else "Weather", fontWeight = FontWeight.Bold)
                    Spacer(Modifier.height(4.dp))
                    InfoRow("Feels like", "${Math.round(inf.feels)}°C")
                    InfoRow("Today", "${Math.round(inf.tmin)}° / ${Math.round(inf.tmax)}°C")
                    InfoRow("Humidity", "${Math.round(inf.humidity)}%")
                    InfoRow("Wind", "${Math.round(inf.wind)} km/h")
                    InfoRow("Sunrise · sunset", "${inf.sunrise} · ${inf.sunset}")
                }
            }
        }

        // ---- device chips
        item {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                if (st != null) {
                    Chip("${batteryEmoji(st.batteryState, st.battery)} ${batteryText(st.batteryState, st.battery)}")
                    Chip(if (st.online) "📶 ${st.ssid}" else "📶 Offline")
                    Chip(
                        when (st.screen) {
                            "clock" -> "🕑 Clock"
                            "off" -> "😴 Asleep"
                            else -> "👀 Awake"
                        }
                    )
                }
            }
        }

        // ---- quick controls
        item {
            val cur = set
            IrisCard {
                ToggleRow("Auto-change", "New expression every few seconds", "🔄", cur?.auto ?: true) { Iris.set("auto", it) }
                ToggleRow("Shake to change", "Give IRIS a little shake", "👋", cur?.shake ?: true) { Iris.set("shake", it) }
                SliderRow("☀️  Brightness", cur?.brightness ?: 200, 10..255, { "${(it / 2.55f).toInt()}%" }) { Iris.set("brightness", it) }
            }
        }
    }
}

// ================================================================ EYES (eye theme + greeting)

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun EyesScreen() {
    val th = LocalAppTheme.current
    val set by Iris.settings.collectAsState()
    val themes by Iris.themes.collectAsState()
    val info by Iris.info.collectAsState()
    var nameField by remember { mutableStateOf<String?>(null) }
    var expanded by remember { mutableStateOf<Int?>(null) }

    LaunchedEffect(Unit) { if (themes.isEmpty()) Iris.refreshAll() }

    val selected = set?.eyeTheme ?: 1
    val name = nameField ?: set?.name.orEmpty()

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        item {
            Text("Eye style", style = MaterialTheme.typography.headlineSmall)
            Text("Pick one look for IRIS's eyes. Choosing another switches the old one off.", color = th.dim, fontSize = 14.sp)
        }
        items(themes, key = { it.id }) { t ->
            val on = t.id == selected
            IrisCard(highlighted = on, onClick = { if (!on) Iris.set("eyeTheme", t.id) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    EmojiBadge(eyeThemeEmoji(t.id), 48.dp, if (on) th.accent.copy(alpha = 0.22f) else null)
                    Spacer(Modifier.width(12.dp))
                    Column(Modifier.weight(1f)) {
                        Text(t.name, fontWeight = FontWeight.Bold, fontSize = 16.sp)
                        Text(t.description, color = th.dim, fontSize = 12.sp)
                    }
                    // radio-style indicator: only one can be on
                    Box(
                        Modifier.size(24.dp).clip(CircleShape)
                            .background(if (on) th.accent else th.card2),
                        contentAlignment = Alignment.Center,
                    ) { if (on) Text("✓", color = th.onAccent, fontSize = 14.sp, fontWeight = FontWeight.Bold) }
                }
                Spacer(Modifier.height(8.dp))
                Text(
                    if (expanded == t.id) "Hide expressions ▴" else "${t.expressions.size} expressions ▾",
                    color = th.accent,
                    fontSize = 13.sp,
                    modifier = Modifier.clip(RoundedCornerShape(8.dp))
                        .clickable { expanded = if (expanded == t.id) null else t.id }
                        .padding(vertical = 4.dp),
                )
                if (expanded == t.id) {
                    Spacer(Modifier.height(8.dp))
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        t.expressions.forEach { Chip(it) }
                    }
                }
            }
        }
        if (themes.isEmpty()) {
            item { Text("Loading eye styles...", color = th.dim) }
        }

        // ---- greeting by name
        item { SectionTitle("Greeting") }
        item {
            IrisCard {
                Box(
                    Modifier.fillMaxWidth()
                        .clip(RoundedCornerShape(th.radius, th.radius, th.radius, 6.dp))
                        .background(Brush.linearGradient(listOf(th.accent.copy(alpha = 0.18f), th.accent2.copy(alpha = 0.18f))))
                        .padding(16.dp),
                ) {
                    Text(
                        buildAnnotatedString {
                            append(info?.greet ?: "Hello")
                            if (name.isNotEmpty()) {
                                append(", ")
                                withStyle(SpanStyle(color = th.accent2)) { append(name) }
                            }
                            append("! 👋")
                        },
                        style = MaterialTheme.typography.titleLarge,
                    )
                }
                Spacer(Modifier.height(12.dp))
                Text("Your name", fontWeight = FontWeight.SemiBold)
                Text("IRIS greets you by name every time it wakes up. English letters only.", color = th.dim, fontSize = 12.sp)
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(
                    value = name,
                    onValueChange = { nameField = it.take(16) },
                    singleLine = true,
                    placeholder = { Text("Your name", color = th.dim) },
                    shape = RoundedCornerShape(th.radius.coerceAtMost(16.dp)),
                    colors = OutlinedTextFieldDefaults.colors(focusedBorderColor = th.accent, unfocusedBorderColor = th.dim.copy(alpha = 0.5f), cursorColor = th.accent),
                    modifier = Modifier.fillMaxWidth(),
                )
                Spacer(Modifier.height(10.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    PrimaryButton("Save", Modifier.weight(1f)) {
                        Iris.set("name", name.trim())
                        nameField = null
                        Iris.say("Saved")
                    }
                    SoftButton("👋 Say hello", Modifier.weight(1f)) { Iris.greet() }
                }
                info?.let {
                    Spacer(Modifier.height(8.dp))
                    Text(
                        if (it.haveLoc) "Greeting picked for ${it.location}" else "Location unknown yet - IRIS says Hello",
                        color = th.dim,
                        fontSize = 12.sp,
                    )
                }
            }
        }
    }
}
