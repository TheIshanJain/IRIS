package com.cuteeyes.app.ui

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
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.cuteeyes.app.DeviceStatus
import com.cuteeyes.app.DeviceViewModel

private fun wifiText(s: DeviceStatus): String = when (s.wifi) {
    "connected" -> "Connected to ${s.ssid}"
    "connecting" -> "Connecting to ${s.ssid}..."
    "no_networks" -> "No Wi-Fi saved yet"
    else -> "Not connected"
}

private fun otaText(s: DeviceStatus?): String = when (s?.ota) {
    null -> "-"
    "checking" -> "Checking..."
    "up_to_date" -> "Up to date"
    "downloading" -> "Updating to v${s.latest}..."
    "success" -> "Updated! Restarting..."
    "failed" -> "Couldn't check"
    else -> "Not checked yet"
}

// ================================================================ DEVICE TAB

@Composable
fun DeviceScreen(vm: DeviceViewModel) {
    val status by vm.status.collectAsState()
    val ota by vm.otaProgress.collectAsState()
    val s = status   // plain local copy (lets Kotlin smart-cast it)

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        item {
            SoftCard {
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                    Box(Modifier.fillMaxWidth()) {
                        IconButton(onClick = { vm.refreshStatus() }, modifier = Modifier.align(Alignment.TopEnd)) {
                            Icon(Icons.Filled.Refresh, contentDescription = "Refresh")
                        }
                        Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                            EyesLogo(96.dp)
                        }
                    }
                    Spacer(Modifier.height(10.dp))
                    Text(s?.name?.takeIf { it.isNotEmpty() } ?: vm.ble.deviceName, fontSize = 22.sp, fontWeight = FontWeight.ExtraBold)
                    Text("● Connected", color = Good, fontSize = 13.sp)
                }
            }
        }
        item { SectionTitle("Status") }
        item {
            SoftCard {
                InfoRow("Firmware", s?.let { "v${it.firmware}" } ?: "-")
                InfoRow("Wi-Fi", s?.let { wifiText(it) } ?: "-")
                if (s != null && s.online) {
                    InfoRow("IP address", s.ip)
                    InfoRow("Signal", "${signalBars(s.rssi)}  ${s.rssi} dBm")
                }
                InfoRow("Saved networks", s?.saved?.toString() ?: "-")
            }
        }
        item { SectionTitle("Software update") }
        item {
            SoftCard {
                InfoRow("Status", otaText(s))
                if (s != null && s.latest.isNotEmpty()) InfoRow("Latest online", "v${s.latest}")
                ota?.let { pct ->
                    Spacer(Modifier.height(8.dp))
                    LinearProgressIndicator(progress = { pct / 100f }, modifier = Modifier.fillMaxWidth())
                    Text("$pct%", color = Dim, fontSize = 12.sp, textAlign = TextAlign.End, modifier = Modifier.fillMaxWidth())
                }
                Spacer(Modifier.height(10.dp))
                FilledTonalButton(
                    onClick = { vm.checkForUpdate() },
                    enabled = s?.online == true,
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(14.dp),
                ) { Text("🔄  Check for update") }
                if (s?.online != true) {
                    Text("The device needs Wi-Fi for updates (Wi-Fi tab).", color = Dim, fontSize = 12.sp)
                }
            }
        }
        item {
            OutlinedButton(
                onClick = { vm.disconnect() },
                modifier = Modifier.fillMaxWidth().height(50.dp),
                shape = RoundedCornerShape(16.dp),
            ) { Text("Disconnect") }
        }
    }
}

// ================================================================ WI-FI TAB

/** ssid == null means "type the name yourself" (hidden network). */
private data class NetDialog(val ssid: String?, val secure: Boolean)

