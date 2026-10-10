# Getting started: step by step

> **Android app:** the app is now a native Kotlin app in `android_app/`. Follow **[APP_GUIDE.md](APP_GUIDE.md)** to build and install it. (The older Flutter `mobile_app` folder was removed; any mention of it below is out of date.)

This guide assumes you know Arduino and hardware, but have never built an app or used GitHub.
You will **not** install Flutter, Android Studio or any programming tools. GitHub builds the app and the firmware updates for you, for free, on its own computers.

**What you'll end up with:**
- The Android app installed on your phone
- The device connected to your Wi-Fi from the app
- A button that sends new firmware to every device, anywhere in the world

Time needed: about 45 minutes the first time.

---

## Part A: Put the project on GitHub (one time, about 15 min)

GitHub is a free website that stores your project. It will also build the app and host your firmware updates.

1. **Create an account** at https://github.com/signup. Remember your **username**; you'll need it in step 6.

2. **Install GitHub Desktop** from https://desktop.github.com. Open it and sign in with the account you just made.

3. **Create a repository** (a project folder that syncs with GitHub). In GitHub Desktop, click **File → New repository...**
   - Name: `cute-eyes`
   - Local path: leave as it is (usually `Documents\GitHub`)
   - Click **Create repository**

4. Click **Repository → Show in Explorer**. A Windows folder opens: `Documents\GitHub\cute-eyes`.

5. **Copy the project in.** Unzip `CuteEyes_Project.zip` somewhere. Open the `CuteEyes_Project` folder, press **Ctrl+A** to select everything, then **Ctrl+C**. Go to the `cute-eyes` folder from step 4 and press **Ctrl+V**.

   Check that it looks like this. The `.github` folder is important; it holds the instructions for building the app.
   ```
   cute-eyes\
     .github\
     firmware\
     mobile_app\
     GETTING_STARTED.md
     README.md
   ```

6. **Tell the firmware where to look for updates.** Open `cute-eyes\firmware\ESP32S3_Cute_Eyes\Config.h` in Notepad. Find this line:
   ```
   #define OTA_MANIFEST_URL  "https://github.com/YOUR_USER/YOUR_REPO/releases/latest/download/manifest.json"
   ```
   Replace `YOUR_USER` with your GitHub username and `YOUR_REPO` with `cute-eyes`. For example, if your username is `jaini-dev`:
   ```
   #define OTA_MANIFEST_URL  "https://github.com/jaini-dev/cute-eyes/releases/latest/download/manifest.json"
   ```
   Save the file. Capital and small letters must match your username exactly.

7. **Upload to GitHub.** Go back to GitHub Desktop. You'll see a list of new files.
   - Bottom-left, in **Summary**, type `First upload`
   - Click **Commit to main**
   - Click **Publish repository** (top bar)
   - **Untick "Keep this code private"**, then click **Publish repository**

   > **Why public?** Your devices download updates from here without a password. People can see the code, but there are no secrets in it. Wi-Fi passwords are only stored on the device, never in the code.

---

## Part B: Get the Android app (about 10 min, mostly waiting)

1. In your browser, go to `https://github.com/YOUR_USERNAME/cute-eyes` and click the **Actions** tab at the top.
   If you see a green button saying *"I understand my workflows, go ahead and enable them"*, click it.

2. On the left, click **Build Android app**.
   - If a run is already listed (yellow dot = running), just wait.
   - If nothing is listed: click **Run workflow** (right side), then the green **Run workflow** button.

3. Wait for a **green tick**. The first build takes about 8–10 minutes. You can close the page and come back.

4. **On your Android phone**, open `https://github.com/YOUR_USERNAME/cute-eyes/releases`. Under **Android app (build 1)**, tap **CuteEyes.apk** to download it.

5. Open the downloaded file. Android will warn you because the app isn't from the Play Store:
   - *"For your security, your phone is not allowed to install unknown apps from this source"* → tap **Settings** → turn on **Allow from this source** → go back → **Install**
   - If Play Protect says *"Unsafe app blocked"* or *"Unknown app"* → tap **More details → Install anyway**

6. Open **Cute Eyes**. When it asks for **Nearby devices** permission, tap **Allow**.

> **Installing a newer version of the app later:** if Android says *"App not installed"*, uninstall the old Cute Eyes app first, then install the new one. Each cloud build gets a new signing key, so Android treats it as a different app.

---

## Part C: Flash the firmware over USB (one time)

You know this part. The only differences from before are the libraries and the partition scheme.

