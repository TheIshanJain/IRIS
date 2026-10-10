/*
 * BleSetup.h - Bluetooth LE link to the mobile app.
 *
 * One service, two characteristics:
 *   RX  (app -> device, write)   JSON commands, one per line ('\n' terminated)
 *   TX  (device -> app, notify)  JSON messages, one per line ('\n' terminated)
 * Messages longer than one BLE packet are split into chunks and re-joined at the newline.
 *
 * Security: when a phone connects, a random 6-digit PIN is shown on the round
 * screen. The app must send {"cmd":"auth","pin":"123456"} before anything else.
 * 3 wrong PINs = disconnect.
 */
#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// Must match lib/device_link.dart in the mobile app
#define BLE_SERVICE_UUID "6e1a0001-7b6e-4c3a-9d0e-c0ffee000001"
#define BLE_RX_UUID      "6e1a0002-7b6e-4c3a-9d0e-c0ffee000001"
#define BLE_TX_UUID      "6e1a0003-7b6e-4c3a-9d0e-c0ffee000001"

#define BLE_LINE_MAX     512
#define BLE_MAX_PIN_FAIL 3

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

class BleSetup {
public:
  void begin(const String &name) {
    _queue = xQueueCreate(6, BLE_LINE_MAX);

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
    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    BLEDevice::startAdvertising();
    Log.printf("[BLE] Advertising as '%s'\n", name.c_str());
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

  bool checkPin(const String &pin) {
    if (!_connected) return false;
    if (pin == _pin) {
      _authed = true;
      return true;
    }
    if (++_pinFails >= BLE_MAX_PIN_FAIL) {
      Log.println("[BLE] Too many wrong PINs, disconnecting");
      _server->disconnect(_connId);
    }
    return false;
  }

  bool isConnected() const { return _connected; }
  bool isAuthed() const { return _connected && _authed; }
  const String &pin() const { return _pin; }

private:
  friend class BleServerCb;
  friend class BleRxCb;

  void handleConnect() {
    _connId   = _server->getConnId();
    _pin      = String(random(100000, 1000000));
    _pinFails = 0;
    _authed   = false;
    _rxBuf    = "";
    _connected = true;
    Log.printf("[BLE] Phone connected, PIN %s\n", _pin.c_str());
  }

  void handleDisconnect() {
    _connected  = false;
    _authed     = false;
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
          xQueueSend(_queue, line, 0);
        }
        _rxBuf = "";
      } else if (ch != '\r') {
        _rxBuf += ch;
        if (_rxBuf.length() >= BLE_LINE_MAX - 1) _rxBuf = "";   // too long, drop
      }
    }
  }

  BLEServer         *_server = nullptr;
  BLECharacteristic *_rx = nullptr;
  BLECharacteristic *_tx = nullptr;
  QueueHandle_t      _queue = nullptr;
  String             _rxBuf;
  String             _pin;
  uint16_t           _connId = 0;
  int                _pinFails = 0;
  volatile bool      _connected = false;
  volatile bool      _authed = false;
  volatile bool      _restartAdv = false;
  uint32_t           _restartAt = 0;
};

inline void BleServerCb::onConnect(BLEServer *) { _o->handleConnect(); }
inline void BleServerCb::onDisconnect(BLEServer *) { _o->handleDisconnect(); }
inline void BleRxCb::onWrite(BLECharacteristic *c) { _o->handleWrite(c->getValue()); }
