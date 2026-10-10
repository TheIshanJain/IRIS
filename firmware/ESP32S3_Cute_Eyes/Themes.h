/*
 * Themes.h - eye expression themes (chosen in the IRIS app).
 *
 *   Selectable (one at a time): Basic Emotions | Playful & Cute | Animated Styles | Special Themes
 *   Status only:                Interaction & Status - used by the device itself to show
 *                               connecting, charging, battery full/low, errors, notifications...
 *
 * "Playful & Cute" is the original set from Eyes.h, used as-is (same order, same
 * drawings), so expression numbers 1-11 mean exactly what they did before.
 *
 * Auto-cycle, "Next", the BOOT button and shake only pick from the selected theme.
 * The app's Developer section can still show any expression.
 */
#pragma once

enum ThemeId : uint8_t { TH_BASIC, TH_PLAYFUL, TH_ANIMATED, TH_SPECIAL, TH_STATUS, TH_COUNT };

const char *const THEME_NAMES[TH_COUNT] = {
  "Basic Emotions", "Playful & Cute", "Animated Styles", "Special Themes", "Interaction & Status"
};
const char *const THEME_DESC[TH_COUNT] = {
  "Core expressions for everyday use",
  "Fun expressions for a friendly desk buddy",
  "Stylish animated patterns for more personality",
  "Themed styles for different moods or modes",
  "Useful states for real-world interaction"
};
#define THEME_DEFAULT      TH_PLAYFUL             // same as before: the original set
#define THEME_DEFAULT_MASK (1 << TH_PLAYFUL)
#define THEME_SELECTABLE(t) ((t) < TH_COUNT && (t) != TH_STATUS)

// ---------------------------------------------------------------------------
//  Extra colours + small drawing helpers
// ---------------------------------------------------------------------------
const uint16_t T_RED     = MK_RGB(255, 70, 90);
const uint16_t T_RED_DK  = MK_RGB(45, 0, 10);
const uint16_t T_ORANGE  = MK_RGB(255, 140, 40);
const uint16_t T_YELLOW  = MK_RGB(255, 215, 70);
const uint16_t T_PURPLE  = MK_RGB(175, 95, 255);
const uint16_t T_VIOLET  = MK_RGB(95, 35, 190);
const uint16_t T_GREEN   = MK_RGB(60, 240, 120);
const uint16_t T_MATRIX  = MK_RGB(60, 255, 110);
const uint16_t T_MAT_DK  = MK_RGB(10, 80, 30);
const uint16_t T_NAVY    = MK_RGB(18, 16, 70);
const uint16_t T_DIMCYAN = MK_RGB(20, 70, 85);
const uint16_t T_PINK_DK = MK_RGB(90, 20, 50);

uint16_t hsv565(int h, uint8_t s, uint8_t v) {
  h = ((h % 360) + 360) % 360;
  uint8_t region = h / 60;
  uint16_t rem = (h % 60) * 255 / 60;
  uint8_t p = (v * (255 - s)) / 255;
  uint8_t q = (v * (255 - (s * rem) / 255)) / 255;
  uint8_t u = (v * (255 - (s * (255 - rem)) / 255)) / 255;
  uint8_t r, g, b;
  switch (region) {
    case 0:  r = v; g = u; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = u; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = u; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
  return MK_RGB(r, g, b);
}

uint16_t scaled(uint8_t r, uint8_t g, uint8_t b, float k) {
  k = constrain(k, 0.0f, 1.0f);
  return MK_RGB((int)(r * k), (int)(g * k), (int)(b * k));
}

void pill(int c, int y, int w, int h, uint16_t col) {
  if (h < 4) h = 4;
  gfx->fillRoundRect(c - w / 2, y - h / 2, w, h, min(w, h) / 2, col);
}

void heartShape(float x, float y, float s, uint16_t col) {   // s = half width
  float r = s * 0.53f;
  gfx->fillCircle(x - s * 0.47f, y - s * 0.22f, r, col);
  gfx->fillCircle(x + s * 0.47f, y - s * 0.22f, r, col);
  gfx->fillTriangle(x - s * 0.98f, y - s * 0.02f, x + s * 0.98f, y - s * 0.02f, x, y + s * 0.95f, col);
}

void star5(float x, float y, float r, uint16_t col) {
  float vx[10], vy[10];
  for (int i = 0; i < 10; i++) {
    float a = (-90 + i * 36) * DEG_TO_RAD;
    float rr = (i % 2 == 0) ? r : r * 0.45f;
    vx[i] = x + cosf(a) * rr;
    vy[i] = y + sinf(a) * rr;
  }
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    gfx->fillTriangle(x, y, vx[i], vy[i], vx[j], vy[j], col);
  }
}

// "u" shaped closed eye
void lowerLid(int c, int y, uint16_t col, int thick = 8) {
  roundArc(c, y - 14, 24, thick, 30, 150, col);
}

// Colour-able happy "^" eye (happyEye from Eyes.h is always cyan)
void happyEyeC(int c, int y, uint16_t col) {
  roundArc(c, y + 12, 26, 9, 205, 335, col);
}

