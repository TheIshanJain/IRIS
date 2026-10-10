/*
 * Eyes.h - Playful & Cute eye expressions (drawing + expression engine)
 * Requires `gfx` (Arduino_Canvas*) and BOOT_BTN to be defined before including.
 */
#pragma once

// ---------- Settings ----------
#define SHOW_LABELS   true     // show expression name at the bottom
#define EXPR_TIME_MS  4000     // time per expression in auto mode

// ---------- Colours ----------
#define MK_RGB(r, g, b) (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
const uint16_t C_BLACK      = MK_RGB(0, 0, 0);
const uint16_t C_WHITE      = MK_RGB(255, 255, 255);
const uint16_t C_CYAN       = MK_RGB(70, 225, 255);
const uint16_t C_HALO       = MK_RGB(0, 22, 32);
const uint16_t C_PINK       = MK_RGB(255, 80, 140);
const uint16_t C_BLUSH_BG   = MK_RGB(80, 15, 45);
const uint16_t C_TONGUE     = MK_RGB(255, 100, 135);
const uint16_t C_TONGUE_DK  = MK_RGB(180, 40, 80);
const uint16_t C_TEAR       = MK_RGB(110, 200, 255);
const uint16_t C_GOLD       = MK_RGB(255, 225, 120);
const uint16_t C_LABEL      = MK_RGB(90, 95, 120);

// ---------- Layout ----------
const int EYE_LX = 78, EYE_RX = 162, EYE_Y = 112;   // left/right eye centres, eye height
const int MID_X = 120;

// =====================================================================
//                              HELPERS
// =====================================================================

// Thick line with rounded ends
void thickLine(float x0, float y0, float x1, float y1, float w, uint16_t c) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = sqrtf(dx * dx + dy * dy);
  float h = w / 2.0f;
  if (len > 0.01f) {
    float nx = -dy / len * h, ny = dx / len * h;
    gfx->fillTriangle(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, c);
    gfx->fillTriangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c);
  }
  gfx->fillCircle(x0, y0, h, c);
  gfx->fillCircle(x1, y1, h, c);
}

// Thick arc with rounded caps. Angles in degrees: 0 = right, 90 = down.
void roundArc(int cx, int cy, int r, int thick, float a0, float a1, uint16_t c) {
  gfx->fillArc(cx, cy, r, r - thick, a0, a1, c);
  float rm = r - thick / 2.0f;
  gfx->fillCircle(cx + cosf(a0 * DEG_TO_RAD) * rm, cy + sinf(a0 * DEG_TO_RAD) * rm, thick / 2, c);
  gfx->fillCircle(cx + cosf(a1 * DEG_TO_RAD) * rm, cy + sinf(a1 * DEG_TO_RAD) * rm, thick / 2, c);
}

// Soft dark glow behind each eye for a neon feel
void halos(int dx = 0, int dy = 0) {
  gfx->fillCircle(EYE_LX + dx, EYE_Y + dy, 42, C_HALO);
  gfx->fillCircle(EYE_RX + dx, EYE_Y + dy, 42, C_HALO);
}

// "^" happy closed eye
void happyEye(int c, int y) {
  roundArc(c, y + 12, 26, 9, 205, 335, C_CYAN);
}

// Closed eye line (for blinks)
void closedEye(int c, int y) {
  thickLine(c - 22, y, c + 22, y, 7, C_CYAN);
}

// Big glossy eye: cyan iris, black pupil, white highlights
void bigEye(int c, int y, float px, float py) {
  gfx->fillCircle(c, y, 28, C_CYAN);
  gfx->fillCircle(c + px, y + py, 19, C_BLACK);
  gfx->fillCircle(c + px - 7, y + py - 7, 6, C_WHITE);
  gfx->fillCircle(c + px + 6, y + py + 6, 2, C_WHITE);
}

void blush(int c, int y, uint16_t col = C_PINK) {
  gfx->fillEllipse(c, y, 13, 6, col);
}

// Water / sweat drop pointing up
void drop(float x, float y, float s, uint16_t c) {
  gfx->fillCircle(x, y, s, c);
  gfx->fillTriangle(x - s, y - 1, x + s, y - 1, x, y - s * 2.3f, c);
  gfx->fillCircle(x - s * 0.35f, y - s * 0.2f, s * 0.3f, C_WHITE);
}

