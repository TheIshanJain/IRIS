package com.cuteeyes.app.ui

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.LinearProgressIndicator
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.cuteeyes.app.Iris
import kotlinx.coroutines.delay
import org.json.JSONObject

private enum class DevPanel(val label: String) {
    Info("📋  Device details"),
    Expressions("👀  Expressions"),
    Sensors("📳  Sensors & battery"),
    Firmware("📦  Firmware"),
    Logs("📜  Logs"),
}

@Composable
fun DevScreen() {
    val unlocked by Iris.devUnlocked.collectAsState()
    if (!unlocked) DevLogin() else DevArea()
}

@Composable
private fun DevLogin() {
    val th = LocalAppTheme.current
    var user by remember { mutableStateOf("") }
    var pass by remember { mutableStateOf("") }
    val colors = OutlinedTextFieldDefaults.colors(focusedBorderColor = th.accent, unfocusedBorderColor = th.dim.copy(alpha = 0.5f), cursorColor = th.accent)
    Column(
        Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text("🔒", fontSize = 46.sp)
        Text("Developer area", style = MaterialTheme.typography.headlineSmall)
        Text("Device details, logs, every expression and firmware tools.", color = th.dim, fontSize = 14.sp, textAlign = TextAlign.Center)
        Spacer(Modifier.height(18.dp))
        OutlinedTextField(user, { user = it }, placeholder = { Text("Username", color = th.dim) }, singleLine = true, colors = colors, modifier = Modifier.fillMaxWidth())
        Spacer(Modifier.height(8.dp))
        OutlinedTextField(
            pass, { pass = it }, placeholder = { Text("Password", color = th.dim) }, singleLine = true,
            visualTransformation = PasswordVisualTransformation(), colors = colors, modifier = Modifier.fillMaxWidth(),
        )
        Spacer(Modifier.height(14.dp))
        PrimaryButton("Unlock", enabled = user.isNotBlank() && pass.isNotBlank()) {
            Iris.devLogin(user.trim(), pass)
            pass = ""
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun DevArea() {
    val th = LocalAppTheme.current
    val ctx = LocalContext.current
    var panel by remember { mutableStateOf(DevPanel.Info) }
    val d by Iris.devStatus.collectAsState()
    val cat by Iris.expressions.collectAsState()
    val logs by Iris.logs.collectAsState()
    val fw by Iris.fwProgress.collectAsState()

    // live refresh for the selected panel
    LaunchedEffect(panel) {
        while (true) {
            if (panel == DevPanel.Logs) Iris.devPollLog() else Iris.devRefresh()
            delay(if (panel == DevPanel.Logs) 1000 else 2000)
        }
    }

    val pickFirmware = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri: Uri? ->
        if (uri != null) {
            val bytes = ctx.contentResolver.openInputStream(uri)?.use { it.readBytes() }
            if (bytes == null) Iris.say("Couldn't read that file") else Iris.uploadFirmware(bytes)
        }
    }

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item {
            IrisCard(padding = 10.dp) {
                SelectRow("Developer", null, DevPanel.entries.map { it to it.label }, panel) { panel = it }
                Spacer(Modifier.height(4.dp))
                SoftButton("🔒  Lock developer area", Modifier.fillMaxWidth(), danger = true) { Iris.devLogout() }
            }
        }

        val o = d
        when (panel) {
            DevPanel.Info -> {
                if (o == null) item { Text("Loading...", color = th.dim) }
                else {
                    item {
                        IrisCard {
                            Text("Device", fontWeight = FontWeight.Bold)
                            InfoRow("Name", o.optString("name"))
                            InfoRow("Firmware", "v" + o.optString("fw"))
                            InfoRow("Uptime", uptime(o.optLong("uptime")))
                            InfoRow("Free heap", "${o.optLong("heap") / 1024} KB (min ${o.optLong("minHeap") / 1024} KB)")
                            InfoRow("Free PSRAM", "${o.optLong("psram") / 1024} KB")
                            InfoRow("Chip temp", String.format("%.1f °C", o.optDouble("temp")))
                            InfoRow("Last reset", o.optString("reset"))
                            InfoRow("Core / IDF", o.optString("sdk"))
                            InfoRow("Trusted phones", o.optInt("trusted").toString())
                        }
                    }
                    item {
                        IrisCard {
                            Text("Wi-Fi", fontWeight = FontWeight.Bold)
                            InfoRow("State", o.optString("wifi"))
                            InfoRow("Network", o.optString("ssid").ifEmpty { "-" })
                            InfoRow("IP", o.optString("ip").ifEmpty { "-" })
                            InfoRow("Signal", if (o.optString("ip").isNotEmpty()) "${o.optInt("rssi")} dBm" else "-")
                            InfoRow("MAC", o.optString("mac"))
                        }
                    }
                    item {
                        IrisCard {
                            Text("Time, place, weather", fontWeight = FontWeight.Bold)
                            InfoRow("Time zone", o.optString("tz"))
                            InfoRow("Location", o.optString("loc").ifEmpty { "unknown" })
                            InfoRow("Weather age", o.optInt("wxAge").let { if (it < 0) "-" else "$it s" })
                            if (o.optString("wxErr").isNotEmpty()) InfoRow("Last error", o.optString("wxErr"))
                            InfoRow("Cloud update", o.optString("otaState") + " " + o.optString("otaMsg"))
                        }
                    }
                    item {
                        IrisCard {
                            Text("Display", fontWeight = FontWeight.Bold)
                            InfoRow("Screen", o.optString("screen"))
                            InfoRow("Expression", "${o.optString("exprName")} (#${o.optInt("expr") + 1})")
                            InfoRow("Notifications shown", o.optInt("notifs").toString())
                        }
                    }
                }
            }

            DevPanel.Expressions -> {
                val c = cat
                if (c == null) item { Text("Loading...", color = th.dim) }
                else {
                    item {
                        Text("Show any expression from any theme - including the status ones.", color = th.dim, fontSize = 13.sp)
                    }
                    c.themeNames.forEachIndexed { ti, tn ->
                        item(key = "th$ti") {
                            IrisCard {
                                Text("${eyeThemeEmoji(ti)}  $tn", fontWeight = FontWeight.Bold)
                                Spacer(Modifier.height(8.dp))
                                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                                    c.names.forEachIndexed { i, n ->
                                        if (c.themes.getOrNull(i) == ti) {
                                            Chip(n, selected = o?.optInt("expr", -1) == i) { Iris.devShowExpression(i) }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            DevPanel.Sensors -> {
                if (o == null) item { Text("Loading...", color = th.dim) }
                else {
                    item {
                        IrisCard {
                            Text("Accelerometer (QMI8658)", fontWeight = FontWeight.Bold)
                            if (!o.optBoolean("imuOk")) Text("Not detected (SDA 6 / SCL 7)", color = th.bad)
                            else {
                                Axis("X", o.optDouble("ax"))
                                Axis("Y", o.optDouble("ay"))
                                Axis("Z", o.optDouble("az"))
                                InfoRow("|a|", String.format("%.2f g (about 1.00 when still)", o.optDouble("g")))
                                InfoRow("Shakes detected", o.optInt("shakes").toString())
                            }
                        }
                    }
                    item {
                        IrisCard {
                            Text("Battery (GPIO1)", fontWeight = FontWeight.Bold)
                            InfoRow("Voltage", String.format("%.2f V", o.optDouble("batV")))
                            InfoRow("Level", "${o.optInt("bat")}%")
                            InfoRow("State (estimated)", o.optString("batState"))
                        }
                    }
                }
            }

            DevPanel.Firmware -> {
                item {
                    IrisCard {
                        Text("Upload firmware over Bluetooth", fontWeight = FontWeight.Bold)
                        Text(
                            "Arduino IDE: Sketch → Export Compiled Binary, copy the ESP32S3_Cute_Eyes.ino.bin file to your phone, then pick it here. " +
                                "Use the plain .ino.bin (not merged / bootloader / partitions). Takes 1-3 minutes - keep the phone close.",
                            color = th.dim, fontSize = 13.sp,
                        )
                        Spacer(Modifier.height(10.dp))
                        val p = fw
                        if (p != null) {
                            LinearProgressIndicator(progress = { p }, color = th.accent, trackColor = th.card2, modifier = Modifier.fillMaxWidth())
                            Text("${(p * 100).toInt()}%  - don't close the app", color = th.dim, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
                        } else {
                            PrimaryButton("📦  Choose .bin file") { pickFirmware.launch(arrayOf("application/octet-stream", "*/*")) }
                        }
                    }
                }
                item {
                    IrisCard {
                        Text("Device", fontWeight = FontWeight.Bold)
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                            SoftButton("🔄  Check cloud update", Modifier.weight(1f)) { Iris.checkForUpdate() }
                            SoftButton("Reboot", Modifier.weight(1f), danger = true) { Iris.devReboot() }
                        }
                    }
                }
            }

            DevPanel.Logs -> {
                item {
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        SoftButton("Clear", Modifier.weight(1f)) { Iris.devClearLog() }
                    }
                }
                item {
                    Column(
                        Modifier.fillMaxWidth()
                            .clip(RoundedCornerShape(th.radius.coerceAtMost(14.dp)))
                            .background(androidx.compose.ui.graphics.Color.Black)
                            .padding(10.dp)
                            .horizontalScroll(rememberScrollState()),
                    ) {
                        if (logs.isEmpty()) Text("Waiting for log lines...", color = th.dim, fontSize = 12.sp, fontFamily = FontFamily.Monospace)
                        logs.takeLast(200).forEach {
                            Text(it, color = androidx.compose.ui.graphics.Color(0xFFB8F5C8), fontSize = 11.sp, fontFamily = FontFamily.Monospace, softWrap = false)
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun Axis(name: String, v: Double) {
    val th = LocalAppTheme.current
    val c = v.coerceIn(-2.0, 2.0)
    Row(Modifier.fillMaxWidth().padding(vertical = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(name, fontWeight = FontWeight.Bold, modifier = Modifier.padding(end = 10.dp))
        LinearProgressIndicator(
            progress = { ((c + 2.0) / 4.0).toFloat() },
            color = th.accent, trackColor = th.card2,
            modifier = Modifier.weight(1f),
        )
        Text(String.format("%6.2f g", v), color = th.dim, fontSize = 12.sp, modifier = Modifier.padding(start = 10.dp))
    }
}

private fun uptime(s: Long): String {
    val d = s / 86400
    val h = s % 86400 / 3600
    val m = s % 3600 / 60
    return (if (d > 0) "${d}d " else "") + (if (h > 0) "${h}h " else "") + "${m}m ${s % 60}s"
}

@Suppress("unused")
private fun JSONObject.str(k: String) = optString(k)