// Eye with the top cut along a slanted line. outerHigh=true -> angry, false -> sad
void slantEye(int c, int y, bool left, bool outerHigh, uint16_t col) {
  gfx->fillEllipse(c, y + 4, 26, 20, col);
  int in = left ? 1 : -1;
  int ox = c - in * 32, ix = c + in * 32;
  int oy = outerHigh ? y - 14 : y + 8;     // where the cut meets the outer side
  int iy = outerHigh ? y + 8 : y - 12;     // where the cut meets the inner side
  gfx->fillTriangle(ox, y - 32, ix, y - 32, ix, iy, C_BLACK);
  gfx->fillTriangle(ox, y - 32, ix, iy, ox, oy, C_BLACK);
}

void ringEye(int c, int y, int r, int hole, uint16_t col) {
  gfx->fillCircle(c, y, r, col);
  gfx->fillCircle(c, y, hole, C_BLACK);
}

void wifiArcs(int cx, int cy, int shown, uint16_t col) {
  gfx->fillCircle(cx, cy, 6, col);
  const int radii[3] = { 20, 34, 48 };
  for (int i = 0; i < 3 && i < shown; i++) roundArc(cx, cy, radii[i], 7, 228, 312, col);
}

void battery(int x, int y, int level, uint16_t col, uint16_t fill) {   // level 0..4
  gfx->drawRoundRect(x, y, 50, 24, 5, col);
  gfx->drawRoundRect(x + 1, y + 1, 48, 22, 4, col);
  gfx->fillRoundRect(x + 50, y + 7, 5, 10, 2, col);
  for (int i = 0; i < level; i++) gfx->fillRoundRect(x + 5 + i * 11, y + 5, 8, 14, 2, fill);
}

// =====================================================================
//                         BASIC EMOTIONS
// =====================================================================

void xNeutral(uint32_t t) {
  halos();
  int h = blinking(t, 3000, 160) ? 8 : 52;
  pill(EYE_LX, EYE_Y, 28, h, C_CYAN);
  pill(EYE_RX, EYE_Y, 28, h, C_CYAN);
}

void xHappy(uint32_t t) {
  float b = sinf(t * 0.004f) * 3;
  halos(0, b);
  happyEye(EYE_LX, EYE_Y + b);
  happyEye(EYE_RX, EYE_Y + b);
}

void xBigSmile(uint32_t t) {
  float b = -fabsf(sinf(t * 0.006f)) * 4;
  halos(0, b);
  happyEye(EYE_LX, EYE_Y + b);
  happyEye(EYE_RX, EYE_Y + b);
  blush(EYE_LX - 6, EYE_Y + 30 + b);
  blush(EYE_RX + 6, EYE_Y + 30 + b);
  roundArc(MID_X, 138 + b, 26, 6, 25, 155, C_CYAN);
}

void xSad(uint32_t t) {
  float sag = sinf(t * 0.0015f) * 3;
  halos(0, sag);
  slantEye(EYE_LX, EYE_Y + sag, true, false, C_CYAN);
  slantEye(EYE_RX, EYE_Y + sag, false, false, C_CYAN);
  roundArc(MID_X, 178, 14, 4, 210, 330, C_CYAN);
  float p = (t % 3000) / 3000.0f;
  if (p > 0.5f) drop(EYE_LX + 12, EYE_Y + 26 + (p - 0.5f) * 70, 5, C_TEAR);
}

void xAngry(uint32_t t) {
  float j = (t % 900 < 300) ? random(-2, 3) : 0;     // angry shudder
  gfx->fillCircle(EYE_LX + j, EYE_Y, 42, T_RED_DK);
  gfx->fillCircle(EYE_RX + j, EYE_Y, 42, T_RED_DK);
  slantEye(EYE_LX + j, EYE_Y, true, true, T_RED);
  slantEye(EYE_RX + j, EYE_Y, false, true, T_RED);
  roundArc(MID_X + j, 182, 16, 5, 215, 325, T_RED);
}

void xSurprised(uint32_t t) {
  int r = 26 + (int)(sinf(t * 0.01f) * 2);
  halos();
  ringEye(EYE_LX, EYE_Y, r, r - 11, C_CYAN);
  ringEye(EYE_RX, EYE_Y, r, r - 11, C_CYAN);
  ringEye(MID_X, 168, 9, 5, C_CYAN);
}

void questionMark(int x, int y, uint16_t col) {
  roundArc(x, y, 10, 4, 190, 420, col);
  float ex = x + cosf(60 * DEG_TO_RAD) * 8, ey = y + sinf(60 * DEG_TO_RAD) * 8;
  thickLine(ex, ey, x, y + 15, 4, col);
  gfx->fillCircle(x, y + 24, 3, col);
}

void xConfused(uint32_t t) {
  float w = sinf(t * 0.003f) * 3;
  halos();
  happyEye(EYE_LX + w, EYE_Y);
  happyEye(EYE_RX + w, EYE_Y + 6);
  questionMark(EYE_RX + 22, 50 + (int)(sinf(t * 0.006f) * 3), C_CYAN);
  wavyMouth(166, t, 2);
}

void xWink(uint32_t t) {
  halos();
  pill(EYE_LX, EYE_Y, 28, blinking(t, 3200, 160) ? 8 : 52, C_CYAN);
  happyEye(EYE_RX, EYE_Y);
  sparkle(EYE_RX + 34, EYE_Y - 30, 4 + (int)(fabsf(sinf(t * 0.006f)) * 6), C_GOLD);
  roundArc(MID_X + 6, 146, 12, 4, 30, 150, C_CYAN);
}

