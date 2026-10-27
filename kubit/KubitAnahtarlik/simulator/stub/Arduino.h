// Tarayıcı simülatörü için Arduino/ESP32 taklidi (WebAssembly). Zaman sanal: delay() saati ilerletir, ekran kareleri JS'e zaman damgasıyla gider.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <cmath>
#define ARDUINO 10800
#define ESP32 1
#define SIMULATOR 1                                   // firmware simülatörde FreeRTOS görevi yerine loop içinden denetler
using std::min; using std::max; using std::abs;
#define constrain(a, lo, hi) ((a) < (lo) ? (lo) : ((a) > (hi) ? (hi) : (a)))
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 3
#define INPUT_PULLUP 5
#define PROGMEM
#define F(s) (s)
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_dword(a) (*(const uint32_t *)(a))
#define RTC_NOINIT_ATTR
#define RTC_DATA_ATTR
typedef bool boolean;
class __FlashStringHelper;
#define bit(b) (1UL << (b))   // Arduino'da da var: bu adla fonksiyon/değişken çakışır
#define radians(d) ((d) * DEG_TO_RAD)
#define degrees(r) ((r) / DEG_TO_RAD)
typedef uint8_t byte;

#define SIM_IMPORT(ad) __attribute__((import_module("env"), import_name(#ad)))
extern "C" {
SIM_IMPORT(js_tus) int js_tus(int pin, uint32_t vt);
SIM_IMPORT(js_kare) void js_kare(const uint8_t *px, uint32_t vt);
SIM_IMPORT(js_parlak) void js_parlak(int v);
SIM_IMPORT(js_bildir) void js_bildir(const char *s, int n);
SIM_IMPORT(js_kayit_al) int js_kayit_al(const char *k, uint8_t *buf, int max);
SIM_IMPORT(js_kayit_yaz) void js_kayit_yaz(const char *k, const uint8_t *buf, int n);
SIM_IMPORT(js_kayit_sil) void js_kayit_sil(const char *k);
SIM_IMPORT(js_olay) void js_olay(int tur);         // 1 derin uyku, 2 yeniden başla
SIM_IMPORT(js_log) void js_log(const char *s, int n);
SIM_IMPORT(js_ters) void js_ters(int v);
}
inline uint32_t simVt = 0;                           // sanal milisaniye
inline uint32_t simAltMs = 0;
inline uint32_t millis() { if (++simAltMs >= 2000) { simAltMs = 0; simVt++; } return simVt; }   // millis'i bekleyen boş döngüler de ilerlesin
inline uint32_t micros() { return simVt * 1000; }
inline void delay(uint32_t ms) { simVt += ms; }
inline void yield() {}
inline int digitalRead(int pin) { return js_tus(pin, simVt) ? LOW : HIGH; }   // tuşlar pull-up: basılı = LOW
inline void digitalWrite(int, int) {}
inline void pinMode(int, int) {}
inline uint32_t simRastgele = 12345;
inline uint32_t esp_random() { simRastgele ^= simRastgele << 13; simRastgele ^= simRastgele >> 17; simRastgele ^= simRastgele << 5; return simRastgele; }
inline void randomSeed(uint32_t s) { simRastgele = s ? s : 1; }
inline long random(long n) { return n <= 0 ? 0 : esp_random() % n; }
inline long random(long a, long b) { return b <= a ? a : a + random(b - a); }

// saat: cihazda RTC 0'dan başlar, telefon "S" satırıyla kurar
inline time_t simSaatTaban = 0;
inline uint32_t simSaatVt = 0;
inline time_t sim_time(time_t *t) { time_t v = simSaatTaban ? simSaatTaban + (simVt - simSaatVt) / 1000 : simVt / 1000; if (t) *t = v; return v; }
inline int sim_settimeofday(const timeval *tv, const void *) { simSaatTaban = tv->tv_sec; simSaatVt = simVt; return 0; }
#define time(x) sim_time(x)
#define settimeofday(a, b) sim_settimeofday(a, b)

