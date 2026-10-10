/*
 * WebUI.h - web server for status, Wi-Fi setup, eye control, firmware upload and logs.
 *
 * Two levels:
 *   - Everyday controls (Home, Wi-Fi, Style, Settings): no login needed.
 *   - Developer (device details, logs, any expression, firmware upload, reboot):
 *     unlocked with WEB_USER / WEB_PASS from Config.h, which returns a session
 *     token the page sends in the "X-Dev-Token" header.
 *
 * Reachable at:
 *   http://192.168.4.1      when your phone/laptop is on the device's own Wi-Fi (always on)
 *   http://<device IP>      or  http://cuteeyes-xxxx.local  from your home network
 *
 * Included from the main sketch AFTER all globals (wifi, ble, ota, imu, settings...).
 */
#pragma once

#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_system.h>
#include "WebPage.h"

WebServer server(80);
DNSServer dnsServer;
String    mdnsName;
uint32_t  rebootAt = 0;

// ---- firmware upload state ----
String  upErr;
bool    upFirst = true;
size_t  upTotal = 0, upDone = 0;
int     upLastPct = -1;

// =====================================================================
//                              HELPERS
// =====================================================================

// ---- developer session tokens (cleared on reboot) ----
#define DEV_TOKENS 4
String   devTokens[DEV_TOKENS];
int      devTokenNext = 0;
uint8_t  loginFails = 0;
uint32_t loginBlockedUntil = 0;

bool devOk() {
#if WEB_AUTH_ENABLED
  String t = server.header("X-Dev-Token");
  if (t.length() < 16) return false;
  for (auto &d : devTokens)
    if (d.length() && d == t) return true;
  return false;
#else
  return true;
#endif
}

void sendJson(JsonDocument &d, int code = 200) {
  String s;
  serializeJson(d, s);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", s);
}

void sendMsg(bool ok, const String &msg, int code = 0) {
  JsonDocument d;
  d["ok"]  = ok;
  d["msg"] = msg;
  sendJson(d, code ? code : (ok ? 200 : 400));
}

bool readBody(JsonDocument &d) {
  if (!server.hasArg("plain")) return false;
  return deserializeJson(d, server.arg("plain")) == DeserializationError::Ok;
}

// Developer-only endpoints: 401 JSON (no browser login pop-up)
bool checkDev() {
  if (devOk()) return true;
  sendMsg(false, "Developer login required", 401);
  return false;
}

const char *resetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "Power on";
    case ESP_RST_SW:       return "Software restart";
    case ESP_RST_PANIC:    return "Crash (panic)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "Watchdog";
    case ESP_RST_BROWNOUT: return "Brownout (low power)";
    case ESP_RST_EXT:      return "Reset button";
    case ESP_RST_DEEPSLEEP:return "Wake from sleep";
    default:               return "Other";
  }
}

