# IRIS Android app: step-by-step guide (first-time setup)

> Already set up? To upgrade to IRIS 2.0, follow **[IRIS_GUIDE.md](IRIS_GUIDE.md)**. The app file is now called **IRIS.apk**.

This guide takes you from "nothing" to "the app is on my phone and talks to my device".
You don't need to install any programming tools. **GitHub builds the app for you, for free.**

**What you need:**
- A Windows PC with internet
- Your Android phone (Android 8 or newer)
- Your Cute Eyes device, powered on, running the latest firmware

**Time:** about 30 minutes the first time, mostly waiting.

---

## Words you'll see (plain English)

| Word | Means |
|---|---|
| **GitHub** | A free website that stores your project and builds the app for you |
| **Repository** ("repo") | Your project folder, as stored on GitHub |
| **GitHub Desktop** | A free Windows program that copies your folder to GitHub with a few clicks |
| **Commit** | "Save a snapshot of my changes" (you type a short note about what changed) |
| **Push** / **Publish** | "Send my snapshots up to GitHub" |
| **Actions** | GitHub's free robot that builds the app every time you push |
| **Release** | A download page on GitHub where the finished app (`CuteEyes.apk`) appears |
| **APK** | The Android app file you install on your phone |

---

## Part 1: Create a free GitHub account (5 min)

> **Already have a GitHub account?** Skip to Part 2.

1. On your PC, open **https://github.com/signup**
2. Enter your email, then click **Continue**.
3. Create a password and click **Continue**.
4. Choose a **username**, for example `jaini-dev`. **Write it down.** Click **Continue**.
5. Solve the little puzzle GitHub shows you, then click **Create account**.
6. GitHub emails you a code. Open your email, copy the code, and paste it into GitHub.
7. If GitHub asks questions about how you'll use it, you can click **Skip personalization**.
8. Choose the **Free** plan if asked. You never need to pay for anything in this guide.

---

## Part 2: Install GitHub Desktop (5 min)

GitHub Desktop is the "copy my folder to GitHub" program. You never have to type Git commands.

1. Open **https://desktop.github.com** and click **Download for Windows**.
2. Run the downloaded file (`GitHubDesktopSetup-x64.exe`). It installs by itself and opens.
3. Click **Sign in to GitHub.com**. Your web browser opens. Click **Authorize desktop**, and if the browser asks "Open GitHub Desktop?", click **Open**.
4. Back in GitHub Desktop, on the **Configure Git** screen, leave the name and email as they are and click **Finish**.

Official help, if a screen looks different: https://docs.github.com/en/desktop/installing-and-authenticating-to-github-desktop

---

## Part 3: Put the project on GitHub (10 min)


1. In GitHub Desktop, click **File** (top-left), then **New repository...**
2. Fill in:
   - **Name:** `cute-eyes`
   - **Local path:** leave it as it is (usually `C:\Users\<you>\Documents\GitHub`)
   - Leave everything else as it is.
3. Click **Create repository**.
4. Click **Repository** (top menu), then **Show in Explorer**. A Windows folder opens. This is your project folder: `Documents\GitHub\cute-eyes`.
5. **Unzip `CuteEyes_Project.zip`.** Right-click it, choose **Extract All...**, then **Extract**.
6. Open the extracted `CuteEyes_Project` folder. Press **Ctrl + A** to select everything, then **Ctrl + C** to copy.
7. Go back to the `cute-eyes` folder from step 4, click inside it, and press **Ctrl + V** to paste.
8. **Check** that the `cute-eyes` folder now contains these folders and files:
   ```
   .github
   android_app
   firmware
   APP_GUIDE.md
   GETTING_STARTED.md
   README.md
   ```
   ⚠️ **`.github` must be there.** It contains the instructions for building the app.
9. Go back to GitHub Desktop. On the left you'll see a long list of files. That's normal.
10. At the bottom-left, in the **Summary** box, type: `First upload`
11. Click the blue **Commit to main** button.
12. Click **Publish repository** (the blue button at the top).
13. In the window that opens:
    - **Untick** "Keep this code private". Public repos get unlimited free builds, and the device's OTA updates need it public anyway.
    - Click **Publish repository**.
14. Done. Your project is on GitHub at `https://github.com/YOUR_USERNAME/cute-eyes`

---

## Part 4: Watch GitHub build your app (5–10 min of waiting)

1. In your browser, open `https://github.com/YOUR_USERNAME/cute-eyes`
2. Click the **Actions** tab at the top.
   - If you see a green button **"I understand my workflows, go ahead and enable them"**, click it.
3. On the left, click **Build Android app**.
   - A run should already be in the list, with a yellow dot (🟡) while it runs.
   - **If the list is empty:** click **Run workflow** (right side), then the green **Run workflow** button.
4. Wait. The first build takes about 5–8 minutes. You can close the page and come back.
5. You'll see one of two results:
   - ✅ **Green tick:** success! Go to Part 5.
   - ❌ **Red cross:** something needs fixing. Send it to me:
     1. Click the run with the red cross.
     2. Click **build** on the left.
     3. Click the step with the red cross to open it.
     4. Scroll to the bottom. Copy the **last 30–40 lines**, or take a screenshot.
     5. Paste it in our chat. I'll send back the fixed files.

