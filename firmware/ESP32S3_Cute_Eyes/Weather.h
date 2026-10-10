/*
 * Weather.h - location, weather, time zone and local greeting.
 *
 *  Location: automatic from the public IP of the Wi-Fi you're on (ip-api.com,
 *            fallback ipapi.co), or a city typed in the web UI (Open-Meteo geocoding).
 *  Weather:  Open-Meteo (free, no API key), refreshed every 15 minutes.
 *  Time:     NTP over the same Wi-Fi; the time zone comes from the location.
 *
 * All network calls run in a background task, so the animations never freeze.
 * Location + time zone are cached in flash so the clock and greeting are right
 * immediately after a reboot, even before Wi-Fi connects.
 */
#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>

#define WEATHER_REFRESH_MS   (15UL * 60UL * 1000UL)   // weather every 15 min
#define LOCATION_REFRESH_MS  (6UL * 60UL * 60UL * 1000UL)  // auto location every 6 h
#define RETRY_MS             (60UL * 1000UL)          // after a failure

struct WeatherData {
  // location
  bool   haveLocation = false;
  String city, region, country, countryCode;
  float  lat = 0, lon = 0;
  bool   manual = false;          // location typed by the user
  // time zone
  bool   haveOffset = false;
  long   utcOffset = 19800;       // seconds east of UTC (default IST)
  // weather
  bool   haveWeather = false;
  float  temp = 0, feels = 0, humidity = 0, wind = 0, tmax = 0, tmin = 0;
  int    code = -1;
  bool   isDay = true;
  String sunrise, sunset;         // "06:12"
  uint32_t updatedMs = 0;         // millis() of last weather update
  String error;
};

// ---------------------------------------------------------------------------
//  WMO weather code -> text (keep ASCII: the screen font has no accents)
// ---------------------------------------------------------------------------
const char *weatherText(int code) {
  switch (code) {
    case 0:  return "Clear sky";
    case 1:  return "Mostly clear";
    case 2:  return "Partly cloudy";
    case 3:  return "Cloudy";
    case 45: case 48: return "Foggy";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: case 73: case 75: case 77: return "Snow";
    case 80: case 81: return "Rain showers";
    case 82: return "Heavy showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Storm + hail";
    default: return "Unknown";
  }
}

