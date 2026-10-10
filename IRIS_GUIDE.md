# IRIS 2.0 upgrade guide

> **IRIS:** in Greek mythology, Iris is the goddess of the rainbow and the messenger of the gods. That's fitting for a device that brings your messages to you.

You already have the app working (great job!). This guide upgrades **both** the device and the app to IRIS 2.0.
Do the parts **in order**. Total time: about 30 minutes.

**What changes in 2.0**
- The web UI and the device's own Wi-Fi network are gone. **Everything is in the app now.**
- Phone notifications appear on IRIS: app logo, who it's from, and the first line of the message.
- One eye theme at a time. Interaction & Status is now reserved for status signals (battery, charging, errors...).
- Battery level and charging indications (if you connect a battery).
- 5 app themes, each with its own fonts, colours and **launcher icon colour**.
- The device remembers your phone, so you type the 6-digit code **only once**.

---

## Part 1: Flash the new firmware over USB (10 min)

The web UI is gone, so this one time the update must go over USB with Arduino IDE (like the very first time).
After this, the app can update the firmware itself.

1. Unzip `CuteEyes_Project.zip`. Open `CuteEyes_Project\firmware\ESP32S3_Cute_Eyes\`.
2. Check that the folder has exactly these **12 files**:
   `ESP32S3_Cute_Eyes.ino`, `Battery.h`, `BleSetup.h`, `Config.h`, `DebugLog.h`, `Eyes.h`, `Faces.h`, `Imu.h`, `Notify.h`, `Themes.h`, `Weather.h`, `WifiManager.h`
   (`WebUI.h` and `WebPage.h` are gone on purpose.)
3. **Optional, only if you use cloud updates:** open `Config.h` and put your GitHub link back in `OTA_MANIFEST_URL`, like before.
4. Double-click `ESP32S3_Cute_Eyes.ino` to open it in Arduino IDE.
5. Check **Tools** (same as always): Board **ESP32S3 Dev Module**, Partition Scheme **16M Flash (3MB APP/9.9MB FATFS)**, PSRAM **QSPI PSRAM**, Flash Size **16MB**.
6. Click **Upload** (→). Wait for "Done uploading".
7. The device greets you. Its Bluetooth name is now **IRIS-XXXX**.

> Your saved Wi-Fi networks, name and settings are kept. If you had several eye themes switched on, IRIS keeps one of them; you can change it in the app.

---

## Part 2: Update your GitHub folder (5 min)

⚠️ **Important:** some old files must be **deleted**, not just replaced. Otherwise the old and new versions clash and the app build fails.

1. Open **GitHub Desktop**, then **Repository → Show in Explorer**. Your `cute-eyes` folder opens.
2. **Delete these two folders** (right-click → Delete):
   - `android_app`
   - `firmware`
3. Open the unzipped `CuteEyes_Project` folder, press **Ctrl + A** to select everything, then **Ctrl + C**.
4. Go back to the `cute-eyes` folder and press **Ctrl + V**. If Windows asks about existing files, choose **Replace the files in the destination**.
5. In GitHub Desktop, type `IRIS 2.0` in **Summary**, click **Commit to main**, then click **Push origin**.
6. In your browser, go to your repo, then **Actions → Build Android app**, and wait for the ✅ green tick (about 5–8 minutes).
   ❌ Red cross? Click it, click **build**, open the red step, copy the last 30–40 lines, and send them to me.

---

## Part 3: Install the IRIS app (3 min)

1. On your phone, open `https://github.com/YOUR_USERNAME/cute-eyes/releases`
2. Under **IRIS app (build N)**, tap **IRIS.apk** and install it, like last time.
   It **installs over** the old Cute Eyes app, so you don't need to uninstall. The app's name and icon change to **IRIS**.
3. Open **IRIS**. Allow:
   - **Nearby devices** (Bluetooth): needed to talk to IRIS
   - **Notifications**: so the app can show its small "IRIS is connected" notification, which keeps it running in the background

---

## Part 4: Connect (one last time with the code)

1. Tap **🔍 Find my IRIS**, find **IRIS-XXXX**, and tap **Connect**.
2. Type the 6-digit code from the round screen and tap **Pair**.
3. From now on, IRIS **remembers your phone**. Next time, it reconnects by itself with no code needed.
   (To make IRIS and your phone forget each other, use **Settings → About → Forget IRIS** in the app.)

---

## Part 5: Turn on notifications on IRIS (5 min)

1. Go to **Settings** (⚙️), then **Notifications on IRIS**, and tap **Open notification access**.
2. Android opens a list of apps. Find **IRIS** and switch it **on**, then confirm.
3. **If the switch is greyed out or says "Restricted setting":** Android 13+ blocks this for apps installed from a file. Fix it like this:
   1. Open your phone's **Settings → Apps → IRIS**
   2. Tap **⋮** (top-right), then **Allow restricted settings**, and confirm with your PIN or fingerprint
   3. Go back and do step 1 again. The switch now works.
   (In the app, tap **Greyed out or "Restricted setting"? Tap here** to see these same steps.)
