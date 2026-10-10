package com.cuteeyes.app

import android.app.Application

class IrisApp : Application() {
    override fun onCreate() {
        super.onCreate()
        Iris.init(this)
    }
}
