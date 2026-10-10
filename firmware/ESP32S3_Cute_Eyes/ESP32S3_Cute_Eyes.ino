/*
 * IRIS - Waveshare ESP32-S3-LCD-1.28 (non-touch, GC9A01 240x240 round)
 * Named after Iris, the rainbow messenger of the Greek gods.
 *
 * Everything is controlled from the IRIS Android app over Bluetooth:
 *   - Eye expressions in 4 selectable themes (one at a time); a 5th "status" theme
 *     shows device states: connecting, charging, battery full/low, errors, notifications
 *   - Phone notifications with the real app logo, sender and first line
 *   - Shake to change expression (QMI8658), clock face when idle, screen sleep
 *   - Wi-Fi (up to 5 networks), location, weather and 24 h clock (NTP)
 *   - Boot greeting in the local language + your name
 *   - Battery level on GPIO1
 *   - Cloud OTA (GitHub) + developer firmware upload over Bluetooth
 *
 * Libraries (Arduino Library Manager):
 *   - "GFX Library for Arduino" by moononournation
 *   - "ArduinoJson" by Benoit Blanchon (v7)
 *   - CloudOTA: copy firmware/libraries/CloudOTA into Documents/Arduino/libraries
 *
 * Tools menu:
 *   Board:            ESP32S3 Dev Module
 *   USB CDC On Boot:  Disabled
 *   Flash Size:       16MB (128Mb)
 *   Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)   <-- needed for OTA
 *   PSRAM:            QSPI PSRAM
 */

#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <CloudOTA.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_system.h>
#include <vector>
#include <algorithm>

#include "Config.h"

// ---------- Pins: ESP32-S3-LCD-1.28 (non-touch) ----------
#define LCD_DC    8
#define LCD_CS    9
#define LCD_SCK   10
#define LCD_MOSI  11
#define LCD_RST   12
#define LCD_BL    40
#define BOOT_BTN  0

// ---------- Display + off-screen canvas ----------
Arduino_DataBus *bus   = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI, GFX_NOT_DEFINED);
Arduino_GFX     *panel = new Arduino_GC9A01(bus, LCD_RST, 0, true);
Arduino_Canvas  *gfx   = new Arduino_Canvas(240, 240, panel);

#include "Eyes.h"          // expressions - unchanged
#include "Themes.h"        // expression themes (uses Eyes.h, adds 45 more)
#include "DebugLog.h"      // Log.printf(): Serial + app log
#include "WifiManager.h"
#include "BleSetup.h"
#include "Imu.h"
#include "Battery.h"
#include "Weather.h"       // location, weather, time zone, greetings

// ---------- Extra colours ----------
const uint16_t C_GREEN    = MK_RGB(60, 230, 120);
const uint16_t C_AMBER    = MK_RGB(255, 180, 40);
const uint16_t C_BLUE     = MK_RGB(60, 140, 255);
const uint16_t C_DIM      = MK_RGB(40, 44, 56);
const uint16_t C_TOAST_BG = MK_RGB(20, 40, 55);
const uint16_t C_TRACK    = MK_RGB(25, 30, 40);

WifiManager    wifi;
BleSetup       ble;
CloudOTA       ota;
Imu            imu;
Battery        batt;   // battery monitor (battery() is the status-face icon in Themes.h)
WeatherService weather;
String         deviceName;

// =====================================================================
//                        SETTINGS (saved in flash)
// =====================================================================

struct Settings {
  int  brightness  = 200;    // 10..255
  bool labels      = true;   // expression name at the bottom
  bool autoCycle   = true;
  bool shake       = true;   // shake to change expression
  int  sens        = 5;      // 1 (hard shake) .. 10 (light shake)
  bool shakeRandom = false;  // false = next expression, true = random
  String name;               // user's name for the boot greeting
  int  screenTimeout = 300;  // seconds without activity -> screen off (0 = never)
  int  clockAfter    = 60;   // seconds without activity -> clock face (0 = never)
  int  eyeTheme      = THEME_DEFAULT;   // the one selected eye theme
  bool notifyOn      = true; // show phone notifications
  int  notifySecs    = 8;    // how long each notification stays on screen
} settings;

void loadSettings() {
  Preferences p;
  p.begin("settings", true);
  settings.brightness    = p.getInt("bright", settings.brightness);
  settings.labels        = p.getBool("labels", settings.labels);
  settings.autoCycle     = p.getBool("auto", settings.autoCycle);
  settings.shake         = p.getBool("shake", settings.shake);
  settings.sens          = p.getInt("sens", settings.sens);
  settings.shakeRandom   = p.getBool("shakeRnd", settings.shakeRandom);
  settings.name          = p.getString("name", "");
  settings.screenTimeout = p.getInt("tout", settings.screenTimeout);
  settings.clockAfter    = p.getInt("clockAfter", settings.clockAfter);
  settings.notifyOn      = p.getBool("notifyOn", settings.notifyOn);
  settings.notifySecs    = p.getInt("notifySecs", settings.notifySecs);
  if (p.isKey("eyeTheme")) {
    settings.eyeTheme = p.getInt("eyeTheme", THEME_DEFAULT);
  } else if (p.isKey("themes")) {               // older firmware stored several themes: keep the first
    int m = p.getInt("themes", THEME_DEFAULT_MASK);
    for (int t = 0; t < TH_COUNT; t++)
      if ((m & (1 << t)) && THEME_SELECTABLE(t)) { settings.eyeTheme = t; break; }
  }
  if (!THEME_SELECTABLE(settings.eyeTheme)) settings.eyeTheme = THEME_DEFAULT;
  p.end();
}