// ---------------------------------------------------------------------------
//  Local greeting from country + state/region (ASCII spellings)
// ---------------------------------------------------------------------------
const char *localGreeting(const String &cc, const String &regionIn) {
  String r = regionIn;
  r.toLowerCase();
  auto has = [&](const char *s) { return r.indexOf(s) >= 0; };

  if (cc == "IN") {
    if (has("kerala") || has("lakshadweep"))       return "Namaskaram";
    if (has("tamil") || has("puducherry"))         return "Vanakkam";
    if (has("karnataka"))                          return "Namaskara";
    if (has("andhra") || has("telangana"))         return "Namaskaram";
    if (has("maharashtra"))                        return "Namaskar";
    if (has("gujarat"))                            return "Kem cho";
    if (has("bengal") || has("tripura"))           return "Nomoshkar";
    if (has("assam"))                              return "Nomoskar";
    if (has("odisha") || has("orissa"))            return "Namaskar";
    if (has("punjab") || has("chandigarh"))        return "Sat Sri Akal";
    if (has("rajasthan"))                          return "Khamma Ghani";
    if (has("bihar") || has("jharkhand"))          return "Pranam";
    if (has("goa"))                                return "Dev borem korum";
    if (has("ladakh"))                             return "Julley";
    if (has("sikkim"))                             return "Tashi Delek";
    if (has("kashmir"))                            return "Aadaab";
    if (has("mizoram"))                            return "Chibai";
    if (has("nagaland"))                           return "Kuknalim";
    if (has("manipur"))                            return "Khurumjari";
    if (has("meghalaya"))                          return "Khublei";
    if (has("arunachal"))                          return "Jai Hind";
    if (has("himachal") || has("uttarakhand"))     return "Ram Ram";
    return "Namaste";                              // Delhi, UP, MP, Haryana...
  }
  if (cc == "US") {
    if (has("texas") || has("oklahoma"))           return "Howdy";
    if (has("hawaii"))                             return "Aloha";
    return "Hey there";
  }
  if (cc == "GB")                                  return "Hiya";
  if (cc == "AU")                                  return "G'day";
  if (cc == "NZ")                                  return "Kia ora";
  if (cc == "IE")                                  return "Dia duit";
  if (cc == "CA")                                  return "Hello";
  if (cc == "NP")                                  return "Namaste";
  if (cc == "LK")                                  return "Ayubowan";
  if (cc == "BD")                                  return "Nomoshkar";
  if (cc == "PK")                                  return "Assalam-o-Alaikum";
  if (cc == "AE" || cc == "SA" || cc == "QA" || cc == "KW" || cc == "OM" ||
      cc == "BH" || cc == "JO" || cc == "EG")      return "Marhaba";
  if (cc == "TR")                                  return "Merhaba";
  if (cc == "JP")                                  return "Konnichiwa";
  if (cc == "KR")                                  return "Annyeong";
  if (cc == "CN" || cc == "TW" || cc == "HK")      return "Ni hao";
  if (cc == "SG")                                  return "Hello lah";
  if (cc == "TH")                                  return "Sawasdee";
  if (cc == "VN")                                  return "Xin chao";
  if (cc == "ID")                                  return "Halo";
  if (cc == "MY")                                  return "Selamat";
  if (cc == "PH")                                  return "Kumusta";
  if (cc == "FR" || cc == "BE" || cc == "LU")      return "Bonjour";
  if (cc == "CH")                                  return "Grueezi";
  if (cc == "DE" || cc == "AT")                    return "Hallo";
  if (cc == "NL")                                  return "Hoi";
  if (cc == "IT")                                  return "Ciao";
  if (cc == "ES" || cc == "MX" || cc == "AR" || cc == "CO" || cc == "CL" ||
      cc == "PE" || cc == "VE" || cc == "UY")      return "Hola";
  if (cc == "PT" || cc == "BR")                    return "Ola";
  if (cc == "RU" || cc == "BY" || cc == "KZ")      return "Privet";
  if (cc == "UA")                                  return "Pryvit";
  if (cc == "PL")                                  return "Czesc";
  if (cc == "GR")                                  return "Yassou";
  if (cc == "SE" || cc == "NO" || cc == "DK")      return "Hej";
  if (cc == "FI")                                  return "Moi";
  if (cc == "KE" || cc == "TZ" || cc == "UG")      return "Jambo";
  if (cc == "ZA")                                  return "Sawubona";
  if (cc == "NG")                                  return "Bawo";
  if (cc == "IL")                                  return "Shalom";
  return "Hello";
}

// ---------------------------------------------------------------------------
//  Service
// ---------------------------------------------------------------------------
class WeatherService {
public:
  void begin() {
    _mtx = xSemaphoreCreateMutex();
    loadCache();
    xTaskCreatePinnedToCore(taskEntry, "weather", 12288, this, 1, nullptr, 0);
  }

  // Thread-safe copy for drawing / web UI
  WeatherData snapshot() {
    xSemaphoreTake(_mtx, portMAX_DELAY);
    WeatherData c = _d;
    xSemaphoreGive(_mtx);
    return c;
  }

  // Empty string = automatic (IP) location
  void setManualCity(const String &city) {
    xSemaphoreTake(_mtx, portMAX_DELAY);
    _manualCity = city;
    _manualCity.trim();
    xSemaphoreGive(_mtx);
    Preferences p;
    p.begin("weather", false);
    p.putString("manual", _manualCity);
    p.end();
    _forceLocation = true;
  }

  String manualCity() {
    xSemaphoreTake(_mtx, portMAX_DELAY);
    String c = _manualCity;
    xSemaphoreGive(_mtx);
    return c;
  }

  void refreshNow() { _forceWeather = true; }

  // True once each time the UTC offset changes (main loop applies it to the clock)
  bool takeOffsetChange(long &offset) {
    if (!_offsetChanged) return false;
    _offsetChanged = false;
    offset = snapshot().utcOffset;
    return true;
  }

  // True once when a location is found for the very first time (no cache before)
  bool takeFirstLocation() {
    if (!_firstLocation) return false;
    _firstLocation = false;
    return true;
  }

private:
  static void taskEntry(void *arg) { static_cast<WeatherService *>(arg)->run(); }