class String {
 public:
  std::string s;
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const char *c, unsigned n) : s(c, n) {}
  String(const std::string &x) : s(x) {}
  String(char c) : s(1, c) {}
  String(unsigned char v, int b = 10) : String((unsigned long)v, b) {}
  String(int v, int b = 10) : String((long)v, b) {}
  String(unsigned v, int b = 10) : String((unsigned long)v, b) {}
  String(long v, int b = 10) { char t[40]; snprintf(t, 40, b == 16 ? "%lx" : "%ld", v); s = t; }
  String(unsigned long v, int b = 10) { char t[40]; snprintf(t, 40, b == 16 ? "%lx" : "%lu", v); s = t; }
  String(long long v, int = 10) { s = std::to_string(v); }
  String(unsigned long long v, int = 10) { s = std::to_string(v); }
  String(double v, int d = 2) { char t[48]; snprintf(t, 48, "%.*f", (int)d, v); s = t; }
  String(float v, int d = 2) : String((double)v, d) {}
  unsigned length() const { return s.size(); }
  const char *c_str() const { return s.c_str(); }
  char operator[](unsigned i) const { return i < s.size() ? s[i] : 0; }
  char &operator[](unsigned i) { return s[i]; }
  char charAt(unsigned i) const { return (*this)[i]; }
  String substring(unsigned a) const { return a >= s.size() ? String() : String(s.substr(a)); }
  String substring(unsigned a, unsigned b) const { if (a > b) std::swap(a, b); if (a >= s.size()) return String(); return String(s.substr(a, std::min<size_t>(b, s.size()) - a)); }
  int indexOf(char c, unsigned f = 0) const { auto p = s.find(c, f); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String &x, unsigned f = 0) const { auto p = s.find(x.s, f); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c) const { auto p = s.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  void replace(const String &a, const String &b) { if (a.s.empty()) return; size_t p = 0; while ((p = s.find(a.s, p)) != std::string::npos) { s.replace(p, a.s.size(), b.s); p += b.s.size(); } }
  void replace(char a, char b) { for (auto &c : s) if (c == a) c = b; }
  void remove(unsigned i, unsigned n = (unsigned)-1) { if (i < s.size()) s.erase(i, n); }
  long toInt() const { return atol(s.c_str()); }
  float toFloat() const { return atof(s.c_str()); }
  void trim() { size_t a = s.find_first_not_of(" \t\r\n"); if (a == std::string::npos) { s.clear(); return; } s = s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1); }
  void toUpperCase() { for (auto &c : s) c = toupper(c); }
  void toLowerCase() { for (auto &c : s) c = tolower(c); }
  bool startsWith(const String &x) const { return s.rfind(x.s, 0) == 0; }
  bool endsWith(const String &x) const { return s.size() >= x.s.size() && s.compare(s.size() - x.s.size(), x.s.size(), x.s) == 0; }
  bool equals(const String &x) const { return s == x.s; }
  bool isEmpty() const { return s.empty(); }
  void reserve(unsigned n) { s.reserve(n); }
  bool concat(const String &x) { s += x.s; return true; }
  String &operator+=(const String &x) { s += x.s; return *this; }
  String &operator+=(const char *x) { s += x; return *this; }
  String &operator+=(char c) { s += c; return *this; }
  template <class T, class = typename std::enable_if<std::is_arithmetic<T>::value>::type> String &operator+=(T v) { s += String(v).s; return *this; }
  bool operator==(const String &x) const { return s == x.s; }
  bool operator==(const char *x) const { return s == x; }
  bool operator!=(const String &x) const { return s != x.s; }
  bool operator!=(const char *x) const { return s != x; }
  bool operator<(const String &x) const { return s < x.s; }
};
inline String operator+(const String &a, const String &b) { return String(a.s + b.s); }
inline String operator+(const String &a, const char *b) { return String(a.s + b); }
inline String operator+(const char *a, const String &b) { return String(a + b.s); }
inline String operator+(const String &a, char b) { return String(a.s + b); }
template <class T, class = typename std::enable_if<std::is_arithmetic<T>::value && !std::is_same<T, char>::value>::type>
inline String operator+(const String &a, T v) { return String(a.s + String(v).s); }

class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *b, size_t n) { size_t k = 0; while (n--) k += write(*b++); return k; }
  size_t print(const String &x) { return write((const uint8_t *)x.c_str(), x.length()); }
  size_t print(const char *x) { return write((const uint8_t *)x, strlen(x)); }
  size_t print(char c) { return write((uint8_t)c); }
  template <class T> size_t print(T v) { return print(String(v)); }
  template <class T> size_t println(T v) { size_t n = print(v); return n + print("\n"); }
  size_t println() { return print("\n"); }
};
class SimSeri : public Print {
 public:
  void begin(long) {}
  size_t write(uint8_t c) override { char k = c; js_log(&k, 1); return 1; }
  operator bool() { return true; }
};
inline SimSeri Serial;
struct SimEsp { [[noreturn]] void restart() { js_olay(2); __builtin_trap(); } uint32_t getFreeHeap() { return 200000; } uint32_t getMinFreeHeap() { return 180000; } };
inline SimEsp ESP;

typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