void saveSettings() {
  Preferences p;
  p.begin("settings", false);
  p.putInt("bright", settings.brightness);
  p.putBool("labels", settings.labels);
  p.putBool("auto", settings.autoCycle);
  p.putBool("shake", settings.shake);
  p.putInt("sens", settings.sens);
  p.putBool("shakeRnd", settings.shakeRandom);
  p.putString("name", settings.name);
  p.putInt("tout", settings.screenTimeout);
  p.putInt("clockAfter", settings.clockAfter);
  p.putInt("eyeTheme", settings.eyeTheme);
  p.putBool("notifyOn", settings.notifyOn);
  p.putInt("notifySecs", settings.notifySecs);
  p.end();
}

// =====================================================================
//                 ACTIVITY: clock face + screen timeout
// =====================================================================

uint32_t lastActivity = 0;
bool     screenOff    = false;

// Shake, app commands, notifications and the BOOT button all count as activity
void markActivity() {
  lastActivity = millis();
  if (screenOff) {
    screenOff = false;
    analogWrite(LCD_BL, settings.brightness);
    Log.println("[Screen] On");
  }
}

Face currentFace() {
  uint32_t idle = millis() - lastActivity;
  if (settings.screenTimeout > 0 && idle >= (uint32_t)settings.screenTimeout * 1000UL) return Face::Off;
  // clock only once the time is known - without Wi-Fi the eyes just keep going
  if (settings.clockAfter > 0 && idle >= (uint32_t)settings.clockAfter * 1000UL && timeValid()) return Face::Clock;
  return Face::Eyes;
}

void applySettings() {
  if (!screenOff) analogWrite(LCD_BL, settings.brightness);
  autoCycle       = settings.autoCycle;     // Eyes.h global
  imu.enabled     = true;                   // always detect shakes (they also wake the screen);
                                            // settings.shake decides if they change the expression
  imu.sensitivity = settings.sens;
  eyesEngine.setTheme(settings.eyeTheme);
}

// Latest Wi-Fi scan results
struct ScanEntry {
  String ssid;
  int    rssi;
  bool   secure;
};
std::vector<ScanEntry> scanCache;

// =====================================================================
//                         SCREEN OVERLAYS
// =====================================================================

String   toastText;
uint32_t toastUntil = 0;

void toast(const String &msg, uint32_t ms = 2500) {
  toastText  = msg;
  toastUntil = millis() + ms;
  Log.println("[info] " + msg);
}

void printCentered(const String &s, int y, uint8_t size, uint16_t col) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(size);
  gfx->getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h);
  gfx->setTextColor(col);
  gfx->setCursor(MID_X - w / 2, y);
  gfx->print(s);
}

void drawToast() {
  String t = toastText.length() > 24 ? toastText.substring(0, 23) + "." : toastText;
  gfx->fillRoundRect(36, 194, 168, 22, 11, C_TOAST_BG);
  printCentered(t, 201, 1, C_WHITE);
}

// Two little dots at the top: Wi-Fi (left) and Bluetooth (right)
void drawStatusDots() {
  uint16_t w = C_DIM;
  switch (wifi.link()) {
    case WifiManager::Link::Connected:  w = C_GREEN; break;
    case WifiManager::Link::Connecting: w = (millis() / 400) % 2 ? C_AMBER : C_DIM; break;
    default: break;
  }
  gfx->fillCircle(110, 20, 4, w);
  gfx->fillCircle(130, 20, 4, ble.isAuthed() ? C_BLUE : C_DIM);
}

void drawPinScreen() {
  gfx->fillScreen(C_BLACK);
  gfx->drawCircle(MID_X, 120, 112, C_HALO);
  gfx->drawCircle(MID_X, 120, 110, C_CYAN);
  printCentered("Pairing code", 62, 2, C_CYAN);
  printCentered(ble.pin(), 102, 4, C_WHITE);
  printCentered("Enter it in the app", 150, 1, C_LABEL);
  printCentered(deviceName, 170, 1, C_LABEL);
}

void drawOtaProgress(int pct) {
  markActivity();                             // make sure the screen is on
  gfx->fillScreen(C_BLACK);
  gfx->fillArc(MID_X, 120, 110, 98, 0, 360, C_TRACK);
  if (pct > 0) gfx->fillArc(MID_X, 120, 110, 98, 270, 270 + pct * 3.6f, C_CYAN);
  printCentered("Updating", 70, 2, C_CYAN);
  printCentered(String(pct) + "%", 106, 3, C_WHITE);
  printCentered("Don't unplug me", 150, 1, C_LABEL);
  gfx->flush();

  // tell the app every 10 %
  static int lastSent = -1;
  if (ble.isAuthed() && (pct / 10 != lastSent / 10 || pct == 100)) {
    lastSent = pct;
    JsonDocument d;
    d["type"] = "ota_progress";
    d["pct"]  = pct;
    ble.send(d);
  }
}