@Composable
fun WifiScreen(vm: DeviceViewModel) {
    val s by vm.status.collectAsState()
    val saved by vm.saved.collectAsState()
    val nearby by vm.nearby.collectAsState()
    val scanning by vm.wifiScanning.collectAsState()
    var dialog by remember { mutableStateOf<NetDialog?>(null) }
    var forget by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(Unit) { vm.refreshSaved() }

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        // ---- current connection
        item {
            SoftCard {
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                    val st = s
                    Text(if (st?.online == true) "📶" else if (st?.wifi == "connecting") "⏳" else "🔌", fontSize = 38.sp)
                    Text(st?.let { wifiText(it) } ?: "Loading...", fontWeight = FontWeight.ExtraBold, fontSize = 18.sp, textAlign = TextAlign.Center)
                    if (st?.online == true) Text("${signalBars(st.rssi)}  ${st.ip}", color = Dim, fontSize = 13.sp)
                }
            }
        }

        // ---- saved networks
        item {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.weight(1f)) { SectionTitle("Saved networks") }
                TextButton(onClick = { dialog = NetDialog(null, true) }) { Text("+ Hidden") }
            }
        }
        if (saved.isEmpty()) {
            item { Text("Nothing saved yet. Find your network below.", color = Dim, fontSize = 13.sp, modifier = Modifier.padding(start = 4.dp)) }
        }
        items(saved, key = { "saved_$it" }) { name ->
            val st = s
            val here = st != null && st.ssid == name
            val sub = when {
                here && st?.online == true -> "Connected"
                here && st?.wifi == "connecting" -> "Connecting..."
                else -> "Saved"
            }
            SavedRow(
                name = name,
                sub = sub,
                connected = here && st?.online == true,
                onConnect = { vm.connectNetwork(name) },
                onPassword = { dialog = NetDialog(name, true) },
                onForget = { forget = name },
            )
        }
        item { Text("Only 2.4 GHz networks work · up to 5 are remembered", color = Dim, fontSize = 12.sp, modifier = Modifier.padding(start = 4.dp)) }

        // ---- nearby
        item {
            Spacer(Modifier.height(4.dp))
            FilledTonalButton(
                onClick = { vm.scanWifi() },
                enabled = !scanning,
                modifier = Modifier.fillMaxWidth().height(50.dp),
                shape = RoundedCornerShape(16.dp),
            ) {
                if (scanning) {
                    CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp)
                    Spacer(Modifier.width(10.dp))
                    Text("The device is looking...")
                } else {
                    Text("🔍  Find networks", fontWeight = FontWeight.Bold)
                }
            }
        }
        if (nearby.isNotEmpty()) item { SectionTitle("Nearby") }
        items(nearby, key = { "near_${it.ssid}" }) { n ->
            SoftCard(Modifier.clickable { dialog = NetDialog(n.ssid, n.secure) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(signalBars(n.rssi), color = Good, fontSize = 13.sp)
                    Spacer(Modifier.width(12.dp))
                    Column(Modifier.weight(1f)) {
                        Text(n.ssid, fontWeight = FontWeight.Bold)
                        Text(
                            (if (saved.contains(n.ssid)) "Saved · " else "") + (if (n.secure) "🔒 Secured" else "Open"),
                            color = Dim,
                            fontSize = 12.sp,
                        )
                    }
                    Text("›", color = Dim, fontSize = 22.sp)
                }
            }
        }
    }

    // ---- dialogs
    dialog?.let { d ->
        NetworkDialog(
            d = d,
            onDismiss = { dialog = null },
            onSave = { ssid, pass ->
                vm.addNetwork(ssid, pass)
                dialog = null
            },
        )
    }
    forget?.let { name ->
        AlertDialog(
            onDismissRequest = { forget = null },
            title = { Text("Forget network?") },
            text = { Text("The device will forget \"$name\" and its password.") },
            confirmButton = {
                TextButton(onClick = { vm.removeNetwork(name); forget = null }) { Text("Forget", color = Bad) }
            },
            dismissButton = { TextButton(onClick = { forget = null }) { Text("Cancel") } },
        )
    }
}

@Composable
private fun SavedRow(
    name: String,
    sub: String,
    connected: Boolean,
    onConnect: () -> Unit,
    onPassword: () -> Unit,
    onForget: () -> Unit,
) {
    var menu by remember { mutableStateOf(false) }
    SoftCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            EmojiBadge(if (connected) "✅" else "📶")
            Spacer(Modifier.width(12.dp))
            Column(Modifier.weight(1f)) {
                Text(name, fontWeight = FontWeight.Bold)
                Text(sub, color = if (connected) Good else Dim, fontSize = 12.sp)
            }
            Box {
                IconButton(onClick = { menu = true }) { Icon(Icons.Filled.MoreVert, contentDescription = "More") }
                DropdownMenu(expanded = menu, onDismissRequest = { menu = false }) {
                    if (!connected) {
                        DropdownMenuItem(text = { Text("Connect now") }, onClick = { menu = false; onConnect() })
                    }
                    DropdownMenuItem(text = { Text("Change password") }, onClick = { menu = false; onPassword() })
                    HorizontalDivider()
                    DropdownMenuItem(text = { Text("Forget", color = Bad) }, onClick = { menu = false; onForget() })
                }
            }
        }
    }
}

@Composable
private fun NetworkDialog(d: NetDialog, onDismiss: () -> Unit, onSave: (String, String) -> Unit) {
    var ssid by remember { mutableStateOf(d.ssid ?: "") }
    var pass by remember { mutableStateOf("") }
    var show by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(d.ssid ?: "Hidden network") },
        text = {
            Column {
                if (d.ssid == null) {
                    OutlinedTextField(
                        value = ssid,
                        onValueChange = { ssid = it },
                        label = { Text("Network name") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Spacer(Modifier.height(8.dp))
                }
                if (d.secure) {
                    OutlinedTextField(
                        value = pass,
                        onValueChange = { pass = it },
                        label = { Text("Password") },
                        singleLine = true,
                        visualTransformation = if (show) VisualTransformation.None else PasswordVisualTransformation(),
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = show, onCheckedChange = { show = it })
                        Text("Show password", fontSize = 13.sp)
                    }
                } else {
                    Text("Open network - no password needed.", color = Dim)
                }
                error?.let { Text(it, color = Bad, fontSize = 13.sp) }
            }
        },
        confirmButton = {
            Button(onClick = {
                val p = if (d.secure) pass else ""
                when {
                    ssid.isBlank() || ssid.length > 32 -> error = "Network name must be 1-32 characters"
                    p.isNotEmpty() && (p.length < 8 || p.length > 63) -> error = "Wi-Fi passwords are 8-63 characters"
                    else -> onSave(ssid, p)
                }
            }) { Text("Connect") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Cancel") } },
    )
}
