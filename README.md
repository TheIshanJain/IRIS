# Cute Eyes: firmware, Cloud OTA and mobile app

> **Android app:** the app is now a native Kotlin app in `android_app/`. Follow **[APP_GUIDE.md](APP_GUIDE.md)** to build and install it. (The older Flutter `mobile_app` folder was removed; any mention of it below is out of date.)

> **New here? Follow [GETTING_STARTED.md](GETTING_STARTED.md).** It walks through everything step by step, with no programming tools to install. This README is the technical reference.

```
CuteEyes_Project/
├── .github/workflows/          ← GitHub builds the APK + publishes OTA updates
├── GETTING_STARTED.md          ← step-by-step guide
├── firmware/
│   ├── ESP32S3_Cute_Eyes/        ← open this sketch in Arduino IDE
│   │   ├── ESP32S3_Cute_Eyes.ino   main: ties everything together
│   │   ├── Config.h                version, OTA URL, BLE name  ← edit this
│   │   ├── Eyes.h                  the 11 expressions
│   │   ├── WifiManager.h           saves up to 5 networks, auto-reconnect
│   │   └── BleSetup.h              Bluetooth link + PIN pairing
│   ├── libraries/CloudOTA/       ← the OTA library (copy into Arduino/libraries)
│   └── tools/make_release.py     ← prepares an update for publishing
└── mobile_app/                   ← Flutter app (Android + iOS)
```

---

## 1. Flash the firmware (first time, over USB)

1. **Libraries.** In Arduino IDE → Library Manager, install **GFX Library for Arduino** and **ArduinoJson** (v7).
   Then copy `firmware/libraries/CloudOTA` into `Documents\Arduino\libraries\` and restart the IDE.
2. **Tools menu:**

   | Setting | Value |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | USB CDC On Boot | Disabled |
   | Flash Size | 16MB (128Mb) |
   | **Partition Scheme** | **16M Flash (3MB APP/9.9MB FATFS)** (gives two app slots, needed for OTA) |
   | PSRAM | QSPI PSRAM |

3. **Edit `Config.h`.** Set `OTA_MANIFEST_URL` to your GitHub repo (see section 2).
4. Upload. The eyes start, and two small dots at the top show status: **left = Wi-Fi** (green = online, blinking amber = connecting), **right = Bluetooth** (blue = phone connected).

> The partition scheme matters. With the default scheme, updates will fail with "No room for update".

---

## 2. Cloud OTA: publish an update from anywhere

The device **pulls** updates. It checks a `manifest.json` on the internet about 10 s after it gets online, and then every 6 hours.
You can also tap **Check for update** in the app. Because the device makes the request itself, you don't need port forwarding or a fixed IP, and the device can be on any network in the world.

### One-time setup
1. Create a **public** GitHub repository (e.g. `cute-eyes-firmware`). Private repo assets need a login token.
2. In `Config.h`, set:
   ```c
   #define OTA_MANIFEST_URL "https://github.com/YOUR_USER/cute-eyes-firmware/releases/latest/download/manifest.json"
   ```
   `latest/download/` always points to your newest release, so this URL never changes.

### Every time you want to ship an update
**Easiest:** bump `FW_VERSION`, push, then Actions → **Publish firmware update** → Run workflow. GitHub compiles, checksums and publishes everything (see GETTING_STARTED.md, Part E).

**Manual alternative** (without GitHub Actions):
1. Bump `FW_VERSION` in `Config.h` (e.g. `"1.0.0"` → `"1.0.1"`).
2. Arduino IDE → **Sketch → Export Compiled Binary**. This creates `build/.../ESP32S3_Cute_Eyes.ino.bin` inside the sketch folder.
3. Prepare the release:
   ```
   python firmware/tools/make_release.py --bin path\to\ESP32S3_Cute_Eyes.ino.bin --version 1.0.1 --repo YOUR_USER/cute-eyes-firmware --notes "New expressions"
   ```
   This writes `release/firmware.bin` and `release/manifest.json` (with an MD5 checksum).
4. Create a GitHub release tagged `v1.0.1` and attach **both** files. You can do this on the GitHub website, or with the command the script prints (`gh release create ...`).

Within the next check (or right away if you tap **Check for update**), every device downloads the new firmware, verifies its MD5, flashes it with a progress ring on screen, and reboots.

If an update fails halfway (power cut, Wi-Fi drop), the device keeps running the old firmware. The new image only becomes active after it has been fully written and verified.

### Using the library in other projects
```cpp
#include <CloudOTA.h>
CloudOTA ota;

