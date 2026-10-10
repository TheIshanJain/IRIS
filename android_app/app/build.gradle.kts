plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "com.cuteeyes.app"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.cuteeyes.app"
        minSdk = 26          // Android 8.0 and newer
        targetSdk = 34
        // Each cloud build gets a higher number automatically, so updates install cleanly
        versionCode = System.getenv("GITHUB_RUN_NUMBER")?.toIntOrNull() ?: 1
        versionName = "0.1.$versionCode"
    }

    // One fixed key signs every build, so a new APK installs over the old one.
    // (Free development key. Make a private one before publishing on the Play Store.)
    signingConfigs {
        create("shared") {
            storeFile = file("cuteeyes-dev.jks")
            storePassword = "cuteeyes"
            keyAlias = "cuteeyes"
            keyPassword = "cuteeyes"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true        // keeps the app small
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            signingConfig = signingConfigs.getByName("shared")
        }
        debug {
            signingConfig = signingConfigs.getByName("shared")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildFeatures {
        compose = true
    }
    lint {
        checkReleaseBuilds = false
        abortOnError = false
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2024.09.00")
    implementation(composeBom)
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.activity:activity-compose:1.9.2")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.6")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.6")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.foundation:foundation")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-core")
}
