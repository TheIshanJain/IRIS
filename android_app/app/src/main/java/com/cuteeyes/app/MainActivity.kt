package com.cuteeyes.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.cuteeyes.app.ui.Bg
import com.cuteeyes.app.ui.Card2
import com.cuteeyes.app.ui.CardColor
import com.cuteeyes.app.ui.ConnectScreen
import com.cuteeyes.app.ui.ConnectingScreen
import com.cuteeyes.app.ui.CuteEyesTheme
import com.cuteeyes.app.ui.DeviceScreen
import com.cuteeyes.app.ui.Dim
import com.cuteeyes.app.ui.PinScreen
import com.cuteeyes.app.ui.TextMain
import com.cuteeyes.app.ui.WifiScreen

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            CuteEyesTheme {
                CuteEyesApp()
            }
        }
    }
}

@Composable
fun CuteEyesApp(vm: DeviceViewModel = viewModel()) {
    val stage by vm.stage.collectAsState()
    val snackbar = remember { SnackbarHostState() }
    var tab by rememberSaveable { mutableIntStateOf(0) }

    // Show messages from the device / connection at the bottom of the screen
    LaunchedEffect(Unit) {
        vm.messages.collect { snackbar.showSnackbar(it) }
    }

    Scaffold(
        containerColor = Bg,
        snackbarHost = { SnackbarHost(snackbar) },
        bottomBar = {
            if (stage == Stage.Ready) {
                NavigationBar(containerColor = CardColor) {
                    val colors = NavigationBarItemDefaults.colors(
                        selectedTextColor = TextMain,
                        unselectedTextColor = Dim,
                        indicatorColor = Card2,
                    )
                    NavigationBarItem(
                        selected = tab == 0,
                        onClick = { tab = 0 },
                        icon = { Text("🏠", fontSize = 20.sp) },
                        label = { Text("Device") },
                        colors = colors,
                    )
                    NavigationBarItem(
                        selected = tab == 1,
                        onClick = { tab = 1 },
                        icon = { Text("📶", fontSize = 20.sp) },
                        label = { Text("Wi-Fi") },
                        colors = colors,
                    )
                }
            }
        },
    ) { padding ->
        Box(Modifier.padding(padding).fillMaxSize()) {
            when (stage) {
                Stage.Disconnected -> ConnectScreen(vm)
                Stage.Connecting -> ConnectingScreen(vm)
                Stage.NeedsPin -> PinScreen(vm)
                Stage.Ready -> if (tab == 0) DeviceScreen(vm) else WifiScreen(vm)
            }
        }
    }
}
