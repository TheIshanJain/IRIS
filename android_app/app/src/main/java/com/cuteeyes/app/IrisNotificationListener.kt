package com.cuteeyes.app

import android.content.ComponentName
import android.content.Context
import android.provider.Settings
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification

/**
 * Android hands every phone notification to this service once the user allows
 * "Notification access" for IRIS. Reference:
 * developer.android.com/reference/android/service/notification/NotificationListenerService
 */
class IrisNotificationListener : NotificationListenerService() {

    override fun onListenerConnected() {
        Iris.init(this)
        Iris.listenerConnected.value = true
    }

    override fun onListenerDisconnected() {
        Iris.listenerConnected.value = false
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        Iris.init(this)
        try {
            Iris.onPhoneNotification(sbn)
        } catch (e: Exception) {
            // never crash the listener because of one odd notification
        }
    }

    companion object {
        /** Has the user granted notification access? */
        fun isEnabled(context: Context): Boolean {
            val flat = Settings.Secure.getString(context.contentResolver, "enabled_notification_listeners") ?: return false
            val me = ComponentName(context, IrisNotificationListener::class.java).flattenToString()
            return flat.split(':').any { it == me }
        }
    }
}