1. **Libraries:**
   - In Arduino IDE → Library Manager, install **ArduinoJson** (by Benoit Blanchon). You already have **GFX Library for Arduino**.
   - Copy the folder `cute-eyes\firmware\libraries\CloudOTA` into `Documents\Arduino\libraries\`. Then restart Arduino IDE.

2. **Open the sketch from the GitHub folder:** `cute-eyes\firmware\ESP32S3_Cute_Eyes\ESP32S3_Cute_Eyes.ino`.
   Always work on this copy from now on. That way GitHub Desktop sees your changes.

3. **Tools menu:**

   | Setting | Value |
   |---|---|
   | Board | ESP32S3 Dev Module |
   | USB CDC On Boot | Disabled |
   | Flash Size | 16MB (128Mb) |
   | **Partition Scheme** | **16M Flash (3MB APP/9.9MB FATFS)** |
   | PSRAM | QSPI PSRAM |

   > The partition scheme is essential. Without it, OTA updates have no space to download into.

4. **Upload.** The eyes start. Two small dots at the top show status:
   - **Left dot (Wi-Fi):** grey = no Wi-Fi, blinking amber = connecting, green = online
   - **Right dot (Bluetooth):** blue = phone connected

---

## Part D: Connect the device to Wi-Fi using the app

1. Open the app. On the **Device** tab, tap **Scan for devices**. Your board appears as `CuteEyes-XXXX`. Tap **Connect**.
2. A **6-digit code** appears on the round screen. Type it into the app and tap **Pair**. This stops neighbours from changing your device's Wi-Fi.
3. Go to the **Wi-Fi** tab and tap **Scan**. Tap your network, type the password, and tap **Save & connect**.
   - The ESP32 only works with **2.4 GHz** Wi-Fi. If your router has two networks (often one ending in "5G"), pick the other one.
   - For a hidden network, tap **Add** next to *Saved networks* and type the name yourself.
4. After a few seconds, the left dot turns green and the eyes laugh. The **Device** tab now shows the IP address.

You can save up to 5 networks (for example home, office and phone hotspot). The device connects to whichever one is in range. Tap the **⋮** menu next to a saved network to connect to it now, change its password, or remove it.

---

## Part E: Send a firmware update to your devices (every time)

1. Make your changes in Arduino IDE, in the `cute-eyes` folder. You can test them over USB first if you want.

2. In `Config.h`, **increase the version number**:
   ```
   #define FW_VERSION  "1.0.1"      // was "1.0.0"
   ```
   It must go up every time: 1.0.1 → 1.0.2 → 1.1.0, and so on. Devices only install a version **higher** than the one they're running.

3. In **GitHub Desktop**, you'll see your changed files. Type a summary like `Version 1.0.1`, click **Commit to main**, then **Push origin**.

4. In your browser, go to **Actions** → **Publish firmware update** → **Run workflow**. Optionally type what changed, then click the green **Run workflow** button.

5. Wait for the **green tick** (about 5 minutes). A new release called **Firmware v1.0.1** now appears on your Releases page.

6. Every device that's online installs it **within 6 hours**. To update one right away, open the app → **Device** tab → **Check for update**. The round screen shows a progress ring, then the device reboots with the new version.

If the power cuts out or Wi-Fi drops during an update, nothing breaks. The device keeps running the old version and tries again later.

---

## If something goes wrong

| What you see | What to do |
|---|---|
| Red ✗ in Actions | Click the run, then the red step. The message at the bottom says what's wrong. The common ones are below. |
| *"OTA_MANIFEST_URL in Config.h must be exactly: ..."* | Copy the URL from that message into `Config.h`, then commit, push and run again. |
| *"Version 1.0.1 was already published"* | Increase `FW_VERSION` again, then commit, push and run again. |
| App finds no device | Check that Bluetooth is on and the board is powered. On older phones (Android 11 or below), Location must also be on. |
| Wrong PIN 3 times | The phone gets disconnected. Tap Connect again; a new code appears. |
| Device never updates | Was the firmware you flashed over USB built with the correct URL from Part A step 6? If not, fix it and flash over USB once more. |
| *"No room for update"* on screen | Wrong partition scheme. Re-flash over USB with the setting from Part C. |
| *"App not installed"* | Uninstall the old Cute Eyes app first. |

**iPhone:** this guide covers Android. Building for iPhone requires a Mac and a paid Apple developer account. The app code supports it if you ever need it.

**Changing the app later:** if you (or I) change files in `mobile_app`, copy them into the `cute-eyes\mobile_app` folder, then commit and push in GitHub Desktop. GitHub rebuilds the app automatically, and the new APK appears on the Releases page.
