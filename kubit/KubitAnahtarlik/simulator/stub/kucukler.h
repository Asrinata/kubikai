// Kübit'in kullandığı diğer ESP32 kütüphanelerinin taklitleri (ekran, NVS, Bluetooth, Wi-Fi, uyku). Tek dosya; diğer stub başlıkları bunu içerir.
#pragma once
#include "Arduino.h"

// ---- Wire
struct TwoWire { void begin(...) {} void setClock(uint32_t) {} };
inline TwoWire Wire;

// ---- Preferences: anahtarlar tarayıcının localStorage'ında (JS tarafı)
class Preferences {
  std::string ad;
  std::string k(const char *a) { return ad + "." + a; }
  int al(const char *a, void *b, int m) { return js_kayit_al(k(a).c_str(), (uint8_t *)b, m); }
 public:
  bool begin(const char *n, bool = false) { ad = n; return true; }
  void end() {}
  int32_t getInt(const char *a, int32_t d = 0) { int32_t v; return al(a, &v, 4) == 4 ? v : d; }
  size_t putInt(const char *a, int32_t v) { js_kayit_yaz(k(a).c_str(), (uint8_t *)&v, 4); return 4; }
  uint32_t getUInt(const char *a, uint32_t d = 0) { return (uint32_t)getInt(a, (int32_t)d); }
  size_t putUInt(const char *a, uint32_t v) { return putInt(a, (int32_t)v); }
  bool getBool(const char *a, bool d = false) { return getInt(a, d) != 0; }
  size_t putBool(const char *a, bool v) { return putInt(a, v); }
  String getString(const char *a, const String &d = String()) { char b[512]; int n = al(a, b, 511); return n < 0 ? d : String(b, (unsigned)n); }
  size_t putString(const char *a, const String &v) { js_kayit_yaz(k(a).c_str(), (const uint8_t *)v.c_str(), v.length()); return v.length(); }
  size_t getBytes(const char *a, void *b, size_t m) { int n = al(a, b, m); return n < 0 ? 0 : n; }
  size_t putBytes(const char *a, const void *b, size_t n) { js_kayit_yaz(k(a).c_str(), (const uint8_t *)b, n); return n; }
  bool isKey(const char *a) { uint8_t b; return al(a, &b, 0) >= 0; }
  bool remove(const char *a) { js_kayit_sil(k(a).c_str()); return true; }
  bool clear() { js_kayit_sil((ad + ".*").c_str()); return true; }
};

// ---- Bluetooth: telefon paneli sim_bagla / sim_gonder ile bağlanır ve satır yollar; notify() telefona gider
class NimBLEServer; class NimBLECharacteristic;
struct NimBLEConnInfo { uint16_t handle = 0; uint16_t getConnHandle() const { return handle; } };
struct NimBLEAttValue { std::string v; const char *c_str() const { return v.c_str(); } size_t length() const { return v.size(); } };
struct NimBLEServerCallbacks { virtual ~NimBLEServerCallbacks() {} virtual void onConnect(NimBLEServer *, NimBLEConnInfo &) {} virtual void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) {} };
struct NimBLECharacteristicCallbacks { virtual ~NimBLECharacteristicCallbacks() {} virtual void onWrite(NimBLECharacteristic *, NimBLEConnInfo &) {} virtual void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t) {} };
namespace NIMBLE_PROPERTY { enum { READ = 2, WRITE = 8, NOTIFY = 16 }; }
class NimBLECharacteristic {
 public:
  NimBLECharacteristicCallbacks *cb = nullptr; NimBLEAttValue deger; int ozellik = 0;
  void setCallbacks(NimBLECharacteristicCallbacks *c) { cb = c; }
  void setValue(const char *s) { deger.v = s; }
  void setValue(const String &s) { deger.v = s.s; }
  NimBLEAttValue getValue() { return deger; }
  void notify() { js_bildir(deger.v.c_str(), deger.v.size()); }
};
inline std::vector<NimBLECharacteristic *> simKarakter;
class NimBLEService {
 public:
  NimBLECharacteristic *createCharacteristic(const char *, int o) { auto *k = new NimBLECharacteristic(); k->ozellik = o; simKarakter.push_back(k); return k; }
  bool start() { return true; }
};
class NimBLEServer {
 public:
  NimBLEServerCallbacks *cb = nullptr;
  void setCallbacks(NimBLEServerCallbacks *c) { cb = c; }
  NimBLEService *createService(const char *) { return new NimBLEService(); }
};
struct NimBLEAdvertising { void setName(const char *) {} void addServiceUUID(const char *) {} void enableScanResponse(bool) {} bool start() { return true; } bool isAdvertising() { return true; } };
inline NimBLEServer *simSunucu = nullptr;
struct NimBLEDevice {
  static void init(const char *) {}
  static bool startAdvertising() { return true; }
  static void setMTU(int) {}
  static bool setPower(int8_t) { return true; }
  static void deinit(bool = false) {}
  static NimBLEServer *createServer() { return simSunucu = new NimBLEServer(); }
  static NimBLEAdvertising *getAdvertising() { static NimBLEAdvertising a; return &a; }
};

