package com.cuteeyes.app

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Snackbar
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.cuteeyes.app.ui.AppThemes
import com.cuteeyes.app.ui.ConnectScreen
import com.cuteeyes.app.ui.ConnectingScreen
import com.cuteeyes.app.ui.DevScreen
import com.cuteeyes.app.ui.EyesScreen
import com.cuteeyes.app.ui.HomeScreen
import com.cuteeyes.app.ui.IrisTheme
import com.cuteeyes.app.ui.LocalAppTheme
import com.cuteeyes.app.ui.PinScreen
import com.cuteeyes.app.ui.SettingsScreen
import com.cuteeyes.app.ui.WifiScreen

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Iris.init(this)
        enableEdgeToEdge()
        setContent {
            val themeId by Iris.appThemeId.collectAsState()
            IrisTheme(AppThemes.byId(themeId)) {
                IrisAppUi()
            }
        }
    }
}

private data class Tab(val emoji: String, val label: String)

private val tabs = listOf(
    Tab("🏠", "Home"),
    Tab("📶", "Wi-Fi"),
    Tab("👀", "Eyes"),
    Tab("⚙️", "Settings"),
    Tab("🛠️", "Developer"),
)

@Composable
fun IrisAppUi() {
    val th = LocalAppTheme.current
    val ctx = LocalContext.current
    val stage by Iris.stage.collectAsState()
    val snackbar = remember { SnackbarHostState() }
    var tab by rememberSaveable { mutableIntStateOf(0) }

    LaunchedEffect(Unit) {
        Iris.messages.collect { snackbar.showSnackbar(it) }
    }

    // Android 13+: allow the small "IRIS is connected" notification of the background service
    val askNotify = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { }
    LaunchedEffect(stage) {
        if (stage == Stage.Ready && Build.VERSION.SDK_INT >= 33 &&
            ContextCompat.checkSelfPermission(ctx, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) {
            askNotify.launch(Manifest.permission.POST_NOTIFICATIONS)
        }
    }

    Scaffold(
        containerColor = th.bg,
        snackbarHost = {
            SnackbarHost(snackbar) { data ->
                Snackbar(data, containerColor = th.card2, contentColor = th.text)
            }
        },
        bottomBar = {
            if (stage == Stage.Ready) {
                NavigationBar(containerColor = th.card) {
                    val colors = NavigationBarItemDefaults.colors(
                        selectedTextColor = th.accent,
                        unselectedTextColor = th.dim,
                        indicatorColor = th.accent.copy(alpha = 0.18f),
                    )
                    tabs.forEachIndexed { i, t ->
                        NavigationBarItem(
                            selected = tab == i,
                            onClick = { tab = i },
                            icon = { Text(t.emoji, fontSize = 20.sp) },
                            label = { Text(t.label, fontSize = 11.sp, maxLines = 1) },
                            colors = colors,
                        )
                    }
                }
            }
        },
    ) { padding ->
        Box(Modifier.padding(padding).fillMaxSize()) {
            when (stage) {
                Stage.Disconnected -> ConnectScreen()
                Stage.Connecting -> ConnectingScreen()
                Stage.NeedsPin -> PinScreen()
                Stage.Ready -> when (tab) {
                    0 -> HomeScreen()
                    1 -> WifiScreen()
                    2 -> EyesScreen()
                    3 -> SettingsScreen()
                    else -> DevScreen()
                }
            }
        }
    }
}