void xExcited(uint32_t t) {
  float b = -fabsf(sinf(t * 0.012f)) * 6;
  float s = 24 + sinf(t * 0.015f) * 3;
  halos(0, b);
  star5(EYE_LX, EYE_Y + b, s, T_YELLOW);
  star5(EYE_RX, EYE_Y + b, s, T_YELLOW);
}

float beatScale(uint32_t t) {     // "lub-dub" heartbeat, ~1 beat / second
  float ph = (t % 1000) / 1000.0f;
  if (ph < 0.10f) return 1.0f + ph * 2.0f;
  if (ph < 0.20f) return 1.2f - (ph - 0.10f) * 2.0f;
  if (ph < 0.30f) return 1.0f + (ph - 0.20f) * 1.2f;
  if (ph < 0.40f) return 1.12f - (ph - 0.30f) * 1.2f;
  return 1.0f;
}

void xLove(uint32_t t) {
  float s = 24 * beatScale(t);
  gfx->fillCircle(EYE_LX, EYE_Y, 40, T_PINK_DK);
  gfx->fillCircle(EYE_RX, EYE_Y, 40, T_PINK_DK);
  heartShape(EYE_LX, EYE_Y, s, C_PINK);
  heartShape(EYE_RX, EYE_Y, s, C_PINK);
  blush(EYE_LX - 6, EYE_Y + 38);
  blush(EYE_RX + 6, EYE_Y + 38);
}

void xSleepy(uint32_t t) {
  float b = sinf(t * 0.002f) * 2;
  halos(0, b);
  lowerLid(EYE_LX, EYE_Y + 4 + b, C_CYAN);
  lowerLid(EYE_RX, EYE_Y + 4 + b, C_CYAN);
  for (int k = 0; k < 2; k++) {
    float p = ((t + k * 1100) % 2200) / 2200.0f;
    gfx->setTextSize(k ? 2 : 3);
    gfx->setTextColor(scaled(70, 225, 255, 1.0f - p));
    gfx->setCursor(168 + p * 22 + k * 14, 66 - p * 34 - k * 4);
    gfx->print("Z");
  }
}

// =====================================================================
//                         ANIMATED STYLES
// =====================================================================

void xBreathing(uint32_t t) {
  float k = 0.35f + 0.65f * (sinf(t * 0.003f) + 1) * 0.5f;
  uint16_t col = scaled(70, 225, 255, k);
  lowerLid(EYE_LX, EYE_Y + 4, col);
  lowerLid(EYE_RX, EYE_Y + 4, col);
}

void xBlink(uint32_t t) {
  float p = (t % 1400) / 1400.0f;
  float open = p < 0.12f ? 1 - p / 0.12f : p < 0.24f ? (p - 0.12f) / 0.12f : 1;
  halos();
  if (open < 0.15f) {
    closedEye(EYE_LX, EYE_Y);
    closedEye(EYE_RX, EYE_Y);
  } else {
    pill(EYE_LX, EYE_Y, 30, 52 * open, C_CYAN);
    pill(EYE_RX, EYE_Y, 30, 52 * open, C_CYAN);
  }
}

void xScanning(uint32_t t) {
  const int cx = MID_X, cy = EYE_Y;
  float a = fmodf(t * 0.18f, 360);
  gfx->fillArc(cx, cy, 58, 54, 0, 360, T_DIMCYAN);
  gfx->fillArc(cx, cy, 58, 50, a, a + 70, C_CYAN);
  gfx->fillArc(cx, cy, 58, 50, a + 180, a + 230, C_CYAN);
  float b = fmodf(360 - t * 0.25f, 360);
  gfx->fillArc(cx, cy, 38, 34, b, b + 90, C_CYAN);
  gfx->fillArc(cx, cy, 38, 34, b + 180, b + 220, T_DIMCYAN);
  gfx->fillCircle(cx, cy, 6 + (int)(fabsf(sinf(t * 0.006f)) * 4), C_CYAN);
}

void xLookingAround(uint32_t t) {
  float px = sinf(t * 0.0015f) * 10, py = sinf(t * 0.0011f) * 5;
  halos();
  for (int c : { EYE_LX, EYE_RX }) {
    gfx->fillCircle(c, EYE_Y, 26, C_CYAN);
    gfx->fillCircle(c + px, EYE_Y + py, 12, C_BLACK);
    gfx->fillCircle(c + px - 4, EYE_Y + py - 4, 3, C_WHITE);
  }
}

void xTracking(uint32_t t) {
  int head = (t / 90) % 8;
  for (int c : { EYE_LX, EYE_RX }) {
    for (int i = 0; i < 8; i++) {
      float a = i * 45 * DEG_TO_RAD;
      int d = (head - i + 8) % 8;                       // 0 = head of the trail
      int r = d == 0 ? 6 : d == 1 ? 5 : d == 2 ? 4 : 3;
      uint16_t col = d < 3 ? C_CYAN : T_DIMCYAN;
      gfx->fillCircle(c + cosf(a) * 22, EYE_Y + sinf(a) * 22, r, col);
    }
  }
}

