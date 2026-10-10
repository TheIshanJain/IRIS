/*
 * Battery.h - battery level for the Waveshare ESP32-S3-LCD-1.28.
 *
 * The battery (MX1.25 connector) is wired to GPIO1 through a 200k/100k divider,
 * so battery voltage = pin voltage x 3 (per the Waveshare documentation).
 *
 * The board has no "charging" signal, so charging/full are ESTIMATED:
 *   charging = voltage has risen noticeably over the last few minutes
 *   full     = voltage at or above ~4.15 V
 *   none     = no battery connected (reading near 0 V)
 */
#pragma once

#define BAT_PIN          1
#define BAT_SAMPLE_MS    10000      // one sample every 10 s
#define BAT_HISTORY      18         // 3 minutes of history

class Battery {
public:
  enum class State { None, Discharging, Charging, Full };

  void begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(BAT_PIN, ADC_11db);
    _v = readVolts();
    for (int i = 0; i < BAT_HISTORY; i++) _hist[i] = _v;
    evaluate();
    Log.printf("[Battery] %.2f V, %d%% (%s)\n", _v, _pct, stateName());
  }

  // Returns true when the state changed (charging started, became full, ...)
  bool update() {
    uint32_t now = millis();
    // fast smoothing every second, history every 10 s
    if (now - _lastFast >= 1000) {
      _lastFast = now;
      _v = _v * 0.8f + readVolts() * 0.2f;
    }
    if (now - _lastSample < BAT_SAMPLE_MS) return false;
    _lastSample = now;
    _hist[_head] = _v;
    _head = (_head + 1) % BAT_HISTORY;
    State before = _state;
    evaluate();
    if (_state != before) {
      Log.printf("[Battery] %.2f V, %d%% -> %s\n", _v, _pct, stateName());
      return true;
    }
    return false;
  }

  float volts() const { return _v; }
  int percent() const { return _pct; }
  State state() const { return _state; }
  bool low() const { return _state == State::Discharging && _pct <= 15; }

  const char *stateName() const {
    switch (_state) {
      case State::None:        return "none";
      case State::Charging:    return "charging";
      case State::Full:        return "full";
      default:                 return "discharging";
    }
  }

private:
  static float readVolts() {
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(BAT_PIN);
    return (mv / 8.0f) * 3.0f / 1000.0f;
  }

  // Typical single-cell Li-ion curve
  static int toPercent(float v) {
    const float vs[] = { 3.30f, 3.40f, 3.50f, 3.60f, 3.70f, 3.80f, 3.90f, 4.00f, 4.10f, 4.20f };
    const int   ps[] = { 0,     5,     10,    20,    35,    50,    65,    80,    90,    100 };
    if (v <= vs[0]) return 0;
    if (v >= vs[9]) return 100;
    for (int i = 1; i < 10; i++)
      if (v < vs[i]) return ps[i - 1] + (int)((v - vs[i - 1]) / (vs[i] - vs[i - 1]) * (ps[i] - ps[i - 1]));
    return 100;
  }

  void evaluate() {
    _pct = toPercent(_v);
    if (_v < 2.5f) {
      _state = State::None;
      return;
    }
    float oldest = _hist[_head];                 // ~3 minutes ago
    float lowest = _hist[0];
    for (int i = 1; i < BAT_HISTORY; i++) lowest = min(lowest, _hist[i]);
    if (_v >= 4.15f) _state = State::Full;
    else if (_v - lowest > 0.03f && _v >= oldest) _state = State::Charging;
    else _state = State::Discharging;
  }

  float    _v = 0;
  int      _pct = 0;
  State    _state = State::None;
  float    _hist[BAT_HISTORY];
  int      _head = 0;
  uint32_t _lastFast = 0, _lastSample = 0;
};
