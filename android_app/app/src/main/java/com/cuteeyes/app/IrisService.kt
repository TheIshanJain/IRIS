package com.cuteeyes.app

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat

/**
 * Keeps IRIS connected while the app is closed (needed for notifications).
 * Android requires a small permanent notification for this ("IRIS is connected").
 * Reference: developer.android.com/develop/background-work/services/fgs
 */
class IrisService : Service() {

    override fun onCreate() {
        super.onCreate()
        Iris.init(this)
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(CHANNEL, "IRIS connection", NotificationManager.IMPORTANCE_MIN).apply {
                description = "Keeps IRIS connected so your notifications reach it"
                setShowBadge(false)
            }
        )
        val open = PendingIntent.getActivity(
            this, 0,
            packageManager.getLaunchIntentForPackage(packageName) ?: Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        val notification = NotificationCompat.Builder(this, CHANNEL)
            .setSmallIcon(R.drawable.ic_stat_iris)
            .setContentTitle("IRIS is connected")
            .setContentText("Your notifications will appear on IRIS")
            .setContentIntent(open)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_MIN)
            .build()
        try {
            if (Build.VERSION.SDK_INT >= 29) {
                startForeground(ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
            } else {
                startForeground(ID, notification)
            }
        } catch (e: Exception) {
            stopSelf()
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int = START_STICKY

    override fun onBind(intent: Intent?): IBinder? = null

    companion object {
        private const val CHANNEL = "iris_link"
        private const val ID = 7

        fun start(context: Context) {
            try {
                ContextCompat.startForegroundService(context, Intent(context, IrisService::class.java))
            } catch (e: Exception) {
                // Android may refuse while the app is in the background; the notification
                // listener keeps the app alive in that case.
            }
        }

        fun stop(context: Context) {
            context.stopService(Intent(context, IrisService::class.java))
        }
    }
}