void xThinkingAnim(uint32_t t) {
  float base = t * 0.006f;
  halos();
  for (int c : { EYE_LX, EYE_RX }) {
    for (int k = 0; k < 4; k++) {
      float a = base - k * 0.55f;
      gfx->fillCircle(c + cosf(a) * 18, EYE_Y + sinf(a) * 18, 6 - k, k == 0 ? C_CYAN : T_DIMCYAN);
    }
    gfx->fillCircle(c, EYE_Y, 3, C_CYAN);
  }
}

void xHappyWiggle(uint32_t t) {
  float w = sinf(t * 0.012f) * 5;
  halos();
  happyEye(EYE_LX, EYE_Y + w);
  happyEye(EYE_RX, EYE_Y - w);
  blush(EYE_LX - 6, EYE_Y + 30 + w);
  blush(EYE_RX + 6, EYE_Y + 30 - w);
  int n = (t / 220) % 4;
  for (int k = 0; k < n && k < 3; k++) roundArc(196, EYE_Y - 10, 8 + k * 8, 3, 300, 420, C_CYAN);
}

void ecg(int x0, int dir, uint16_t col) {   // one ECG trace, drawn from x0 outward
  const int pts[][2] = { {0, 0}, {14, 0}, {20, -12}, {26, 14}, {32, -22}, {38, 8}, {44, 0}, {66, 0} };
  for (int i = 0; i < 7; i++)
    thickLine(x0 + dir * pts[i][0], 112 + pts[i][1], x0 + dir * pts[i + 1][0], 112 + pts[i + 1][1], 3, col);
}

void xHeartbeat(uint32_t t) {
  float bs = beatScale(t);
  uint16_t line = bs > 1.05f ? C_PINK : T_PINK_DK;
  ecg(88, -1, line);
  ecg(152, 1, line);
  gfx->fillCircle(MID_X, EYE_Y, 34, T_PINK_DK);
  heartShape(MID_X, EYE_Y, 26 * bs, C_PINK);
}

void xSparkle(uint32_t t) {
  halos();
  for (int c : { EYE_LX, EYE_RX }) {
    gfx->fillCircle(c, EYE_Y, 27, T_YELLOW);
    gfx->fillCircle(c + 4, EYE_Y + 3, 14, C_BLACK);
    gfx->fillCircle(c - 5, EYE_Y - 8, 6, C_WHITE);
    gfx->fillCircle(c + 9, EYE_Y + 10, 2, C_WHITE);
  }
  sparkle(EYE_RX + 30, EYE_Y - 34, 5 + (int)(fabsf(sinf(t * 0.007f)) * 8), C_GOLD);
  sparkle(EYE_LX - 32, EYE_Y + 30, 3 + (int)(fabsf(cosf(t * 0.006f)) * 5), C_WHITE);
}

void xRainbow(uint32_t t) {
  int h = (t / 6) % 360;
  int hs[2] = { h, h + 180 };
  int cs[2] = { EYE_LX, EYE_RX };
  for (int i = 0; i < 2; i++) {
    gfx->fillCircle(cs[i], EYE_Y, 28, hsv565(hs[i], 255, 255));
    gfx->fillCircle(cs[i], EYE_Y, 22, hsv565(hs[i] + 90, 255, 255));
    gfx->fillCircle(cs[i], EYE_Y, 15, C_WHITE);
    gfx->fillCircle(cs[i], EYE_Y, 7, T_VIOLET);
  }
}

void flame(int c, int y, bool left) {
  int j1 = random(-4, 5), j2 = random(-6, 3);
  gfx->fillCircle(c, y + 10, 21, T_RED);
  gfx->fillTriangle(c - 21, y + 8, c + 21, y + 8, c + j1, y - 34 + j2, T_RED);
  gfx->fillTriangle(c - 21, y + 12, c - 6, y + 4, c - 18 + j1, y - 18, T_RED);
  gfx->fillCircle(c, y + 12, 14, T_ORANGE);
  gfx->fillTriangle(c - 14, y + 10, c + 14, y + 10, c + j1 / 2, y - 18 + j2, T_ORANGE);
  gfx->fillCircle(c, y + 15, 7, T_YELLOW);
  gfx->fillTriangle(c - 7, y + 14, c + 7, y + 14, c, y - 2 + j2 / 2, T_YELLOW);
  int in = left ? 1 : -1;      // angry cut on the inner top corner
  gfx->fillTriangle(c + in * 6, y - 40, c + in * 30, y - 40, c + in * 30, y + 4, C_BLACK);
}

void xFire(uint32_t t) {
  (void)t;
  gfx->fillCircle(EYE_LX, EYE_Y, 40, T_RED_DK);
  gfx->fillCircle(EYE_RX, EYE_Y, 40, T_RED_DK);
  flame(EYE_LX, EYE_Y, true);
  flame(EYE_RX, EYE_Y, false);
}

// =====================================================================
//                         SPECIAL THEMES
// =====================================================================

void xRobot(uint32_t t) {
  float dx = sinf(t * 0.002f) * 4;
  halos();
  for (int c : { EYE_LX, EYE_RX }) {
    ringEye(c, EYE_Y, 28, 24, C_CYAN);
    ringEye(c, EYE_Y, 17, 13, C_CYAN);
    gfx->fillCircle(c + dx, EYE_Y, 5 + (int)(fabsf(sinf(t * 0.005f)) * 2), C_CYAN);
  }
  for (int i = 0; i < 5; i++) gfx->fillRoundRect(96 + i * 10, 166, 6, 10, 2, T_DIMCYAN);
}

