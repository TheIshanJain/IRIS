/*
 * CloudOTA - pull-based, over-the-internet firmware updates for ESP32.
 *
 * How it works:
 *   1. You host a small JSON "manifest" anywhere on the internet
 *      (GitHub Releases, S3, Firebase Hosting, your own server...):
 *
 *        {
 *          "device":  "cute-eyes-s3",
 *          "version": "1.0.1",
 *          "url":     "https://example.com/firmware-1.0.1.bin",
 *          "md5":     "9e107d9d372bb6826bd81d3542a419d6",
 *          "notes":   "What changed"
 *        }
 *
 *   2. Every device periodically downloads the manifest. If "version" is newer
 *      than the running firmware, it downloads the .bin, verifies it and
 *      flashes it into the spare OTA partition, then reboots.
 *
 * Because the device *pulls* the update, it works from anywhere on the planet:
 * no port forwarding, no static IP, no being on the same network.
 *
 * Requirements: an OTA-capable partition scheme (two app slots), ArduinoJson v7.
 */
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <functional>
#include <memory>

class CloudOTA {
public:
  enum class State { Idle, Checking, UpToDate, Downloading, Success, Failed };

  using ProgressCallback = std::function<void(int percent)>;
  using StateCallback    = std::function<void(State state, const String &message)>;

  // Required. currentVersion like "1.0.0", manifestUrl = http(s) URL of the JSON manifest.
  void begin(const char *currentVersion, const char *manifestUrl);

  // How often to check while Wi-Fi is up (0 = only when requested). Default 6 hours.
  void setCheckInterval(uint32_t ms) { _interval = ms; }

  // Delay after Wi-Fi connects before the first automatic check. Default 10 s.
  void setFirstCheckDelay(uint32_t ms) { _firstDelay = ms; }

  // TLS: verify the server with this root CA (recommended for production)...
  void setRootCA(const char *pem) { _rootCA = pem; }
  // ...or skip certificate verification (easy to start with, NOT secure).
  void setInsecure() { _rootCA = nullptr; }

  // Optional: only accept manifests whose "device" field matches.
  void setDeviceType(const char *type) { _deviceType = type ? type : ""; }

  // Reboot automatically after a successful update. Default true.
  void setAutoReboot(bool on) { _autoReboot = on; }

  void onProgress(ProgressCallback cb) { _onProgress = cb; }
  void onStateChange(StateCallback cb) { _onState = cb; }

  // Call from loop(). Performs scheduled / requested checks (blocking while it runs).
  void loop();

  // Ask for a check on the next loop() call. Safe to call from any callback.
  void requestCheck() { _checkRequested = true; }

  // Check right now (blocking). Downloads and installs if a newer version exists.
  State checkNow();

  State state() const { return _state; }
  const String &latestVersion() const { return _latest; }
  const String &lastMessage() const { return _message; }
  const char *currentVersion() const { return _current; }

  static int compareVersions(const String &a, const String &b);  // -1, 0, 1
  static const char *stateName(State s);

private:
  bool fetchManifest(String &version, String &url, String &md5);
  bool downloadAndFlash(const String &url, const String &md5);
  WiFiClient *makeClient(const String &url, std::unique_ptr<WiFiClient> &holder);
  void setState(State s, const String &msg);

  const char *_current     = "0.0.0";
  String      _manifestUrl;
  String      _deviceType;
  const char *_rootCA      = nullptr;
  uint32_t    _interval    = 6UL * 60UL * 60UL * 1000UL;
  uint32_t    _firstDelay  = 10000;
  bool        _autoReboot  = true;

  State    _state          = State::Idle;
  String   _latest;
  String   _message;
  String   _error;
  bool     _busy           = false;
  bool     _checkRequested = false;
  bool     _everChecked    = false;
  uint32_t _lastCheck      = 0;
  uint32_t _connectedSince = 0;

  ProgressCallback _onProgress;
  StateCallback    _onState;
};