// ---------- status faces (Interaction & Status theme) ----------
const char *statusName = nullptr;
uint32_t    statusStart = 0, statusUntil = 0;

void showStatus(const char *name, uint32_t ms) {
  statusName  = name;
  statusStart = millis();
  statusUntil = statusStart + ms;
  markActivity();
}

bool statusActive() { return statusName && (int32_t)(millis() - statusUntil) < 0; }

// Draw one frame of a status face right now (for blocking moments like OTA checks)
void flashStatus(const char *name) {
  if (screenOff) return;
  eyesEngine.drawNamed(name, millis() % 2000);
  gfx->flush();
}

// =====================================================================
//         GREETING + WI-FI BANNER + CLOCK + NOTIFICATION FACES
// =====================================================================

#include "Faces.h"
#include "Notify.h"

NotifyCenter notifications;

// Greeting
uint32_t greetStart = 0, greetUntil = 0;
bool     greetPending = false;           // waiting for the first location
uint32_t greetDeadline = 0;
String   greetWord, greetSub;

String currentGreetingWord() {
  WeatherData w = weather.snapshot();
  return w.haveLocation ? String(localGreeting(w.countryCode, w.region)) : String("Hello");
}

void startGreeting() {
  greetWord  = currentGreetingWord();
  greetSub   = timeOfDayLine();
  greetStart = millis();
  greetUntil = greetStart + 4000;
  markActivity();
  Log.printf("[Greet] %s %s\n", greetWord.c_str(), settings.name.c_str());
}

// Wi-Fi connected banner, then a happy expression from the selected theme
uint32_t bannerStart = 0, bannerUntil = 0;
bool     pendingCheer = false;
String   bannerSsid, bannerIp;

void startWifiBanner() {
  uint32_t now = millis();
  bannerStart  = (int32_t)(greetUntil - now) > 0 ? greetUntil : now;   // after the greeting
  bannerUntil  = bannerStart + 2800;
  bannerSsid   = WiFi.SSID();
  bannerIp     = WiFi.localIP().toString();
  pendingCheer = true;
  markActivity();
}

void cheerUp() {
  eyesEngine.set(eyesEngine.pleasant());
}

void applyTimezone(long offset) {
  String tz = tzFromOffset(offset);
  setenv("TZ", tz.c_str(), 1);
  tzset();
  Log.printf("[Time] Time zone %s\n", tz.c_str());
}

// =====================================================================
//                FIRMWARE UPLOAD OVER BLUETOOTH (developer)
// =====================================================================

volatile bool   fwActive = false;
volatile bool   fwFailed = false;
volatile size_t fwGot = 0;
size_t          fwSize = 0;
uint32_t        rebootAt = 0;

// Runs in the Bluetooth task for every chunk the app writes
void onFirmwareChunk(const uint8_t *data, size_t len) {
  if (!fwActive || fwFailed) return;
  if (Update.write((uint8_t *)data, len) != len) {
    fwFailed = true;
    return;
  }
  fwGot += len;
}

void scheduleReboot(uint32_t ms) {
  rebootAt = millis() + ms;
  showStatus("Goodbye", ms + 500);
}

// =====================================================================
//                       MESSAGES TO THE APP
// =====================================================================

const char *wifiStateName() {
  switch (wifi.link()) {
    case WifiManager::Link::Connected:  return "connected";
    case WifiManager::Link::Connecting: return "connecting";
    case WifiManager::Link::NoNetworks: return "no_networks";
    default:                            return "disconnected";
  }
}

const char *screenName() {
  if (screenOff) return "off";
  return currentFace() == Face::Clock ? "clock" : "eyes";
}

void sendStatus() {
  if (!ble.isAuthed()) return;
  bool up = wifi.link() == WifiManager::Link::Connected;
  JsonDocument d;
  d["type"]      = "status";
  d["name"]      = deviceName;
  d["fw"]        = FW_VERSION;
  d["wifi"]      = wifiStateName();
  d["ssid"]      = wifi.currentSsid();
  d["ip"]        = up ? WiFi.localIP().toString() : "";
  d["rssi"]      = up ? WiFi.RSSI() : 0;
  d["saved"]     = wifi.count();
  d["ota"]       = CloudOTA::stateName(ota.state());
  d["latest"]    = ota.latestVersion();
  d["bat"]       = batt.percent();
  d["batV"]      = batt.volts();
  d["batState"]  = batt.stateName();
  d["expr"]      = eyesEngine.name(eyesEngine.current);
  d["exprTheme"] = eyesEngine.theme(eyesEngine.current);
  d["screen"]    = screenName();
  ble.send(d);
}

void sendList() {
  if (!ble.isAuthed()) return;
  JsonDocument d;
  d["type"] = "list";
  JsonArray a = d["networks"].to<JsonArray>();
  for (int i = 0; i < wifi.count(); i++) a.add(wifi.net(i).ssid);
  ble.send(d);
}