// =====================================================================
//                              HANDLERS
// =====================================================================

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  if (!checkDev()) return;
  bool up = wifi.link() == WifiManager::Link::Connected;
  JsonDocument d;
  d["name"]   = deviceName;
  d["fw"]     = FW_VERSION;
  d["uptime"] = millis() / 1000;
  d["heap"]   = ESP.getFreeHeap();
  d["minHeap"]= ESP.getMinFreeHeap();
  d["psram"]  = ESP.getFreePsram();
  d["temp"]   = temperatureRead();
  d["reset"]  = resetReason();
  d["sdk"]    = String(ESP_ARDUINO_VERSION_MAJOR) + "." + ESP_ARDUINO_VERSION_MINOR + "." +
                ESP_ARDUINO_VERSION_PATCH + " / " + ESP.getSdkVersion();
  d["mdns"]   = mdnsName + ".local";

  JsonObject sta = d["sta"].to<JsonObject>();
  sta["state"] = wifiStateName();
  sta["ssid"]  = wifi.currentSsid();
  sta["ip"]    = up ? WiFi.localIP().toString() : "";
  sta["rssi"]  = up ? WiFi.RSSI() : 0;
  sta["mac"]   = WiFi.macAddress();

  JsonObject ap = d["ap"].to<JsonObject>();
  ap["ssid"]    = deviceName;
  ap["ip"]      = WiFi.softAPIP().toString();
  ap["clients"] = WiFi.softAPgetStationNum();
  ap["channel"] = WiFi.channel();

  JsonObject b = d["ble"].to<JsonObject>();
  b["connected"] = ble.isConnected();
  b["paired"]    = ble.isAuthed();

  JsonObject im = d["imu"].to<JsonObject>();
  im["ok"]     = imu.ok();
  im["ax"]     = imu.ax;
  im["ay"]     = imu.ay;
  im["az"]     = imu.az;
  im["g"]      = imu.magnitude();
  im["shakes"] = imu.shakeCount;

  JsonObject e = d["eyes"].to<JsonObject>();
  e["index"] = eyesEngine.current;
  e["name"]  = eyesEngine.name(eyesEngine.current);
  e["theme"] = eyesEngine.theme(eyesEngine.current);
  e["auto"]  = autoCycle;
  e["count"] = eyesEngine.count();

  JsonObject s = d["settings"].to<JsonObject>();
  s["brightness"]  = settings.brightness;
  s["labels"]      = settings.labels;
  s["shake"]       = settings.shake;
  s["sens"]        = settings.sens;
  s["shakeRandom"] = settings.shakeRandom;

  JsonObject o = d["ota"].to<JsonObject>();
  o["state"]  = CloudOTA::stateName(ota.state());
  o["latest"] = ota.latestVersion();
  o["msg"]    = ota.lastMessage();

  s["name"]          = settings.name;
  s["screenTimeout"] = settings.screenTimeout;
  s["clockAfter"]    = settings.clockAfter;
  s["city"]          = weather.manualCity();
  s["themes"]        = settings.themes;

  // ---- time ----
  JsonObject c = d["clock"].to<JsonObject>();
  c["valid"] = timeValid();
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char b[40];
    strftime(b, sizeof(b), "%H:%M:%S", &t);
    c["time"] = b;
    strftime(b, sizeof(b), "%A, %d %B %Y", &t);
    c["date"] = b;
  }
  WeatherData w = weather.snapshot();
  long off = w.utcOffset, a = labs(off);
  char tz[16];
  snprintf(tz, sizeof(tz), "UTC%c%02ld:%02ld", off >= 0 ? '+' : '-', a / 3600, (a % 3600) / 60);
  c["tz"] = tz;
  c["screen"] = screenOff ? "off" : (currentFace() == Face::Clock ? "clock" : "eyes");

  // ---- location + weather ----
  JsonObject wx = d["weather"].to<JsonObject>();
  wx["haveLoc"] = w.haveLocation;
  String loc = w.city;
  if (w.region.length() && w.region != w.city) loc += ", " + w.region;
  if (w.country.length()) loc += ", " + w.country;
  wx["loc"]     = loc;
  wx["manual"]  = w.manual;
  wx["greet"]   = currentGreetingWord();
  wx["have"]    = w.haveWeather;
  if (w.haveWeather) {
    wx["temp"]    = w.temp;
    wx["feels"]   = w.feels;
    wx["hum"]     = w.humidity;
    wx["wind"]    = w.wind;
    wx["tmax"]    = w.tmax;
    wx["tmin"]    = w.tmin;
    wx["text"]    = weatherText(w.code);
    wx["sunrise"] = w.sunrise;
    wx["sunset"]  = w.sunset;
    wx["age"]     = (millis() - w.updatedMs) / 1000;
  }
  wx["err"] = w.error;

  sendJson(d);
}

