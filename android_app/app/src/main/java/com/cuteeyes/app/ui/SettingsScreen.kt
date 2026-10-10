package com.cuteeyes.app.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.PowerManager
import android.provider.Settings
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.cuteeyes.app.IconSwitcher
import com.cuteeyes.app.Iris
import com.cuteeyes.app.IrisNotificationListener
import kotlinx.coroutines.delay

private fun otaText(state: String?, latest: String): String = when (state) {
    null -> "-"
    "checking" -> "Checking..."
    "up_to_date" -> "Up to date"
    "downloading" -> "Updating to v$latest..."
    "success" -> "Updated! Restarting..."
    "failed" -> "Couldn't check"
    else -> "Not checked yet"
}

private fun ignoringBatteryOptimizations(ctx: Context): Boolean =
    ctx.getSystemService(PowerManager::class.java)?.isIgnoringBatteryOptimizations(ctx.packageName) == true

@Composable
fun SettingsScreen() {
    val th = LocalAppTheme.current
    val ctx = LocalContext.current
    val set by Iris.settings.collectAsState()
    val s by Iris.status.collectAsState()
    val ota by Iris.otaProgress.collectAsState()
    val appThemeId by Iris.appThemeId.collectAsState()
    var notifAccess by remember { mutableStateOf(IrisNotificationListener.isEnabled(ctx)) }
    var batteryFree by remember { mutableStateOf(ignoringBatteryOptimizations(ctx)) }
    var forward by remember { mutableStateOf(Iris.forwardNotifications) }
    var showText by remember { mutableStateOf(Iris.showMessageText) }
    var appsDialog by remember { mutableStateOf(false) }
    var restrictedHelp by remember { mutableStateOf(false) }
    var forgetDialog by remember { mutableStateOf(false) }
    var cityField by remember { mutableStateOf<String?>(null) }

    // re-check permissions when coming back from Android settings
    LaunchedEffect(Unit) {
        while (true) {
            notifAccess = IrisNotificationListener.isEnabled(ctx)
            batteryFree = ignoringBatteryOptimizations(ctx)
            delay(1500)
        }
    }

    val cur = set
    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        // ------------------------------------------------ app theme
        item {
            Text("Settings", style = androidx.compose.material3.MaterialTheme.typography.headlineSmall)
            SectionTitle("App theme")
        }
        item {
            LazyRow(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                items(AppThemes.all, key = { it.id }) { t ->
                    ThemeCard(t, selected = t.id == appThemeId) {
                        if (t.id != appThemeId) {
                            Iris.setAppTheme(t.id)          // only one theme at a time
                            IconSwitcher.apply(ctx, t.id)    // home-screen icon colour follows
                        }
                    }
                }
            }
            Text("The home-screen icon changes colour too (it may take a few seconds).", color = th.dim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp, start = 4.dp))
        }

        // ------------------------------------------------ notifications
        item { SectionTitle("Notifications on IRIS") }
        item {
            IrisCard {
                if (!notifAccess) {
                    Text("🔔  Allow notification access", fontWeight = FontWeight.Bold)
                    Text("IRIS needs this to show who's messaging you. Turn on IRIS on the next screen.", color = th.dim, fontSize = 13.sp)
                    Spacer(Modifier.height(10.dp))
                    PrimaryButton("Open notification access") {
                        ctx.startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
                    }
                    Spacer(Modifier.height(6.dp))
                    Text(
                        "Greyed out or \"Restricted setting\"? Tap here",
                        color = th.accent,
                        fontSize = 13.sp,
                        modifier = Modifier.clickable { restrictedHelp = true }.padding(vertical = 6.dp),
                    )
                } else {
                    ToggleRow("Show notifications", "Phone notifications appear on IRIS", "🔔", forward && (cur?.notifyOn ?: true)) {
                        forward = it
                        Iris.forwardNotifications = it
                        Iris.set("notifyOn", it)
                    }
                    ToggleRow("Show message text", "Off = only the app and sender", "💬", showText) {
                        showText = it
                        Iris.showMessageText = it
                    }
                    SelectRow(
                        "Time on screen", null,
                        listOf(5 to "5 seconds", 8 to "8 seconds", 12 to "12 seconds", 20 to "20 seconds"),
                        cur?.notifySecs ?: 8,
                    ) { Iris.set("notifySecs", it) }
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp), modifier = Modifier.padding(top = 6.dp)) {
                        SoftButton("Choose apps", Modifier.weight(1f)) { appsDialog = true }
                        SoftButton("Send a test", Modifier.weight(1f)) { Iris.sendTestNotification() }
                    }
                }
            }
        }

        // ------------------------------------------------ display
        item { SectionTitle("Display") }
        item {
            IrisCard {
                SliderRow("Brightness", cur?.brightness ?: 200, 10..255, { "${(it / 2.55f).toInt()}%" }) { Iris.set("brightness", it) }
                ToggleRow("Show expression name", checked = cur?.labels ?: true) { Iris.set("labels", it) }
                SelectRow(
                    "Clock appears after", "when IRIS is left alone",
                    listOf(15 to "15 seconds", 30 to "30 seconds", 60 to "1 minute", 120 to "2 minutes", 300 to "5 minutes", 600 to "10 minutes", 0 to "Never"),
                    cur?.clockAfter ?: 60,
                ) { Iris.set("clockAfter", it) }
                SelectRow(
                    "Screen sleeps after", "shake to wake",
                    listOf(30 to "30 seconds", 60 to "1 minute", 120 to "2 minutes", 300 to "5 minutes", 600 to "10 minutes", 1800 to "30 minutes", 0 to "Never"),
                    cur?.screenTimeout ?: 300,
                ) { Iris.set("screenTimeout", it) }
            }
        }

        // ------------------------------------------------ shake
        item { SectionTitle("Shake") }
        item {
            IrisCard {
                ToggleRow("Shake to change", checked = cur?.shake ?: true) { Iris.set("shake", it) }
                SelectRow("When shaken", null, listOf(false to "Next expression", true to "Surprise me"), cur?.shakeRandom ?: false) {
                    Iris.set("shakeRandom", it)
                }
                SliderRow("Sensitivity", cur?.sens ?: 5, 1..10, { if (it <= 3) "Strong shake" else if (it >= 8) "Gentle shake" else "Normal" }) {
                    Iris.set("sens", it)
                }
            }
        }

        // ------------------------------------------------ location
        item { SectionTitle("Location") }
        item {
            val city = cityField ?: cur?.city.orEmpty()
            IrisCard {
                Text("Where is IRIS?", fontWeight = FontWeight.SemiBold)
                Text("Leave empty to detect it from the Wi-Fi. Used for weather, time and greeting.", color = th.dim, fontSize = 12.sp)
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(
                    value = city,
                    onValueChange = { cityField = it.take(40) },
                    singleLine = true,
                    placeholder = { Text("Automatic", color = th.dim) },
                    shape = RoundedCornerShape(th.radius.coerceAtMost(16.dp)),
                    colors = OutlinedTextFieldDefaults.colors(focusedBorderColor = th.accent, unfocusedBorderColor = th.dim.copy(alpha = 0.5f), cursorColor = th.accent),
                    modifier = Modifier.fillMaxWidth(),
                )
                Spacer(Modifier.height(8.dp))
                PrimaryButton("Save location") {
                    Iris.set("city", city.trim())
                    cityField = null
                    Iris.say("Location saved")
                }
            }
        }

        // ------------------------------------------------ background
        item { SectionTitle("Stay connected") }
        item {
            IrisCard {
                Text(if (batteryFree) "✅  Running freely in the background" else "🔋  Let IRIS run in the background", fontWeight = FontWeight.SemiBold)
                Text(
                    "Some phones close apps to save battery, which would stop notifications reaching IRIS.",
                    color = th.dim, fontSize = 12.sp,
                )
                if (!batteryFree) {
                    Spacer(Modifier.height(10.dp))
                    PrimaryButton("Allow background use") {
                        try {
                            ctx.startActivity(
                                Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:${ctx.packageName}"))
                            )
                        } catch (e: Exception) {
                            ctx.startActivity(Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS))
                        }
                    }
                }
            }
        }

        // ------------------------------------------------ software
        item { SectionTitle("Software") }
        item {
            IrisCard {
                InfoRow("IRIS firmware", s?.let { "v${it.firmware}" } ?: "-")
                InfoRow("Update", otaText(s?.ota, s?.latest.orEmpty()))
                ota?.let { pct ->
                    Spacer(Modifier.height(6.dp))
                    LinearProgressIndicator(progress = { pct / 100f }, color = th.accent, trackColor = th.card2, modifier = Modifier.fillMaxWidth())
                }
                Spacer(Modifier.height(10.dp))
                SoftButton("🔄  Check for updates", Modifier.fillMaxWidth(), enabled = s?.online == true) { Iris.checkForUpdate() }
                if (s?.online != true) Text("IRIS needs Wi-Fi to update.", color = th.dim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
            }
        }

        // ------------------------------------------------ about
        item { SectionTitle("About") }
        item {
            IrisCard {
                InfoRow("Device", s?.name ?: Iris.ble.deviceName)
                InfoRow("App", "IRIS")
                Spacer(Modifier.height(10.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    SoftButton("Disconnect", Modifier.weight(1f)) { Iris.disconnect() }
                    SoftButton("Forget IRIS", Modifier.weight(1f), danger = true) { forgetDialog = true }
                }
            }
        }
    }

    if (appsDialog) AppsDialog { appsDialog = false }
    if (restrictedHelp) {
        AlertDialog(
            onDismissRequest = { restrictedHelp = false },
            containerColor = th.card,
            title = { Text("Unlock notification access") },
            text = {
                Text(
                    "Android protects this setting for apps installed outside the Play Store.\n\n" +
                        "1. Tap \"Open app info\" below\n" +
                        "2. Tap ⋮ (top-right corner)\n" +
                        "3. Tap \"Allow restricted settings\" and confirm\n" +
                        "4. Come back and tap \"Open notification access\" again"
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    restrictedHelp = false
                    ctx.startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:${ctx.packageName}")))
                }) { Text("Open app info", color = th.accent, fontWeight = FontWeight.Bold) }
            },
            dismissButton = { TextButton(onClick = { restrictedHelp = false }) { Text("Close", color = th.dim) } },
        )
    }
    if (forgetDialog) {
        AlertDialog(
            onDismissRequest = { forgetDialog = false },
            containerColor = th.card,
            title = { Text("Forget this IRIS?") },
            text = { Text("The phone and IRIS forget each other. You'll need the pairing code to connect again.") },
            confirmButton = { TextButton(onClick = { forgetDialog = false; Iris.forgetDevice() }) { Text("Forget", color = th.bad) } },
            dismissButton = { TextButton(onClick = { forgetDialog = false }) { Text("Cancel", color = th.dim) } },
        )
    }
}