  void run() {
    uint32_t nextLoc = 0, nextWx = 0;
    for (;;) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (WiFi.status() != WL_CONNECTED) continue;
      uint32_t now = millis();

      if (_forceLocation || !_locFetchedThisBoot || (int32_t)(now - nextLoc) >= 0) {
        _forceLocation = false;
        bool ok = fetchLocation();
        _locFetchedThisBoot = _locFetchedThisBoot || ok;
        nextLoc = now + (ok ? LOCATION_REFRESH_MS : RETRY_MS);
        if (ok) nextWx = now;                       // weather right after a new location
      }

      bool haveLoc;
      xSemaphoreTake(_mtx, portMAX_DELAY);
      haveLoc = _d.haveLocation;
      xSemaphoreGive(_mtx);

      if (haveLoc && (_forceWeather || (int32_t)(now - nextWx) >= 0)) {
        _forceWeather = false;
        bool ok = fetchWeather();
        nextWx = now + (ok ? WEATHER_REFRESH_MS : RETRY_MS);
      }
    }
  }

  // ---- HTTP helper ----
  bool getJson(const String &url, JsonDocument &doc) {
    HTTPClient http;
    WiFiClient plain;
    WiFiClientSecure secure;
    bool https = url.startsWith("https://");
    if (https) secure.setInsecure();
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    bool ok = https ? http.begin(secure, url) : http.begin(plain, url);
    if (!ok) return false;
    http.addHeader("User-Agent", "CuteEyes-ESP32");
    int code = http.GET();
    if (code != 200) {
      setError("HTTP " + String(code));
      http.end();
      return false;
    }
    String body = http.getString();
    http.end();
    return deserializeJson(doc, body) == DeserializationError::Ok;
  }

  static String urlEncode(const String &s) {
    String o;
    const char *hex = "0123456789ABCDEF";
    for (size_t i = 0; i < s.length(); i++) {
      uint8_t c = s[i];
      if (isalnum(c) || c == '-' || c == '_' || c == '.') o += (char)c;
      else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
    }
    return o;
  }

  // ---- location ----
  bool fetchLocation() {
    String manual = manualCity();
    WeatherData n;
    JsonDocument doc;

    if (manual.length()) {
      String url = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=en&format=json&name=" + urlEncode(manual);
      if (!getJson(url, doc) || !doc["results"][0].is<JsonObject>()) {
        setError("City '" + manual + "' not found");
        Log.printf("[Weather] City '%s' not found\n", manual.c_str());
        return false;
      }
      JsonObject r = doc["results"][0];
      n.city        = r["name"] | manual.c_str();
      n.region      = r["admin1"] | "";
      n.country     = r["country"] | "";
      n.countryCode = r["country_code"] | "";
      n.lat         = r["latitude"] | 0.0f;
      n.lon         = r["longitude"] | 0.0f;
      n.manual      = true;
    } else if (getJson("http://ip-api.com/json/?fields=status,country,countryCode,regionName,city,lat,lon,offset", doc) &&
               String(doc["status"] | "") == "success") {
      n.city        = doc["city"] | "";
      n.region      = doc["regionName"] | "";
      n.country     = doc["country"] | "";
      n.countryCode = doc["countryCode"] | "";
      n.lat         = doc["lat"] | 0.0f;
      n.lon         = doc["lon"] | 0.0f;
    } else {
      doc.clear();
      if (!getJson("https://ipapi.co/json/", doc) || doc["error"].is<bool>()) {
        setError("Location lookup failed");
        Log.println("[Weather] Location lookup failed");
        return false;
      }
      n.city        = doc["city"] | "";
      n.region      = doc["region"] | "";
      n.country     = doc["country_name"] | "";
      n.countryCode = doc["country_code"] | "";
      n.lat         = doc["latitude"] | 0.0f;
      n.lon         = doc["longitude"] | 0.0f;
    }

    bool hadLocation;
    xSemaphoreTake(_mtx, portMAX_DELAY);
    hadLocation      = _d.haveLocation;
    _d.city          = n.city;
    _d.region        = n.region;
    _d.country       = n.country;
    _d.countryCode   = n.countryCode;
    _d.lat           = n.lat;
    _d.lon           = n.lon;
    _d.manual        = n.manual;
    _d.haveLocation  = true;
    _d.error         = "";
    xSemaphoreGive(_mtx);

    if (!hadLocation) _firstLocation = true;
    saveCache();
    Log.printf("[Weather] Location: %s, %s, %s (%.3f, %.3f)%s\n", n.city.c_str(), n.region.c_str(),
               n.countryCode.c_str(), n.lat, n.lon, n.manual ? " [manual]" : "");
    return true;
  }

  // ---- weather (also gives the exact UTC offset for the location) ----
  bool fetchWeather() {
    WeatherData s = snapshot();
    char url[400];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m,is_day"
             "&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset&timezone=auto&forecast_days=1",
             s.lat, s.lon);
    JsonDocument doc;
    if (!getJson(url, doc) || !doc["current"].is<JsonObject>()) {
      setError("Weather download failed");
      Log.println("[Weather] Weather download failed");
      return false;
    }
    JsonObject c = doc["current"];
    long offset = doc["utc_offset_seconds"] | s.utcOffset;
    String sr = doc["daily"]["sunrise"][0] | "";
    String ss = doc["daily"]["sunset"][0] | "";

    xSemaphoreTake(_mtx, portMAX_DELAY);
    _d.temp        = c["temperature_2m"] | 0.0f;
    _d.feels       = c["apparent_temperature"] | 0.0f;
    _d.humidity    = c["relative_humidity_2m"] | 0.0f;
    _d.wind        = c["wind_speed_10m"] | 0.0f;
    _d.code        = c["weather_code"] | -1;
    _d.isDay       = (c["is_day"] | 1) == 1;
    _d.tmax        = doc["daily"]["temperature_2m_max"][0] | 0.0f;
    _d.tmin        = doc["daily"]["temperature_2m_min"][0] | 0.0f;
    _d.sunrise     = sr.length() >= 16 ? sr.substring(11, 16) : "";
    _d.sunset      = ss.length() >= 16 ? ss.substring(11, 16) : "";
    _d.haveWeather = true;
    _d.updatedMs   = millis();
    _d.error       = "";
    bool offChanged = !_d.haveOffset || _d.utcOffset != offset;
    _d.utcOffset   = offset;
    _d.haveOffset  = true;
    int code = _d.code;
    float temp = _d.temp;
    xSemaphoreGive(_mtx);

    if (offChanged) {
      _offsetChanged = true;
      saveCache();
    }
    Log.printf("[Weather] %.1f C, %s\n", temp, weatherText(code));
    return true;
  }

  void setError(const String &e) {
    xSemaphoreTake(_mtx, portMAX_DELAY);
    _d.error = e;
    xSemaphoreGive(_mtx);
  }

  // ---- cache (flash) ----
  void loadCache() {
    Preferences p;
    p.begin("weather", true);
    _manualCity      = p.getString("manual", "");
    _d.haveLocation  = p.getBool("hasLoc", false);
    _d.city          = p.getString("city", "");
    _d.region        = p.getString("region", "");
    _d.country       = p.getString("country", "");
    _d.countryCode   = p.getString("cc", "");
    _d.lat           = p.getFloat("lat", 0);
    _d.lon           = p.getFloat("lon", 0);
    _d.manual        = p.getBool("isManual", false);
    _d.haveOffset    = p.isKey("offset");
    _d.utcOffset     = p.getLong("offset", 19800);
    p.end();
  }

  void saveCache() {
    WeatherData s = snapshot();
    Preferences p;
    p.begin("weather", false);
    p.putBool("hasLoc", s.haveLocation);
    p.putString("city", s.city);
    p.putString("region", s.region);
    p.putString("country", s.country);
    p.putString("cc", s.countryCode);
    p.putFloat("lat", s.lat);
    p.putFloat("lon", s.lon);
    p.putBool("isManual", s.manual);
    if (s.haveOffset) p.putLong("offset", s.utcOffset);
    p.end();
  }

  WeatherData       _d;
  String            _manualCity;
  SemaphoreHandle_t _mtx = nullptr;
  volatile bool     _forceLocation = false;
  volatile bool     _forceWeather = false;
  volatile bool     _offsetChanged = false;
  volatile bool     _firstLocation = false;
  bool              _locFetchedThisBoot = false;
};

// UTC offset (seconds) -> POSIX TZ string, e.g. 19800 -> "<+0530>-05:30"
String tzFromOffset(long off) {
  long a = labs(off);
  char buf[24];
  snprintf(buf, sizeof(buf), "<%c%02ld%02ld>%c%02ld:%02ld", off >= 0 ? '+' : '-', a / 3600, (a % 3600) / 60,
           off >= 0 ? '-' : '+', a / 3600, (a % 3600) / 60);
  return String(buf);
}

bool timeValid() { return time(nullptr) > 1700000000; }
