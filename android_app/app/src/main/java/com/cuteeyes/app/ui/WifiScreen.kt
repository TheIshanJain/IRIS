package com.cuteeyes.app.ui

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
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CheckboxDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
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
import com.cuteeyes.app.Iris

private fun wifiText(s: DeviceStatus): String = when (s.wifi) {
    "connected" -> "Connected to ${s.ssid}"
    "connecting" -> "Connecting to ${s.ssid}..."
    "no_networks" -> "No Wi-Fi saved yet"
    else -> "Not connected"
}

/** ssid == null means "type the name yourself" (hidden network). */
private data class NetDialog(val ssid: String?, val secure: Boolean)

@Composable
fun WifiScreen() {
    val th = LocalAppTheme.current
    val s by Iris.status.collectAsState()
    val saved by Iris.saved.collectAsState()
    val nearby by Iris.nearby.collectAsState()
    val scanning by Iris.wifiScanning.collectAsState()
    var dialog by remember { mutableStateOf<NetDialog?>(null) }
    var forget by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(Unit) { Iris.refreshSaved() }

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        item {
            val st = s
            IrisCard {
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(if (st?.online == true) "📶" else if (st?.wifi == "connecting") "⏳" else "🔌", fontSize = 40.sp)
                    Text(st?.let { wifiText(it) } ?: "Loading...", style = MaterialTheme.typography.titleLarge, textAlign = TextAlign.Center)
                    if (st?.online == true) Text("${signalBars(st.rssi)}  ${st.ip}", color = th.dim, fontSize = 13.sp)
                }
            }
        }

        item {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.weight(1f)) { SectionTitle("Saved networks") }
                TextButton(onClick = { dialog = NetDialog(null, true) }) { Text("+ Hidden", color = th.accent) }
            }
        }
        if (saved.isEmpty()) {
            item { Text("Nothing saved yet. Find your network below.", color = th.dim, fontSize = 13.sp, modifier = Modifier.padding(start = 4.dp)) }
        }
        items(saved, key = { "saved_$it" }) { name ->
            val st = s
            val here = st != null && st.ssid == name
            val connected = here && st?.online == true
            val sub = when {
                connected -> "Connected"
                here && st?.wifi == "connecting" -> "Connecting..."
                else -> "Saved"
            }
            SavedRow(name, sub, connected,
                onConnect = { Iris.connectNetwork(name) },
                onPassword = { dialog = NetDialog(name, true) },
                onForget = { forget = name })
        }
        item { Text("Only 2.4 GHz networks work · up to 5 are remembered", color = th.dim, fontSize = 12.sp, modifier = Modifier.padding(start = 4.dp)) }

        item {
            Spacer(Modifier.height(4.dp))
            if (scanning) {
                IrisCard {
                    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.Center) {
                        CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp, color = th.accent)
                        Spacer(Modifier.width(10.dp))
                        Text("IRIS is looking...")
                    }
                }
            } else {
                PrimaryButton("🔍  Find networks") { Iris.scanWifi() }
            }
        }
        if (nearby.isNotEmpty()) item { SectionTitle("Nearby") }
        items(nearby, key = { "near_${it.ssid}" }) { n ->
            IrisCard(onClick = { dialog = NetDialog(n.ssid, n.secure) }) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(signalBars(n.rssi), color = th.good, fontSize = 13.sp)
                    Spacer(Modifier.width(12.dp))
                    Column(Modifier.weight(1f)) {
                        Text(n.ssid, fontWeight = FontWeight.Bold)
                        Text((if (saved.contains(n.ssid)) "Saved · " else "") + (if (n.secure) "🔒 Secured" else "Open"), color = th.dim, fontSize = 12.sp)
                    }
                    Text("›", color = th.dim, fontSize = 22.sp)
                }
            }
        }
    }

    dialog?.let { d ->
        NetworkDialog(d, onDismiss = { dialog = null }) { ssid, pass ->
            Iris.addNetwork(ssid, pass)
            dialog = null
        }
    }
    forget?.let { name ->
        AlertDialog(
            onDismissRequest = { forget = null },
            containerColor = th.card,
            title = { Text("Forget network?") },
            text = { Text("IRIS will forget \"$name\" and its password.") },
            confirmButton = { TextButton(onClick = { Iris.removeNetwork(name); forget = null }) { Text("Forget", color = th.bad) } },
            dismissButton = { TextButton(onClick = { forget = null }) { Text("Cancel", color = th.dim) } },
        )
    }
}

@Composable
private fun SavedRow(name: String, sub: String, connected: Boolean, onConnect: () -> Unit, onPassword: () -> Unit, onForget: () -> Unit) {
    val th = LocalAppTheme.current
    var menu by remember { mutableStateOf(false) }
    IrisCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            EmojiBadge(if (connected) "✅" else "📶")
            Spacer(Modifier.width(12.dp))
            Column(Modifier.weight(1f)) {
                Text(name, fontWeight = FontWeight.Bold)
                Text(sub, color = if (connected) th.good else th.dim, fontSize = 12.sp)
            }
            Box {
                IconButton(onClick = { menu = true }) { Icon(Icons.Filled.MoreVert, contentDescription = "More", tint = th.dim) }
                DropdownMenu(expanded = menu, onDismissRequest = { menu = false }) {
                    if (!connected) DropdownMenuItem(text = { Text("Connect now") }, onClick = { menu = false; onConnect() })
                    DropdownMenuItem(text = { Text("Change password") }, onClick = { menu = false; onPassword() })
                    HorizontalDivider()
                    DropdownMenuItem(text = { Text("Forget", color = th.bad) }, onClick = { menu = false; onForget() })
                }
            }
        }
    }
}

@Composable
private fun NetworkDialog(d: NetDialog, onDismiss: () -> Unit, onSave: (String, String) -> Unit) {
    val th = LocalAppTheme.current
    var ssid by remember { mutableStateOf(d.ssid ?: "") }
    var pass by remember { mutableStateOf("") }
    var show by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }
    val fieldColors = OutlinedTextFieldDefaults.colors(focusedBorderColor = th.accent, unfocusedBorderColor = th.dim.copy(alpha = 0.5f), cursorColor = th.accent)

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = th.card,
        title = { Text(d.ssid ?: "Hidden network") },
        text = {
            Column {
                if (d.ssid == null) {
                    OutlinedTextField(ssid, { ssid = it }, label = { Text("Network name") }, singleLine = true, colors = fieldColors, modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(8.dp))
                }
                if (d.secure) {
                    OutlinedTextField(
                        pass, { pass = it },
                        label = { Text("Password") },
                        singleLine = true,
                        visualTransformation = if (show) VisualTransformation.None else PasswordVisualTransformation(),
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                        colors = fieldColors,
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(show, { show = it }, colors = CheckboxDefaults.colors(checkedColor = th.accent))
                        Text("Show password", fontSize = 13.sp)
                    }
                } else {
                    Text("Open network - no password needed.", color = th.dim)
                }
                error?.let { Text(it, color = th.bad, fontSize = 13.sp) }
            }
        },
        confirmButton = {
            TextButton(onClick = {
                val p = if (d.secure) pass else ""
                when {
                    ssid.isBlank() || ssid.length > 32 -> error = "Network name must be 1-32 characters"
                    p.isNotEmpty() && (p.length < 8 || p.length > 63) -> error = "Wi-Fi passwords are 8-63 characters"
                    else -> onSave(ssid, p)
                }
            }) { Text("Connect", color = th.accent, fontWeight = FontWeight.Bold) }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Cancel", color = th.dim) } },
    )
}