void xPixel(uint32_t t) {
  const char *rows[3] = { ".X.", "XXX", ".X." };
  bool closed = blinking(t, 2800, 180);
  for (int c : { EYE_LX, EYE_RX }) {
    for (int r = 0; r < 3; r++)
      for (int k = 0; k < 3; k++) {
        if (rows[r][k] != 'X' || (closed && r != 1)) continue;
        gfx->fillRect(c - 21 + k * 14, EYE_Y - 21 + r * 14, 12, 12, C_CYAN);
      }
  }
  if ((t / 300) % 3 == 0) gfx->fillRect(EYE_RX + 14, EYE_Y - 34, 6, 6, C_CYAN);   // pixel twinkle
}

void xAnime(uint32_t t) {
  bool closed = blinking(t, 3200, 150);
  float sh = sinf(t * 0.004f) * 2;
  for (int c : { EYE_LX, EYE_RX }) {
    if (closed) {
      roundArc(c, EYE_Y - 10, 24, 6, 30, 150, T_PURPLE);
      continue;
    }
    gfx->fillEllipse(c, EYE_Y, 25, 31, T_PURPLE);
    gfx->fillEllipse(c, EYE_Y + 3, 19, 25, T_VIOLET);
    gfx->fillEllipse(c, EYE_Y + 6, 11, 15, C_BLACK);
    gfx->fillEllipse(c - 8 + sh, EYE_Y - 11, 7, 9, C_WHITE);
    gfx->fillCircle(c + 8, EYE_Y + 14, 3, C_WHITE);
  }
}

void xCyber(uint32_t t) {
  float a = fmodf(t * 0.2f, 360), b = fmodf(360 - t * 0.3f, 360);
  for (int c : { EYE_LX, EYE_RX }) {
    gfx->fillArc(c, EYE_Y, 28, 24, a, a + 110, C_CYAN);
    gfx->fillArc(c, EYE_Y, 28, 24, a + 180, a + 290, C_CYAN);
    gfx->fillArc(c, EYE_Y, 18, 15, b, b + 200, C_CYAN);
    gfx->fillCircle(c, EYE_Y, 5, C_CYAN);
  }
  roundArc(EYE_RX - 8, EYE_Y + 6, 40, 4, 20, 80, T_DIMCYAN);
}

void xMinimal(uint32_t t) {
  float dx = sinf(t * 0.0012f) * 6;
  int h = blinking(t, 3000, 150) ? 4 : 12;
  gfx->fillRoundRect(EYE_LX - 24 + dx, EYE_Y - h / 2, 48, h, h / 2, C_CYAN);
  gfx->fillRoundRect(EYE_RX - 24 + dx, EYE_Y - h / 2, 48, h, h / 2, C_CYAN);
}

void xMatrix(uint32_t t) {
  const int rx = 22, ry = 30;
  int ph = t / 70;
  for (int c : { EYE_LX, EYE_RX }) {
    for (int dx = -rx; dx <= rx; dx += 5) {
      int col = (dx + rx) / 5;
      for (int dy = -ry; dy <= ry; dy += 5) {
        float e = (float)(dx * dx) / (rx * rx) + (float)(dy * dy) / (ry * ry);
        if (e > 1.0f) continue;
        int row = (dy + ry) / 5;
        int m = (row - ph - col * 3) % 7;
        if (m < 0) m += 7;
        gfx->fillRect(c + dx - 1, EYE_Y + dy - 1, 3, 3, m == 0 ? C_WHITE : m < 3 ? T_MATRIX : T_MAT_DK);
      }
    }
  }
}

void xGalaxy(uint32_t t) {
  for (int e = 0; e < 2; e++) {
    int c = e ? EYE_RX : EYE_LX;
    gfx->fillEllipse(c, EYE_Y, 28, 32, T_VIOLET);
    gfx->fillEllipse(c, EYE_Y, 24, 28, T_NAVY);
    for (int i = 0; i < 14; i++) {                     // stable "random" stars that twinkle
      uint32_t hsh = (i + 1) * 2654435761u + e * 97u;
      int sx = (int)(hsh % 36) - 18, sy = (int)((hsh >> 8) % 44) - 22;
      if ((sx * sx) / 324.0f + (sy * sy) / 484.0f > 1) continue;
      float tw = (sinf(t * 0.004f + i) + 1) * 0.5f;
      gfx->fillCircle(c + sx, EYE_Y + sy, tw > 0.7f ? 2 : 1, i % 3 ? C_WHITE : T_PURPLE);
    }
    sparkle(c - 2, EYE_Y - 4, 7 + (int)(fabsf(sinf(t * 0.005f + e)) * 5), C_WHITE);
  }
}