// 4-point sparkle star
void sparkle(int x, int y, int s, uint16_t c) {
  int w = max(1, s / 4);
  gfx->fillTriangle(x, y - s, x - w, y, x + w, y, c);
  gfx->fillTriangle(x, y + s, x - w, y, x + w, y, c);
  gfx->fillTriangle(x - s, y, x, y - w, x, y + w, c);
  gfx->fillTriangle(x + s, y, x, y - w, x, y + w, c);
}

// Tongue hanging below a point
void tongue(float x, float y) {
  gfx->fillCircle(x, y, 14, C_TONGUE);
  gfx->fillRect(x - 15, y - 15, 31, 15, C_BLACK);   // keep only the lower half
  thickLine(x, y + 1, x, y + 8, 3, C_TONGUE_DK);
}

// Periodic blink: true for `dur` ms every `period` ms
bool blinking(uint32_t t, uint32_t period = 2600, uint32_t dur = 140) {
  return (t % period) > (period - dur);
}

// Wavy nervous mouth
void wavyMouth(int y, uint32_t t, float amp = 3) {
  for (int x = 100; x <= 140; x += 2) {
    float yy = y + sinf(x * 0.35f + t * 0.012f) * amp;
    gfx->fillCircle(x, yy, 2, C_CYAN);
  }
}

// =====================================================================
//                            EXPRESSIONS
// =====================================================================

void exprTongueOut(uint32_t t) {
  halos();
  happyEye(EYE_LX, EYE_Y);
  happyEye(EYE_RX, EYE_Y);
  float tx = MID_X + sinf(t * 0.006f) * 4;
  tongue(tx, 162);
  roundArc(MID_X, 140, 22, 5, 30, 150, C_CYAN);           // smile
}

void exprLaughing(uint32_t t) {
  float dy = -fabsf(sinf(t * 0.015f)) * 7;            // bouncing
  halos(0, dy);
  // ">"
  thickLine(EYE_LX - 18, EYE_Y - 16 + dy, EYE_LX + 16, EYE_Y + dy, 9, C_CYAN);
  thickLine(EYE_LX + 16, EYE_Y + dy, EYE_LX - 18, EYE_Y + 16 + dy, 9, C_CYAN);
  // "<"
  thickLine(EYE_RX + 18, EYE_Y - 16 + dy, EYE_RX - 16, EYE_Y + dy, 9, C_CYAN);
  thickLine(EYE_RX - 16, EYE_Y + dy, EYE_RX + 18, EYE_Y + 16 + dy, 9, C_CYAN);
  blush(EYE_LX - 6, EYE_Y + 34 + dy);
  blush(EYE_RX + 6, EYE_Y + 34 + dy);
  // open laughing mouth
  gfx->fillArc(MID_X, 150 + dy, 18, 0, 0, 180, C_CYAN);
  gfx->fillArc(MID_X, 150 + dy, 13, 0, 0, 180, C_BLACK);
}

void exprGiggle(uint32_t t) {
  float dx = sinf(t * 0.03f) * 4;                      // side-to-side shake
  halos(dx, 0);
  happyEye(EYE_LX + dx, EYE_Y);
  happyEye(EYE_RX + dx, EYE_Y);
  blush(EYE_LX - 4 + dx, EYE_Y + 30);
  blush(EYE_RX + 4 + dx, EYE_Y + 30);
  roundArc(MID_X + dx, 148, 10, 4, 20, 160, C_CYAN);       // tiny smile
}

void exprShy(uint32_t t) {
  halos();
  // eyes slowly look away (down-right) then back
  float k  = (sinf(t * 0.002f) + 1) * 0.5f;
  float px = 4 + k * 4, py = 4 + k * 3;
  if (blinking(t)) {
    closedEye(EYE_LX, EYE_Y + 4);
    closedEye(EYE_RX, EYE_Y + 4);
  } else {
    bigEye(EYE_LX, EYE_Y, px, py);
    bigEye(EYE_RX, EYE_Y, px, py);
  }
  blush(EYE_LX - 4, EYE_Y + 40);
  blush(EYE_RX + 4, EYE_Y + 40);
  roundArc(MID_X + 6, 150, 8, 4, 30, 150, C_CYAN);
}