// Everyday status: only what the consumer screens need
void handleStatusLite() {
  bool up = wifi.link() == WifiManager::Link::Connected;
  JsonDocument d;
  d["name"]    = deviceName;
  d["fw"]      = FW_VERSION;
  d["devOpen"] = WEB_AUTH_ENABLED == 0;

  JsonObject sta = d["sta"].to<JsonObject>();
  sta["state"] = wifiStateName();
  sta["ssid"]  = wifi.currentSsid();
  sta["rssi"]  = up ? WiFi.RSSI() : 0;

  JsonObject b = d["ble"].to<JsonObject>();
  b["connected"] = ble.isConnected();
  b["paired"]    = ble.isAuthed();

  JsonObject e = d["eyes"].to<JsonObject>();
  e["index"] = eyesEngine.current;
  e["name"]  = eyesEngine.name(eyesEngine.current);
  e["theme"] = eyesEngine.theme(eyesEngine.current);
  e["auto"]  = autoCycle;
  e["count"] = eyesEngine.count();

  JsonObject s = d["settings"].to<JsonObject>();
  s["brightness"]    = settings.brightness;
  s["labels"]        = settings.labels;
  s["shake"]         = settings.shake;
  s["sens"]          = settings.sens;
  s["shakeRandom"]   = settings.shakeRandom;
  s["name"]          = settings.name;
  s["screenTimeout"] = settings.screenTimeout;
  s["clockAfter"]    = settings.clockAfter;
  s["city"]          = weather.manualCity();
  s["themes"]        = settings.themes;

  JsonObject c = d["clock"].to<JsonObject>();
  c["valid"]  = timeValid();
  c["screen"] = screenOff ? "off" : (currentFace() == Face::Clock ? "clock" : "eyes");
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char buf[24];
    strftime(buf, sizeof(buf), "%H:%M", &t);
    c["time"] = buf;
    strftime(buf, sizeof(buf), "%a, %d %b", &t);
    c["date"] = buf;
  }

  WeatherData w = weather.snapshot();
  JsonObject wx = d["weather"].to<JsonObject>();
  wx["haveLoc"] = w.haveLocation;
  String loc = w.city;
  if (w.region.length() && w.region != w.city) loc += ", " + w.region;
  wx["loc"]    = loc;
  wx["manual"] = w.manual;
  wx["greet"]  = currentGreetingWord();
  wx["have"]   = w.haveWeather;
  if (w.haveWeather) {
    wx["temp"]    = w.temp;
    wx["feels"]   = w.feels;
    wx["hum"]     = w.humidity;
    wx["wind"]    = w.wind;
    wx["tmax"]    = w.tmax;
    wx["tmin"]    = w.tmin;
    wx["text"]    = weatherText(w.code);
    wx["code"]    = w.code;
    wx["isDay"]   = w.isDay;
    wx["sunrise"] = w.sunrise;
    wx["sunset"]  = w.sunset;
  }

  JsonObject o = d["ota"].to<JsonObject>();
  o["state"]  = CloudOTA::stateName(ota.state());
  o["latest"] = ota.latestVersion();
  o["msg"]    = ota.lastMessage();

  sendJson(d);
}

void handleDevLogin() {
  JsonDocument in;
  readBody(in);
#if WEB_AUTH_ENABLED
  if ((int32_t)(millis() - loginBlockedUntil) < 0) return sendMsg(false, "Too many tries - wait 30 seconds", 429);
  String u = in["user"] | "";
  String p = in["pass"] | "";
  if (u != WEB_USER || p != WEB_PASS) {
    if (++loginFails >= 5) {
      loginFails = 0;
      loginBlockedUntil = millis() + 30000;
    }
    Log.println("[Web] Developer login failed");
    return sendMsg(false, "Wrong username or password", 401);
  }
  loginFails = 0;
#endif
  char tok[33];
  for (int i = 0; i < 4; i++) snprintf(tok + i * 8, 9, "%08lx", (unsigned long)esp_random());
  devTokens[devTokenNext] = tok;
  devTokenNext = (devTokenNext + 1) % DEV_TOKENS;
  Log.println("[Web] Developer unlocked");
  JsonDocument d;
  d["ok"]    = true;
  d["token"] = tok;
  sendJson(d);
}

void handleDevLogout() {
  String t = server.header("X-Dev-Token");
  for (auto &d : devTokens)
    if (d.length() && d == t) d = "";
  sendMsg(true, "Developer locked");
}