void xHeartSparkle(uint32_t t) {
  float s = 23 + sinf(t * 0.008f) * 2;
  heartShape(EYE_LX, EYE_Y, s, C_PINK);
  heartShape(EYE_RX, EYE_Y, s, C_PINK);
  sparkle(EYE_LX - 30, EYE_Y - 26, 3 + (int)(fabsf(sinf(t * 0.007f)) * 6), C_PINK);
  sparkle(EYE_RX + 30, EYE_Y - 30, 3 + (int)(fabsf(cosf(t * 0.006f)) * 6), C_PINK);
  sparkle(EYE_RX + 26, EYE_Y + 28, 2 + (int)(fabsf(sinf(t * 0.009f + 1)) * 4), C_WHITE);
  for (int c : { EYE_LX, EYE_RX })
    for (int i = -1; i <= 1; i++) thickLine(c + i * 8 - 3, EYE_Y + 42, c + i * 8 + 3, EYE_Y + 32, 3, C_PINK);
}

void xXoxo(uint32_t t) {
  float k = (sinf(t * 0.006f) + 1) * 0.5f;
  int w = 9 + (int)(k * 3), s = 20;
  halos();
  thickLine(EYE_LX - s, EYE_Y - s, EYE_LX + s, EYE_Y + s, w, C_WHITE);
  thickLine(EYE_LX - s, EYE_Y + s, EYE_LX + s, EYE_Y - s, w, C_WHITE);
  int r = 22 + (int)((1 - k) * 4);
  ringEye(EYE_RX, EYE_Y, r, r - 9, C_WHITE);
}

void xCool(uint32_t t) {
  int y = EYE_Y;
  gfx->fillRect(EYE_LX - 40, y - 24, (EYE_RX + 40) - (EYE_LX - 40), 7, C_CYAN);
  for (int c : { EYE_LX, EYE_RX }) {
    gfx->fillRoundRect(c - 36, y - 20, 72, 44, 20, C_CYAN);
    gfx->fillRect(c - 36, y - 20, 72, 18, C_CYAN);
    float p = ((t % 2400) / 2400.0f);              // light sweep across the lenses
    if (p < 0.6f) {
      int sx = c - 30 + (int)(p / 0.6f * 60);
      thickLine(sx - 6, y + 14, sx + 6, y - 12, 5, C_WHITE);
    }
  }
}

void xGlitch(uint32_t t) {
  int off = (t % 1000 < 120) ? 6 : 3;
  for (int c : { EYE_LX, EYE_RX }) {
    ringEye(c - off, EYE_Y, 26, 16, C_PINK);
    ringEye(c + off, EYE_Y, 26, 16, C_CYAN);
    ringEye(c, EYE_Y, 24, 15, C_WHITE);
    gfx->fillCircle(c, EYE_Y, 8, C_WHITE);
  }
  if (random(10) < 4) {                            // random slices shifted sideways
    for (int i = 0; i < 3; i++) {
      int y = random(80, 145), h = random(2, 6), dx = random(-12, 13);
      gfx->fillRect(40, y, 160, h, C_BLACK);
      gfx->fillRect(40 + dx, y, 60, h / 2 + 1, random(2) ? C_PINK : C_CYAN);
    }
  }
}

// =====================================================================
//                       INTERACTION & STATUS
// =====================================================================

void xIdle(uint32_t t) {
  float dx = sinf(t * 0.0008f) * 5;
  halos(dx, 0);
  if (blinking(t, 4000, 160)) {
    closedEye(EYE_LX + dx, EYE_Y);
    closedEye(EYE_RX + dx, EYE_Y);
  } else {
    happyEye(EYE_LX + dx, EYE_Y);
    happyEye(EYE_RX + dx, EYE_Y);
  }
}

void bang(int x, int y, uint16_t col) {
  thickLine(x, y, x, y + 20, 5, col);
  gfx->fillCircle(x, y + 30, 3, col);
}

void xAttention(uint32_t t) {
  float p = (t % 1200) / 1200.0f;
  int h = 54 + (int)(p < 0.15f ? p / 0.15f * 6 : 0);
  halos();
  pill(EYE_LX, EYE_Y, 30, h, C_CYAN);
  pill(EYE_RX, EYE_Y, 30, h, C_CYAN);
  if ((t / 400) % 2 == 0) {
    bang(32, 72, T_RED);
    bang(208, 72, T_RED);
  }
}

void xListening(uint32_t t) {
  halos();
  happyEye(EYE_LX, EYE_Y);
  happyEye(EYE_RX, EYE_Y);
  int n = (t / 250) % 4;
  for (int k = 0; k < n && k < 3; k++) {
    roundArc(40, EYE_Y, 8 + k * 7, 3, 125, 235, C_CYAN);    // "("
    roundArc(200, EYE_Y, 8 + k * 7, 3, 305, 415, C_CYAN);   // ")"
  }
}

void xSpeaking(uint32_t t) {
  halos();
  happyEye(EYE_LX, EYE_Y);
  happyEye(EYE_RX, EYE_Y);
  for (int i = 0; i < 5; i++) {
    int h = 6 + (int)(fabsf(sinf(t * 0.015f + i * 1.3f)) * 18);
    gfx->fillRoundRect(98 + i * 10, 168 - h / 2, 6, h, 3, C_CYAN);
  }
}

void xProcessing(uint32_t t) {
  int head = (t / 220) % 4;
  for (int i = 0; i < 4; i++) {
    int x = 66 + i * 36;
    bool on = i == head;
    gfx->fillCircle(x, EYE_Y, on ? 12 : 8, on ? C_CYAN : T_DIMCYAN);
  }
}

void xConnecting(uint32_t t) {
  int shown = (t / 300) % 5;
  wifiArcs(MID_X, 136, shown, C_CYAN);
}