// ---- Wi-Fi ve güncelleme: simülatörde ağ yok, güncelleme "bağlanamadı" der
#define WIFI_STA 1
#define WIFI_OFF 0
#define WL_CONNECTED 3
#define WIFI_AUTH_OPEN 0
struct SimWiFi { bool baglandi = false; bool sonuc = true;
  void mode(int m) { if (m == WIFI_OFF) baglandi = false; }
  void begin(const char *, const char *) { baglandi = sonuc; } int status() { return baglandi ? WL_CONNECTED : 0; } void disconnect(bool = false) { baglandi = false; }
  int scanComplete() { return 4; }
  int scanNetworks(bool = false) { return 4; } String SSID(int i) { const char *a[] = {"deCo", "Komsu_5G", "deCo", "Kafe Misafir"}; return a[i]; }
  int RSSI(int i) { return -50 - i * 12; } int encryptionType(int i) { return i == 3 ? WIFI_AUTH_OPEN : 3; } void scanDelete() {} };
inline SimWiFi WiFi;
inline void configTime(long, int, const char *, const char * = nullptr, const char * = nullptr) {}
struct WiFiClientSecure { void setCACert(const char *) {} void useBuiltinCACertBundle() {} void setInsecure() {} int lastError(char *b, int n) { if (n) *b = 0; return 0; } };
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
struct HTTPClient { void setFollowRedirects(int) {} void setTimeout(int) {} bool begin(WiFiClientSecure &, const char *) { return false; } int GET() { return -1; } String getString() { return ""; } void end() {} };
enum t_httpUpdate_return { HTTP_UPDATE_FAILED, HTTP_UPDATE_NO_UPDATES, HTTP_UPDATE_OK };
struct SimHttpUpdate { void setFollowRedirects(int) {} void rebootOnUpdate(bool) {} void onProgress(std::function<void(int, int)>) {}
  t_httpUpdate_return update(WiFiClientSecure &, const char *) { return HTTP_UPDATE_FAILED; } String getLastErrorString() { return "simulator"; } };
inline SimHttpUpdate httpUpdate;

// ---- derin uyku
enum { ESP_SLEEP_WAKEUP_UNDEFINED = 0, ESP_SLEEP_WAKEUP_EXT0 = 2, ESP_SLEEP_WAKEUP_EXT1 = 3 };
enum { ESP_EXT1_WAKEUP_ALL_LOW = 0, ESP_EXT1_WAKEUP_ANY_HIGH = 1 };
inline void esp_sleep_enable_ext1_wakeup(uint64_t, int) {}
enum gpio_num_t { GPIO_NUM_25 = 25, GPIO_NUM_26 = 26 };
inline int simUyanma = 0;
inline int esp_sleep_get_wakeup_cause() { return simUyanma; }
inline void esp_sleep_enable_ext0_wakeup(gpio_num_t, int) {}
[[noreturn]] inline void esp_deep_sleep_start() { js_olay(1); __builtin_trap(); }
inline void rtc_gpio_pullup_en(gpio_num_t) {}
inline void rtc_gpio_pulldown_dis(gpio_num_t) {}
inline void rtc_gpio_deinit(gpio_num_t) {}
