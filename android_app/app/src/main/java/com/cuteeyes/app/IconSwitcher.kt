package com.cuteeyes.app

import android.content.ComponentName
import android.content.Context
import android.content.pm.PackageManager

/**
 * Changes the home-screen icon colour to match the app theme.
 * Each theme has an <activity-alias> in AndroidManifest.xml with its own icon;
 * exactly one is enabled at a time. (Widely used "dynamic app icon" technique.)
 */
object IconSwitcher {
    val aliases = mapOf(
        "neon" to "LauncherNeon",
        "sakura" to "LauncherSakura",
        "gold" to "LauncherGold",
        "terminal" to "LauncherTerminal",
        "ocean" to "LauncherOcean",
    )

    fun apply(context: Context, themeId: String) {
        val pm = context.packageManager
        val wanted = aliases[themeId] ?: aliases.getValue("neon")
        // enable the new one first, so there is never a moment without an icon
        aliases.values.sortedBy { if (it == wanted) 0 else 1 }.forEach { alias ->
            val state = if (alias == wanted) PackageManager.COMPONENT_ENABLED_STATE_ENABLED
            else PackageManager.COMPONENT_ENABLED_STATE_DISABLED
            try {
                pm.setComponentEnabledSetting(
                    ComponentName(context.packageName, "com.cuteeyes.app.$alias"),
                    state,
                    PackageManager.DONT_KILL_APP,
                )
            } catch (e: Exception) {
            }
        }
    }
}
