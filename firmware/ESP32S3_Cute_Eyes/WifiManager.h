/*
 * WifiManager.h - stores up to MAX_SAVED_NETWORKS Wi-Fi networks in flash (NVS)
 * and keeps the device connected without ever blocking the animation loop.
 *
 *  - After a reboot it first tries the network it was last connected to,
 *    then the others (most recently added first). Passwords are kept in flash.
 *  - If the current network drops, it cycles through the saved ones.
 *  - Async scans for the web UI / mobile app "nearby networks" list.
 *  - Its own access point stays on the whole time (AP + STA mode), so the
 *    web UI is always reachable at http://192.168.4.1
 */
#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include <functional>

#define MAX_SAVED_NETWORKS   5
#define WIFI_ATTEMPT_MS      12000   // time to wait for one network
#define WIFI_ROUND_PAUSE_MS  20000   // pause after trying all networks

struct SavedNet {
  String ssid;
  String pass;
};

class WifiManager {
public:
  enum class Link { NoNetworks, Connecting, Connected, Disconnected };

  std::function<void()>      onStateChange;   // connected / disconnected / attempting
  std::function<void(int n)> onScanDone;      // read results with WiFi.SSID(i) etc.

  void begin(const char *hostname, const char *apSsid, const char *apPass) {
    load();
    WiFi.persistent(false);          // we store credentials ourselves
    WiFi.setHostname(hostname);
    WiFi.mode(WIFI_AP_STA);          // access point + station at the same time
    WiFi.setAutoReconnect(false);    // we handle reconnection/round-robin
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    if (WiFi.softAP(apSsid, apPass, 1, 0, 4)) {
      Log.printf("[WiFi] Access point '%s' up at %s\n", apSsid, WiFi.softAPIP().toString().c_str());
    } else {
      Log.println("[WiFi] Failed to start access point");
    }
    _nextAttemptAt = millis() + 1000;
  }

  void loop() {
    uint32_t now = millis();

    // ---- async scan in progress ----
    if (_scanning) {
      int16_t n = WiFi.scanComplete();
      if (n == WIFI_SCAN_RUNNING) return;
      _scanning = false;
      if (onScanDone) onScanDone(n < 0 ? 0 : n);
      WiFi.scanDelete();
      _nextAttemptAt = now + 300;
    }

    // ---- user asked to switch network ----
    if (_forceIndex >= 0) {
      int i = _forceIndex;
      _forceIndex = -1;
      if (i < _count) {
        _tryIndex = i;
        attempt(i);
      }
      return;
    }

    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected != _wasConnected) {
      _wasConnected = connected;
      if (connected) {
        _attempting = false;
        if (WiFi.SSID() != _last) {        // remember for the next boot
          _last = WiFi.SSID();
          save();
        }
        Log.printf("[WiFi] Connected to '%s', IP %s, %d dBm\n", WiFi.SSID().c_str(),
                   WiFi.localIP().toString().c_str(), WiFi.RSSI());
      } else {
        Log.println("[WiFi] Disconnected");
      }
      fire();
    }
    if (connected || _count == 0) return;

    // ---- currently trying a network ----
    if (_attempting) {
      if (now - _attemptStart < WIFI_ATTEMPT_MS) return;
      _attempting = false;                 // timed out, move on
      Log.printf("[WiFi] '%s' did not connect\n", _nets[_tryIndex].ssid.c_str());
      _tryIndex++;
      if (_tryIndex >= _count) {
        _tryIndex = 0;
        _nextAttemptAt = now + WIFI_ROUND_PAUSE_MS;
      }
      fire();
    }

