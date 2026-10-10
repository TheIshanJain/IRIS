/*
 * Notify.h - phone notifications on the round screen.
 *
 * The app sends:
 *   {"cmd":"icon","pkg":"com.whatsapp","i":0,"n":20,"d":"<base64 part>"}   (once per app)
 *   {"cmd":"notify","pkg":"com.whatsapp","app":"WhatsApp","title":"Amma",
 *    "text":"Did you eat?","color":2016}
 * Icons are 48x48 RGB565 (little-endian), kept in PSRAM so each app logo is
 * only sent once. Text arrives already cleaned to plain ASCII by the app.
 */
#pragma once
#include <mbedtls/base64.h>

#define NOTIF_ICON      48
#define NOTIF_ICON_PX   (NOTIF_ICON * NOTIF_ICON)
#define NOTIF_ICONS_MAX 16
#define NOTIF_QUEUE_MAX 5

struct NotifIcon {
  String    pkg;
  uint16_t *px = nullptr;
  uint32_t  used = 0;
};

struct Notification {
  String   pkg, app, title, text;
  uint16_t color = 0;
};

class NotifyCenter {
public:
  // ---------------- icons ----------------
  // Collect base64 parts; when complete, decode into the cache
  void iconPart(const String &pkg, int i, int n, const String &part) {
    if (i == 0 || pkg != _pendPkg) {
      _pendPkg = pkg;
      _pend = "";
      _pendNext = 0;
    }
    if (i != _pendNext) return;                // out of order -> ignore until a new i=0
    _pend += part;
    _pendNext++;
    if (_pendNext < n) return;

    size_t outLen = 0;
    uint8_t *buf = (uint8_t *)ps_malloc(NOTIF_ICON_PX * 2);
    if (!buf) buf = (uint8_t *)malloc(NOTIF_ICON_PX * 2);
    if (!buf) return;
    int rc = mbedtls_base64_decode(buf, NOTIF_ICON_PX * 2, &outLen,
                                   (const unsigned char *)_pend.c_str(), _pend.length());
    _pend = "";
    if (rc != 0 || outLen != NOTIF_ICON_PX * 2) {
      free(buf);
      Log.printf("[Notify] Bad icon for %s\n", pkg.c_str());
      return;
    }
    NotifIcon *slot = findIcon(pkg);
    if (!slot) slot = freeSlot();
    if (slot->px) free(slot->px);
    slot->pkg  = pkg;
    slot->px   = (uint16_t *)buf;
    slot->used = millis();
  }

  bool hasIcon(const String &pkg) { return findIcon(pkg) != nullptr; }

  // ---------------- queue ----------------
  void push(const Notification &n) {
    if (_count == NOTIF_QUEUE_MAX) {           // drop the oldest waiting one
      for (int i = 1; i < NOTIF_QUEUE_MAX; i++) _q[i - 1] = _q[i];
      _count--;
    }
    _q[_count++] = n;
    if (_count == 1) _shownAt = millis();
    _total++;
    Log.printf("[Notify] %s: %s\n", n.app.c_str(), n.title.c_str());
  }

  bool active() const { return _count > 0; }
  int waiting() const { return _count > 0 ? _count - 1 : 0; }
  uint32_t total() const { return _total; }

  // Move to the next notification (shake or timeout)
  void dismiss() {
    if (!_count) return;
    for (int i = 1; i < _count; i++) _q[i - 1] = _q[i];
    _count--;
    _shownAt = millis();
  }

  void clear() { _count = 0; }

  // Call each frame; handles the on-screen time. Returns true while something is showing.
  bool update(uint32_t durationMs) {
    if (!_count) return false;
    if (millis() - _shownAt > durationMs + ATTENTION_MS) dismiss();
    return _count > 0;
  }

  // First ~0.7 s: the "Attention" status face, then the card
  bool inAttention() const { return _count && millis() - _shownAt < ATTENTION_MS; }