void sendSettings() {
  if (!ble.isAuthed()) return;
  JsonDocument d;
  d["type"]          = "settings";
  d["brightness"]    = settings.brightness;
  d["labels"]        = settings.labels;
  d["auto"]          = autoCycle;
  d["shake"]         = settings.shake;
  d["sens"]          = settings.sens;
  d["shakeRandom"]   = settings.shakeRandom;
  d["name"]          = settings.name;
  d["screenTimeout"] = settings.screenTimeout;
  d["clockAfter"]    = settings.clockAfter;
  d["eyeTheme"]      = settings.eyeTheme;
  d["notifyOn"]      = settings.notifyOn;
  d["notifySecs"]    = settings.notifySecs;
  d["city"]          = weather.manualCity();
  ble.send(d);
}

// Selectable eye themes with their expressions (the status theme is not included)
void sendThemes() {
  if (!ble.isAuthed()) return;
  JsonDocument d;
  d["type"] = "themes";
  JsonArray arr = d["themes"].to<JsonArray>();
  for (int t = 0; t < TH_COUNT; t++) {
    if (!THEME_SELECTABLE(t)) continue;
    JsonObject o = arr.add<JsonObject>();
    o["id"]   = t;
    o["name"] = THEME_NAMES[t];
    o["desc"] = THEME_DESC[t];
    JsonArray ex = o["expr"].to<JsonArray>();
    for (int i = 0; i < eyesEngine.count(); i++)
      if (eyesEngine.theme(i) == t) ex.add(eyesEngine.name(i));
  }
  ble.send(d);
}

// Time, location, weather and greeting for the app's Home screen
void sendInfo() {
  if (!ble.isAuthed()) return;
  JsonDocument d;
  d["type"]  = "info";
  d["valid"] = timeValid();
  if (timeValid()) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char b[32];
    strftime(b, sizeof(b), "%H:%M", &t);
    d["time"] = b;
    strftime(b, sizeof(b), "%a, %d %b", &t);
    d["date"] = b;
  }
  WeatherData w = weather.snapshot();
  String loc = w.city;
  if (w.region.length() && w.region != w.city) loc += ", " + w.region;
  d["haveLoc"] = w.haveLocation;
  d["loc"]     = loc;
  d["manual"]  = w.manual;
  d["greet"]   = currentGreetingWord();
  d["have"]    = w.haveWeather;
  if (w.haveWeather) {
    d["temp"]    = w.temp;
    d["feels"]   = w.feels;
    d["hum"]     = w.humidity;
    d["wind"]    = w.wind;
    d["tmax"]    = w.tmax;
    d["tmin"]    = w.tmin;
    d["text"]    = weatherText(w.code);
    d["code"]    = w.code;
    d["isDay"]   = w.isDay;
    d["sunrise"] = w.sunrise;
    d["sunset"]  = w.sunset;
  }
  ble.send(d);
}

void sendAck(const char *cmd, bool ok, const String &msg) {
  JsonDocument d;
  d["type"] = "ack";
  d["cmd"]  = cmd;
  d["ok"]   = ok;
  d["msg"]  = msg;
  ble.send(d);
}

void sendError(const String &msg) {
  JsonDocument d;
  d["type"] = "error";
  d["msg"]  = msg;
  ble.send(d);
}

void onScanDone(int n) {
  // sort by signal, drop hidden + duplicate SSIDs, max 20
  std::vector<int> idx;
  for (int i = 0; i < n; i++) idx.push_back(i);
  std::sort(idx.begin(), idx.end(), [](int a, int b) { return WiFi.RSSI(a) > WiFi.RSSI(b); });

  scanCache.clear();
  for (int i : idx) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    bool dup = false;
    for (auto &e : scanCache) if (e.ssid == ssid) { dup = true; break; }
    if (dup) continue;
    scanCache.push_back({ ssid, (int)WiFi.RSSI(i), WiFi.encryptionType(i) != WIFI_AUTH_OPEN });
    if (scanCache.size() >= 20) break;
  }
  Log.printf("[WiFi] Scan found %d networks\n", (int)scanCache.size());

  if (!ble.isAuthed()) return;
  JsonDocument d;
  d["type"] = "scan";
  JsonArray arr = d["networks"].to<JsonArray>();
  for (auto &e : scanCache) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"]   = e.ssid;
    o["rssi"]   = e.rssi;
    o["secure"] = e.secure;
  }
  ble.send(d);
}

// ---------------- developer replies ----------------

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