void exprThinking(uint32_t t) {
  halos();
  // eyes looking up-right
  gfx->fillCircle(EYE_LX, EYE_Y, 24, C_CYAN);
  gfx->fillCircle(EYE_RX, EYE_Y, 24, C_CYAN);
  float drift = sinf(t * 0.002f) * 2;
  gfx->fillCircle(EYE_LX + 7 + drift, EYE_Y - 8, 11, C_BLACK);
  gfx->fillCircle(EYE_RX + 7 + drift, EYE_Y - 8, 11, C_BLACK);
  // squint left eye top a bit
  gfx->fillRect(EYE_LX - 30, EYE_Y - 30, 60, 10, C_BLACK);
  // thinking mouth
  thickLine(104, 166, 132, 160, 5, C_CYAN);
  // thought bubbles appear one by one
  int n = (t / 450) % 5;
  if (n >= 1) gfx->fillCircle(150, 58, 4, C_CYAN);
  if (n >= 2) gfx->fillCircle(166, 44, 6, C_CYAN);
  if (n >= 3) gfx->fillCircle(186, 34, 8, C_CYAN);
}

void spiral(int c, int y, float rot) {
  for (float th = 0; th < 3.3f * PI; th += 0.12f) {
    float r = 3 + th * 2.4f;
    gfx->fillCircle(c + cosf(th + rot) * r, y + sinf(th + rot) * r, 3, C_CYAN);
  }
}

void exprDizzy(uint32_t t) {
  float wob = sinf(t * 0.004f) * 3;
  halos(wob, 0);
  spiral(EYE_LX + wob, EYE_Y, t * 0.008f);
  spiral(EYE_RX + wob, EYE_Y, -t * 0.008f);
  wavyMouth(168, t, 4);
}

// Filled eye with a slanted "worried" cut on top
void worriedEye(int c, int y, bool left, float jitter) {
  gfx->fillCircle(c, y, 25, C_CYAN);
  int inner = left ? 30 : -30;   // inner side is towards the centre
  // cut: higher on the inner side, lower on the outer side
  gfx->fillTriangle(c - 32, y - 32, c + 32, y - 32, c + inner, y - 20, C_BLACK);
  gfx->fillTriangle(c - inner, y - 32, c + inner, y - 20, c - inner, y - 2, C_BLACK);
  gfx->fillCircle(c + jitter, y + 6, 8, C_BLACK);
  gfx->fillCircle(c + jitter - 3, y + 3, 2, C_WHITE);
}

void exprNervous(uint32_t t) {
  float jit = (random(0, 3) - 1) * 1.5f;               // jittery pupils
  halos();
  worriedEye(EYE_LX, EYE_Y, true, jit);
  worriedEye(EYE_RX, EYE_Y, false, jit);
  // sweat drop sliding down
  float s = (t % 1600) / 1600.0f;
  drop(EYE_RX + 40, 70 + s * 30, 7, C_TEAR);
  wavyMouth(166, t, 2.5f);
}

void exprBlushing(uint32_t t) {
  halos();
  happyEye(EYE_LX, EYE_Y);
  happyEye(EYE_RX, EYE_Y);
  // pulsing soft blush + "///" marks
  int pr = 15 + sinf(t * 0.006f) * 3;
  gfx->fillEllipse(EYE_LX - 4, EYE_Y + 32, pr, pr / 2 + 2, C_BLUSH_BG);
  gfx->fillEllipse(EYE_RX + 4, EYE_Y + 32, pr, pr / 2 + 2, C_BLUSH_BG);
  for (int i = 0; i < 3; i++) {
    int ox = (i - 1) * 9;
    thickLine(EYE_LX - 4 + ox - 4, EYE_Y + 38, EYE_LX - 4 + ox + 4, EYE_Y + 26, 3, C_PINK);
    thickLine(EYE_RX + 4 + ox - 4, EYE_Y + 38, EYE_RX + 4 + ox + 4, EYE_Y + 26, 3, C_PINK);
  }
  roundArc(MID_X, 150, 8, 4, 30, 150, C_CYAN);
}

void exprCheeky(uint32_t t) {
  halos();
  // left: wink
  roundArc(EYE_LX, EYE_Y - 14, 24, 9, 30, 150, C_CYAN);
  // right: ring eye with darting pupil
  float dx = sinf(t * 0.005f) * 5;
  gfx->fillCircle(EYE_RX, EYE_Y, 26, C_CYAN);
  gfx->fillCircle(EYE_RX + dx, EYE_Y, 12, C_BLACK);
  // smirk + tongue to the side
  float tw = sinf(t * 0.008f) * 2;
  tongue(108 + tw, 162);
  roundArc(116, 140, 22, 5, 45, 140, C_CYAN);
}