  // ---------------- drawing ----------------
  void draw() {
    if (!_count) return;
    const Notification &n = _q[0];
    uint32_t t = millis() - _shownAt - ATTENTION_MS;
    int slide = t < 220 ? (int)((220 - t) * 0.25f) : 0;    // small slide-up on arrival
    uint16_t accent = n.color ? n.color : C_CYAN;

    gfx->fillScreen(C_BLACK);
    gfx->fillArc(MID_X, 120, 119, 115, 0, 360, accent);    // accent ring
    gfx->fillArc(MID_X, 120, 113, 112, 0, 360, MK_RGB(30, 34, 48));

    // logo
    int iy = 30 + slide;
    NotifIcon *ic = findIcon(n.pkg);
    if (ic && ic->px) {
      ic->used = millis();
      gfx->draw16bitRGBBitmap(MID_X - NOTIF_ICON / 2, iy, ic->px, NOTIF_ICON, NOTIF_ICON);
    } else {
      gfx->fillCircle(MID_X, iy + 24, 24, accent);
      String l = n.app.length() ? n.app.substring(0, 1) : String("?");
      l.toUpperCase();
      printCentered(l, iy + 13, 3, C_BLACK);
    }

    // app name (small caps feel)
    String app = n.app;
    app.toUpperCase();
    if (app.length() > 22) app = app.substring(0, 21) + ".";
    printCentered(app, 84 + slide, 1, MK_RGB(140, 150, 175));

    // sender
    String title = n.title.length() > 16 ? n.title.substring(0, 15) + "." : n.title;
    if (title.length()) printCentered(title, 100 + slide, 2, C_WHITE);

    // message: up to 3 wrapped lines
    String lines[3];
    int nl = wrap(n.text, lines, 3, 17);
    int y = (title.length() ? 128 : 108) + slide;
    for (int i = 0; i < nl; i++) printCentered(lines[i], y + i * 20, 2, MK_RGB(200, 210, 230));

    // "+2 more" and a time-left bar
    if (waiting()) printCentered("+" + String(waiting()) + " more", 196, 1, accent);
    else {
      float left = 1.0f - min(1.0f, (float)t / (float)_durationHint);
      int w = (int)(60 * left);
      gfx->fillRoundRect(MID_X - 30, 202, 60, 3, 1, MK_RGB(30, 34, 48));
      if (w > 0) gfx->fillRoundRect(MID_X - 30, 202, w, 3, 1, accent);
    }
  }

  void setDurationHint(uint32_t ms) { _durationHint = ms ? ms : 1; }

private:
  static const uint32_t ATTENTION_MS = 700;

  // Word-wrap into at most maxLines lines of maxChars; last line gets "..." if cut
  static int wrap(const String &textIn, String *out, int maxLines, int maxChars) {
    String text = textIn;
    text.trim();
    int n = 0;
    int pos = 0;
    while (pos < (int)text.length() && n < maxLines) {
      int remain = text.length() - pos;
      if (remain <= maxChars) {
        out[n++] = text.substring(pos);
        return n;
      }
      int cut = pos + maxChars;
      int sp = text.lastIndexOf(' ', cut);
      if (sp <= pos) sp = cut;                 // one long word: hard cut
      out[n] = text.substring(pos, sp);
      out[n].trim();
      n++;
      pos = sp;
      while (pos < (int)text.length() && text[pos] == ' ') pos++;
    }
    if (pos < (int)text.length() && n == maxLines) {
      String &last = out[maxLines - 1];
      if ((int)last.length() > maxChars - 3) last = last.substring(0, maxChars - 3);
      last += "...";
    }
    return n;
  }

  NotifIcon *findIcon(const String &pkg) {
    for (auto &i : _icons)
      if (i.px && i.pkg == pkg) return &i;
    return nullptr;
  }

  NotifIcon *freeSlot() {
    NotifIcon *best = &_icons[0];
    for (auto &i : _icons) {
      if (!i.px) return &i;
      if (i.used < best->used) best = &i;     // least recently used
    }
    return best;
  }

  NotifIcon    _icons[NOTIF_ICONS_MAX];
  String       _pendPkg, _pend;
  int          _pendNext = 0;
  Notification _q[NOTIF_QUEUE_MAX];
  int          _count = 0;
  uint32_t     _shownAt = 0;
  uint32_t     _total = 0;
  uint32_t     _durationHint = 8000;
};