void xCharging(uint32_t t) {
  halos();
  happyEyeC(EYE_LX, EYE_Y - 6, T_GREEN);
  happyEyeC(EYE_RX, EYE_Y - 6, T_GREEN);
  battery(93, 150, (t / 400) % 5, T_GREEN, T_GREEN);
}

void xBatteryFull(uint32_t t) {
  float b = sinf(t * 0.004f) * 2;
  halos(0, b);
  happyEyeC(EYE_LX, EYE_Y - 6 + b, T_GREEN);
  happyEyeC(EYE_RX, EYE_Y - 6 + b, T_GREEN);
  battery(93, 150, 4, T_GREEN, T_GREEN);
  sparkle(160, 146, 3 + (int)(fabsf(sinf(t * 0.008f)) * 5), C_WHITE);
}

void xLowBattery(uint32_t t) {
  lowerLid(EYE_LX, EYE_Y - 2, T_RED);
  lowerLid(EYE_RX, EYE_Y - 2, T_RED);
  battery(93, 150, (t / 500) % 2 ? 1 : 0, T_RED, T_RED);
}

void xError(uint32_t t) {
  float j = (t % 1500 < 300) ? random(-3, 4) : 0;
  const int s = 18;
  for (int c : { EYE_LX, EYE_RX }) {
    gfx->fillCircle(c + j, EYE_Y, 40, T_RED_DK);
    thickLine(c - s + j, EYE_Y - s, c + s + j, EYE_Y + s, 9, T_RED);
    thickLine(c - s + j, EYE_Y + s, c + s + j, EYE_Y - s, 9, T_RED);
  }
}

void xGreeting(uint32_t t) {
  float b = -fabsf(sinf(t * 0.008f)) * 4;
  halos(0, b);
  happyEye(EYE_LX, EYE_Y + b);
  happyEye(EYE_RX, EYE_Y + b);
  int L = 6 + (int)(fabsf(sinf(t * 0.01f)) * 6);
  for (int i = -1; i <= 1; i++) {                 // little burst lines on both sides
    float a = (180 + i * 30) * DEG_TO_RAD;
    thickLine(36 + cosf(a) * 6, EYE_Y - 8 + sinf(a) * 6, 36 + cosf(a) * (6 + L), EYE_Y - 8 + sinf(a) * (6 + L), 3, C_PINK);
    float a2 = (i * 30) * DEG_TO_RAD;
    thickLine(204 + cosf(a2) * 6, EYE_Y - 8 + sinf(a2) * 6, 204 + cosf(a2) * (6 + L), EYE_Y - 8 + sinf(a2) * (6 + L), 3, C_PINK);
  }
}

void xGoodbye(uint32_t t) {
  halos();
  happyEye(EYE_LX, EYE_Y);
  happyEye(EYE_RX, EYE_Y);
  float w = sinf(t * 0.012f) * 25;                // waving arc
  roundArc(192, 150, 16, 4, 300 + w, 380 + w, C_CYAN);
  roundArc(192, 150, 26, 3, 305 + w, 365 + w, T_DIMCYAN);
}

// =====================================================================
//                              ENGINE
// =====================================================================

struct ThemedExpr {
  const char *name;
  uint8_t     theme;
  void (*draw)(uint32_t t);
};

const ThemedExpr NEW_EXPRESSIONS[] = {
  { "Neutral", TH_BASIC, xNeutral },       { "Happy", TH_BASIC, xHappy },
  { "Big Smile", TH_BASIC, xBigSmile },    { "Sad", TH_BASIC, xSad },
  { "Angry", TH_BASIC, xAngry },           { "Surprised", TH_BASIC, xSurprised },
  { "Confused", TH_BASIC, xConfused },     { "Wink", TH_BASIC, xWink },
  { "Excited", TH_BASIC, xExcited },       { "Love", TH_BASIC, xLove },
  { "Sleepy", TH_BASIC, xSleepy },

  { "Breathing", TH_ANIMATED, xBreathing },         { "Blink", TH_ANIMATED, xBlink },
  { "Scanning", TH_ANIMATED, xScanning },           { "Looking Around", TH_ANIMATED, xLookingAround },
  { "Tracking", TH_ANIMATED, xTracking },           { "Thinking (Anim)", TH_ANIMATED, xThinkingAnim },
  { "Happy Wiggle", TH_ANIMATED, xHappyWiggle },    { "Heartbeat", TH_ANIMATED, xHeartbeat },
  { "Sparkle", TH_ANIMATED, xSparkle },             { "Rainbow", TH_ANIMATED, xRainbow },
  { "Fire", TH_ANIMATED, xFire },

  { "Robot", TH_SPECIAL, xRobot },          { "Pixel", TH_SPECIAL, xPixel },
  { "Anime", TH_SPECIAL, xAnime },          { "Cyber", TH_SPECIAL, xCyber },
  { "Minimal", TH_SPECIAL, xMinimal },      { "Matrix", TH_SPECIAL, xMatrix },
  { "Galaxy", TH_SPECIAL, xGalaxy },        { "Heart Sparkle", TH_SPECIAL, xHeartSparkle },
  { "XOXO", TH_SPECIAL, xXoxo },            { "Cool", TH_SPECIAL, xCool },
  { "Glitch", TH_SPECIAL, xGlitch },

  { "Idle", TH_STATUS, xIdle },             { "Attention", TH_STATUS, xAttention },
  { "Listening", TH_STATUS, xListening },   { "Speaking", TH_STATUS, xSpeaking },
  { "Processing", TH_STATUS, xProcessing }, { "Connecting", TH_STATUS, xConnecting },
  { "Charging", TH_STATUS, xCharging },     { "Low Battery", TH_STATUS, xLowBattery },
  { "Battery Full", TH_STATUS, xBatteryFull },
  { "Error", TH_STATUS, xError },           { "Greeting", TH_STATUS, xGreeting },
  { "Goodbye", TH_STATUS, xGoodbye },
};