void exprCoolShades(uint32_t t) {
  float bob = sinf(t * 0.004f) * 2;                    // slight head-nod
  gfx->fillRect(EYE_LX - 40, EYE_Y - 24 + bob, (EYE_RX + 40) - (EYE_LX - 40), 7, C_CYAN);  // top bar
  // lenses
  int y = EYE_Y + bob;
  gfx->fillRoundRect(EYE_LX - 36, y - 20, 72, 44, 20, C_CYAN);
  gfx->fillRect(EYE_LX - 36, y - 20, 72, 18, C_CYAN);
  gfx->fillRoundRect(EYE_RX - 36, y - 20, 72, 44, 20, C_CYAN);
  gfx->fillRect(EYE_RX - 36, y - 20, 72, 18, C_CYAN);
  thickLine(EYE_LX - 20, y + 10, EYE_LX - 8, y - 8, 4, C_WHITE);
  thickLine(EYE_RX - 20, y + 10, EYE_RX - 8, y - 8, 4, C_WHITE);
  // twinkling sparkle on the corner
  int s = 6 + (int)(fabsf(sinf(t * 0.006f)) * 8);
  sparkle(EYE_RX + 38, y - 30, s, C_GOLD);
  // smug smile
  roundArc(MID_X + 8, 148, 16, 5, 30, 120, C_CYAN);
}

void exprCrying(uint32_t t) {
  float tr = sinf(t * 0.05f) * 1.2f;                  // trembling
  halos(tr, 0);
  bigEye(EYE_LX + tr, EYE_Y, 0, -5);
  bigEye(EYE_RX + tr, EYE_Y, 0, -5);
  // falling tears (two per eye, staggered)
  for (int k = 0; k < 2; k++) {
    float p = ((t + k * 650) % 1300) / 1300.0f;
    float y = EYE_Y + 30 + p * 80;
    drop(EYE_LX - 22 + tr, y, 6, C_TEAR);
    drop(EYE_RX + 22 + tr, y, 6, C_TEAR);
  }
  // sad wobbly mouth
  roundArc(MID_X + tr, 182, 16, 5, 210, 330, C_CYAN);
}

// =====================================================================
//                              ENGINE
// =====================================================================

struct Expression {
  const char *name;
  void (*draw)(uint32_t t);
};

Expression expressions[] = {
  { "Tongue Out",  exprTongueOut  },
  { "Laughing",    exprLaughing   },
  { "Giggle",      exprGiggle     },
  { "Shy",         exprShy        },
  { "Thinking",    exprThinking   },
  { "Dizzy",       exprDizzy      },
  { "Nervous",     exprNervous    },
  { "Blushing",    exprBlushing   },
  { "Cheeky",      exprCheeky     },
  { "Cool Shades", exprCoolShades },
  { "Crying",      exprCrying     },
};
const int NUM_EXPR = sizeof(expressions) / sizeof(expressions[0]);

int      current    = 0;
uint32_t exprStart  = 0;
bool     autoCycle  = true;

void drawLabel(const char *name) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(1);
  gfx->getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  gfx->setTextColor(C_LABEL);
  gfx->setCursor(MID_X - w / 2, 208);
  gfx->print(name);
}

// Quick blink-style transition between expressions
void transition() {
  for (int i = 0; i < 3; i++) {
    gfx->fillScreen(C_BLACK);
    halos();
    closedEye(EYE_LX, EYE_Y);
    closedEye(EYE_RX, EYE_Y);
    gfx->flush();
    delay(40);
  }
}

void setExpression(int idx) {
  current = (idx % NUM_EXPR + NUM_EXPR) % NUM_EXPR;
  transition();
  exprStart = millis();
  Serial.printf("Expression %d: %s\n", current + 1, expressions[current].name);
}

void eyesHandleInput() {
  // BOOT button -> next expression
  static bool lastBtn = HIGH;
  bool btn = digitalRead(BOOT_BTN);
  if (lastBtn == HIGH && btn == LOW) {
    setExpression(current + 1);
  }
  lastBtn = btn;

  // Serial: 1..N = lock, 0 = auto
  if (Serial.available()) {
    int n = Serial.parseInt();
    while (Serial.available()) Serial.read();
    if (n == 0) {
      autoCycle = true;
      Serial.println("Auto-cycle ON");
    } else if (n >= 1 && n <= NUM_EXPR) {
      autoCycle = false;
      setExpression(n - 1);
    }
  }
}

// Advance auto-cycle and draw the current expression (no flush)
void eyesTick() {
  uint32_t t = millis() - exprStart;
  if (autoCycle && t > EXPR_TIME_MS) {
    setExpression(current + 1);
    t = 0;
  }
  gfx->fillScreen(C_BLACK);
  expressions[current].draw(t);
}