> I checked all the Bluetooth and app logic with the official Kotlin compiler against real Android 14 code. The screen code can only be fully checked on GitHub, so a first-build hiccup is possible. That's exactly what step 5 is for.

---

## Part 5: Install the app on your phone (5 min)

1. **On your Android phone**, open Chrome and go to:
   `https://github.com/YOUR_USERNAME/cute-eyes/releases`
2. Find **Android app (build 1)** (the number may differ) and tap **CuteEyes.apk** under **Assets**.
   If you don't see Assets, tap **Assets** to expand it.
3. Chrome may warn *"File might be harmful"*. Tap **Download anyway**. It's your own app.
4. When it finishes, tap **Open**, or find `CuteEyes.apk` in your Downloads.
5. Android will say *"For your security, your phone is not allowed to install unknown apps from this source"*:
   1. Tap **Settings**.
   2. Turn on **Allow from this source**.
   3. Press **Back**.
   4. Tap **Install**.
6. If **Google Play Protect** shows a warning (*"Unsafe app blocked"* or *"App scan recommended"*):
   - Tap **More details**, then **Install anyway**, or **Install without scanning**.
   - This happens to every app that isn't from the Play Store. It's expected.
7. Tap **Open**. You'll see the blinking eyes logo and **"Hi there!"** 🎉

Different phone brands word these screens slightly differently, but the steps are the same.

---

## Part 6: Connect to your device

1. Make sure your **Cute Eyes device is on** and close to the phone.
2. In the app, tap **Allow**, then allow **Nearby devices** (Bluetooth) access.
   - On Android 11 or older it asks for **Location** instead, and Location must be switched on. That's an old-Android rule for Bluetooth scanning.
3. If Bluetooth is off, tap **Turn on**.
4. Tap **🔍 Find my device**. Your device appears as **CuteEyes-XXXX**. Tap **Connect**.
5. A **6-digit code** appears on the round screen. Type it in the app and tap **Pair**.
6. You're in! Two tabs appear at the bottom:
   - **🏠 Device:** firmware version, Wi-Fi status, IP address, and check for update.
   - **📶 Wi-Fi:** tap **Find networks**, tap your network, type the password, and tap **Connect**. Saved networks have a **⋮** menu for Connect now / Change password / Forget.
7. Next time you open the app, your device appears at the top as **⭐ Last used**. Tap **Connect** and enter the new code from the screen. The device shows a new code on every connection, as a security feature.

---

## Part 7: When I send you an updated app

1. Unzip the new files and copy them into your `Documents\GitHub\cute-eyes` folder (choose **Replace**).
2. In GitHub Desktop, type a **Summary** (e.g. `App update`), click **Commit to main**, then **Push origin**.
3. Wait for the green tick under **Actions → Build Android app**.
4. On your phone, open the Releases page again, download the new `CuteEyes.apk` and install it.
   It **installs over the old version**; you don't need to uninstall first. Every build is signed with the same key, which is included in the project (`android_app/app/cuteeyes-dev.jks`).

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Actions tab shows no **Build Android app** | The `.github` folder wasn't copied. Copy it from the zip into `cute-eyes`, then commit and push (Part 7, step 2). |
| Red cross in Actions | Copy the last 30–40 lines of the failed step and send them to me (Part 4, step 5). |
| Releases page has no **Android app** | The build hasn't finished or it failed. Check Actions. |
| "App not installed" | An old version signed with a different key is installed. Uninstall Cute Eyes once, then install again. After that, updates install normally. |
| App finds nothing | Is the device on? Did you allow Nearby devices? Is Bluetooth on? Android 11 or older: is Location on? Is another phone already connected to the device? It talks to one phone at a time. |
| "Couldn't connect (error 133)" | A common Android Bluetooth hiccup. The app already retries once automatically. Wait 5 seconds and tap Connect again. Toggling Bluetooth off/on fixes stubborn cases. |
| Wrong code 3 times | The device disconnects you. Tap Connect again; a new code appears. |
| Wi-Fi network not in the list | The ESP32 only sees **2.4 GHz** networks. Use **+ Hidden** for hidden networks. |

---

## Proven references (for when we're stuck)

These are the official and widely used sources this app follows:

- **Android Bluetooth Low Energy (official):** https://developer.android.com/develop/connectivity/bluetooth/ble/ble-overview
- **Connecting to a BLE device (official):** https://developer.android.com/develop/connectivity/bluetooth/ble/connect-gatt-server
- **Bluetooth permissions (official):** https://developer.android.com/develop/connectivity/bluetooth/bt-permissions
- **The Ultimate Guide to Android BLE (Punch Through):** https://punchthrough.com/android-ble-guide/
- **Jetpack Compose (official):** https://developer.android.com/develop/ui/compose
- **GitHub Desktop help (official):** https://docs.github.com/en/desktop
- **GitHub Actions / Gradle build action:** https://github.com/gradle/actions
- **Installing apps from outside the Play Store:** search "install unknown apps" plus your phone brand.