    if ((int32_t)(now - _nextAttemptAt) < 0) return;
    attempt(_tryIndex);
  }

  // Add a network or update its password. It becomes the top priority.
  bool add(const String &ssid, const String &pass) {
    int existing = indexOf(ssid);
    if (existing >= 0) {
      for (int i = existing; i > 0; i--) _nets[i] = _nets[i - 1];
    } else {
      int last = min(_count, MAX_SAVED_NETWORKS - 1);
      for (int i = last; i > 0; i--) _nets[i] = _nets[i - 1];
      if (_count < MAX_SAVED_NETWORKS) _count++;
    }
    _nets[0] = { ssid, pass };
    save();
    return true;
  }

  bool remove(const String &ssid) {
    int idx = indexOf(ssid);
    if (idx < 0) return false;
    bool wasCurrent = (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) ||
                      (_attempting && _tryIndex == idx);
    for (int i = idx; i < _count - 1; i++) _nets[i] = _nets[i + 1];
    _count--;
    save();
    if (wasCurrent) {
      WiFi.disconnect();
      _attempting = false;
      _tryIndex = 0;
      _nextAttemptAt = millis() + 500;
    }
    if (_tryIndex >= _count) _tryIndex = 0;
    fire();
    return true;
  }

  void connectTo(const String &ssid) {
    int idx = indexOf(ssid);
    if (idx >= 0) _forceIndex = idx;
  }

  void startScan() {
    if (_scanning) return;
    if (WiFi.status() != WL_CONNECTED && _attempting) {
      WiFi.disconnect();          // a pending connect attempt blocks scanning
      _attempting = false;
    }
    if (WiFi.scanNetworks(true, false) == WIFI_SCAN_FAILED) {
      if (onScanDone) onScanDone(0);
      return;
    }
    _scanning = true;
  }

  Link link() const {
    if (WiFi.status() == WL_CONNECTED) return Link::Connected;
    if (_count == 0) return Link::NoNetworks;
    if (_attempting) return Link::Connecting;
    return Link::Disconnected;
  }

  // Connected SSID, or the one currently being tried
  String currentSsid() const {
    if (WiFi.status() == WL_CONNECTED) return WiFi.SSID();
    if (_attempting && _tryIndex < _count) return _nets[_tryIndex].ssid;
    return "";
  }

  int count() const { return _count; }
  const SavedNet &net(int i) const { return _nets[i]; }
  bool scanning() const { return _scanning; }

private:
  void attempt(int i) {
    if (i >= _count) return;
    WiFi.disconnect();
    const SavedNet &n = _nets[i];
    WiFi.begin(n.ssid.c_str(), n.pass.length() ? n.pass.c_str() : nullptr);
    _attempting   = true;
    _attemptStart = millis();
    Log.printf("[WiFi] Trying '%s'\n", n.ssid.c_str());
    fire();
  }

  int indexOf(const String &ssid) const {
    for (int i = 0; i < _count; i++)
      if (_nets[i].ssid == ssid) return i;
    return -1;
  }

  void load() {
    _prefs.begin("wifinets", true);
    _count = min((int)_prefs.getUChar("n", 0), MAX_SAVED_NETWORKS);
    for (int i = 0; i < _count; i++) {
      _nets[i].ssid = _prefs.getString(("s" + String(i)).c_str(), "");
      _nets[i].pass = _prefs.getString(("p" + String(i)).c_str(), "");
    }
    _last = _prefs.getString("last", "");
    _prefs.end();
    int li = indexOf(_last);
    _tryIndex = li >= 0 ? li : 0;          // start with the last connected network
    if (_count) Log.printf("[WiFi] %d saved network(s), trying '%s' first\n", _count, _nets[_tryIndex].ssid.c_str());
  }

  void save() {
    _prefs.begin("wifinets", false);
    _prefs.clear();
    _prefs.putUChar("n", _count);
    for (int i = 0; i < _count; i++) {
      _prefs.putString(("s" + String(i)).c_str(), _nets[i].ssid);
      _prefs.putString(("p" + String(i)).c_str(), _nets[i].pass);
    }
    _prefs.putString("last", _last);
    _prefs.end();
  }

  void fire() {
    if (onStateChange) onStateChange();
  }

  Preferences _prefs;
  SavedNet _nets[MAX_SAVED_NETWORKS];
  int      _count         = 0;
  int      _tryIndex      = 0;
  int      _forceIndex    = -1;
  bool     _attempting    = false;
  bool     _wasConnected  = false;
  bool     _scanning      = false;
  uint32_t _attemptStart  = 0;
  uint32_t _nextAttemptAt = 0;
  String   _last;                          // last successfully connected SSID
};
