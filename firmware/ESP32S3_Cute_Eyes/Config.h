#pragma once
/*
 * IRIS - configuration
 * (Named after Iris, the rainbow messenger of the Greek gods.)
 */

// ===================== Firmware identity =====================
// Bump this every time you publish an OTA update (must be higher than the last one).
#define FW_VERSION        "2.0.0"

// Devices only accept OTA manifests whose "device" field matches this.
#define OTA_DEVICE_TYPE   "cute-eyes-s3"

// ===================== Cloud OTA =====================
// Where the device looks for updates. With GitHub Releases, "latest/download/"
// always points to the newest release, so this URL never needs to change.
#define OTA_MANIFEST_URL  "https://github.com/YOUR_USER/YOUR_REPO/releases/latest/download/manifest.json"

// How often to check for updates while online (first check is ~10 s after Wi-Fi connects)
#define OTA_CHECK_INTERVAL_MS  (6UL * 60UL * 60UL * 1000UL)   // 6 hours

// For production, paste your host's root CA certificate here to verify the server:
// #define OTA_ROOT_CA "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n"

// ===================== Bluetooth =====================
// Advertised name becomes e.g. "IRIS-3F2A"
#define BLE_NAME_PREFIX   "IRIS"

// ===================== Developer section of the app =====================
// Device details, logs, every expression, firmware upload and reboot.
#define DEV_USER          "admin"
#define DEV_PASS          "cuteeyes"

// ===================== Internal (don't edit) =====================
// Which face is on the screen. Defined here so Arduino IDE's automatic
// function declarations at the top of the sketch can see it.
enum class Face { Eyes, Clock, Off };