#define MAX_EXPRESSIONS 64

class EyeEngine {
public:
  int      current = 0;
  uint32_t start   = 0;
  uint8_t  mask    = THEME_DEFAULT_MASK;

  // Playful & Cute first (taken straight from Eyes.h), then the new themes
  void begin() {
    _n = 0;
    for (int i = 0; i < NUM_EXPR; i++) add(expressions[i].name, TH_PLAYFUL, expressions[i].draw);
    for (const auto &e : NEW_EXPRESSIONS) add(e.name, e.theme, e.draw);
  }

  int count() const { return _n; }
  const char *name(int i) const { return _items[i].name; }
  uint8_t theme(int i) const { return _items[i].theme; }
  bool enabled(int i) const { return mask & (1 << _items[i].theme); }

  int find(const char *n) const {
    for (int i = 0; i < _n; i++)
      if (strcmp(_items[i].name, n) == 0) return i;
    return -1;
  }

  // Same behaviour as setExpression() in Eyes.h
  void set(int i) {
    current = (i % _n + _n) % _n;
    transition();
    start = millis();
    Serial.printf("Expression %d: %s\n", current + 1, _items[current].name);
  }

  int nextIndex(int from) const {
    for (int k = 1; k <= _n; k++) {
      int j = (from + k) % _n;
      if (enabled(j)) return j;
    }
    return (from + 1) % _n;
  }

  int firstEnabled() const { return enabled(0) ? 0 : nextIndex(0); }

  void next() { set(nextIndex(current)); }

  int randomOther() const {
    int pool[MAX_EXPRESSIONS], m = 0;
    for (int i = 0; i < _n; i++)
      if (enabled(i) && i != current) pool[m++] = i;
    return m ? pool[random(m)] : nextIndex(current);
  }

  // Exactly one selectable theme at a time
  void setTheme(int t) {
    if (!THEME_SELECTABLE(t)) t = THEME_DEFAULT;
    mask = 1 << t;
  }
  int selectedTheme() const {
    for (int t = 0; t < TH_COUNT; t++)
      if (mask & (1 << t)) return t;
    return THEME_DEFAULT;
  }

  // Draw a named expression (status faces) without changing the current one
  bool drawNamed(const char *n, uint32_t t) {
    int i = find(n);
    if (i < 0) return false;
    gfx->fillScreen(C_BLACK);
    _items[i].draw(t);
    return true;
  }

  // A friendly expression from the selected theme (after pairing / Wi-Fi connect)
  int pleasant() const {
    static const char *const happy[TH_COUNT][6] = {
      { "Happy", "Big Smile", "Excited", "Love", "Wink", "Happy" },
      { "Tongue Out", "Laughing", "Giggle", "Blushing", "Cheeky", "Cool Shades" },
      { "Happy Wiggle", "Sparkle", "Rainbow", "Heartbeat", "Breathing", "Happy Wiggle" },
      { "Heart Sparkle", "Cool", "Anime", "Galaxy", "XOXO", "Robot" },
      { "Greeting", "Idle", "Greeting", "Idle", "Greeting", "Idle" },
    };
    int i = find(happy[selectedTheme()][random(6)]);
    return i >= 0 ? i : firstEnabled();
  }

  // If the current expression's theme was just switched off, move on
  void ensureEnabled() {
    if (!enabled(current)) set(nextIndex(current));
  }

  // Same as eyesTick() in Eyes.h, but within the enabled themes
  void tick() {
    uint32_t t = millis() - start;
    if (autoCycle && t > EXPR_TIME_MS) {
      next();
      t = 0;
    }
    gfx->fillScreen(C_BLACK);
    _items[current].draw(t);
  }

  // Same as eyesHandleInput() in Eyes.h: BOOT = next, Serial number = pick, 0 = auto
  void handleInput() {
    static bool lastBtn = HIGH;
    bool btn = digitalRead(BOOT_BTN);
    if (lastBtn == HIGH && btn == LOW) next();
    lastBtn = btn;

    if (Serial.available()) {
      int n = Serial.parseInt();
      while (Serial.available()) Serial.read();
      if (n == 0) {
        autoCycle = true;
        Serial.println("Auto-cycle ON");
      } else if (n >= 1 && n <= _n) {
        autoCycle = false;
        set(n - 1);
      }
    }
  }

private:
  void add(const char *name, uint8_t theme, void (*draw)(uint32_t)) {
    if (_n < MAX_EXPRESSIONS) _items[_n++] = { name, theme, draw };
  }
  ThemedExpr _items[MAX_EXPRESSIONS];
  int _n = 0;
};

EyeEngine eyesEngine;