void sendDevStatus() {
  bool up = WiFi.status() == WL_CONNECTED;
  JsonDocument d;
  d["type"]     = "dev_status";
  d["name"]     = deviceName;
  d["fw"]       = FW_VERSION;
  d["uptime"]   = millis() / 1000;
  d["heap"]     = ESP.getFreeHeap();
  d["minHeap"]  = ESP.getMinFreeHeap();
  d["psram"]    = ESP.getFreePsram();
  d["temp"]     = temperatureRead();
  d["reset"]    = resetReason();
  d["sdk"]      = String(ESP_ARDUINO_VERSION_MAJOR) + "." + ESP_ARDUINO_VERSION_MINOR + "." +
                  ESP_ARDUINO_VERSION_PATCH + " / " + ESP.getSdkVersion();
  d["mac"]      = WiFi.macAddress();
  d["wifi"]     = wifiStateName();
  d["ssid"]     = wifi.currentSsid();
  d["ip"]       = up ? WiFi.localIP().toString() : "";
  d["rssi"]     = up ? WiFi.RSSI() : 0;
  d["trusted"]  = ble.trustedCount();
  d["imuOk"]    = imu.ok();
  d["ax"]       = imu.ax;
  d["ay"]       = imu.ay;
  d["az"]       = imu.az;
  d["g"]        = imu.magnitude();
  d["shakes"]   = imu.shakeCount;
  d["bat"]      = batt.percent();
  d["batV"]     = batt.volts();
  d["batState"] = batt.stateName();
  d["expr"]     = eyesEngine.current;
  d["exprName"] = eyesEngine.name(eyesEngine.current);
  d["screen"]   = screenName();
  d["notifs"]   = notifications.total();
  WeatherData w = weather.snapshot();
  long off = w.utcOffset, a = labs(off);
  char tz[16];
  snprintf(tz, sizeof(tz), "UTC%c%02ld:%02ld", off >= 0 ? '+' : '-', a / 3600, (a % 3600) / 60);
  d["tz"] = tz;
  String loc = w.city;
  if (w.region.length()) loc += ", " + w.region;
  if (w.country.length()) loc += ", " + w.country;
  d["loc"]      = loc;
  d["wxErr"]    = w.error;
  d["wxAge"]    = w.haveWeather ? (int)((millis() - w.updatedMs) / 1000) : -1;
  d["otaState"] = CloudOTA::stateName(ota.state());
  d["otaMsg"]   = ota.lastMessage();
  ble.send(d);
}

void sendExpressions() {
  JsonDocument d;
  d["type"] = "expressions";
  JsonArray names = d["names"].to<JsonArray>();
  JsonArray th = d["themes"].to<JsonArray>();
  for (int i = 0; i < eyesEngine.count(); i++) {
    names.add(eyesEngine.name(i));
    th.add(eyesEngine.theme(i));
  }
  JsonArray tn = d["themeNames"].to<JsonArray>();
  for (int t = 0; t < TH_COUNT; t++) tn.add(THEME_NAMES[t]);
  ble.send(d);
}

void sendLog(uint32_t since) {
  JsonDocument d;
  d["type"] = "log";
  JsonArray a = d["lines"].to<JsonArray>();
  int n = 0;
  Log.forEachSince(since, [&](uint32_t seq, uint32_t ms, const char *text) {
    if (n >= 30) return;                       // keep each reply small
    JsonObject o = a.add<JsonObject>();
    o["s"] = seq;
    o["t"] = ms;
    o["m"] = text;
    n++;
  });
  d["last"] = Log.lastSeq();
  ble.send(d);
}

// =====================================================================
//                       COMMANDS FROM THE APP
// =====================================================================

// Apply the fields present in a {"cmd":"set",...} message
void applySetCommand(JsonDocument &in) {
  if (in["brightness"].is<int>())    settings.brightness    = constrain(in["brightness"].as<int>(), 10, 255);
  if (in["labels"].is<bool>())       settings.labels        = in["labels"].as<bool>();
  if (in["auto"].is<bool>())         settings.autoCycle     = in["auto"].as<bool>();
  if (in["shake"].is<bool>())        settings.shake         = in["shake"].as<bool>();
  if (in["sens"].is<int>())          settings.sens          = constrain(in["sens"].as<int>(), 1, 10);
  if (in["shakeRandom"].is<bool>())  settings.shakeRandom   = in["shakeRandom"].as<bool>();
  if (in["screenTimeout"].is<int>()) settings.screenTimeout = constrain(in["screenTimeout"].as<int>(), 0, 3600);
  if (in["clockAfter"].is<int>())    settings.clockAfter    = constrain(in["clockAfter"].as<int>(), 0, 3600);
  if (in["notifyOn"].is<bool>())     settings.notifyOn      = in["notifyOn"].as<bool>();
  if (in["notifySecs"].is<int>())    settings.notifySecs    = constrain(in["notifySecs"].as<int>(), 3, 30);
  if (in["name"].is<const char *>()) {
    String n = in["name"].as<String>();
    n.trim();
    if (n.length() > 16) n = n.substring(0, 16);
    settings.name = n;
  }
  bool themeChanged = false;
  if (in["eyeTheme"].is<int>()) {
    int t = in["eyeTheme"].as<int>();
    if (THEME_SELECTABLE(t) && t != settings.eyeTheme) {
      settings.eyeTheme = t;                   // the newest choice replaces the old one
      themeChanged = true;
    }
  }
  if (in["city"].is<const char *>()) weather.setManualCity(in["city"].as<String>());
  applySettings();
  saveSettings();
  if (themeChanged) {
    eyesEngine.set(eyesEngine.firstEnabled());
    Log.printf("[App] Eye theme -> %s\n", THEME_NAMES[settings.eyeTheme]);
  }
}