void handleExpressions() {
  JsonDocument d;
  JsonArray a = d["names"].to<JsonArray>();
  JsonArray th = d["themes"].to<JsonArray>();
  for (int i = 0; i < eyesEngine.count(); i++) {
    a.add(eyesEngine.name(i));
    th.add(eyesEngine.theme(i));
  }
  JsonArray tn = d["themeNames"].to<JsonArray>();
  JsonArray td = d["themeDesc"].to<JsonArray>();
  for (int i = 0; i < TH_COUNT; i++) {
    tn.add(THEME_NAMES[i]);
    td.add(THEME_DESC[i]);
  }
  sendJson(d);
}

void handleExpr() {
  JsonDocument in;
  if (!readBody(in)) return sendMsg(false, "Bad request");
  if (in["next"] | false) {
    eyesEngine.next();
  } else {
    if (!checkDev()) return;             // any expression from any theme: developer only
    int i = in["index"] | -1;
    if (i < 0 || i >= eyesEngine.count()) return sendMsg(false, "No such expression");
    eyesEngine.set(i);
  }
  Log.printf("[Web] Expression -> %s\n", eyesEngine.name(eyesEngine.current));
  sendMsg(true, eyesEngine.name(eyesEngine.current));
}

void handleSettings() {
  JsonDocument in;
  if (!readBody(in)) return sendMsg(false, "Bad request");
  if (in["brightness"].is<int>())  settings.brightness  = constrain(in["brightness"].as<int>(), 10, 255);
  if (in["labels"].is<bool>())     settings.labels      = in["labels"].as<bool>();
  if (in["shake"].is<bool>())      settings.shake       = in["shake"].as<bool>();
  if (in["sens"].is<int>())        settings.sens        = constrain(in["sens"].as<int>(), 1, 10);
  if (in["shakeRandom"].is<bool>())settings.shakeRandom = in["shakeRandom"].as<bool>();
  if (in["auto"].is<bool>())       settings.autoCycle   = in["auto"].as<bool>();
  if (in["name"].is<const char *>()) {
    String n = in["name"].as<String>();
    n.trim();
    if (n.length() > 16) n = n.substring(0, 16);
    settings.name = n;
    Log.printf("[Web] Name set to '%s'\n", n.c_str());
  }
  if (in["screenTimeout"].is<int>()) settings.screenTimeout = constrain(in["screenTimeout"].as<int>(), 0, 3600);
  if (in["clockAfter"].is<int>())    settings.clockAfter    = constrain(in["clockAfter"].as<int>(), 0, 3600);
  bool themesChanged = false;
  if (in["themes"].is<int>()) {
    int m = in["themes"].as<int>() & ((1 << TH_COUNT) - 1);
    if (!m) return sendMsg(false, "Keep at least one theme on");
    themesChanged = m != settings.themes;
    settings.themes = m;
  }
  if (in["city"].is<const char *>()) {
    weather.setManualCity(in["city"].as<String>());
    Log.printf("[Web] Location set to '%s'\n", weather.manualCity().length() ? weather.manualCity().c_str() : "automatic");
  }
  applySettings();
  saveSettings();
  if (themesChanged) {
    Log.printf("[Web] Themes mask 0x%02X\n", settings.themes);
    eyesEngine.ensureEnabled();      // switch away if the current theme was turned off
  }
  sendMsg(true, "Saved");
}

// ---- Wi-Fi ----
void handleScanStart() {
  wifi.startScan();
  sendMsg(true, "");
}

void handleScanGet() {
  JsonDocument d;
  d["scanning"] = wifi.scanning();
  JsonArray a = d["networks"].to<JsonArray>();
  for (auto &n : scanCache) {
    JsonObject o = a.add<JsonObject>();
    o["ssid"]   = n.ssid;
    o["rssi"]   = n.rssi;
    o["secure"] = n.secure;
  }
  sendJson(d);
}

void handleWifiList() {
  JsonDocument d;
  JsonArray a = d["networks"].to<JsonArray>();
  for (int i = 0; i < wifi.count(); i++) a.add(wifi.net(i).ssid);
  sendJson(d);
}

