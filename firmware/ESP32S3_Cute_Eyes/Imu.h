/*
 * Imu.h - minimal QMI8658 accelerometer driver + shake detector.
 * Waveshare ESP32-S3-LCD-1.28: I2C on SDA=GPIO6, SCL=GPIO7, address 0x6B (or 0x6A).
 *
 * Shake = several sharp acceleration peaks within a short window.
 * Sensitivity 1 (needs a hard shake) .. 10 (a light shake is enough).
 */
#pragma once
#include <Wire.h>

#define IMU_SDA 6
#define IMU_SCL 7

class Imu {
public:
  bool begin() {
    Wire.begin(IMU_SDA, IMU_SCL, 400000);
    const uint8_t addrs[] = { 0x6B, 0x6A };
    for (uint8_t a : addrs) {
      _addr = a;
      if (read8(REG_WHO_AM_I) == 0x05) {
        _ok = true;
        break;
      }
    }
    if (!_ok) {
      Log.println("[IMU] QMI8658 not found on I2C (SDA 6 / SCL 7)");
      return false;
    }
    write8(REG_RESET, 0xB0);      // soft reset
    delay(20);
    write8(REG_CTRL1, 0x40);      // address auto-increment, little endian
    write8(REG_CTRL2, 0x25);      // accel +-8 g, ~250 Hz
    write8(REG_CTRL7, 0x01);      // accelerometer on, gyro off
    delay(20);
    update();
    Log.printf("[IMU] QMI8658 at 0x%02X, resting |a| = %.2f g\n", _addr, _mag);
    return true;
  }

  // Call often (every loop). Returns true once when a shake is detected.
  bool update() {
    if (!_ok) return false;
    uint8_t b[6];
    if (!readBytes(REG_AX_L, b, 6)) return false;
    ax = (int16_t)(b[0] | (b[1] << 8)) / 4096.0f;   // 4096 LSB/g at +-8 g
    ay = (int16_t)(b[2] | (b[3] << 8)) / 4096.0f;
    az = (int16_t)(b[4] | (b[5] << 8)) / 4096.0f;
    _mag = sqrtf(ax * ax + ay * ay + az * az);

    if (!enabled) return false;

    uint32_t now = millis();
    float dyn = fabsf(_mag - 1.0f);                  // remove gravity
    float thr = 2.4f - sensitivity * 0.18f;          // 1 -> 2.2 g, 10 -> 0.6 g

    if (now - _windowStart > 800) {                  // peaks must happen within 0.8 s
      _peaks = 0;
      _windowStart = now;
    }
    bool above = dyn > thr;
    if (above && !_wasAbove) _peaks++;               // count rising edges only
    _wasAbove = above;

    if (_peaks >= 3 && now - _lastShake > 1200) {    // 1.2 s cooldown
      _lastShake = now;
      _peaks = 0;
      shakeCount++;
      return true;
    }
    return false;
  }

  bool ok() const { return _ok; }
  float magnitude() const { return _mag; }

  float ax = 0, ay = 0, az = 0;
  bool enabled = true;
  int sensitivity = 5;
  uint32_t shakeCount = 0;

private:
  static constexpr uint8_t REG_WHO_AM_I = 0x00;
  static constexpr uint8_t REG_CTRL1    = 0x02;
  static constexpr uint8_t REG_CTRL2    = 0x03;
  static constexpr uint8_t REG_CTRL7    = 0x08;
  static constexpr uint8_t REG_AX_L     = 0x35;
  static constexpr uint8_t REG_RESET    = 0x60;

  uint8_t read8(uint8_t reg) {
    uint8_t v = 0xFF;
    readBytes(reg, &v, 1);
    return v;
  }

  bool readBytes(uint8_t reg, uint8_t *buf, uint8_t n) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(_addr, n) != n) return false;
    for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
  }

  void write8(uint8_t reg, uint8_t v) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(v);
    Wire.endTransmission();
  }

  uint8_t  _addr = 0x6B;
  bool     _ok = false;
  float    _mag = 1.0f;
  int      _peaks = 0;
  bool     _wasAbove = false;
  uint32_t _windowStart = 0;
  uint32_t _lastShake = 0;
};