4. Back in IRIS, under **Notifications on IRIS**:
   - **Show notifications:** the main on/off switch.
   - **Show message text:** turn it off if you only want "who" and not "what" (privacy).
   - **Time on screen:** how long each notification stays on IRIS (5–20 seconds).
   - **Choose apps:** every app that sends you a notification appears in this list. Switch off the ones you don't want on IRIS.
   - Tap **Send a test** to see how it looks.
5. Send yourself a WhatsApp message or similar. IRIS wakes up, looks at you with **Attention**, and shows the card. **Shake** IRIS to dismiss it early.

---

## Part 6: Keep it connected all day (2 min)

Android likes to stop apps in the background to save battery. To keep IRIS connected:

1. **Settings → Stay connected → Allow background use**, then choose **Allow**.
2. **Xiaomi / Redmi / POCO, Oppo, Realme, Vivo, OnePlus** phones have an extra switch. Open phone **Settings → Apps → IRIS** and turn on **Autostart** and/or set **Battery** to **No restrictions**.
   This differs per brand. **https://dontkillmyapp.com** shows the exact steps for your phone.

---

## Part 7: Tour of the app

| Tab | What's there |
|---|---|
| 🏠 **Home** | Live face, greeting with your name, current expression, **Next**, time, weather, IRIS's battery, quick switches |
| 📶 **Wi-Fi** | Current connection, saved networks (⋮ = connect / change password / forget), find networks, hidden network |
| 👀 **Eyes** | **Eye style** for the round screen (pick **one**; picking another switches the old one off), greeting by name, **👋 Say hello** |
| ⚙️ **Settings** | **App theme** (pick one: Iris Neon, Sakura, Midnight Gold, Terminal, Ocean Breeze), Notifications on IRIS, Display, Shake, Location, Stay connected, Software, About (Disconnect / Forget IRIS) |
| 🛠️ **Developer** | Locked with **admin / cuteeyes** (change them in `Config.h`): device details, every expression from every theme, sensors, logs, firmware upload, reboot |

**Two different "themes":**
- **Eye style** (Eyes tab) changes how the **eyes on IRIS** look.
- **App theme** (Settings tab) changes how the **phone app** looks: colours, fonts, buttons, and the **launcher icon colour**. The eyes in the icon stay the same.

> When you change the app theme, Android swaps the launcher icon. On some phones the icon on your **home screen** disappears for a moment, or needs to be added again from the app drawer. That's a normal Android limitation for icon changes.

**Status signals (Interaction & Status theme, shown automatically):**

| You'll see | When |
|---|---|
| **Connecting** | You tell IRIS to join a Wi-Fi network |
| Wi-Fi icon + happy face | Wi-Fi connected (the happy face comes from **your** eye theme) |
| **Low Battery** | Battery under 15% (repeats every 10 minutes) |
| **Charging** / **Battery Full** | Charging detected / battery full |
| **Attention** | A notification arrives |
| **Processing** | A firmware update is being received |
| **Error** | An update failed |
| **Goodbye** | Just before IRIS restarts |

> **About the battery:** the board measures battery voltage on GPIO1, so the level and low-battery warning are reliable. The board has no "charging" signal wire, so **Charging** and **Battery Full** are worked out from the voltage rising or staying at the top. They're good estimates, not exact readings. Without a battery connected, the battery reading is hidden.

---

## Updating firmware from the app (from now on)

**Developer → Firmware → 📦 Choose .bin file**
1. In Arduino IDE, use **Sketch → Export Compiled Binary**.
2. Copy `ESP32S3_Cute_Eyes.ino.bin` to your phone. Use that exact file, not the `merged` one.
3. In the app, tap **📦 Choose .bin file** and pick it. The upload starts and shows progress.

Over Bluetooth this takes about 1–3 minutes. Keep the phone next to IRIS. If it's interrupted, IRIS keeps running the old version.

Cloud updates (GitHub) still work: **Settings → Software → Check for updates**.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| App build fails with "Redeclaration" or "Conflicting declarations" | Old files are left over. Redo Part 2: delete `android_app` and `firmware` first, then paste. |
| App can't find IRIS | Did you flash firmware 2.0 (Part 1)? Is the old Cute Eyes app or another phone still connected to it? |
| Asks for the code every time | Use **Settings → About → Forget IRIS**, then pair once more. |
| No notifications on IRIS | Is notification access on (Part 5)? Is the app switched on in the Apps list? Is IRIS connected (Home tab)? |
| Disconnects in the background | Do Part 6, including the brand-specific steps. |
| Text looks shortened / missing characters | The round screen's font has English letters only. Other scripts and emoji are removed, and the message falls back to "New message" if nothing is left. |