void handleWifiAdd() {
  JsonDocument in;
  if (!readBody(in)) return sendMsg(false, "Bad request");
  String ssid = in["ssid"] | "";
  String pass = in["pass"] | "";
  if (ssid.length() == 0 || ssid.length() > 32) return sendMsg(false, "Network name must be 1-32 characters");
  if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63))
    return sendMsg(false, "Password must be 8-63 characters");
  wifi.add(ssid, pass);
  wifi.connectTo(ssid);
  Log.printf("[Web] Saved Wi-Fi '%s'\n", ssid.c_str());
  sendList();
  sendMsg(true, "Saved '" + ssid + "', connecting...");
}

void handleWifiConnect() {
  JsonDocument in;
  if (!readBody(in)) return sendMsg(false, "Bad request");
  String ssid = in["ssid"] | "";
  wifi.connectTo(ssid);
  sendMsg(true, "Connecting to '" + ssid + "'...");
}

void handleWifiForget() {
  JsonDocument in;
  if (!readBody(in)) return sendMsg(false, "Bad request");
  String ssid = in["ssid"] | "";
  bool ok = wifi.remove(ssid);
  if (ok) Log.printf("[Web] Forgot Wi-Fi '%s'\n", ssid.c_str());
  sendList();
  sendMsg(ok, ok ? String("Forgot '") + ssid + "'" : String("Network not found"));
}

// ---- system ----
void handleLog() {
  if (!checkDev()) return;
  uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  JsonDocument d;
  JsonArray a = d["lines"].to<JsonArray>();
  uint32_t first = 0;
  Log.forEachSince(since, [&](uint32_t seq, uint32_t ms, const char *text) {
    if (!first) first = seq;
    JsonObject o = a.add<JsonObject>();
    o["t"] = ms;
    o["m"] = text;
  });
  d["first"] = first;
  d["last"]  = Log.lastSeq();
  sendJson(d);
}

void handleOtaCheck() {
  if (wifi.link() != WifiManager::Link::Connected) return sendMsg(false, "Device is not on the internet");
  ota.requestCheck();
  sendMsg(true, "Checking for updates...");
}

void handleGreet() {
  startGreeting();
  sendMsg(true, "Hello!");
}

void handleReboot() {
  if (!checkDev()) return;
  Log.println("[Web] Reboot requested");
  rebootAt = millis() + 800;
  sendMsg(true, "Rebooting...");
}

// ---- firmware upload (body arrives in chunks) ----
void handleUploadChunk() {
  HTTPUpload &u = server.upload();

  if (u.status == UPLOAD_FILE_START) {
    upErr = "";
    upFirst = true;
    upDone = 0;
    upLastPct = -1;
    upTotal = server.hasArg("size") ? server.arg("size").toInt() : 0;
    if (!devOk()) { upErr = "Developer login required"; return; }

    String fn = u.filename;
    fn.toLowerCase();
    if (fn.indexOf("merged") >= 0 || fn.indexOf("bootloader") >= 0 || fn.indexOf("partitions") >= 0) {
      upErr = "Use the plain .ino.bin file";
      return;
    }
    Log.printf("[Web] Firmware upload: %s (%u bytes)\n", u.filename.c_str(), (unsigned)upTotal);
    if (!Update.begin(upTotal > 0 ? upTotal : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      upErr = String("Cannot start update: ") + Update.errorString();
      return;
    }
    drawOtaProgress(0);

  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upErr.length()) return;
    if (upFirst) {
      upFirst = false;
      // ESP32 app image: 0xE9 header + app description magic 0xABCD5432 at offset 32
      const uint8_t *b = u.buf;
      bool looksOk = u.currentSize >= 36 && b[0] == 0xE9 &&
                     b[32] == 0x32 && b[33] == 0x54 && b[34] == 0xCD && b[35] == 0xAB;
      if (!looksOk) {
        upErr = "This is not an ESP32 app firmware file";
        Update.abort();
        return;
      }
    }
    if (Update.write(u.buf, u.currentSize) != u.currentSize) {
      upErr = String("Flash write failed: ") + Update.errorString();
      Update.abort();
      return;
    }
    upDone += u.currentSize;
    if (upTotal) {
      int pct = (int)((uint64_t)upDone * 100 / upTotal);
      if (pct != upLastPct) {
        upLastPct = pct;
        drawOtaProgress(pct);
      }
    }

  } else if (u.status == UPLOAD_FILE_END) {
    if (upErr.length()) return;
    if (!Update.end(true)) {
      upErr = String("Verify failed: ") + Update.errorString();
    } else {
      Log.printf("[Web] Firmware installed (%u bytes)\n", (unsigned)upDone);
    }

  } else if (u.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    upErr = "Upload aborted";
  }
}