void setup() {
  // ... connect Wi-Fi ...
  ota.begin("1.0.0", "https://.../manifest.json");
  ota.setInsecure();                     // or ota.setRootCA(pem)
  ota.onProgress([](int pct) { /* draw progress */ });
}
void loop() { ota.loop(); }
```
Other options: `setCheckInterval(ms)`, `setDeviceType("...")` (refuses manifests made for other boards), `requestCheck()`, `checkNow()`, `setAutoReboot(false)`. The manifest can be hosted on any HTTP(S) server, such as S3, Firebase Hosting or your own VPS. It doesn't have to be GitHub.

### Security: read before shipping to other people
* By default, the TLS certificate of the server is **not verified** (`setInsecure()`). That's fine while you're developing. However, someone who can intercept the device's traffic could feed it fake firmware. For production, put your host's root CA in `OTA_ROOT_CA` in `Config.h`.
* The MD5 in the manifest protects against corrupted downloads, not against tampering.
* Anyone who can publish to your release page can update every device, so keep your GitHub account secured with 2FA.

---

## 3. Mobile app (Flutter)

**Easiest:** push the repo to GitHub. The **Build Android app** workflow builds the APK and puts it on the Releases page under "Android app" (see GETTING_STARTED.md, Part B).

### Build it on your own computer (optional)
1. Install Flutter: https://docs.flutter.dev/get-started/install
2. Create the platform folders and drop in this code:
   ```
   flutter create --org com.yourname --project-name cute_eyes_app cute_eyes_app
   ```
   Copy `mobile_app/lib/`, `pubspec.yaml` and `analysis_options.yaml` over the generated ones. Delete the generated `test/` folder.
3. **Android:** paste `platform_setup/AndroidManifest_permissions.xml` into `android/app/src/main/AndroidManifest.xml`.
   **iOS:** paste `platform_setup/ios_Info_plist.xml` into `ios/Runner/Info.plist`.
4. Run it on a real phone (emulators have no Bluetooth):
   ```
   flutter pub get
   flutter run
   ```
   To get an installable Android file: `flutter build apk --release` creates `build/app/outputs/flutter-apk/app-release.apk`.

### Using it
* **Device tab.** Scan, tap **Connect**, then type the 6-digit code that appears on the round screen. Once paired, the tab shows firmware version, Wi-Fi status and IP, and lets you check for firmware updates with a progress bar.
* **Wi-Fi tab.** Shows the current connection. **Saved networks** (up to 5) can be connected, have their password changed, or be removed. **Nearby networks** uses the device's own scan: tap one and enter the password. **Add** handles hidden networks.

---

## 4. Bluetooth protocol (for reference)

Service `6e1a0001-7b6e-4c3a-9d0e-c0ffee000001`
* RX `…0002…`: app writes JSON commands
* TX `…0003…`: device sends JSON by notify

Each message is a single JSON line ending in `\n`. Long lines are split into packets and re-joined at the newline.

| App sends | Device answers |
|---|---|
| `{"cmd":"auth","pin":"123456"}` | `{"type":"auth","ok":true}` then status + list |
| `{"cmd":"status"}` | `{"type":"status","fw":"1.0.0","wifi":"connected","ssid":"…","ip":"…","rssi":-52,"ota":"idle","latest":""}` |
| `{"cmd":"list"}` | `{"type":"list","networks":["Home","Office"]}` |
| `{"cmd":"scan"}` | `{"type":"scan","networks":[{"ssid":"Home","rssi":-40,"secure":true}]}` |
| `{"cmd":"add","ssid":"Home","pass":"secret123"}` | `ack`, then connects; also updates an existing network's password |
| `{"cmd":"delete","ssid":"Home"}` | `ack` |
| `{"cmd":"connect","ssid":"Home"}` | `ack` |
| `{"cmd":"ota"}` | `ack`, then `ota_progress` messages while updating |

Every command except `auth` is rejected until the correct PIN is entered. Three wrong PINs disconnect the phone, and a new PIN is generated on each connection.

---

## 5. Troubleshooting

| Problem | Fix |
|---|---|
| App finds no device | Bluetooth + permissions on? On Android 11 or older, Location must also be on. |
| "No room for update" | Wrong partition scheme. Use 16M Flash (3MB APP/9.9MB FATFS) and re-flash over USB once. |
| "Manifest HTTP 404" | The repo is private, the release has no `manifest.json`, or the URL in `Config.h` is wrong. |
| Update never installs | `FW_VERSION` in the new build must be **higher** than the running one, and the manifest's `version` must match it. |
| Wi-Fi won't connect | ESP32 only supports **2.4 GHz** networks, not 5 GHz. |
| Screen freezes for a second every few hours | That's the OTA check (HTTPS request). It's normal. |
