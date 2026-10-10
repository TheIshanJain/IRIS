/*
 * Faces.h - extra full-screen faces (the eye expressions in Eyes.h are untouched):
 *   - clock face (24 h) with date, weather and locality
 *   - boot greeting in the local language + the user's name
 *   - "Wi-Fi connected" screen
 * Needs printCentered(), the colours and helpers from Eyes.h, and Weather.h.
 */
#pragma once

const uint16_t C_SUN   = MK_RGB(255, 200, 60);
const uint16_t C_CLOUD = MK_RGB(200, 210, 225);
const uint16_t C_DARKCLOUD = MK_RGB(130, 140, 160);

// ---------------------------------------------------------------------------
//  Weather icons (about 32 x 28 px, centred on x, y)
// ---------------------------------------------------------------------------
void iconCloud(int x, int y, uint16_t c) {
  gfx->fillCircle(x - 7, y + 2, 7, c);
  gfx->fillCircle(x + 1, y - 3, 9, c);
  gfx->fillCircle(x + 9, y + 2, 7, c);
  gfx->fillRoundRect(x - 14, y + 2, 30, 8, 4, c);
}

void iconSun(int x, int y, int r) {
  for (int a = 0; a < 360; a += 45) {
    float rad = a * DEG_TO_RAD;
    thickLine(x + cosf(rad) * (r + 3), y + sinf(rad) * (r + 3),
              x + cosf(rad) * (r + 7), y + sinf(rad) * (r + 7), 2, C_SUN);
  }
  gfx->fillCircle(x, y, r, C_SUN);
}

void iconMoon(int x, int y, int r) {
  gfx->fillCircle(x, y, r, C_SUN);
  gfx->fillCircle(x + r / 2, y - r / 3, r - 1, C_BLACK);
}

void drawWeatherIcon(int x, int y, int code, bool day) {
  if (code == 0 || code == 1) {                       // clear
    if (day) iconSun(x, y, 7); else iconMoon(x, y, 9);
  } else if (code == 2) {                             // partly cloudy
    if (day) iconSun(x - 6, y - 6, 5); else iconMoon(x - 6, y - 6, 6);
    iconCloud(x + 3, y + 3, C_CLOUD);
  } else if (code == 3) {
    iconCloud(x, y, C_CLOUD);
  } else if (code == 45 || code == 48) {              // fog
    for (int i = 0; i < 3; i++) thickLine(x - 13 + i * 3, y - 6 + i * 6, x + 13 - i * 3, y - 6 + i * 6, 3, C_CLOUD);
  } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {   // snow
    iconCloud(x, y - 4, C_CLOUD);
    for (int i = -1; i <= 1; i++) gfx->fillCircle(x + i * 8, y + 12, 2, C_WHITE);
  } else if (code >= 95) {                            // thunder
    iconCloud(x, y - 4, C_DARKCLOUD);
    gfx->fillTriangle(x + 1, y + 4, x - 5, y + 13, x + 1, y + 12, C_SUN);
    gfx->fillTriangle(x + 1, y + 10, x + 5, y + 10, x - 2, y + 19, C_SUN);
  } else if (code >= 51) {                            // drizzle / rain / showers
    iconCloud(x, y - 4, C_DARKCLOUD);
    for (int i = -1; i <= 1; i++) thickLine(x + i * 8 + 2, y + 8, x + i * 8 - 1, y + 15, 2, C_TEAR);
  } else {
    iconCloud(x, y, C_CLOUD);
  }
}

