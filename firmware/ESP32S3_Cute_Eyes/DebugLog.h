/*
 * DebugLog.h - prints to Serial AND keeps the last LOG_LINES lines in RAM
 * so the web UI can show a live log. Use Log.printf(...) / Log.println(...).
 */
#pragma once
#include <Arduino.h>

#define LOG_LINES     80
#define LOG_LINE_MAX  160

class DebugLog : public Print {
public:
  size_t write(uint8_t c) override {
    Lock lk(this);
    Serial.write(c);
    if (c == '\r') return 1;
    if (c == '\n') {
      commit();
    } else if (_len < LOG_LINE_MAX - 1) {
      _cur[_len++] = (char)c;
    }
    return 1;
  }

  size_t write(const uint8_t *buf, size_t n) override {
    Lock lk(this);
    for (size_t i = 0; i < n; i++) write(buf[i]);
    return n;
  }

  // Lines newer than `since` (sequence numbers start at 1)
  template <typename F>
  void forEachSince(uint32_t since, F fn) {
    Lock lk(this);
    uint32_t first = _seq > LOG_LINES ? _seq - LOG_LINES + 1 : 1;
    if (since + 1 > first) first = since + 1;
    for (uint32_t s = first; s <= _seq; s++) {
      const Line &l = _lines[s % LOG_LINES];
      fn(s, l.ms, l.text);
    }
  }

  uint32_t lastSeq() const { return _seq; }

private:
  // Recursive mutex: the BLE task and the main loop may log at the same time
  struct Lock {
    DebugLog *d;
    explicit Lock(DebugLog *o) : d(o) {
      if (!d->_mtx) d->_mtx = xSemaphoreCreateRecursiveMutex();
      xSemaphoreTakeRecursive(d->_mtx, portMAX_DELAY);
    }
    ~Lock() { xSemaphoreGiveRecursive(d->_mtx); }
  };

  struct Line {
    uint32_t ms;
    char text[LOG_LINE_MAX];
  };

  void commit() {
    _cur[_len] = 0;
    _seq++;
    Line &l = _lines[_seq % LOG_LINES];
    l.ms = millis();
    memcpy(l.text, _cur, _len + 1);
    _len = 0;
  }

  Line     _lines[LOG_LINES];
  char     _cur[LOG_LINE_MAX];
  size_t   _len = 0;
  uint32_t _seq = 0;
  SemaphoreHandle_t _mtx = nullptr;
};

DebugLog Log;
