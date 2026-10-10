/*
 * Cute Eyes - Waveshare ESP32-S3-LCD-1.28 (non-touch, GC9A01 240x240 round)
 *
 * Features:
 *   - Playful & Cute animated eye expressions (Eyes.h)
 *   - Shake the device to change expression (QMI8658 accelerometer)
 *   - Web UI on its own Wi-Fi (always on) + your home Wi-Fi at the same time:
 *     status, Wi-Fi scan/connect, eye control, firmware upload, live logs
 *   - Stores up to 5 Wi-Fi networks, auto-connects/reconnects
 *   - Bluetooth LE setup for the mobile app (PIN shown on screen)
 *   - Cloud OTA: updates itself from the internet (see CloudOTA library)
 *   - Location, weather and 24 h clock (NTP) over Wi-Fi
 *   - Clock face when idle, screen timeout, boot greeting in the local language
 *   - Personalization: 5 expression themes (55 expressions), pick any combination
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
#include <Preferences.h>
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
#include "Themes.h"        // expression themes (uses Eyes.h, adds 44 more)
#include "DebugLog.h"      // Log.printf(): Serial + web log
#include "WifiManager.h"
#include "BleSetup.h"
#include "Imu.h"
#include "Weather.h"       // location, weather, time zone, greetings

// ---------- Extra colours ----------
const uint16_t C_GREEN    = MK_RGB(60, 230, 120);
const uint16_t C_AMBER    = MK_RGB(255, 180, 40);
const uint16_t C_BLUE     = MK_RGB(60, 140, 255);
const uint16_t C_DIM      = MK_RGB(40, 44, 56);
const uint16_t C_TOAST_BG = MK_RGB(20, 40, 55);
const uint16_t C_TRACK    = MK_RGB(25, 30, 40);

WifiManager wifi;
BleSetup    ble;
CloudOTA    ota;
Imu            imu;
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
  int  themes        = THEME_DEFAULT_MASK;   // enabled expression themes (bit per theme)
} settings;

void loadSettings() {
  Preferences p;
  p.begin("settings", true);
  settings.brightness  = p.getInt("bright", settings.brightness);
  settings.labels      = p.getBool("labels", settings.labels);
  settings.autoCycle   = p.getBool("auto", settings.autoCycle);
  settings.shake       = p.getBool("shake", settings.shake);
  settings.sens        = p.getInt("sens", settings.sens);
  settings.shakeRandom = p.getBool("shakeRnd", settings.shakeRandom);
  settings.name          = p.getString("name", "");
  settings.screenTimeout = p.getInt("tout", settings.screenTimeout);
  settings.clockAfter    = p.getInt("clockAfter", settings.clockAfter);
  settings.themes        = p.getInt("themes", settings.themes) & ((1 << TH_COUNT) - 1);
  if (!settings.themes) settings.themes = THEME_DEFAULT_MASK;
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
  p.putInt("themes", settings.themes);
  p.end();
}

// =====================================================================
//                 ACTIVITY: clock face + screen timeout
// =====================================================================

uint32_t lastActivity = 0;
bool     screenOff    = false;

// Shake, web UI controls, app commands and the BOOT button all count as activity
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
  autoCycle         = settings.autoCycle;     // Eyes.h global
  imu.enabled       = true;                   // always detect shakes (they also wake the screen);
                                              // settings.shake decides if they change the expression
  imu.sensitivity   = settings.sens;
  eyesEngine.mask   = settings.themes;
}

// Latest Wi-Fi scan results (shared by web UI and app)
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
  gfx->fillCircle(130, 20, 4, ble.isConnected() ? C_BLUE : C_DIM);
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

// =====================================================================
//                GREETING + WI-FI BANNER + CLOCK FACES
// =====================================================================

#include "Faces.h"

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

// Wi-Fi connected banner, then a random happy expression
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

int findExpression(const char *name) {
  return eyesEngine.find(name);
}

void cheerUp() {
  const char *happy[] = { "Tongue Out", "Laughing", "Giggle", "Blushing", "Cheeky", "Cool Shades" };
  int idx = findExpression(happy[random(6)]);
  if (idx >= 0) eyesEngine.set(idx);
}

void applyTimezone(long offset) {
  String tz = tzFromOffset(offset);
  setenv("TZ", tz.c_str(), 1);
  tzset();
  Log.printf("[Time] Time zone %s\n", tz.c_str());
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

void sendStatus() {
  if (!ble.isAuthed()) return;
  bool up = wifi.link() == WifiManager::Link::Connected;
  JsonDocument d;
  d["type"]   = "status";
  d["name"]   = deviceName;
  d["fw"]     = FW_VERSION;
  d["wifi"]   = wifiStateName();
  d["ssid"]   = wifi.currentSsid();
  d["ip"]     = up ? WiFi.localIP().toString() : "";
  d["rssi"]   = up ? WiFi.RSSI() : 0;
  d["saved"]  = wifi.count();
  d["ota"]    = CloudOTA::stateName(ota.state());
  d["latest"] = ota.latestVersion();
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

// =====================================================================
//                       COMMANDS FROM THE APP
// =====================================================================

void handleCommand(JsonDocument &in) {
  markActivity();
  String cmd = in["cmd"] | "";

  if (cmd == "auth") {
    String pin = in["pin"] | "";
    bool ok = ble.checkPin(pin);
    JsonDocument d;
    d["type"] = "auth";
    d["ok"]   = ok;
    d["msg"]  = ok ? String("Paired with ") + deviceName : String("Wrong PIN");
    ble.send(d);
    if (ok) {
      toast("Phone paired!");
      eyesEngine.set(findExpression("Shy"));   // Shy :)
      sendStatus();
      sendList();
    }
    return;
  }

  if (!ble.isAuthed()) {
    sendError("Enter the pairing code first");
    return;
  }

  if (cmd == "status") {
    sendStatus();
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
      sendAck("ota", false, "Device is not online");
    } else {
      ota.requestCheck();
      sendAck("ota", true, "Checking for updates...");
    }
  } else {
    sendError("Unknown command '" + cmd + "'");
  }
}

#include "WebUI.h"         // needs the globals above

// =====================================================================
//                            SHAKE
// =====================================================================

void onShake() {
  // next / random within the enabled themes (never the same one)
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
  eyesEngine.begin();              // all 55 expressions
  loadSettings();
  applySettings();                 // backlight on at saved brightness

  // Unique name from the chip MAC, e.g. CuteEyes-3F2A
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
  wifi.begin(deviceName.c_str(), deviceName.c_str(), AP_PASSWORD);

  // ---- Time (NTP) + location/weather ----
  weather.begin();                                   // loads cached location + time zone
  configTzTime(tzFromOffset(weather.snapshot().utcOffset).c_str(),
               "pool.ntp.org", "time.google.com", "time.cloudflare.com");

  // ---- Web UI ----
  webBegin();

  // ---- Accelerometer ----
  imu.begin();
  applySettings();

  // ---- Bluetooth ----
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
    if (s != CloudOTA::State::Checking) toast(msg, 3500);
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
  webLoop();             // web UI + captive portal
  wifi.loop();
  ble.loop();

  JsonDocument cmd;
  while (ble.poll(cmd)) handleCommand(cmd);

  uint32_t now = millis();

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

  bool pinScreen = ble.isConnected() && !ble.isAuthed();
  if (pinScreen) markActivity();
  bool greeting = (int32_t)(now - greetUntil) < 0;
  bool banner   = (int32_t)(now - bannerStart) >= 0 && (int32_t)(now - bannerUntil) < 0;
  Face face     = currentFace();

  // ---- shake: change expression when the eyes are showing, otherwise just wake ----
  if (imu.update()) {
    bool overlay = pinScreen || greeting || banner;
    if (face == Face::Eyes && !overlay && settings.shake) onShake();
    else if (face != Face::Eyes) Log.println("[IMU] Shake -> wake");
    markActivity();
    face = Face::Eyes;
  }

  // ---- screen off after the timeout ----
  if (face == Face::Off && !greeting && !banner && !pinScreen) {
    if (!screenOff) {
      screenOff = true;
      analogWrite(LCD_BL, 0);
      Log.println("[Screen] Off (timeout) - shake to wake");
    }
    ota.loop();
    delay(20);
    return;
  }

  if (pinScreen) {
    drawPinScreen();
  } else if (greeting) {
    gfx->fillScreen(C_BLACK);
    drawGreeting(now - greetStart, greetWord, settings.name, greetSub);
  } else if (banner) {
    gfx->fillScreen(C_BLACK);
    drawWifiBanner(now - bannerStart, bannerSsid, bannerIp);
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