// ---------------------------------------------------------------------------
//  Clock face (24-hour)
// ---------------------------------------------------------------------------
void drawClockFace(const WeatherData &w) {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  bool valid = timeValid();

  // seconds ring around the edge
  gfx->fillArc(MID_X, 120, 118, 114, 0, 360, C_TRACK);
  if (valid) gfx->fillArc(MID_X, 120, 118, 114, 270, 270 + (t.tm_sec + 1) * 6, C_CYAN);

  char buf[24];
  if (valid) {
    strftime(buf, sizeof(buf), "%a %d %b", &t);
    String d = buf;
    d.toUpperCase();
    printCentered(d, 44, 2, C_CYAN);
    strftime(buf, sizeof(buf), "%H:%M", &t);
    printCentered(buf, 72, 6, C_WHITE);
  } else {
    printCentered("--:--", 72, 6, C_DIM);
    printCentered("Waiting for time", 44, 1, C_LABEL);
  }

  if (w.haveWeather) {
    String ts = String((int)lroundf(w.temp));
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->setTextSize(3);
    gfx->getTextBounds(ts.c_str(), 0, 0, &x1, &y1, &tw, &th);
    int total = 32 + 8 + tw + 8 + 16;
    int x0 = MID_X - total / 2;
    drawWeatherIcon(x0 + 16, 144, w.code, w.isDay);
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(x0 + 40, 134);
    gfx->print(ts);
    gfx->drawCircle(x0 + 40 + tw + 3, 136, 3, C_WHITE);   // degree sign
    gfx->drawCircle(x0 + 40 + tw + 3, 136, 2, C_WHITE);
    gfx->setCursor(x0 + 40 + tw + 9, 134);
    gfx->print("C");
    printCentered(weatherText(w.code), 166, 2, C_CYAN);
  } else if (WiFi.status() == WL_CONNECTED) {
    printCentered("Getting weather...", 146, 1, C_LABEL);
  }

  if (w.haveLocation) {
    String loc = w.city;
    if (w.region.length() && w.region != w.city) loc += ", " + w.region;
    if (loc.length() > 28) loc = loc.substring(0, 27) + ".";
    printCentered(loc, 192, 1, C_LABEL);
  }
}

// ---------------------------------------------------------------------------
//  Boot greeting:  local hello + name, with happy eyes and sparkles
// ---------------------------------------------------------------------------
uint8_t fitSize(const String &s, uint8_t preferred, int maxW) {
  for (uint8_t sz = preferred; sz > 1; sz--)
    if ((int)s.length() * 6 * sz <= maxW) return sz;
  return 1;
}

String timeOfDayLine() {
  if (!timeValid()) return "Nice to see you!";
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  int h = t.tm_hour;
  if (h >= 5 && h < 12)  return "Good morning!";
  if (h >= 12 && h < 17) return "Good afternoon!";
  if (h >= 17 && h < 21) return "Good evening!";
  if (h >= 21)           return "Good night!";
  return "Up late, huh?";
}

void drawGreeting(uint32_t t, const String &word, const String &name, const String &sub) {
  float bob = -fabsf(sinf(t * 0.008f)) * 4;
  happyEye(92, 58 + bob);
  happyEye(148, 58 + bob);
  blush(80, 92 + bob);
  blush(160, 92 + bob);
  sparkle(46, 70, 3 + (int)(fabsf(sinf(t * 0.006f)) * 6), C_GOLD);
  sparkle(196, 62, 3 + (int)(fabsf(cosf(t * 0.005f)) * 6), C_GOLD);
  sparkle(186, 178, 2 + (int)(fabsf(sinf(t * 0.007f + 1)) * 4), C_PINK);

  String line1 = name.length() ? word + "," : word + "!";
  printCentered(line1, 112, fitSize(line1, 3, 200), C_CYAN);
  if (name.length()) {
    String line2 = name + "!";
    printCentered(line2, 146, fitSize(line2, 3, 190), C_PINK);
  }
  if (t > 600) printCentered(sub, 184, 1, C_LABEL);   // appears a moment later
}

// ---------------------------------------------------------------------------
//  "Wi-Fi connected" screen
// ---------------------------------------------------------------------------
void drawWifiBanner(uint32_t t, const String &ssid, const String &ip) {
  const int cx = MID_X, cy = 112;
  gfx->fillCircle(cx, cy, 6, C_GREEN);
  const int radii[3] = { 20, 34, 48 };
  for (int i = 0; i < 3; i++)
    if (t > (uint32_t)(i + 1) * 180) roundArc(cx, cy, radii[i], 7, 228, 312, C_GREEN);

  printCentered("Wi-Fi connected", 132, 2, C_GREEN);
  printCentered(ssid, 158, fitSize(ssid, 2, 180), C_WHITE);
  printCentered(ip, 184, 1, C_LABEL);
}
