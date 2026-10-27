#pragma once
#include "kucukler.h"
#include <Adafruit_GFX.h>
#define SH110X_WHITE 1
#define SH110X_BLACK 0
#define SH110X_INVERSE 2
#define SH110X_DISPLAYOFF 0xAE
#define SH110X_DISPLAYON 0xAF
class Adafruit_SH1106G : public Adafruit_GFX {       // 128x64 piksel tamponu; display() kareyi zaman damgasıyla JS'e yollar
 public:
  uint8_t px[128 * 64];
  Adafruit_SH1106G(int w, int h, TwoWire *, int) : Adafruit_GFX(w, h) { memset(px, 0, sizeof(px)); }
  bool begin(uint8_t = 0x3C, bool = true) { return true; }
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x < 0 || y < 0 || x >= 128 || y >= 64) return;
    uint8_t &p = px[y * 128 + x]; p = c == SH110X_INVERSE ? !p : c ? 1 : 0;
  }
  bool getPixel(int16_t x, int16_t y) { return x >= 0 && y >= 0 && x < 128 && y < 64 && px[y * 128 + x]; }
  void clearDisplay() { memset(px, 0, sizeof(px)); }
  void display() { js_kare(px, simVt); simVt += 25; }   // 400 kHz I2C ile 1 KB kare ~25 ms sürer (cihazdaki gibi)
  void setContrast(uint8_t v) { js_parlak(v); }
  void invertDisplay(bool i) override { js_ters(i); }
  void oled_command(uint8_t c) { if (c == SH110X_DISPLAYOFF) js_parlak(-1); }
};
