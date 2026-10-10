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
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.FilledTonalButton
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.cuteeyes.app.DeviceViewModel
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
fun ConnectScreen(vm: DeviceViewModel) {
    val ctx = LocalContext.current
    var granted by remember { mutableStateOf(hasBlePermissions(ctx)) }
    var btOn by remember { mutableStateOf(vm.ble.isBluetoothOn) }
    val scanning by vm.ble.scanning.collectAsState()
    val found by vm.ble.found.collectAsState()
    val last = vm.lastDevice

    val askPermissions = rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { result ->
        granted = result.values.all { it }
        if (!granted) vm.say("Bluetooth permission is needed to find your device")
    }
    val askBluetoothOn = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) {
        btOn = vm.ble.isBluetoothOn
    }

    // Keep an eye on Bluetooth being switched on/off from the quick settings
    LaunchedEffect(Unit) {
        while (true) {
            btOn = vm.ble.isBluetoothOn
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
            Column(Modifier.fillMaxWidth().padding(top = 24.dp, bottom = 8.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                EyesLogo(110.dp)
                Spacer(Modifier.height(16.dp))
                Text("Hi there!", fontSize = 26.sp, fontWeight = FontWeight.ExtraBold)
                Text("Let's find your Cute Eyes", color = Dim, fontSize = 15.sp)
            }
        }

        when {
            !granted -> item {
                ActionCard(
                    emoji = "🔐",
                    title = "Allow Bluetooth",
                    text = "Needed to find and talk to your device. Nothing else is accessed.",
                    button = "Allow",
                ) { askPermissions.launch(blePermissions()) }
            }

            !btOn -> item {
                ActionCard(
                    emoji = "📴",
                    title = "Bluetooth is off",
                    text = "Turn it on to find your device.",
                    button = "Turn on",
                ) { askBluetoothOn.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)) }
            }

            else -> {
                if (last != null && found.none { it.address == last.first }) {
                    item {
                        DeviceRow(emoji = "⭐", name = last.second, sub = "Last used - tap to reconnect") {
                            vm.connect(last.first, last.second)
                        }
                    }
                }
                item {
                    FilledTonalButton(
                        onClick = { if (scanning) vm.ble.stopScan() else vm.ble.startScan() },
                        modifier = Modifier.fillMaxWidth().height(52.dp),
                        shape = RoundedCornerShape(16.dp),
                    ) {
                        if (scanning) {
                            CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp)
                            Spacer(Modifier.width(10.dp))
                            Text("Looking... tap to stop")
                        } else {
                            Text("🔍  Find my device", fontWeight = FontWeight.Bold)
                        }
                    }
                }
                items(found, key = { it.address }) { d ->
                    DeviceRow(emoji = "👀", name = d.name, sub = "Signal ${signalBars(d.rssi)}") {
                        vm.connect(d.address, d.name)
                    }
                }
                if (found.isEmpty() && !scanning) {
                    item {
                        Text(
                            "Switch on the device and keep it close to your phone." +
                                if (Build.VERSION.SDK_INT < 31) "\nOn this Android version, Location must also be on." else "",
                            color = Dim,
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
    SoftCard {
        Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
            Text(emoji, fontSize = 36.sp)
            Spacer(Modifier.height(6.dp))
            Text(title, fontWeight = FontWeight.Bold, fontSize = 18.sp)
            Text(text, color = Dim, fontSize = 14.sp, textAlign = TextAlign.Center)
            Spacer(Modifier.height(12.dp))
            Button(onClick = onClick, shape = RoundedCornerShape(14.dp)) { Text(button, fontWeight = FontWeight.Bold) }
        }
    }
}

@Composable
fun DeviceRow(emoji: String, name: String, sub: String, onConnect: () -> Unit) {
    SoftCard {
        Row(verticalAlignment = Alignment.CenterVertically) {
            EmojiBadge(emoji)
            Spacer(Modifier.width(12.dp))
            Column(Modifier.weight(1f)) {
                Text(name, fontWeight = FontWeight.Bold)
                Text(sub, color = Dim, fontSize = 12.sp)
            }
            Button(onClick = onConnect, shape = RoundedCornerShape(12.dp)) { Text("Connect") }
        }
    }
}

// ------------------------------------------------------------ connecting

@Composable
fun ConnectingScreen(vm: DeviceViewModel) {
    Column(
        Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        EyesLogo(110.dp)
        Spacer(Modifier.height(24.dp))
        CircularProgressIndicator()
        Spacer(Modifier.height(16.dp))
        Text("Connecting to ${vm.ble.deviceName}...", fontSize = 17.sp, fontWeight = FontWeight.Bold)
        Text("This takes a few seconds", color = Dim, fontSize = 13.sp)
        Spacer(Modifier.height(16.dp))
        TextButton(onClick = { vm.disconnect() }) { Text("Cancel") }
    }
}

// ------------------------------------------------------------ PIN

@Composable
fun PinScreen(vm: DeviceViewModel) {
    var pin by remember { mutableStateOf("") }
    Column(
        Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text("🔒", fontSize = 44.sp)
        Spacer(Modifier.height(8.dp))
        Text("Enter the pairing code", fontSize = 22.sp, fontWeight = FontWeight.ExtraBold)
        Text(
            "A 6-digit code is now showing on the round screen.",
            color = Dim,
            fontSize = 14.sp,
            textAlign = TextAlign.Center,
        )
        Spacer(Modifier.height(20.dp))
        OutlinedTextField(
            value = pin,
            onValueChange = { v -> pin = v.filter { it.isDigit() }.take(6) },
            singleLine = true,
            placeholder = { Text("000000", fontSize = 28.sp, color = Dim) },
            textStyle = TextStyle(fontSize = 30.sp, letterSpacing = 10.sp, textAlign = TextAlign.Center),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.NumberPassword),
            shape = RoundedCornerShape(16.dp),
            modifier = Modifier.fillMaxWidth(),
        )
        Spacer(Modifier.height(16.dp))
        Button(
            onClick = { vm.sendPin(pin); pin = "" },
            enabled = pin.length == 6,
            modifier = Modifier.fillMaxWidth().height(52.dp),
            shape = RoundedCornerShape(16.dp),
        ) { Text("Pair", fontWeight = FontWeight.Bold, fontSize = 16.sp) }
        TextButton(onClick = { vm.disconnect() }) { Text("Cancel") }
        Text("3 wrong codes disconnect - just connect again for a new one.", color = Dim, fontSize = 12.sp, textAlign = TextAlign.Center)
    }
}
