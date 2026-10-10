#include "CloudOTA.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

void CloudOTA::begin(const char *currentVersion, const char *manifestUrl) {
  _current     = currentVersion;
  _manifestUrl = manifestUrl;
}

const char *CloudOTA::stateName(State s) {
  switch (s) {
    case State::Idle:        return "idle";
    case State::Checking:    return "checking";
    case State::UpToDate:    return "up_to_date";
    case State::Downloading: return "downloading";
    case State::Success:     return "success";
    case State::Failed:      return "failed";
  }
  return "unknown";
}

void CloudOTA::setState(State s, const String &msg) {
  _state   = s;
  _message = msg;
  log_i("[CloudOTA] %s: %s", stateName(s), msg.c_str());
  if (_onState) _onState(s, msg);
}

// "1.2.10" vs "1.2.9" -> 1.  Leading 'v' is ignored. Up to 4 numeric parts.
int CloudOTA::compareVersions(const String &a, const String &b) {
  const char *pa = a.c_str();
  const char *pb = b.c_str();
  if (*pa == 'v' || *pa == 'V') pa++;
  if (*pb == 'v' || *pb == 'V') pb++;
  for (int i = 0; i < 4; i++) {
    char *ea, *eb;
    long va = strtol(pa, &ea, 10);
    long vb = strtol(pb, &eb, 10);
    if (va != vb) return va < vb ? -1 : 1;
    pa = (*ea == '.') ? ea + 1 : ea;
    pb = (*eb == '.') ? eb + 1 : eb;
  }
  return 0;
}

WiFiClient *CloudOTA::makeClient(const String &url, std::unique_ptr<WiFiClient> &holder) {
  if (url.startsWith("https://")) {
    WiFiClientSecure *c = new WiFiClientSecure();
    if (_rootCA) c->setCACert(_rootCA);
    else         c->setInsecure();
    holder.reset(c);
  } else {
    holder.reset(new WiFiClient());
  }
  return holder.get();
}

bool CloudOTA::fetchManifest(String &version, String &url, String &md5) {
  std::unique_ptr<WiFiClient> holder;
  WiFiClient *client = makeClient(_manifestUrl, holder);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // GitHub release links redirect
  http.setTimeout(15000);
  if (!http.begin(*client, _manifestUrl)) {
    _error = "Bad manifest URL";
    return false;
  }
  http.addHeader("Cache-Control", "no-cache");

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    _error = "Manifest HTTP " + String(code);
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    _error = "Manifest is not valid JSON";
    return false;
  }

  version = doc["version"] | "";
  url     = doc["url"] | "";
  md5     = doc["md5"] | "";
  String device = doc["device"] | "";

  if (_deviceType.length() && device.length() && device != _deviceType) {
    _error = "Manifest is for '" + device + "'";
    return false;
  }
  if (!version.length() || !url.length()) {
    _error = "Manifest needs 'version' and 'url'";
    return false;
  }
  return true;
}

bool CloudOTA::downloadAndFlash(const String &url, const String &md5) {
  std::unique_ptr<WiFiClient> holder;
  WiFiClient *client = makeClient(url, holder);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  if (!http.begin(*client, url)) {
    _error = "Bad firmware URL";
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    _error = "Firmware HTTP " + String(code);
    http.end();
    return false;
  }

  int total = http.getSize();
  if (total <= 0) {
    _error = "Server did not send the file size";
    http.end();
    return false;
  }

  if (!Update.begin(total, U_FLASH)) {
    _error = String("No room for update: ") + Update.errorString();
    http.end();
    return false;
  }
  if (md5.length() == 32) Update.setMD5(md5.c_str());

  WiFiClient *stream = http.getStreamPtr();
  static uint8_t buf[2048];
  int written = 0, lastPct = -1;
  uint32_t lastData = millis();

  if (_onProgress) _onProgress(0);

  while (written < total) {
    size_t avail = stream->available();
    if (avail) {
      int n = stream->readBytes(buf, min(avail, sizeof(buf)));
      if (n > 0) {
        if (Update.write(buf, n) != (size_t)n) {
          _error = String("Flash write failed: ") + Update.errorString();
          Update.abort();
          http.end();
          return false;
        }
        written += n;
        lastData = millis();
        int pct = (int)((int64_t)written * 100 / total);
        if (pct != lastPct) {
          lastPct = pct;
          if (_onProgress) _onProgress(pct);
        }
      }
    } else {
      if (millis() - lastData > 20000) {
        _error = "Download stalled";
        Update.abort();
        http.end();
        return false;
      }
      delay(2);
    }
  }
  http.end();

  if (!Update.end(true)) {   // verifies size + MD5 and marks the new slot bootable
    _error = String("Verify failed: ") + Update.errorString();
    return false;
  }
  return true;
}

CloudOTA::State CloudOTA::checkNow() {
  if (_busy) return _state;
  if (WiFi.status() != WL_CONNECTED) {
    setState(State::Failed, "No internet connection");
    return _state;
  }
  _busy        = true;
  _everChecked = true;
  _lastCheck   = millis();

  setState(State::Checking, "Checking for updates");

  String version, url, md5;
  if (!fetchManifest(version, url, md5)) {
    setState(State::Failed, _error);
    _busy = false;
    return _state;
  }
  _latest = version;

  if (compareVersions(version, _current) <= 0) {
    setState(State::UpToDate, String("Up to date (v") + _current + ")");
    _busy = false;
    return _state;
  }

  setState(State::Downloading, "Updating to v" + version);
  if (!downloadAndFlash(url, md5)) {
    setState(State::Failed, _error);
    _busy = false;
    return _state;
  }

  setState(State::Success, "Updated to v" + version + ", rebooting");
  _busy = false;
  if (_autoReboot) {
    delay(1500);
    ESP.restart();
  }
  return _state;
}

void CloudOTA::loop() {
  if (_busy) return;

  if (WiFi.status() != WL_CONNECTED) {
    _connectedSince = 0;
    return;
  }
  uint32_t now = millis();
  if (_connectedSince == 0) _connectedSince = now ? now : 1;

  bool due = _checkRequested;
  if (!due && !_everChecked && now - _connectedSince > _firstDelay) due = true;
  if (!due && _everChecked && _interval && now - _lastCheck > _interval) due = true;

  if (due) {
    _checkRequested = false;
    checkNow();
  }
}