/** A small preview card drawn in the theme's own colours and fonts. */
@Composable
private fun ThemeCard(t: AppTheme, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(t.radius.coerceAtMost(22.dp))
    Column(
        Modifier
            .width(132.dp)
            .clip(shape)
            .background(t.bg)
            .border(if (selected) 2.dp else 1.dp, if (selected) t.accent else t.dim.copy(alpha = 0.3f), shape)
            .clickable { onClick() }
            .padding(12.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        EyesLogo(52.dp, t.accent)
        Spacer(Modifier.height(8.dp))
        Text(t.name, color = t.text, fontFamily = t.display, fontSize = 15.sp, maxLines = 1)
        Text(t.tagline, color = t.dim, fontFamily = t.body, fontSize = 10.sp, maxLines = 2, lineHeight = 13.sp)
        Spacer(Modifier.height(8.dp))
        Box(
            Modifier.fillMaxWidth().height(26.dp)
                .clip(RoundedCornerShape(t.radius.coerceAtMost(13.dp)))
                .background(Brush.horizontalGradient(listOf(t.accent, t.accent2))),
            contentAlignment = Alignment.Center,
        ) {
            Text(if (selected) "✓ In use" else "Use", color = t.onAccent, fontFamily = t.body, fontSize = 12.sp, fontWeight = FontWeight.Bold)
        }
    }
}

/** Choose which apps may show notifications on IRIS. */
@Composable
private fun AppsDialog(onClose: () -> Unit) {
    val th = LocalAppTheme.current
    val apps = remember { Iris.seenApps() }
    var tick by remember { mutableIntStateOf(0) }
    AlertDialog(
        onDismissRequest = onClose,
        containerColor = th.card,
        title = { Text("Apps on IRIS") },
        text = {
            if (apps.isEmpty()) {
                Text("Apps appear here after they send you a notification.", color = th.dim)
            } else {
                Column(Modifier.heightIn(max = 420.dp).verticalScroll(rememberScrollState())) {
                    apps.forEach { a ->
                        key(tick, a.pkg) {
                            ToggleRow(a.label, null, null, !Iris.isAppBlocked(a.pkg)) {
                                Iris.setAppBlocked(a.pkg, !it)
                                tick++
                            }
                        }
                    }
                }
            }
        },
        confirmButton = { TextButton(onClick = onClose) { Text("Done", color = th.accent, fontWeight = FontWeight.Bold) } },
    )
}