// Developer-only commands (after dev_login)
void handleDevCommand(const String &cmd, JsonDocument &in) {
  if (cmd == "dev_status") {
    sendDevStatus();
  } else if (cmd == "dev_logout") {
    ble.setDev(false);
    sendAck("dev_logout", true, "Developer locked");
  } else if (cmd == "expressions") {
    sendExpressions();
  } else if (cmd == "expr") {
    int i = in["index"] | -1;
    if (i < 0 || i >= eyesEngine.count()) {
      sendError("No such expression");
      return;
    }
    eyesEngine.set(i);
    sendStatus();
  } else if (cmd == "log") {
    sendLog(in["since"] | 0);
  } else if (cmd == "reboot") {
    sendAck("reboot", true, "Rebooting...");
    scheduleReboot(1200);
  } else if (cmd == "fw_begin") {
    fwSize = in["size"] | 0;
    String md5 = in["md5"] | "";
    fwGot = 0;
    fwFailed = false;
    if (fwSize == 0 || !Update.begin(fwSize, U_FLASH)) {
      sendAck("fw_begin", false, String("Cannot start: ") + Update.errorString());
      return;
    }
    if (md5.length() == 32) Update.setMD5(md5.c_str());
    fwActive = true;
    Log.printf("[FW] Upload started, %u bytes\n", (unsigned)fwSize);
    sendAck("fw_begin", true, "Ready");
  } else if (cmd == "fw_end") {
    bool ok = fwActive && !fwFailed && fwGot == fwSize && Update.end(true);
    String msg = ok ? String("Installed! Restarting...") : String("Failed: ") + Update.errorString();
    if (!ok && fwActive) Update.abort();
    fwActive = false;
    Log.println("[FW] " + msg);
    sendAck("fw_end", ok, msg);
    if (ok) scheduleReboot(1500);
    else showStatus("Error", 2500);
  } else if (cmd == "fw_abort") {
    if (fwActive) Update.abort();
    fwActive = false;
    sendAck("fw_abort", true, "Upload cancelled");
  } else {
    sendError("Unknown developer command '" + cmd + "'");
  }
}

bool isDevCommand(const String &cmd) {
  return cmd.startsWith("dev_") || cmd.startsWith("fw_") || cmd == "expressions" || cmd == "expr" ||
         cmd == "log" || cmd == "reboot";
}

void handleCommand(JsonDocument &in) {
  markActivity();
  String cmd = in["cmd"] | "";

  // ---------------- pairing ----------------
  if (cmd == "auth") {
    String token = in["token"] | "";
    String pin   = in["pin"] | "";
    bool ok = false;
    String newToken;
    if (token.length()) {
      ok = ble.checkToken(token);
    } else {
      newToken = ble.checkPin(pin);
      ok = newToken.length() > 0;
    }
    JsonDocument d;
    d["type"] = "auth";
    d["ok"]   = ok;
    if (newToken.length()) d["token"] = newToken;
    d["msg"]  = ok ? String("Connected to ") + deviceName
                   : (token.length() ? String("Pair again with the code") : String("Wrong code"));
    ble.send(d);
    if (ok) {
      showStatus("Listening", 1500);
      pendingCheer = false;
      eyesEngine.set(eyesEngine.pleasant());
      sendStatus();
      sendSettings();
      sendList();
    }
    return;
  }

  if (!ble.isAuthed()) {
    sendError("Pair with the code first");
    return;
  }

  // ---------------- developer ----------------
  if (cmd == "dev_login") {
    String u = in["user"] | "";
    String p = in["pass"] | "";
    bool ok = (u == DEV_USER && p == DEV_PASS);
    ble.setDev(ok);
    Log.println(ok ? "[App] Developer unlocked" : "[App] Developer login failed");
    JsonDocument d;
    d["type"] = "dev_login";
    d["ok"]   = ok;
    d["msg"]  = ok ? "Developer unlocked" : "Wrong username or password";
    ble.send(d);
    return;
  }
  if (isDevCommand(cmd)) {
    if (!ble.isDev()) sendError("Developer login required");
    else handleDevCommand(cmd, in);
    return;
  }

  // ---------------- everyday ----------------
  if (cmd == "status") {
    sendStatus();
  } else if (cmd == "info") {
    sendInfo();
  } else if (cmd == "get_settings") {
    sendSettings();
  } else if (cmd == "themes") {
    sendThemes();
  } else if (cmd == "set") {
    applySetCommand(in);
    sendSettings();
    sendStatus();
  } else if (cmd == "next") {
    eyesEngine.next();
    sendStatus();
  } else if (cmd == "greet") {
    startGreeting();
    sendAck("greet", true, "Hello!");
  } else if (cmd == "list") {
    sendList();
  } else if (cmd == "scan") {
    wifi.startScan();
  } else if (cmd == "add") {
    String ssid = in["ssid"] | "";
    String pass = in["pass"] | "";
    if (ssid.length() == 0 || ssid.length() > 32) {
      sendAck("add", false, "Network name must be 1-32 characters");
      return;
    }
    if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) {
      sendAck("add", false, "Password must be 8-63 characters");
      return;
    }
    wifi.add(ssid, pass);
    wifi.connectTo(ssid);
    sendAck("add", true, "Saved '" + ssid + "', connecting...");
    sendList();
    toast("Wi-Fi saved");
  } else if (cmd == "delete") {
    String ssid = in["ssid"] | "";
    bool ok = wifi.remove(ssid);
    sendAck("delete", ok, ok ? String("Removed '") + ssid + "'" : String("Network not found"));
    sendList();
    sendStatus();
  } else if (cmd == "connect") {
    String ssid = in["ssid"] | "";
    wifi.connectTo(ssid);
    sendAck("connect", true, "Connecting to '" + ssid + "'...");
  } else if (cmd == "ota") {
    if (wifi.link() != WifiManager::Link::Connected) {
      sendAck("ota", false, "IRIS is not on Wi-Fi");
    } else {
      ota.requestCheck();
      sendAck("ota", true, "Checking for updates...");
    }
  } else if (cmd == "forget_me") {
    ble.forgetToken(in["token"] | "");
    sendAck("forget_me", true, "This phone was forgotten");

  // ---------------- notifications ----------------
  } else if (cmd == "icon") {
    notifications.iconPart(in["pkg"] | "", in["i"] | 0, in["n"] | 1, in["d"] | "");
  } else if (cmd == "has_icon") {
    JsonDocument d;
    d["type"] = "has_icon";
    d["pkg"]  = in["pkg"] | "";
    d["ok"]   = notifications.hasIcon(in["pkg"] | "");
    ble.send(d);
  } else if (cmd == "notify") {
    if (!settings.notifyOn) return;
    Notification n;
    n.pkg   = in["pkg"] | "";
    n.app   = in["app"] | "";
    n.title = in["title"] | "";
    n.text  = in["text"] | "";
    n.color = in["color"] | 0;
    notifications.push(n);
    markActivity();
  } else {
    sendError("Unknown command '" + cmd + "'");
  }
}

