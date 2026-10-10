/*
 * BleSetup.h - Bluetooth LE link to the IRIS app.
 *
 * Characteristics:
 *   RX  (app -> device, write)   JSON commands, one per line ('\n' terminated)
 *   TX  (device -> app, notify)  JSON messages, one per line ('\n' terminated)
 *   FW  (app -> device, write)   raw firmware bytes during a developer upload
 * Messages longer than one BLE packet are split into chunks and re-joined at the newline.
 *
 * Security:
 *   - First time: a random 6-digit PIN is shown on the screen; the app sends
 *     {"cmd":"auth","pin":"123456"}. On success the phone receives a secret token.
 *   - Afterwards the app reconnects with {"cmd":"auth","token":"..."} - no PIN, so it
 *     can reconnect by itself in the background. Up to 4 trusted phones are remembered.
 *   - The PIN screen only appears if the phone hasn't proven itself within ~2.5 s.
 *   - 3 wrong PINs = disconnect.
 */
#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Must match BleClient.kt in the app
#define BLE_SERVICE_UUID "6e1a0001-7b6e-4c3a-9d0e-c0ffee000001"
#define BLE_RX_UUID      "6e1a0002-7b6e-4c3a-9d0e-c0ffee000001"
#define BLE_TX_UUID      "6e1a0003-7b6e-4c3a-9d0e-c0ffee000001"
#define BLE_FW_UUID      "6e1a0004-7b6e-4c3a-9d0e-c0ffee000001"

#define BLE_LINE_MAX     512
#define BLE_QUEUE_LEN    32       // notification icons arrive as ~20 quick lines
#define BLE_MAX_PIN_FAIL 3
#define BLE_TRUSTED_MAX  4
#define BLE_PIN_DELAY_MS 2500     // trusted phones prove themselves within this time

class BleSetup;

class BleServerCb : public BLEServerCallbacks {
public:
  explicit BleServerCb(BleSetup *o) : _o(o) {}
  void onConnect(BLEServer *s) override;
  void onDisconnect(BLEServer *s) override;
private:
  BleSetup *_o;
};

class BleRxCb : public BLECharacteristicCallbacks {
public:
  explicit BleRxCb(BleSetup *o) : _o(o) {}
  void onWrite(BLECharacteristic *c) override;
private:
  BleSetup *_o;
};

class BleFwCb : public BLECharacteristicCallbacks {
public:
  explicit BleFwCb(BleSetup *o) : _o(o) {}
  void onWrite(BLECharacteristic *c) override;
private:
  BleSetup *_o;
};

class BleSetup {
public:
  // Called from the BLE task for every firmware chunk (keep it short)
  std::function<void(const uint8_t *data, size_t len)> onFirmwareData;

  void begin(const String &name) {
    _queue = xQueueCreate(BLE_QUEUE_LEN, BLE_LINE_MAX);
    loadTrusted();

    BLEDevice::init(name.c_str());
    BLEDevice::setMTU(517);

    _server = BLEDevice::createServer();
    _server->setCallbacks(new BleServerCb(this));

    BLEService *svc = _server->createService(BLE_SERVICE_UUID);
    _rx = svc->createCharacteristic(BLE_RX_UUID,
                                    BLECharacteristic::PROPERTY_WRITE |
                                    BLECharacteristic::PROPERTY_WRITE_NR);
    _rx->setCallbacks(new BleRxCb(this));

    _tx = svc->createCharacteristic(BLE_TX_UUID,
                                    BLECharacteristic::PROPERTY_NOTIFY |
                                    BLECharacteristic::PROPERTY_READ);
    _tx->addDescriptor(new BLE2902());

    _fw = svc->createCharacteristic(BLE_FW_UUID,
                                    BLECharacteristic::PROPERTY_WRITE |
                                    BLECharacteristic::PROPERTY_WRITE_NR);
    _fw->setCallbacks(new BleFwCb(this));
    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    BLEDevice::startAdvertising();
    Log.printf("[BLE] Advertising as '%s' (%d trusted phone(s))\n", name.c_str(), _trustedCount);
  }

  // Call from loop()
  void loop() {
    if (_restartAdv && (int32_t)(millis() - _restartAt) >= 0) {
      _restartAdv = false;
      _server->startAdvertising();
    }
  }

  // Pop the next complete command (returns false if none / invalid JSON)
  bool poll(JsonDocument &out) {
    char line[BLE_LINE_MAX];
    while (xQueueReceive(_queue, line, 0) == pdTRUE) {
      if (deserializeJson(out, line) == DeserializationError::Ok) return true;
      Log.println("[BLE] Ignored invalid JSON");
    }
    return false;
  }

  // Send one JSON message (split into MTU-sized notifications)
  void send(JsonDocument &doc) {
    if (!_connected) return;
    String s;
    serializeJson(doc, s);
    s += '\n';

    uint16_t mtu = _server->getPeerMTU(_connId);
    if (mtu < 23) mtu = 23;
    size_t chunk = min((size_t)(mtu - 3), (size_t)500);

    for (size_t off = 0; off < s.length(); off += chunk) {
      size_t n = min(chunk, s.length() - off);
      _tx->setValue((uint8_t *)s.c_str() + off, n);
      _tx->notify();
      delay(8);   // give the stack time to send
    }
  }

  // ---- first-time pairing with the PIN; returns a new token ("" if wrong) ----
  String checkPin(const String &pin) {
    if (!_connected) return "";
    if (pin == _pin) {
      _authed = true;
      return addTrusted();
    }
    if (++_pinFails >= BLE_MAX_PIN_FAIL) {
      Log.println("[BLE] Too many wrong PINs, disconnecting");
      _server->disconnect(_connId);
    }
    return "";
  }