void handleUploadDone() {
  if (!checkDev()) return;
  if (upErr.length() || !Update.isFinished()) {
    String e = upErr.length() ? upErr : String("Upload incomplete");
    Log.printf("[Web] Firmware update failed: %s\n", e.c_str());
    toast("Update failed", 3000);
    return sendMsg(false, e);
  }
  toast("Update done! Rebooting", 5000);
  rebootAt = millis() + 1500;
  sendMsg(true, "Installed, rebooting...");
}

// ---- captive portal: send phones/laptops on our Wi-Fi to the UI ----
void handleNotFound() {
  String host = server.hostHeader();
  bool ours = host == WiFi.softAPIP().toString() || host == WiFi.localIP().toString() ||
              host.startsWith(mdnsName);
  if (!ours) {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

// =====================================================================
//                              SETUP / LOOP
// =====================================================================

void webBegin() {
  // Every control (POST) from the web UI counts as activity: wakes the screen
  // and restarts the clock / screen-off timers. Status polling (GET) does not.
  auto control = [](void (*fn)()) {
    return [fn]() {
      markActivity();
      fn();
    };
  };
  server.on("/",                 HTTP_GET,  handleRoot);
  server.on("/api/status",       HTTP_GET,  handleStatusLite);
  server.on("/api/dev/status",   HTTP_GET,  handleStatus);
  server.on("/api/dev/login",    HTTP_POST, handleDevLogin);
  server.on("/api/dev/logout",   HTTP_POST, handleDevLogout);
  server.on("/api/expressions",  HTTP_GET,  handleExpressions);
  server.on("/api/expr",         HTTP_POST, control(handleExpr));
  server.on("/api/settings",     HTTP_POST, control(handleSettings));
  server.on("/api/scan",         HTTP_POST, control(handleScanStart));
  server.on("/api/scan",         HTTP_GET,  handleScanGet);
  server.on("/api/wifi",         HTTP_GET,  handleWifiList);
  server.on("/api/wifi/add",     HTTP_POST, control(handleWifiAdd));
  server.on("/api/wifi/connect", HTTP_POST, control(handleWifiConnect));
  server.on("/api/wifi/forget",  HTTP_POST, control(handleWifiForget));
  server.on("/api/log",          HTTP_GET,  handleLog);
  server.on("/api/ota/check",    HTTP_POST, control(handleOtaCheck));
  server.on("/api/greet",        HTTP_POST, control(handleGreet));
  server.on("/api/reboot",       HTTP_POST, control(handleReboot));
  server.on("/api/update",       HTTP_POST, handleUploadDone, handleUploadChunk);
  server.onNotFound(handleNotFound);
  const char *hdrs[] = { "X-Dev-Token" };
  server.collectHeaders(hdrs, 1);
  server.begin();

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());

  mdnsName = deviceName;
  mdnsName.toLowerCase();
  if (MDNS.begin(mdnsName.c_str())) MDNS.addService("http", "tcp", 80);

  Log.printf("[Web] UI at http://%s/ (Wi-Fi '%s') and http://%s.local/\n",
             WiFi.softAPIP().toString().c_str(), deviceName.c_str(), mdnsName.c_str());
}

void webLoop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (rebootAt && (int32_t)(millis() - rebootAt) >= 0) {
    delay(100);
    ESP.restart();
  }
}