// =====================================================================
//                            SHAKE
// =====================================================================

void onShake() {
  // next / random within the selected theme (never the same one)
  int next = settings.shakeRandom ? eyesEngine.randomOther() : eyesEngine.nextIndex(eyesEngine.current);
  eyesEngine.set(next);
  Log.printf("[IMU] Shake! -> %s\n", eyesEngine.name(eyesEngine.current));
}

// =====================================================================
//                            SETUP / LOOP
// =====================================================================

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);
  pinMode(BOOT_BTN, INPUT_PULLUP);

  if (!gfx->begin(80000000)) Log.println("Canvas init failed (enable PSRAM?)");
  randomSeed(esp_random());
  eyesEngine.begin();              // all expressions
  loadSettings();
  applySettings();                 // backlight on at saved brightness

  // Unique name from the chip MAC, e.g. IRIS-3F2A
  uint64_t mac = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  deviceName = String(BLE_NAME_PREFIX) + "-" + suffix;
  Log.printf("\n%s  firmware v%s\n", deviceName.c_str(), FW_VERSION);

  // ---- Wi-Fi ----
  wifi.onScanDone = onScanDone;
  wifi.onStateChange = []() {
    static WifiManager::Link last = WifiManager::Link::NoNetworks;
    WifiManager::Link now = wifi.link();
    if (now != last) {
      if (now == WifiManager::Link::Connected) {
        startWifiBanner();  // Wi-Fi icon + "Wi-Fi connected", then a happy face
      } else if (last == WifiManager::Link::Connected) {
        toast("Wi-Fi lost", 1500);   // brief - the eyes keep running
      }
      last = now;
    }
    sendStatus();
  };
  wifi.begin(deviceName.c_str());

  // ---- Time (NTP) + location/weather ----
  weather.begin();                                   // loads cached location + time zone
  configTzTime(tzFromOffset(weather.snapshot().utcOffset).c_str(),
               "pool.ntp.org", "time.google.com", "time.cloudflare.com");

  // ---- Sensors ----
  imu.begin();
  batt.begin();
  applySettings();

  // ---- Bluetooth ----
  ble.onFirmwareData = onFirmwareChunk;
  ble.begin(deviceName);

  // ---- Cloud OTA ----
  ota.begin(FW_VERSION, OTA_MANIFEST_URL);
  ota.setDeviceType(OTA_DEVICE_TYPE);
  ota.setCheckInterval(OTA_CHECK_INTERVAL_MS);
#ifdef OTA_ROOT_CA
  ota.setRootCA(OTA_ROOT_CA);
#else
  ota.setInsecure();
#endif
  ota.onProgress(drawOtaProgress);
  ota.onStateChange([](CloudOTA::State s, const String &msg) {
    if (s == CloudOTA::State::Checking) flashStatus("Processing");
    else toast(msg, 3500);
    if (s == CloudOTA::State::Failed) showStatus("Error", 2000);
    if (s == CloudOTA::State::Success) flashStatus("Goodbye");
    sendStatus();
  });

  eyesEngine.set(eyesEngine.firstEnabled());
  autoCycle = settings.autoCycle;
  markActivity();

  // ---- Boot greeting ----
  if (weather.snapshot().haveLocation) {
    startGreeting();                                 // location known from last time
  } else {
    greetPending  = true;                            // first boot: wait for the location
    greetDeadline = millis() + 10000;
  }
}

