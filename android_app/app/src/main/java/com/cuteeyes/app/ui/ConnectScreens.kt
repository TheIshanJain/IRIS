package com.cuteeyes.app.ui

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.material3.CircularProgressIndicator
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.cuteeyes.app.Iris
import kotlinx.coroutines.delay

// ------------------------------------------------------------ permissions

fun blePermissions(): Array<String> =
    if (Build.VERSION.SDK_INT >= 31) {
        arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
    } else {
        arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
    }

fun hasBlePermissions(ctx: Context): Boolean =
    blePermissions().all { ContextCompat.checkSelfPermission(ctx, it) == PackageManager.PERMISSION_GRANTED }

// ------------------------------------------------------------ find + connect

@Composable
fun ConnectScreen() {
    val th = LocalAppTheme.current
    val ctx = LocalContext.current
    var granted by remember { mutableStateOf(hasBlePermissions(ctx)) }
    var btOn by remember { mutableStateOf(Iris.ble.isBluetoothOn) }
    val scanning by Iris.ble.scanning.collectAsState()
    val found by Iris.ble.found.collectAsState()
    val last = Iris.lastDevice

    val askPermissions = rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { result ->
        granted = result.values.all { it }
        if (!granted) Iris.say("Bluetooth permission is needed to find IRIS")
    }
    val askBluetoothOn = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) {
        btOn = Iris.ble.isBluetoothOn
    }

    LaunchedEffect(Unit) {
        while (true) {
            btOn = Iris.ble.isBluetoothOn
            granted = hasBlePermissions(ctx)
            delay(1000)
        }
    }

    LazyColumn(
        Modifier.fillMaxSize(),
        contentPadding = PaddingValues(20.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        item {
            Column(Modifier.fillMaxWidth().padding(top = 28.dp, bottom = 8.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                EyesLogo(120.dp)
                Spacer(Modifier.height(18.dp))
                Text("IRIS", style = androidx.compose.material3.MaterialTheme.typography.displaySmall, color = th.accent)
                Text("Let's find your little messenger", color = th.dim, fontSize = 15.sp)
            }
        }

        when {
            !granted -> item {
                ActionCard("🔐", "Allow Bluetooth", "Needed to find and talk to IRIS. Nothing else is accessed.", "Allow") {
                    askPermissions.launch(blePermissions())
                }
            }

            !btOn -> item {
                ActionCard("📴", "Bluetooth is off", "Turn it on to find IRIS.", "Turn on") {
                    askBluetoothOn.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
                }
            }

            else -> {
                if (last != null && found.none { it.address == last.first }) {
                    item {
                        DeviceRow("⭐", last.second, "Last used - tap to reconnect") { Iris.connect(last.first, last.second) }
                    }
                }
                item {
                    if (scanning) {
                        IrisCard(onClick = { Iris.ble.stopScan() }) {
                            Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.Center) {
                                CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp, color = th.accent)
                                Spacer(Modifier.width(10.dp))
                                Text("Looking... tap to stop")
                            }
                        }
                    } else {
                        PrimaryButton("🔍  Find my IRIS") { Iris.ble.startScan() }
                    }
                }
                items(found, key = { it.address }) { d ->
                    DeviceRow("👀", d.name, "Signal ${signalBars(d.rssi)}") { Iris.connect(d.address, d.name) }
                }
                if (found.isEmpty() && !scanning) {
                    item {
                        Text(
                            "Switch on IRIS and keep it close to your phone." +
                                if (Build.VERSION.SDK_INT < 31) "\nOn this Android version, Location must also be on." else "",
                            color = th.dim,
                            fontSize = 13.sp,
                            textAlign = TextAlign.Center,
                            modifier = Modifier.fillMaxWidth().padding(16.dp),
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun ActionCard(emoji: String, title: String, text: String, button: String, onClick: () -> Unit) {
    val th = LocalAppTheme.current
    IrisCard {
        Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
            Text(emoji, fontSize = 36.sp)
            Spacer(Modifier.height(6.dp))
            Text(title, fontWeight = FontWeight.Bold, fontSize = 18.sp)
            Text(text, color = th.dim, fontSize = 14.sp, textAlign = TextAlign.Center)
            Spacer(Modifier.height(14.dp))
            PrimaryButton(button, onClick = onClick)
        }
    }
}

@Composable
fun DeviceRow(emoji: String, name: String, sub: String, onConnect: () -> Unit) {
    val th = LocalAppTheme.current
    IrisCard(onClick = onConnect) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            EmojiBadge(emoji)
            Spacer(Modifier.width(12.dp))
            Column(Modifier.weight(1f)) {
                Text(name, fontWeight = FontWeight.Bold)
                Text(sub, color = th.dim, fontSize = 12.sp)
            }
            Text("Connect ›", color = th.accent, fontWeight = FontWeight.Bold)
        }
    }
}

// ------------------------------------------------------------ connecting

@Composable
fun ConnectingScreen() {
    val th = LocalAppTheme.current
    Column(
        Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        EyesLogo(120.dp)
        Spacer(Modifier.height(26.dp))
        CircularProgressIndicator(color = th.accent)
        Spacer(Modifier.height(16.dp))
        Text("Connecting to ${Iris.ble.deviceName.ifEmpty { "IRIS" }}...", fontSize = 17.sp, fontWeight = FontWeight.Bold)
        Text("This takes a few seconds", color = th.dim, fontSize = 13.sp)
        Spacer(Modifier.height(16.dp))
        TextButton(onClick = { Iris.disconnect() }) { Text("Cancel", color = th.dim) }
    }
}

// ------------------------------------------------------------ PIN (first time only)

@Composable
fun PinScreen() {
    val th = LocalAppTheme.current
    var pin by remember { mutableStateOf("") }
    Column(
        Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text("🔒", fontSize = 44.sp)
        Spacer(Modifier.height(8.dp))
        Text("Enter the pairing code", style = androidx.compose.material3.MaterialTheme.typography.headlineSmall)
        Text(
            "A 6-digit code is showing on IRIS. You only need it the first time.",
            color = th.dim,
            fontSize = 14.sp,
            textAlign = TextAlign.Center,
        )
        Spacer(Modifier.height(20.dp))
        OutlinedTextField(
            value = pin,
            onValueChange = { v -> pin = v.filter { it.isDigit() }.take(6) },
            singleLine = true,
            placeholder = { Text("000000", fontSize = 28.sp, color = th.dim) },
            textStyle = TextStyle(fontSize = 30.sp, letterSpacing = 10.sp, textAlign = TextAlign.Center, color = th.text),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.NumberPassword),
            shape = RoundedCornerShape(th.radius.coerceAtMost(18.dp)),
            colors = OutlinedTextFieldDefaults.colors(focusedBorderColor = th.accent, unfocusedBorderColor = th.dim, cursorColor = th.accent),
            modifier = Modifier.fillMaxWidth(),
        )
        Spacer(Modifier.height(16.dp))
        PrimaryButton("Pair", enabled = pin.length == 6) {
            Iris.sendPin(pin)
            pin = ""
        }
        TextButton(onClick = { Iris.disconnect() }) { Text("Cancel", color = th.dim) }
        Text("3 wrong codes disconnect - just connect again for a new one.", color = th.dim, fontSize = 12.sp, textAlign = TextAlign.Center)
    }
}