  // ---- reconnect of a phone that paired before ----
  bool checkToken(const String &token) {
    if (!_connected || token.length() != 32) return false;
    for (int i = 0; i < _trustedCount; i++) {
      if (_trusted[i] == token) {
        _authed = true;
        return true;
      }
    }
    return false;
  }

  void forgetToken(const String &token) {
    for (int i = 0; i < _trustedCount; i++) {
      if (_trusted[i] == token) {
        for (int j = i; j < _trustedCount - 1; j++) _trusted[j] = _trusted[j + 1];
        _trustedCount--;
        saveTrusted();
        Log.println("[BLE] Phone forgotten");
        return;
      }
    }
  }

  void setDev(bool on) { _dev = on; }
  bool isDev() const { return _connected && _authed && _dev; }

  bool isConnected() const { return _connected; }
  bool isAuthed() const { return _connected && _authed; }
  // Show the PIN only if the phone hasn't used its token right after connecting
  bool showPin() const { return _connected && !_authed && millis() - _connectedAt > BLE_PIN_DELAY_MS; }
  const String &pin() const { return _pin; }
  int trustedCount() const { return _trustedCount; }

  void disconnectPhone() {
    if (_connected) _server->disconnect(_connId);
  }

private:
  friend class BleServerCb;
  friend class BleRxCb;
  friend class BleFwCb;

  void handleConnect() {
    _connId      = _server->getConnId();
    _pin         = String(random(100000, 1000000));
    _pinFails    = 0;
    _authed      = false;
    _dev         = false;
    _rxBuf       = "";
    _connectedAt = millis();
    _connected   = true;
    Log.println("[BLE] Phone connected");
  }

  void handleDisconnect() {
    _connected  = false;
    _authed     = false;
    _dev        = false;
    _restartAdv = true;
    _restartAt  = millis() + 300;
    Log.println("[BLE] Phone disconnected");
  }

  // Runs in the BLE task: just collect bytes and queue full lines
  void handleWrite(const String &data) {
    for (size_t i = 0; i < data.length(); i++) {
      char ch = data[i];
      if (ch == '\n') {
        if (_rxBuf.length()) {
          char line[BLE_LINE_MAX];
          strncpy(line, _rxBuf.c_str(), BLE_LINE_MAX - 1);
          line[BLE_LINE_MAX - 1] = 0;
          if (xQueueSend(_queue, line, pdMS_TO_TICKS(100)) != pdTRUE) Log.println("[BLE] Queue full, line dropped");
        }
        _rxBuf = "";
      } else if (ch != '\r') {
        _rxBuf += ch;
        if (_rxBuf.length() >= BLE_LINE_MAX - 1) _rxBuf = "";   // too long, drop
      }
    }
  }

  void handleFw(BLECharacteristic *c) {
    if (!isDev() || !onFirmwareData) return;
    String v = c->getValue();
    if (v.length()) onFirmwareData((const uint8_t *)v.c_str(), v.length());
  }

  // ---- trusted phone tokens (flash) ----
  String addTrusted() {
    char tok[33];
    for (int i = 0; i < 4; i++) snprintf(tok + i * 8, 9, "%08lx", (unsigned long)esp_random());
    if (_trustedCount == BLE_TRUSTED_MAX) {          // drop the oldest
      for (int j = 0; j < BLE_TRUSTED_MAX - 1; j++) _trusted[j] = _trusted[j + 1];
      _trustedCount--;
    }
    _trusted[_trustedCount++] = tok;
    saveTrusted();
    Log.printf("[BLE] Phone trusted (%d total)\n", _trustedCount);
    return String(tok);
  }

  void loadTrusted() {
    Preferences p;
    p.begin("trusted", true);
    _trustedCount = min((int)p.getUChar("n", 0), BLE_TRUSTED_MAX);
    for (int i = 0; i < _trustedCount; i++) _trusted[i] = p.getString(("t" + String(i)).c_str(), "");
    p.end();
  }

  void saveTrusted() {
    Preferences p;
    p.begin("trusted", false);
    p.clear();
    p.putUChar("n", _trustedCount);
    for (int i = 0; i < _trustedCount; i++) p.putString(("t" + String(i)).c_str(), _trusted[i]);
    p.end();
  }

  BLEServer         *_server = nullptr;
  BLECharacteristic *_rx = nullptr;
  BLECharacteristic *_tx = nullptr;
  BLECharacteristic *_fw = nullptr;
  QueueHandle_t      _queue = nullptr;
  String             _rxBuf;
  String             _pin;
  String             _trusted[BLE_TRUSTED_MAX];
  int                _trustedCount = 0;
  uint16_t           _connId = 0;
  int                _pinFails = 0;
  uint32_t           _connectedAt = 0;
  volatile bool      _connected = false;
  volatile bool      _authed = false;
  volatile bool      _dev = false;
  volatile bool      _restartAdv = false;
  uint32_t           _restartAt = 0;
};

inline void BleServerCb::onConnect(BLEServer *) { _o->handleConnect(); }
inline void BleServerCb::onDisconnect(BLEServer *) { _o->handleDisconnect(); }
inline void BleRxCb::onWrite(BLECharacteristic *c) { _o->handleWrite(c->getValue()); }
inline void BleFwCb::onWrite(BLECharacteristic *c) { _o->handleFw(c); }