void loop() {
  eyesEngine.handleInput();   // BOOT button + Serial expression picker
  if (digitalRead(BOOT_BTN) == LOW) markActivity();
  wifi.loop();
  ble.loop();

  JsonDocument cmd;
  while (ble.poll(cmd)) handleCommand(cmd);

  uint32_t now = millis();

  // ---- reboot requested (firmware upload / developer) ----
  if (rebootAt && (int32_t)(now - rebootAt) >= 0) {
    delay(100);
    ESP.restart();
  }

  // ---- time zone / time sync / pending greeting / cheer ----
  long offset;
  if (weather.takeOffsetChange(offset)) applyTimezone(offset);
  static bool timeLogged = false;
  if (!timeLogged && timeValid()) {
    timeLogged = true;
    time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char b[32];
    strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &tmv);
    Log.printf("[Time] Synced: %s\n", b);
  }
  // greet as soon as the location is known; don't wait if there is no Wi-Fi to wait for
  if (greetPending && (weather.takeFirstLocation() || (int32_t)(now - greetDeadline) >= 0 ||
                       wifi.link() == WifiManager::Link::NoNetworks)) {
    greetPending = false;
    startGreeting();
  }
  if (pendingCheer && (int32_t)(now - bannerUntil) >= 0) {
    pendingCheer = false;
    cheerUp();
    markActivity();
  }

  // ---- status faces from device events ----
  static uint32_t lastConnectingFace = 0;
  if (wifi.takeAttemptStarted() && (lastConnectingFace == 0 || now - lastConnectingFace > 60000) &&
      currentFace() == Face::Eyes && !screenOff) {
    lastConnectingFace = now;
    statusName  = "Connecting";              // brief and quiet: doesn't wake the screen
    statusStart = now;
    statusUntil = now + 1500;
  }
  if (batt.update()) {
    if (batt.state() == Battery::State::Charging) showStatus("Charging", 3000);
    else if (batt.state() == Battery::State::Full) showStatus("Battery Full", 3000);
  }
  static uint32_t lastLowWarn = 0;
  if (batt.low() && (lastLowWarn == 0 || now - lastLowWarn > 10UL * 60UL * 1000UL)) {
    lastLowWarn = now;
    showStatus("Low Battery", 3000);
  }
  // keep the app's battery/status display fresh
  static uint32_t lastStatusPush = 0;
  if (ble.isAuthed() && now - lastStatusPush > 30000) {
    lastStatusPush = now;
    sendStatus();
  }

  bool pinScreen = ble.showPin();
  if (pinScreen) markActivity();
  if (fwActive) markActivity();
  bool greeting = (int32_t)(now - greetUntil) < 0;
  bool banner   = (int32_t)(now - bannerStart) >= 0 && (int32_t)(now - bannerUntil) < 0;
  notifications.setDurationHint(settings.notifySecs * 1000UL);
  bool notif    = notifications.update(settings.notifySecs * 1000UL);
  bool status   = statusActive();
  Face face     = currentFace();

  // ---- shake: dismiss a notification, change expression, or just wake ----
  if (imu.update()) {
    bool overlay = pinScreen || greeting || banner || status;
    if (notif) notifications.dismiss();
    else if (face == Face::Eyes && !overlay && settings.shake) onShake();
    else if (face != Face::Eyes) Log.println("[IMU] Shake -> wake");
    markActivity();
    face = Face::Eyes;
  }

  // ---- screen off after the timeout ----
  if (face == Face::Off && !greeting && !banner && !pinScreen && !notif && !fwActive) {
    if (!screenOff) {
      screenOff = true;
      analogWrite(LCD_BL, 0);
      Log.println("[Screen] Off (timeout) - shake to wake");
    }
    ota.loop();
    delay(20);
    return;
  }

  if (fwActive) {
    drawOtaProgress(fwSize ? (int)((uint64_t)fwGot * 100 / fwSize) : 0);   // flushes itself
    delay(30);
    return;
  } else if (pinScreen) {
    drawPinScreen();
  } else if (notif) {
    if (notifications.inAttention()) eyesEngine.drawNamed("Attention", now % 1200);
    else notifications.draw();
  } else if (greeting) {
    gfx->fillScreen(C_BLACK);
    drawGreeting(now - greetStart, greetWord, settings.name, greetSub);
  } else if (banner) {
    gfx->fillScreen(C_BLACK);
    drawWifiBanner(now - bannerStart, bannerSsid, bannerIp);
  } else if (status) {
    eyesEngine.drawNamed(statusName, now - statusStart);
  } else if (face == Face::Clock) {
    WeatherData w = weather.snapshot();
    gfx->fillScreen(C_BLACK);
    drawClockFace(w);
  } else {
    eyesEngine.tick();
    if (millis() < toastUntil)  drawToast();
    else if (settings.labels)   drawLabel(eyesEngine.name(eyesEngine.current));
  }
  drawStatusDots();
  gfx->flush();

  ota.loop();            // blocks briefly while checking / updating
  delay(10);
}
