# Updating to IRIS 2.0: step by step

IRIS is now controlled **only through the app**. The web page and the device's own Wi-Fi network are gone, and everything they did is in the app. Do the three parts below in order.

---

## Part 1: Update the firmware (USB, about 10 min)

1. Open your project folder `Documents\GitHub\cute-eyes\firmware\ESP32S3_Cute_Eyes`.
2. **Before replacing anything**, open the old `Config.h` in Notepad and copy the line that starts with `#define OTA_MANIFEST_URL` (it has your GitHub username). Keep it in Notepad for step 5.
3. **Delete** these two files from the folder: `WebUI.h` and `WebPage.h`.
4. Unzip the new `CuteEyes_Project.zip`. Copy everything from its `firmware\ESP32S3_Cute_Eyes` folder into your folder, and choose **Replace** when asked.
   The folder should now contain these 12 files:
   `Battery.h, BleSetup.h, Config.h, DebugLog.h, ESP32S3_Cute_Eyes.ino, Eyes.h, Faces.h, Imu.h, Notify.h, Themes.h, Weather.h, WifiManager.h`
5. Open the new `Config.h`. Replace its `OTA_MANIFEST_URL` line with the line you copied in step 2, then save.
6. Open `ESP32S3_Cute_Eyes.ino` in Arduino IDE. Keep the same Tools settings as before (16M Flash, 3MB APP, QSPI PSRAM) and click **Upload**.
7. The device now shows up as **IRIS-XXXX**.

> Your saved Wi-Fi networks, name, brightness and other settings are kept. If you had several eye themes switched on before, IRIS keeps the first one.

---

## Part 2: Update the app (about 10 min, mostly waiting)

1. Copy the new `android_app` folder from the zip into `Documents\GitHub\cute-eyes`, and choose **Replace**.
2. Copy `.github\workflows\build-android-app.yml` too (it's unchanged, so you can skip this if it's already there).
3. Open **GitHub Desktop**. In **Summary**, type `IRIS 2.0`, click **Commit to main**, then **Push origin**.
4. In your browser, open your repository, go to **Actions → Build Android app**, and wait for the green tick.
5. On your phone, open your **Releases** page, download `CuteEyes.apk` and install it.
   It installs **over** the old app, so you don't need to uninstall. Your home-screen icon may move or reappear as **IRIS**.

> Red cross in Actions? Click it, open the failed step, and copy the last 30–40 lines to me.

---

## Part 3: First connection and notifications (5 min)

1. Open **IRIS** and tap **Find my IRIS**, then **Connect**.
2. Type the **6-digit code** shown on the device. **This is the last time you'll need it.** After this, the phone reconnects to IRIS automatically, even in the background.
3. When Android asks to allow **notifications** for IRIS, tap **Allow**. That's only for the small "IRIS is connected" notice that keeps the connection alive.
4. Go to the **⚙️ Settings** tab and tap **Open notification access**:
   - Find **IRIS** in the list and switch it **on**, then confirm.
   - **If IRIS is greyed out or says "Restricted setting"**, Android is protecting apps installed outside the Play Store. Tap the blue *"Greyed out?"* link in the app, then:
     1. Tap **Open app info**.
     2. Tap **⋮** (top-right corner).
     3. Tap **Allow restricted settings**.
     4. Go back and try **Open notification access** again.
5. Still in Settings, under **Stay connected**, tap **Allow background use**, then **Allow**.
   On Xiaomi, Oppo, Vivo or Realme phones, also look for **Autostart** in the phone's settings for IRIS and turn it on.
6. Tap **Send a test**. IRIS should flash its *Attention* eyes, then show the IRIS logo with "Hello from IRIS". 🎉
7. Ask someone to message you. You'll see their app's logo, their name and the first line of the message.
   - **Shake IRIS** to dismiss a notification.
   - Use **Choose apps** to turn off apps you don't want on IRIS.
   - Turn **Show message text** off for privacy (it then shows only the app and sender).

---

## Where everything moved

| Was in the web page | Now in the app |
|---|---|
| Home: greeting, time, weather, Next, Auto-change, Shake, Brightness | 🏠 **Home** |
| Wi-Fi: scan, connect, change password, forget, hidden | 📶 **Wi-Fi** |
| Themes + Greeting by name | 👀 **Eyes** (one eye style at a time) |
| Display, Shake, Location, Software update | ⚙️ **Settings** |
| Device details, any expression, sensors, firmware upload, reboot, logs | 🛠️ **Developer** (`admin` / `cuteeyes`, changeable in `Config.h`) |
| *(new)* App themes | ⚙️ **Settings → App theme** (5 styles; the home-screen icon colour follows) |
| *(new)* Notifications | ⚙️ **Settings → Notifications on IRIS** |

## Status faces on IRIS

The **Interaction & Status** expressions no longer appear in the eye styles. IRIS uses them by itself:

| You'll see | When |
|---|---|
| Connecting (Wi-Fi waves) | A short moment while joining Wi-Fi (at most once a minute) |
| Listening | The phone app just connected |
| Attention | A notification arrives, just before the notification card |
| Charging / Battery Full | A battery is charging / full |
| Low Battery | Battery at 15% or below (every 10 minutes) |
| Processing | Checking for a cloud update |
| Error | An update failed |
| Goodbye | Just before a restart |

> **About the battery:** the board measures battery voltage on GPIO1, but it has no "charging" signal. So *charging* means the voltage is rising, and *full* means it's above about 4.15 V. These are good estimates, not exact readings. With no battery connected, the app shows **USB power**.
