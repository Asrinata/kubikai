// Kübit anahtarlık: Beetle ESP32 + 1,3" OLED (SH1106, I2C) + 2 tuş, telefona Bluetooth (BLE) ile bağlı.
// Tuşlar her yerde aynı: Tuş 1 kaydırır, Tuş 2 tıklar, Tuş 1 uzun geri. Oyunlarda Tuş 1 zıplar, Tuş 2 çıkar.
//
// Telefondan gelen (ilk harf türü belirler):  S<derece>|<model>|<hal>|<yerel saat sn>   H<kod>|<sic>|<max>|<min>|<yağmur%>|<yer>
//   O<yeni kayıt>|<işaret>|<bugün mesaj>|<aktif>|<açık sn>|<küp>   N<uygulama>|<başlık>|<metin>   M<çalıyor 0/1>|<şarkı>|<sanatçı>
//   A<cevap başı>  C<devamı>  E (cevap bitti)   !<geçici mesaj>
// Telefona giden: AC  YENILE  CMD:<uyandir|uyut|yeniden|kapat>  SOR:<soru>  BUL  BULDUR  MUZ:<oynat|sonraki|onceki|durum>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <NimBLEDevice.h>                            // Bluedroid'den ~500 KB küçük: Wi-Fi güncellemesiyle birlikte sığsın
#include <Preferences.h>
#include <sys/time.h>
#include <vector>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#if __has_include("wifi_ayar.h")                     // yalnız USB'den yüklenen yerel sürümde var; GitHub'daki .bin şifresiz derlenir
#include "wifi_ayar.h"
#endif
#include "esp_sleep.h"
#ifndef SIMULATOR
#include "esp_task_wdt.h"
#endif
#include "driver/rtc_io.h"

const int TUS1 = 25, TUS2 = 26, LED = 2;            // Beetle D2, D3, D9 (kartta Tuş 2 solda, Tuş 1 sağda)
const int SURUM = 25;                                 // yeni sürümü yayınlarken artır (yayinla.sh surum.txt'yi buradan yazar)
const char *SURUM_URL = "https://github.com/Asrinata/kubikai/releases/download/kubit/surum.txt";
const char *BIN_URL = "https://github.com/Asrinata/kubikai/releases/download/kubit/kubit.bin";
const uint32_t UZUN_BASIS_MS = 700, ESKI_VERI_MS = 120000;
// Telefon uygulamasıyla aynı olmalı (KubitServis.java)
#define SERVIS_UUID "8f1d0001-6b75-4269-7421-4b7562697400"
#define DURUM_UUID  "8f1d0002-6b75-4269-7421-4b7562697400"
#define TUS_UUID    "8f1d0003-6b75-4269-7421-4b7562697400"
#define EKRAN_UUID  "8f1d0004-6b75-4269-7421-4b7562697400"
#define WA_KOMUT_UUID "8f1d0005-6b75-4269-7421-4b7562697400"
#define WA_SONUC_UUID "8f1d0006-6b75-4269-7421-4b7562697400"
#define W SH110X_WHITE
#define B SH110X_BLACK

// tipler en üstte: Arduino fonksiyon bildirimlerini ilk fonksiyondan önce ekler
enum Yuz { NORMAL, MUTLU, UYKULU, DUSUNCELI, TERLI };
enum Sayfa { ANA, MENU, SAAT, HAVA, SOR, KLAVYE, CEVAP, OZET, BILDIRIM, MUZIK, BUL, FB_SEC, FLAPPY, GD_SEC, GD, RC_MOD, RC_PIST, RC_ARABA, RC_YARIS, SANS, ZAR, YAZITURA, CARK, KUMANDA, ONAY, AYAR,
             OYUNLAR, OYUN, ARACLAR, TELEFON_S, ZAMANLAYICI, KRONOMETRE, POMODORO, ALARM_KUR, BUYUK_SAAT, ROZETLER, PIN_GIR, MENU_DUZENLE, HAKKINDA, GELISTIRICI,
             DOLAP, CARK_GUNLUK, PC_S, SUNUM_S, KAMERA_S, YAZICI_S, GD_ED, WEB_S, YT_S, WIFI_S, RADAR_S, UZAY_S, PCMON_S, WHATSAPP_S };
enum Olay { YOK, BASTI, KISA, UZUN };
// Oyunlar klasöründeki oyunlara giden girdi. b1/b2 basılı mı; combo: iki tuş birlikte basılıp bırakıldı (comboSure ms birlikte basılı kaldı), ikiSure: şu an kaç ms birlikte basılı.
struct Girdi { Olay o1, o2; bool b1, b2, ilk, combo; uint32_t comboSure, ikiSure; float dt; };
struct OyunTanim { const char *ad, *nasil; bool (*f)(Girdi &); uint8_t cikis; const char *rekor; uint8_t birim; };   // cikis: 0 iki tuş 3 sn, 1 sol tuş 3 sn, 2 oyunun kendi menüsü; birim: 0 sayı, 1 onda bir sn, 2 ms
enum Hedef { SORU, ISIM, WEB, WIFI_SIFRE, WIFI_AD, WA_AD, WA_NO, WA_METIN };
struct Tus { int pin; bool basili; uint32_t basla, tekrar; };
struct Kontrol { bool onceki, sec, geri; };          // menü düzeni: sol = önceki, iki tuş kısa = seç, iki tuş 1 sn = geri
struct Bildirim { String app, baslik, metin; };
struct Seviye { const char *ad, *harita; float hiz; };
enum SusTur : uint8_t { CAM, PALMIYE, TABELA, BINA, LAMBA };
struct Sus { SusTur tur; float x; int8_t yon; };
struct RSeg { float viraj, y1, y2; Sus sus[2]; uint8_t n; };
struct Araba { const char *ad; float hiz, ivme, tutus; };
struct Bot { float z, x, hiz; int tur; uint8_t cizim; };
struct Proj { float x, y, w, s, cz; };

Adafruit_SH1106G ekran(128, 64, &Wire, -1);         // 1,3" ekran SH1106; SSD1306 sürücüsüyle karıncalı görünür
Preferences kayit;
NimBLECharacteristic *tusKar, *ekranKar, *waKar;
volatile bool waIstemci = false; bool waPCHazir = false; uint16_t waBaglantiNo = 0xFFFF;
volatile bool ekranIstemci = false;
uint16_t ekranBaglantiNo = 0xFFFF;
volatile bool bagli = false;
uint16_t baglantiNo = 0xFFFF;                        // sinyal gücünü (RSSI) sormak için
int ayarBtGuc = 0;                                   // Bluetooth gönderme gücü: 0 normal (+3 dBm), 1 güçlü (+9 dBm, menzil ~%30-50 fazla, pil biraz daha çok)
void btGucUygula() { NimBLEDevice::setPower(ayarBtGuc ? 9 : 3); }
int btSinyal() {                                     // bağlı telefonun sinyali (dBm, 0: bilinmiyor); -60 iyi, -80 sınırda, -90 kopmak üzere
#ifndef SIMULATOR
  int8_t r; if (bagli && ble_gap_conn_rssi(baglantiNo, &r) == 0) return r;
#endif
  return 0;
}
std::vector<String> gelenler;                       // BLE geri çağrısından ana döngüye
portMUX_TYPE kilit = portMUX_INITIALIZER_UNLOCKED;
Tus tus1 = {TUS1, false, 0, 0}, tus2 = {TUS2, false, 0, 0};

struct { int sicaklik = -1; String model, hal; uint32_t zaman = 0; } durum;
struct { int kod = -1, sic, max, min, yagmur; String yer; } hava;
struct { int yeni = 0, isaret = 0, mesaj = 0, aktif = 0, kup = 0; long acik = -1; bool var = false; } ozet;
struct { bool caliyor = false, begen = false, karistir = false; String sarki, sanatci; int poz = 0, sure = 0; uint32_t zaman = 0; } muzik;
std::vector<Bildirim> bildirimler;
String cevap, mesaj, yazi;
bool beklemede = false, buluyor = false;
uint32_t sonDokunus = 0, mesajBitis = 0, popupBitis = 0;
Sayfa sayfa = ANA;
int imlec = 0, satirKay = 0, gdSeviye = 0, zarSayisi = 2, zarlar[2] = {6, 6}, zarAnim = 0, paraAnim = 0, paraSonuc = 0, carkAnim = 0, carkSecili = -1;
Hedef klavyeHedef = SORU;
const char *onayKomut = "";

// ayarlar (Preferences): açılış 0 Kübit 1 menü, uyku sırası, parlaklık sırası
int ayarAcilis, ayarUyku, ayarParlak, dil;         // dil: 0 Türkçe, 1 English (sistem yazıları; oyunlar Türkçe kalır); açılış 0 Kübit 1 menü 2 büyük saat
int ayarUykuTipi, ayarUnutma = 1; bool ekranUyku;                 // uyku tipi: 0 derin (pil dostu), 1 yalnız ekran kapanır (bağlı kalır)
int ayarMenuStil, ayarTers, ayarBuyukYazi, ayarKoruyucu, ayarDuvar, ayarPilTas, ayarGece, ayarGizli, ayarProfil, ayarSinir, ayarPin = -1;
// sistem.h'deki (sonra eklenen) işlevler: Arduino başlıklar için ön bildirim üretmez
void rozetKontrol(const char *anahtar); void sayac(const char *anahtar, int artis); void bleKaydet(const String &s);
void menuListeKur(); bool favMi(int g); void favDegistir(int g); void oyunSuresiEkle(float dt); bool oyunSuresiBitti();
extern int menuListe[24], menuListeN;
void pcSatiri(const String &g); void yaziciSatiri(const String &g); void hataAlindi(int id); void seviyeSatiri(const String &g);
void kiyafetCiz(int x, int y, uint32_t t); bool carkHazir(); void unutmaKontrol(); void uykuOncesiHaber(); void hataKontrol(); void hataGonder(); void izGuncelle(); void kameraKapat();
void duvarCiz(uint32_t t); void geriSatir(const String &g); void pinIste(int hedef); void yedekle();
extern const char *webKlavyeBaslik; extern bool webKlavyeBitti; extern String webKlavyeMetin;   // tarayici.h
void ucakSatiri(const String &g); void rotaSatiri(const String &g); void issSatiri(const String &g); void issYolSatiri(const String &g);   // kesif.h
extern bool radarDetay; extern int ayarYakinlik;
int wifiDevam = 0;  // 1: tarayici, 2: YouTube, 3: guncelleme; ag kurulduktan sonra devam
extern String wifiSecilen; void wifiDene(const String &ad, const String &sifre); void wifiKaydet(const String &ad, const String &sifre); String wifiAdi(); void wifiEkranAc(); void wifiVazgec(); extern int wifiHal;   // kablosuz.h
void waSonuc(const String &g); void waKlavyeBitti(Hedef h, const String &v); void waGeri(); void waAc();
bool klavyeKucuk = false;                            // klavyede küçük harf (aA tuşu)
const char *dl(const char *tr, const char *en) { return dil ? en : tr; }
bool tusYut = false;                                 // oyundan basılı tutarak çıkınca, tuşlar bırakılana kadar olay üretme
bool geriAdim = false; uint16_t tekrarMs = 110;       // tekrarMs: basılı tutunca kaç ms'de bir adım (tarayıcıda daha hızlı)                               // menülerde sol tuş: önceki öğe (liste/menü/ızgara bunu okur)
int ayarOyunCikis = 0;                               // oyundan çıkış: sol N sn, bırak, sağ N sn (0: 10 sn, 1: 5 sn)
const uint32_t UYKULAR[] = {30000, 90000, 180000, 600000};
const char *UYKU_AD[] = {"30 sn", "90 sn", "3 dk", "10 dk"};
const uint8_t PARLAKLAR[] = {10, 90, 255};
const char *PARLAK_AD[] = {"dusuk", "orta", "yuksek"};

String ascii(String s) {                             // varsayılan yazı tipinde Türkçe harf yok
  const char *harf[][2] = {{"ü", "u"}, {"Ü", "U"}, {"ö", "o"}, {"Ö", "O"}, {"ç", "c"}, {"Ç", "C"}, {"ş", "s"}, {"Ş", "S"},
                           {"ğ", "g"}, {"Ğ", "G"}, {"ı", "i"}, {"İ", "I"}, {"’", "'"}, {"“", "\""}, {"”", "\""}, {"…", "..."}};
  for (auto &h : harf) s.replace(h[0], h[1]);
  return s;
}
std::vector<String> parcala(const String &v, char ayrac) {
  std::vector<String> p; int bas = 0;
  for (int i; (i = v.indexOf(ayrac, bas)) >= 0; bas = i + 1) p.push_back(v.substring(bas, i));
  p.push_back(v.substring(bas));
  return p;
}
String ikiHane(int n) { return (n < 10 ? "0" : "") + String(n); }

// ---------------------------------------------------------------- Bluetooth
class Baglanti : public NimBLEServerCallbacks {     // bağlantı kopunca yayın kendiliğinden yeniden başlar
  void onConnect(NimBLEServer *, NimBLEConnInfo &) override { NimBLEDevice::startAdvertising(); }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &i, int) override {
    if (i.getConnHandle() == baglantiNo) { bagli = false; baglantiNo = 0xFFFF; }
    if (i.getConnHandle() == ekranBaglantiNo) { ekranIstemci = false; ekranBaglantiNo = 0xFFFF; }
    if (i.getConnHandle() == waBaglantiNo) { waIstemci = false; waPCHazir = false; waBaglantiNo = 0xFFFF; }
    NimBLEDevice::startAdvertising();
  }
};
class TelefonAbone : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &i, uint16_t deger) override { bagli = deger != 0; baglantiNo = deger ? i.getConnHandle() : 0xFFFF; }
};
class EkranAbone : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &i, uint16_t deger) override { ekranIstemci = deger != 0; ekranBaglantiNo = deger ? i.getConnHandle() : 0xFFFF; }
};
class GelenYaz : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *k, NimBLEConnInfo &) override {
    NimBLEAttValue v = k->getValue();
    String satir(v.c_str(), v.length());
    portENTER_CRITICAL(&kilit); gelenler.push_back(satir); portEXIT_CRITICAL(&kilit);
  }
};

class WaAbone : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &i, uint16_t v) override { waIstemci = v != 0; waPCHazir = false; waBaglantiNo = v ? i.getConnHandle() : 0xFFFF; }
};
class WaGelen : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *k, NimBLEConnInfo &i) override {
    if (i.getConnHandle() != waBaglantiNo) return;
    NimBLEAttValue v = k->getValue(); String s(v.c_str(), v.length());
    if (!s.startsWith("F") || s.length() > 180) return;
    portENTER_CRITICAL(&kilit); gelenler.push_back(s); portEXIT_CRITICAL(&kilit);
  }
};
void goster(const String &m, uint32_t sure = 2500) { mesaj = m; mesajBitis = millis() + sure; }

struct { int pil = -1; bool sarj = false; long adim = -1, hedef = 8000; String etkinlik; uint32_t etkinlikEpoch = 0; String arayan; bool araniyor = false; } tel;
void telefonaGonder(const String &komut, const char *yazi) {
  if (!bagli) { goster(dl("telefon yok", "no phone")); return; }
  bleKaydet(">" + komut);
  tusKar->setValue(komut.c_str());
  tusKar->notify();
  if (yazi) goster(yazi);
}

void isle(String v) {
  if (!v.length()) return;
  char tur = v[0];
  String g = v.substring(1);
  bleKaydet("<" + v);
  auto p = parcala(g, '|');
  switch (tur) {
    case 'F': waSonuc(g); break;
    case '!': goster(ascii(g), 4000); break;
    case 'S':
      if (p.size() < 3) return;
      durum.sicaklik = p[0].toInt(); durum.model = ascii(p[1]); durum.hal = ascii(p[2]); durum.zaman = millis();
      if (p.size() > 3) { timeval tv = {(time_t)p[3].toInt(), 0}; if (tv.tv_sec > 1600000000) settimeofday(&tv, nullptr); }   // RTC uykuda da işler
      break;
    case 'H': if (p.size() >= 6) { hava.kod = p[0].toInt(); hava.sic = p[1].toInt(); hava.max = p[2].toInt(); hava.min = p[3].toInt(); hava.yagmur = p[4].toInt(); hava.yer = ascii(p[5]); } break;
    case 'O': if (p.size() >= 6) { ozet.yeni = p[0].toInt(); ozet.isaret = p[1].toInt(); ozet.mesaj = p[2].toInt(); ozet.aktif = p[3].toInt(); ozet.acik = p[4].toInt(); ozet.kup = p[5].toInt(); ozet.var = true; } break;
    case 'M': if (p.size() >= 3) { muzik.caliyor = p[0] == "1"; muzik.sarki = ascii(p[1]); muzik.sanatci = ascii(p[2]);   // + konum|süre|beğenildi|karışık (sn)
                if (p.size() >= 7) { muzik.poz = p[3].toInt(); muzik.sure = p[4].toInt(); muzik.begen = p[5] == "1"; muzik.karistir = p[6] == "1"; } muzik.zaman = millis(); } break;
    case 'Q': pcSatiri(g); break;                                                                                   // PC durumu
    case 'B': yaziciSatiri(g); break;                                                                               // 3B yazıcı
    case 'x': hataAlindi(g.toInt()); break;                                                                         // hata raporu alındı
    case 'L': seviyeSatiri(g); break;                                                                               // telefondan bölüm
    case 'N':
      if (p.size() >= 3) {
        bildirimler.insert(bildirimler.begin(), {ascii(p[0]), ascii(p[1]), ascii(p[2])});
        if (bildirimler.size() > 6) bildirimler.pop_back();
        sayac("c_bild", 1);
        popupBitis = millis() + 5000;
      }
      break;
    case 'A': cevap = ascii(g); break;
    case 'P': if (p.size() >= 2) { tel.pil = p[0].toInt(); tel.sarj = p[1] == "1"; } break;                       // telefon pili
    case 'R': tel.araniyor = g.length() > 0; tel.arayan = ascii(g); if (tel.araniyor) sonDokunus = millis(); break;   // gelen arama (boş: bitti)
    case 'T': if (p.size() >= 2) { tel.etkinlik = ascii(p[0]); tel.etkinlikEpoch = p[1].toInt(); } else tel.etkinlik = ""; break;   // takvim
    case 'D': if (p.size() >= 1) { tel.adim = p[0].toInt(); if (p.size() > 1 && p[1].toInt() > 0) tel.hedef = p[1].toInt(); } break;   // adım
    case 'Z': geriSatir(g); break;
    case 'U': ucakSatiri(g); break;                                                                                 // uçak radarı
    case 'V': rotaSatiri(g); break;                                                                                 // uçuşun rotası
    case 'I': issSatiri(g); break;                                                                                  // ISS konumu
    case 'J': issYolSatiri(g); break;                                                                               // ISS'in önümüzdeki yolu
    case 'W': { int i = g.indexOf('\x1f'); if (i > 0) { wifiKaydet(g.substring(0, i), g.substring(i + 1)); goster(String(dl("Wi-Fi kaydedildi: ", "Wi-Fi saved: ")) + ascii(g.substring(0, i)), 3500); } } break;   // telefondan Wi-Fi                                                                                  // yedekten geri yükleme
    case 'C': if (cevap.length() < 1500) cevap += ascii(g); break;
    case 'E': if (beklemede) { beklemede = false; sayfa = CEVAP; satirKay = 0; } break;
  }
}

bool saatVar() { return time(nullptr) > 1600000000; }
tm simdiTm() { time_t t = time(nullptr); tm s; gmtime_r(&t, &s); return s; }   // telefon yerel saati gönderiyor
bool gunduz() { return !saatVar() || (simdiTm().tm_hour >= 7 && simdiTm().tm_hour < 19); }

// ---------------------------------------------------------------- yazılar
void yaz(int x, int y, int boy, const String &s) { ekran.setTextSize(boy); ekran.setCursor(x, y); ekran.print(s); }
void ortala(int y, int boy, const String &s) { yaz((128 - (int)s.length() * 6 * boy) / 2, y, boy, s); }
void sigdir(int x, int y, const String &s) { yaz(x, y, (int)s.length() * 12 <= 128 - x ? 2 : 1, s); }   // sığarsa büyük yaz
std::vector<String> sar(const String &s, int genislik) {   // kelime kelime satıra böl
  std::vector<String> satirlar; String satir;
  String duz = s; duz.replace("\n", " ");
  for (auto &k : parcala(duz, ' ')) {
    String kelime = k;
    while ((int)kelime.length() > genislik) { if (satir.length()) { satirlar.push_back(satir); satir = ""; } satirlar.push_back(kelime.substring(0, genislik)); kelime = kelime.substring(genislik); }
    if (satir.length() && (int)(satir.length() + 1 + kelime.length()) > genislik) { satirlar.push_back(satir); satir = ""; }
    satir += (satir.length() ? " " : "") + kelime;
  }
  if (satir.length()) satirlar.push_back(satir);
  return satirlar;
}
void baslik(const String &s) { yaz(0, 0, 1, s); ekran.drawFastHLine(0, 9, 128, W); }
void altMesaj() { if (millis() < mesajBitis) { ekran.fillRect(0, 54, 128, 10, B); ortala(55, 1, mesaj); } }

// ---------------------------------------------------------------- hava simgeleri
void gunes(int x, int y, int r) {
  ekran.fillCircle(x, y, r, W);
  for (int i = 0; i < 8; i++) { float a = i * PI / 4; ekran.drawLine(x + (r + 2) * cos(a), y + (r + 2) * sin(a), x + (r + 4) * cos(a), y + (r + 4) * sin(a), W); }
}
void bulut(int x, int y, int s) {                    // s yaklaşık yarı genişlik
  ekran.fillCircle(x - s / 2, y, s / 2, W); ekran.fillCircle(x + s / 3, y - s / 4, s * 2 / 3, W); ekran.fillCircle(x + s, y + 1, s / 2 - 1, W);
  ekran.fillRect(x - s / 2, y, s + s / 2, s / 2, W);
}
bool karli(int kod) { return kod >= 71 && kod <= 77; }
void havaSimge(int x, int y, int kod, int kare) {   // WMO hava kodu
  if (kod <= 1) { if (gunduz()) gunes(x, y, 6); else { ekran.fillCircle(x, y, 7, W); ekran.fillCircle(x + 4, y - 3, 6, B); } }
  else if (kod <= 3 || kod == 45 || kod == 48) { if (kod == 2) gunes(x + 6, y - 5, 4); bulut(x, y, 9); }
  else {
    bulut(x, y - 3, 9);
    for (int i = 0; i < 4; i++) {
      int dx = x - 6 + i * 5, dy = y + 7 + (kare + i * 3) % 6;
      if (karli(kod)) { ekran.drawPixel(dx, dy, W); ekran.drawPixel(dx + 1, dy + 1, W); ekran.drawPixel(dx - 1, dy + 1, W); }
      else ekran.drawLine(dx, dy, dx - 1, dy + 2, W);
    }
    if (kod >= 95) { ekran.drawLine(x + 2, y + 4, x - 1, y + 9, W); ekran.drawLine(x - 1, y + 9, x + 3, y + 9, W); ekran.drawLine(x + 3, y + 9, x, y + 14, W); }
  }
}
String havaAd(int kod) {
  if (kod <= 1) return gunduz() ? "Gunesli" : "Acik";
  if (kod <= 3) return "Bulutlu";
  if (kod == 45 || kod == 48) return "Sisli";
  if (kod >= 95) return "Firtina";
  if (karli(kod)) return "Karli";
  return "Yagmurlu";
}

// ---------------------------------------------------------------- arayüz (arayuz.html simülatöründen birebir)
void hat(float x0, float y0, float x1, float y1) { ekran.drawLine(lround(x0), lround(y0), lround(x1), lround(y1), W); }
int yuvarla(float v) { return lround(v); }

// Ana ekrandaki Kübit: 41x50, sol üst (x, y)
void kubit(Yuz yuz, int bakX, float acik, int zipla, int kare) {
  int x = 4, y = 14 - zipla;
  const int p[8][2] = {{6, 0}, {34, 0}, {40, 6}, {40, 14}, {34, 20}, {6, 20}, {0, 14}, {0, 6}};
  for (int i = 0; i < 8; i++) {                       // sekizgen kafa, kalın kenar
    auto &a = p[i], &b = p[(i + 1) % 8];
    hat(x + a[0], y + a[1], x + b[0], y + b[1]);
    hat(x + a[0] + (a[0] < 20 ? 1 : -1), y + a[1] + (a[1] < 10 ? 1 : -1), x + b[0] + (b[0] < 20 ? 1 : -1), y + b[1] + (b[1] < 10 ? 1 : -1));
  }
  bool gozluk = hava.kod >= 0 && hava.kod <= 1 && gunduz() && yuz != UYKULU;
  for (int gx : {12, 28}) {
    if (gozluk) ekran.fillRoundRect(x + gx - 4, y + 7, 9, 5, 1, W);
    else if (yuz == UYKULU) hat(x + gx - 3, y + 10, x + gx + 3, y + 10);
    else if (yuz == MUTLU) { hat(x + gx - 3, y + 10, x + gx, y + 7); hat(x + gx, y + 7, x + gx + 3, y + 10); }
    else if (acik < 0.5) hat(x + gx - 3, y + 10, x + gx + 3, y + 10);
    else ekran.fillCircle(x + gx + bakX, y + 9, 3, W);
  }
  if (gozluk) hat(x + 17, y + 9, x + 23, y + 9);
  if (yuz == DUSUNCELI || yuz == TERLI) ekran.drawCircle(x + 20, y + 15, 1, W);
  else if (yuz == UYKULU) hat(x + 17, y + 15, x + 23, y + 15);
  else { hat(x + 16, y + 14, x + 20, y + 16); hat(x + 20, y + 16, x + 24, y + 14); }
  ekran.drawRect(x + 16, y + 21, 9, 6, W); hat(x + 16, y + 23, x + 24, y + 23);                         // boyun
  ekran.drawRoundRect(x + 7, y + 27, 27, 18, 2, W);                                                    // gövde
  ekran.fillCircle(x + 11, y + 31, 1, W); hat(x + 14, y + 31, x + 30, y + 31);
  ekran.fillCircle(x + 11, y + 35, 1, W); hat(x + 14, y + 35, x + 26, y + 35);
  hat(x + 7, y + 38, x + 33, y + 38);
  for (int i = x + 9; i < x + 32; i += 3) hat(i, y + 43, min(i + 4, x + 32), y + 39);                  // taralı şerit
  hat(x + 7, y + 30, x + 2, y + 24);                                                                   // kollar: mutluyken el sallar
  if (yuz == MUTLU && (kare / 4) % 2) hat(x + 33, y + 30, x + 40, y + 21); else hat(x + 33, y + 30, x + 38, y + 24);
  hat(x + 14, y + 45, x + 14, y + 49); hat(x + 27, y + 45, x + 27, y + 49);                             // bacaklar
  if (yuz == UYKULU) { yaz(x + 42, y + 4 - (kare / 3) % 10, 1, "z"); }
  if (yuz == TERLI) { int dy = y + 2 + (kare / 2) % 10; ekran.fillCircle(x + 44, dy + 2, 2, W); ekran.fillTriangle(x + 42, dy + 2, x + 46, dy + 2, x + 44, dy - 2, W); }
}

// ---------------------------------------------------------------- uygulama simgeleri (s = 16 ya da 24, x,y sol üst)
void sKubit(int x, int y, int s, uint32_t t) {
  float k = s / 24.0; const int p[8][2] = {{4, 6}, {20, 6}, {23, 9}, {23, 15}, {20, 18}, {4, 18}, {1, 15}, {1, 9}};
  for (int i = 0; i < 8; i++) hat(x + p[i][0] * k, y + p[i][1] * k, x + p[(i + 1) % 8][0] * k, y + p[(i + 1) % 8][1] * k);
  bool kirp = t % 3000 < 140;
  for (int gx : {8, 16}) { if (kirp) hat(x + (gx - 2) * k, y + 12 * k, x + (gx + 2) * k, y + 12 * k); else ekran.fillCircle(yuvarla(x + gx * k), yuvarla(y + 11 * k), max(1, yuvarla(1.6 * k)), W); }
  hat(x + 10 * k, y + 15 * k, x + 12 * k, y + 16 * k); hat(x + 12 * k, y + 16 * k, x + 14 * k, y + 15 * k);
  hat(x + 12 * k, y + 18 * k, x + 12 * k, y + 22 * k); hat(x + 7 * k, y + 22 * k, x + 17 * k, y + 22 * k);
}
void sSaat(int x, int y, int s) {
  float r = s / 2.0 - 1, cx = x + s / 2.0, cy = y + s / 2.0;
  ekran.drawCircle(yuvarla(cx), yuvarla(cy), yuvarla(r), W); if (s > 16) ekran.drawCircle(yuvarla(cx), yuvarla(cy), yuvarla(r) - 1, W);
  for (int i = 0; i < 12; i += 3) { float a = i / 12.0 * 2 * PI; ekran.drawPixel(yuvarla(cx + sin(a) * (r - 2)), yuvarla(cy - cos(a) * (r - 2)), W); }
  int saat = 10, dakika = 10, saniye = 0;
  if (saatVar()) { tm z = simdiTm(); saat = z.tm_hour; dakika = z.tm_min; saniye = z.tm_sec; }
  float sa = ((saat % 12) + dakika / 60.0) / 12 * 2 * PI, dk = dakika / 60.0 * 2 * PI;
  hat(cx, cy, cx + sin(sa) * r * .5, cy - cos(sa) * r * .5);
  hat(cx, cy, cx + sin(dk) * r * .8, cy - cos(dk) * r * .8);
  if (s > 16) { float sn = saniye / 60.0 * 2 * PI; ekran.drawPixel(yuvarla(cx + sin(sn) * (r - 2)), yuvarla(cy - cos(sn) * (r - 2)), W); }
}
void sHava(int x, int y, int s, uint32_t t) {          // o anki havaya göre: güneş, bulut, yağmur, kar
  float k = s / 24.0;
  int kod = hava.kod < 0 ? 2 : hava.kod;
  if (kod <= 1) { if (gunduz()) gunes(yuvarla(x + 12 * k), yuvarla(y + 12 * k), yuvarla(5 * k)); else { ekran.fillCircle(yuvarla(x + 12 * k), yuvarla(y + 12 * k), yuvarla(7 * k), W); ekran.fillCircle(yuvarla(x + 16 * k), yuvarla(y + 9 * k), yuvarla(6 * k), B); } return; }
  if (kod <= 3) gunes(yuvarla(x + 15 * k), yuvarla(y + 8 * k), yuvarla(3.5 * k + .4));
  bulut(yuvarla(x + 9 * k), yuvarla(y + 13 * k), yuvarla(9 * k) + 2); ekran.fillRect(x, yuvarla(y + 18 * k) + 1, s, 2, B);   // gölge payı
  bulut(yuvarla(x + 9 * k), yuvarla(y + 13 * k), yuvarla(9 * k));
  if (kod >= 51) for (int i = 0; i < 3; i++) {
    int dx = yuvarla(x + (6 + i * 6) * k), dy = yuvarla(y + 19 * k) + (t / 120 + i * 2) % 4;
    if (karli(kod)) ekran.drawPixel(dx, dy, W); else hat(dx, dy, dx - 1, dy + 2);
  }
}
void sSor(int x, int y, int s) {
  float k = s / 24.0;
  ekran.drawRoundRect(x + 1, yuvarla(y + 2 * k), s - 2, yuvarla(15 * k), yuvarla(4 * k), W);
  ekran.fillTriangle(yuvarla(x + 6 * k), yuvarla(y + 16 * k), yuvarla(x + 11 * k), yuvarla(y + 16 * k), yuvarla(x + 5 * k), yuvarla(y + 22 * k), W);
  for (int i = 0; i < 3; i++) ekran.fillCircle(yuvarla(x + (7 + i * 5) * k), yuvarla(y + 9 * k), max(0, yuvarla(1.2 * k)), W);
}
void sOzet(int x, int y, int s) {
  float k = s / 24.0; const int b[4] = {7, 13, 10, 18};
  for (int i = 0; i < 4; i++) ekran.fillRect(yuvarla(x + (2 + i * 5.5) * k), yuvarla(y + (22 - b[i]) * k), max(2, yuvarla(4 * k)), yuvarla(b[i] * k), W);
  hat(x, y + 23 * k, x + s - 1, y + 23 * k);
}
void sZil(int x, int y, int s) {
  float k = s / 24.0;
  for (int j = 4; j <= 17; j++) { float w = j < 8 ? 4 + (j - 4) * 1.2 : 8.5; hat(x + (12 - w) * k, y + j * k, x + (12 + w) * k, y + j * k); }
  hat(x + 2 * k, y + 18 * k, x + 22 * k, y + 18 * k);
  ekran.fillCircle(yuvarla(x + 12 * k), yuvarla(y + 2.5 * k), max(1, yuvarla(1.5 * k)), W);
  ekran.fillCircle(yuvarla(x + 12 * k), yuvarla(y + 20.5 * k), max(1, yuvarla(2 * k)), W);
}
void sMuzik(int x, int y, int s) {
  float k = s / 24.0;
  ekran.fillCircle(yuvarla(x + 6 * k), yuvarla(y + 18 * k), yuvarla(3 * k), W); ekran.fillCircle(yuvarla(x + 17 * k), yuvarla(y + 15 * k), yuvarla(3 * k), W);
  ekran.fillRect(yuvarla(x + 8 * k), yuvarla(y + 4 * k), max(1, yuvarla(1.4 * k)), yuvarla(14 * k), W);
  ekran.fillRect(yuvarla(x + 19 * k), yuvarla(y + 2 * k), max(1, yuvarla(1.4 * k)), yuvarla(13 * k), W);
  for (int i = 0; i < max(2, yuvarla(3 * k)); i++) hat(x + 8 * k, y + (4 + i) * k, x + 20 * k, y + (2 + i) * k);
}
void sBul(int x, int y, int s, uint32_t t) {
  float k = s / 24.0;
  ekran.drawRoundRect(yuvarla(x + 7 * k), y + 1, yuvarla(10 * k), yuvarla(21 * k), 2, W); hat(x + 10 * k, y + 19 * k, x + 14 * k, y + 19 * k);
  int f = (t / 300) % 3;
  for (int i = 0; i <= f; i++) { float r = (4 + i * 3) * k;
    for (int a = -40; a <= 40; a += 8) { float ra = a * PI / 180;
      ekran.drawPixel(yuvarla(x + 17 * k + cos(ra) * r), yuvarla(y + 8 * k + sin(ra) * r), W); ekran.drawPixel(yuvarla(x + 7 * k - cos(ra) * r), yuvarla(y + 8 * k + sin(ra) * r), W); } }
}
void sFlappy(int x, int y, int s, uint32_t t) {
  float k = s / 24.0, yy = y + sin(t / 250.0) * 2 * k;
  ekran.drawRoundRect(yuvarla(x + 4 * k), yuvarla(yy + 6 * k), yuvarla(15 * k), yuvarla(12 * k), yuvarla(3 * k), W);
  ekran.fillCircle(yuvarla(x + 14 * k), yuvarla(yy + 10 * k), max(1, yuvarla(1.3 * k)), W);
  ekran.fillTriangle(yuvarla(x + 18 * k), yuvarla(yy + 11 * k), yuvarla(x + 23 * k), yuvarla(yy + 12 * k), yuvarla(x + 18 * k), yuvarla(yy + 14 * k), W);
  bool kanat = (t / 150) % 2;
  ekran.fillTriangle(yuvarla(x + 1 * k), yuvarla(yy + (kanat ? 8 : 13) * k), yuvarla(x + 9 * k), yuvarla(yy + 12 * k), yuvarla(x + 4 * k), yuvarla(yy + 14 * k), W);
}
void sGeometry(int x, int y, int s, uint32_t t) {
  float k = s / 24.0, a = fmod(t / 400.0, HALF_PI), cx = x + 9 * k, cy = y + 10 * k, r = 7 * k;
  for (int i = 0; i < 4; i++) { float a1 = a + i * HALF_PI + PI / 4, a2 = a1 + HALF_PI; hat(cx + r * cos(a1), cy + r * sin(a1), cx + r * cos(a2), cy + r * sin(a2)); }
  ekran.fillTriangle(yuvarla(x + 15 * k), yuvarla(y + 22 * k), yuvarla(x + 19 * k), yuvarla(y + 14 * k), yuvarla(x + 23 * k), yuvarla(y + 22 * k), W);
  hat(x, y + 23 * k, x + s - 1, y + 23 * k);
}
void sZar(int x, int y, int s) {
  float k = s / 24.0; int d = yuvarla(18 * k), ox = yuvarla(x + 3 * k), oy = yuvarla(y + 3 * k);
  ekran.drawRoundRect(ox, oy, d, d, yuvarla(3 * k), W);
  const float n[5][2] = {{.25, .25}, {.75, .25}, {.5, .5}, {.25, .75}, {.75, .75}};
  for (auto &p : n) ekran.fillCircle(yuvarla(ox + d * p[0]), yuvarla(oy + d * p[1]), max(0, yuvarla(1.4 * k)), W);
}
void sKumanda(int x, int y, int s) {
  float k = s / 24.0, cx = x + s / 2.0, cy = y + s / 2.0 + k, r = 8 * k;
  for (int a = 40; a <= 320; a += 6) { float ra = (a - 90) * PI / 180;
    ekran.drawPixel(yuvarla(cx + cos(ra) * r), yuvarla(cy + sin(ra) * r), W); if (s > 16) ekran.drawPixel(yuvarla(cx + cos(ra) * (r - 1)), yuvarla(cy + sin(ra) * (r - 1)), W); }
  ekran.fillRect(yuvarla(cx - .5), yuvarla(y + 2 * k), max(1, yuvarla(2 * k)), yuvarla(10 * k), W);
}
void sAyar(int x, int y, int s, uint32_t t) {
  float k = s / 24.0, cx = x + s / 2.0, cy = y + s / 2.0, a0 = t / 1500.0; int d = max(2, yuvarla(3 * k));
  for (int i = 0; i < 8; i++) { float a = a0 + i * PI / 4; ekran.fillRect(yuvarla(cx + cos(a) * 9 * k - 1.5 * k), yuvarla(cy + sin(a) * 9 * k - 1.5 * k), d, d, W); }
  ekran.fillCircle(yuvarla(cx), yuvarla(cy), yuvarla(7.5 * k), W); ekran.fillCircle(yuvarla(cx), yuvarla(cy), yuvarla(3 * k), B);
}
void sOyun(int x, int y, int s, uint32_t t) {         // oyun kolu
  float k = s / 24.0;
  ekran.drawRoundRect(x, yuvarla(y + 6 * k), s, yuvarla(13 * k), yuvarla(5 * k), W);
  ekran.fillRect(yuvarla(x + 4 * k), yuvarla(y + 11.5 * k), yuvarla(7 * k), max(1, yuvarla(2 * k)), W); ekran.fillRect(yuvarla(x + 6.5 * k), yuvarla(y + 9 * k), max(1, yuvarla(2 * k)), yuvarla(7 * k), W);
  bool yan = (t / 400) % 2;
  ekran.fillCircle(yuvarla(x + 16 * k), yuvarla(y + 14 * k), max(1, yuvarla(1.5 * k)), W);
  if (yan) ekran.fillCircle(yuvarla(x + 19.5 * k), yuvarla(y + 10.5 * k), max(1, yuvarla(1.5 * k)), W); else ekran.drawCircle(yuvarla(x + 19.5 * k), yuvarla(y + 10.5 * k), max(1, yuvarla(1.5 * k)), W);
}
void sAraclar(int x, int y, int s, uint32_t t) {     // kronometre
  float k = s / 24.0, cx = x + s / 2.0, cy = y + 13 * k, r = 9 * k;
  ekran.drawCircle(yuvarla(cx), yuvarla(cy), yuvarla(r), W); ekran.fillRect(yuvarla(cx - 2 * k), yuvarla(y + 1), max(2, yuvarla(4 * k)), max(1, yuvarla(2 * k)), W);
  float a = t / 600.0; hat(cx, cy, cx + sin(a) * r * .75, cy - cos(a) * r * .75);
}
void sDolap(int x, int y, int s) {                    // askı
  float k = s / 24.0;
  ekran.drawCircleHelper(yuvarla(x + 12 * k), yuvarla(y + 5 * k), yuvarla(3 * k), 1 | 2, W); hat(x + 15 * k, y + 5 * k, x + 12 * k, y + 9 * k);
  hat(x + 12 * k, y + 9 * k, x + 1 * k, y + 18 * k); hat(x + 12 * k, y + 9 * k, x + 23 * k, y + 18 * k); hat(x + 1 * k, y + 18 * k, x + 23 * k, y + 18 * k);
}
void sRozet(int x, int y, int s) {                    // kupa
  float k = s / 24.0;
  ekran.fillRoundRect(yuvarla(x + 6 * k), yuvarla(y + 2 * k), yuvarla(12 * k), yuvarla(10 * k), yuvarla(3 * k), W);
  ekran.drawCircleHelper(yuvarla(x + 6 * k), yuvarla(y + 6 * k), yuvarla(4 * k), 8 | 1, W); ekran.drawCircleHelper(yuvarla(x + 18 * k), yuvarla(y + 6 * k), yuvarla(4 * k), 2 | 4, W);
  ekran.fillRect(yuvarla(x + 11 * k), yuvarla(y + 12 * k), max(2, yuvarla(2 * k)), yuvarla(5 * k), W); ekran.fillRect(yuvarla(x + 7 * k), yuvarla(y + 17 * k), yuvarla(10 * k), max(2, yuvarla(3 * k)), W);
}
void simge(int i, int x, int y, int s, uint32_t t) {
  switch (i) {
    case 0: sKubit(x, y, s, t); break;   case 1: sSaat(x, y, s); break;        case 2: sHava(x, y, s, t); break;
    case 3: sSor(x, y, s); break;        case 4: sOzet(x, y, s); break;        case 5: sZil(x, y, s); break;
    case 6: sMuzik(x, y, s); break;      case 7: sBul(x, y, s, t); break;      case 8: sOyun(x, y, s, t); break;
    case 9: sZar(x, y, s); break;        case 10: sKumanda(x, y, s); break;
    case 12: sAraclar(x, y, s, t); break; case 13: sRozet(x, y, s); break; case 14: sDolap(x, y, s); break;
    default: sAyar(x, y, s, t);
  }
}

// ---------------------------------------------------------------- 3B küp (açılış animasyonu)
const int8_t KOSE[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
const uint8_t YUZ_KOSE[6][4] = {{3, 2, 6, 7}, {0, 1, 5, 4}, {4, 5, 6, 7}, {1, 0, 3, 2}, {5, 1, 2, 6}, {0, 4, 7, 3}};
const int8_t YUZ_NORMAL[6][3] = {{0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}};
const float IZO = atan(1 / sqrt(2.0));               // izometrik eğim: üst, sol ve sağ yüz eşit görünür
void dondur(float x, float y, float z, float teta, float fi, float o[3]) {
  float x1 = x * cos(teta) + z * sin(teta), z1 = -x * sin(teta) + z * cos(teta);
  o[0] = x1; o[1] = y * cos(fi) - z1 * sin(fi); o[2] = y * sin(fi) + z1 * cos(fi);
}
void kup(float cx, float cy, float s, float teta, float fi, float dagilma) {   // yalnız bize bakan yüzler; dagilma: yüzler kendi yönlerine açılır
  for (int f = 0; f < 6; f++) {
    float n[3]; dondur(YUZ_NORMAL[f][0], YUZ_NORMAL[f][1], YUZ_NORMAL[f][2], teta, fi, n);
    if (n[2] <= 0.01) continue;
    float p[4][2];
    for (int i = 0; i < 4; i++) { float v[3]; const int8_t *k = KOSE[YUZ_KOSE[f][i]]; dondur(k[0], k[1], k[2], teta, fi, v); p[i][0] = cx + n[0] * dagilma + v[0] * s; p[i][1] = cy - n[1] * dagilma - v[1] * s; }
    for (int i = 0; i < 4; i++) hat(p[i][0], p[i][1], p[(i + 1) % 4][0], p[(i + 1) % 4][1]);
  }
}
float yumusak(float e) { return e < .5 ? 4 * e * e * e : 1 - pow(-2 * e + 2, 3) / 2; }
void acilisAnimasyonu() {                            // yüzler birleşir -> küp bir tur döner ve aynı açıya oturur -> KUBIT
  const float BIRLES = 900, DON = 1000;
  uint32_t bas = millis();
  for (float t = 0; t < BIRLES + DON + 1100; t = millis() - bas) {
    ekran.clearDisplay();
    if (t < BIRLES) kup(64, 24, 10.5, PI / 4, IZO, pow(1 - t / BIRLES, 3) * 46);
    else { float e = yumusak(min(1.0f, (t - BIRLES) / DON)); kup(64, 24 - sin(e * PI) * 3, 10.5, PI / 4 + e * 2 * PI, IZO, 0); }
    if (t > BIRLES + DON) { int n = min(5, (int)((t - BIRLES - DON) / 70) + 1); String y = String("KUBIT").substring(0, n); yaz((128 - 59) / 2, 46, 2, y); }
    ekran.display();
    if (digitalRead(TUS1) == LOW || digitalRead(TUS2) == LOW) break;   // tuşa basınca atla
  }
}

// ---------------------------------------------------------------- durum çubuğu: saat · hava · uyarı · Bluetooth
struct { int dk = 5; uint32_t bitis = 0, kalan = 0; bool calisiyor = false; } zam;
struct { uint32_t bas = 0, birikim = 0; bool calisiyor = false; uint32_t tur[2] = {0, 0}; } kro;
struct { bool calisiyor = false, mola = false; uint32_t bitis = 0, kalan = 25 * 60000UL; int tamam = 0; } pom;
void durumCubugu() {
  if (saatVar()) { tm s = simdiTm(); yaz(0, 0, 1, ikiHane(s.tm_hour) + ((s.tm_sec % 2) ? ":" : " ") + ikiHane(s.tm_min)); }
  else yaz(0, 0, 1, "--:--");
  if (hava.kod >= 0) {
    if (hava.kod <= 1 && gunduz()) { ekran.fillCircle(41, 3, 1, W); for (int i = 0; i < 8; i++) { float a = i * PI / 4; ekran.drawPixel(41 + lround(cos(a) * 3), 3 + lround(sin(a) * 3), W); } }
    else bulut(40, 4, 4);
    yaz(47, 0, 1, String(hava.sic) + "C");
  }
  if (tel.pil >= 0 && bagli) { String p2 = String(tel.pil) + "%"; yaz(104 - p2.length() * 6, 0, 1, p2); }
  if (zam.calisiyor || pom.calisiyor || kro.calisiyor) { ekran.drawLine(64, 1, 68, 1, W); ekran.drawLine(64, 7, 68, 7, W); ekran.drawLine(64, 1, 68, 7, W); ekran.drawLine(68, 1, 64, 7, W); }   // kum saati
  if (ozet.isaret || ozet.yeni) { ekran.fillRoundRect(108, 0, 7, 8, 2, W); ekran.setTextColor(B); yaz(109, 0, 1, "!"); ekran.setTextColor(W); }
  if (bagli) { hat(119, 2, 123, 5); hat(123, 5, 121, 7); hat(121, 7, 121, 0); hat(121, 0, 123, 2); hat(123, 2, 119, 5); }   // Bluetooth
  else { hat(119, 1, 124, 6); hat(124, 1, 119, 6); }
}

// ---------------------------------------------------------------- liste menüsü (Tuş 1 kaydırır, Tuş 2 seçer)
int liste(const String &bas, const String *ogeler, int n, Olay o1, Olay o2) {
  if (o1 == KISA) imlec = (imlec + 1) % n;
  if (geriAdim) imlec = (imlec - 1 + n) % n;
  imlec = min(imlec, n - 1);
  baslik(bas);
  if (ayarBuyukYazi) {                                // büyük yazı: 3 satır, 2 kat
    int ilk = constrain(imlec - 1, 0, max(0, n - 3));
    for (int i = ilk; i < ilk + 3 && i < n; i++) { int y = 12 + (i - ilk) * 17; if (i == imlec) { ekran.fillRect(0, y - 1, 126, 17, W); ekran.setTextColor(B); } yaz(2, y, 2, ogeler[i].substring(0, 10)); ekran.setTextColor(W); }
    if (n > 3) ekran.fillRect(126, 11 + 52 * ilk / n, 2, max(2, 52 * 3 / n), W);
    return o2 == KISA ? imlec : -1;
  }
  int ilk = constrain(imlec - 2, 0, max(0, n - 5));
  for (int i = ilk; i < ilk + 5 && i < n; i++) {
    int y = 12 + (i - ilk) * 10;
    if (i == imlec) { ekran.fillRect(0, y - 1, 126, 10, W); ekran.setTextColor(B); }
    yaz(3, y, 1, ogeler[i]);
    ekran.setTextColor(W);
  }
  if (n > 5) ekran.fillRect(126, 11 + 52 * ilk / n, 2, 52 * 5 / n, W);   // kaydırma çubuğu
  return o2 == KISA ? imlec : -1;
}
void git(Sayfa s) { sayfa = s; imlec = 0; }

// ---------------------------------------------------------------- sayfalar
void anaSayfa(Olay o1, Olay o2, int kare) {
  static uint32_t gozKirp = 0, sonrakiKirp = 0, bakDegis = 0, zipla = 0;
  static int bak = 0;
  if (o1 == KISA) telefonaGonder("AC", "Kubik aciliyor");
  if (o2 == KISA) { git(MENU); return; }
  uint32_t simdi = millis();
  if (simdi > bakDegis) { int s[] = {-1, 0, 0, 1}; bak = s[random(4)]; bakDegis = simdi + random(2000, 5000); }
  if (simdi > sonrakiKirp) { gozKirp = simdi; sonrakiKirp = simdi + random(2500, 5000); }
  if (!zipla && random(300) == 0) zipla = simdi;
  int yukseklik = 0;
  if (zipla) { uint32_t g = simdi - zipla; if (g > 500) zipla = 0; else yukseklik = (int)(2 * sin(PI * g / 500.0)); }
  Yuz yuz = NORMAL;
  if (simdi < mesajBitis) yuz = MUTLU;
  else if (durum.zaman && durum.sicaklik >= 80) yuz = TERLI;
  else if (durum.zaman && durum.hal == "UYKUDA") yuz = UYKULU;
  duvarCiz(simdi);
  durumCubugu();
  kubit(yuz, bak, simdi - gozKirp < 150 ? 0.1 : 1, yukseklik, kare);
  kiyafetCiz(4, 14 - yukseklik, simdi);
  const int x = 54;                                   // sağ panel
  if (simdi < mesajBitis) { int y = 16; for (auto &s : sar(mesaj, 12)) { yaz(x, y, 1, s); y += 10; } }
  else if (!bagli) { yaz(x, 16, 1, "telefon"); yaz(x, 26, 1, "bekleniyor"); }
  else if (!durum.zaman) { yaz(x, 16, 1, "veri"); yaz(x, 26, 1, "bekleniyor"); }
  else {
    sigdir(x, 14, durum.hal);
    if (durum.sicaklik >= 0) yaz(x, 33, 1, "PI " + String(durum.sicaklik) + "C");
    yaz(x, 43, 1, millis() - durum.zaman > ESKI_VERI_MS ? "eski veri" : durum.model);
  }
  ekran.drawFastHLine(x, 54, 128 - x, W);
  yaz(x, 56, 1, dl("2: uygulama", "2: apps"));   // 12 karakter: sağ panele sığar
}

const String MENU_OGE[] = {"KUBIT", "SAAT", "HAVA", "KUBIK'E SOR", "OZET", "BILDIRIMLER", "MUZIK", "TELEFONU BUL",
                           "OYUNLAR", "SANS", "KUMANDA", "AYARLAR", "ARACLAR", "ROZETLER", "DOLAP"};
const String MENU_OGE_EN[] = {"KUBIT", "CLOCK", "WEATHER", "ASK KUBIK", "SUMMARY", "NOTIFICATIONS", "MUSIC", "FIND PHONE",
                              "GAMES", "LUCK", "REMOTE", "SETTINGS", "TOOLS", "BADGES", "WARDROBE"};
const String &menuAd(int i) { return dil ? MENU_OGE_EN[i] : MENU_OGE[i]; }
const Sayfa MENU_HEDEF[] = {ANA, SAAT, HAVA, SOR, OZET, BILDIRIM, MUZIK, BUL, OYUNLAR, SANS, KUMANDA, AYAR, ARACLAR, ROZETLER, DOLAP};
const int MENU_N = 15;
extern int oyunImlec;                                // Oyunlar ızgarasında son seçilen (aşağıda tanımlı)
int menuSecili = 0;
float menuKay = 0;                                   // kayan şeridin konumu (yay gibi hedefe oturur)

// ---------------------------------------------------------------- ortak çizim: ters kutu, bit simgesi, açılış geçişi
void tersKutu(int x, int y, int w, int h, int r) {   // yuvarlak köşeli alanın piksellerini tersine çevirir (mercek / seçim)
  for (int yy = max(0, y); yy < min(64, y + h); yy++) for (int xx = max(0, x); xx < min(128, x + w); xx++) {
    int dx = xx < x + r ? x + r - xx : xx > x + w - 1 - r ? xx - (x + w - 1 - r) : 0, dy = yy < y + r ? y + r - yy : yy > y + h - 1 - r ? yy - (y + h - 1 - r) : 0;
    if (dx * dx + dy * dy <= r * r) ekran.drawPixel(xx, yy, ekran.getPixel(xx, yy) ? B : W);
  }
}
void bitSimge(const uint16_t *b, int x, int y, int o = 1) {   // 16x16 simge, o kat büyük
  for (int r = 0; r < 16; r++) for (int c = 0; c < 16; c++) if (b[r] >> (15 - c) & 1) ekran.fillRect(x + c * o, y + r * o, o, o, W);
}
struct { uint32_t bas = 0; float x, y, w, h; int menuSimge; const uint16_t *bit; Sayfa hedef; } gecis;   // uygulama açılırken kutu büyür
const uint32_t GECIS_MS = 220;
void gecisBaslat(float x, float y, float w, float h, int menuSimge, const uint16_t *bit, Sayfa hedef) { gecis = {millis(), x, y, w, h, menuSimge, bit, hedef}; }
bool gecisCiz() {                                     // true: geçiş sürüyor, sayfayı çizme
  if (!gecis.bas) return false;
  float p = min(1.0f, (millis() - gecis.bas) / (float)GECIS_MS), e = 1 - (1 - p) * (1 - p) * (1 - p);   // hızlı başlar, yumuşak durur
  int x = lround(gecis.x * (1 - e)), y = lround(gecis.y * (1 - e)), w = lround(gecis.w + (128 - gecis.w) * e), h = lround(gecis.h + (64 - gecis.h) * e);
  ekran.fillRoundRect(x, y, w, h, max(2, (int)(8 * (1 - e))), W);
  ekran.fillRoundRect(x + 2, y + 2, w - 4, h - 4, max(1, (int)(6 * (1 - e))), B);
  int s = lround(24 + 16 * e), cx = x + w / 2, cy = y + h / 2;
  if (gecis.menuSimge >= 0) simge(gecis.menuSimge, cx - s / 2, cy - s / 2, s, millis());
  else bitSimge(gecis.bit, cx - (p > .5 ? 16 : 8), cy - (p > .5 ? 16 : 8), p > .5 ? 2 : 1);
  if (p >= 1) { gecis.bas = 0; sayfa = gecis.hedef; }
  return true;
}

String menuIpucu(int i) {                            // seçili uygulamanın altında canlı bilgi
  switch (i) {
    case 0: return !bagli ? "telefon bekleniyor" : durum.zaman ? durum.hal + "  Pi " + String(durum.sicaklik) + "C" : "veri bekleniyor";
    case 1: if (!saatVar()) return "saat bekleniyor";
            { const char *GUN[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"}; tm z = simdiTm();
              return ikiHane(z.tm_hour) + ":" + ikiHane(z.tm_min) + "  " + GUN[z.tm_wday]; }
    case 2: return hava.kod >= 0 ? String(hava.sic) + "C " + havaAd(hava.kod) : "veri yok";
    case 3: return beklemede ? "dusunuyor..." : cevap.length() ? "son cevap hazir" : "aklina takilani sor";
    case 4: return ozet.var ? String(ozet.yeni) + " yeni  " + String(ozet.kup) + " kup" : "veri yok";
    case 5: return bildirimler.empty() ? "bildirim yok" : String((int)bildirimler.size()) + " bildirim";
    case 6: return muzik.caliyor ? muzik.sarki : String("calmiyor");
    case 7: return buluyor ? "caliyor!" : bagli ? "telefonu caldir" : "telefon yok";
    case 8: return String(oyunSayisi()) + " oyun";
    case 9: return "zar, yazi tura, cark";
    case 10: return durum.zaman ? "Pi " + String(durum.sicaklik) + "C  " + durum.hal : String("Pi'yi yonet");
    case 12: return zam.calisiyor ? dl("zamanlayici ", "timer ") + String((zam.bitis - millis()) / 60000 + 1) + dl(" dk", " min") : pom.calisiyor ? String("pomodoro") : String(dl("zaman, alarm, telefon", "timer, alarm, phone"));
    case 13: return String(kayit.getInt("c_rozet", 0)) + "/100";
    case 14: return carkHazir() ? dl("gunluk cark hazir!", "daily wheel ready!") : String(__builtin_popcount(kayit.getInt("kiyafet", 0))) + dl("/16 kiyafet", "/16 outfits");
    default: return dl("surum ", "version ") + String(SURUM);
  }
}
String gridAd(int g);
void oyunAc(int g, float x, float y);
const uint16_t *gridSimge(int g);
void ogeSimge(int id, int x, int y, int s, uint32_t t) { if (id < 100) simge(id, x, y, s, t); else bitSimge(gridSimge(id - 100), x + (s - 16) / 2, y + (s - 16) / 2); }
String ogeAd(int id) { if (id < 100) return menuAd(id); String a = gridAd(id - 100); a.toUpperCase(); return a; }
String ogeIpucu(int id) { if (id < 100) return menuIpucu(id); return favMi(id - 100) ? dl("favori oyun", "favorite game") : dl("son oynanan", "last played"); }
void ogeAc(int id, float x, float y, float w, float h) {
  if (id >= 100) { oyunAc(id - 100, x, y); return; }
  Sayfa s = MENU_HEDEF[id];
  if (s == MUZIK) telefonaGonder("MUZ:durum", nullptr);
  if (s == ARACLAR && false) {}
  if (s == OYUNLAR) imlec = oyunImlec; else imlec = 0;
  gecisBaslat(x, y, w, h, id, nullptr, s);
}
void menuSayfasi(Olay o1, Olay o2) {                 // stil 0: ortada sabit ters "mercek", 1: liste, 2: ızgara
  static float hiz = 0; static int hedef = 0;
  if (!menuListeN) menuListeKur();
  const int n = menuListeN;
  menuSecili = min(menuSecili, n - 1);
  if (ayarMenuStil == 1) {                            // liste
    String og[24]; for (int i = 0; i < n; i++) og[i] = ogeAd(menuListe[i]);
    imlec = menuSecili; int s = liste(dl("MENU", "MENU"), og, n, o1, o2); menuSecili = imlec;
    if (s >= 0) ogeAc(menuListe[s], 0, 12, 128, 10);
    return;
  }
  if (ayarMenuStil == 2) {                            // ızgara: 4 sütun, 2 satır görünür
    static float kay = 0;
    if (o1 == KISA) menuSecili = (menuSecili + 1) % n;
    if (geriAdim) menuSecili = (menuSecili - 1 + n) % n;
    int satir = menuSecili / 4, ust = constrain(satir - 1 + (satir == 0), 0, max(0, (n + 3) / 4 - 2));
    kay += (ust * 22 - kay) * .35f;
    if (o2 == KISA) { ogeAc(menuListe[menuSecili], (menuSecili % 4) * 32 + 4, 11 + satir * 22 - kay, 24, 20); return; }
    for (int i = 0; i < n; i++) { int y = lround(11 + (i / 4) * 22 - kay); if (y < -20 || y > 54) continue; ogeSimge(menuListe[i], (i % 4) * 32 + 8, y + 2, 16, millis()); }
    tersKutu((menuSecili % 4) * 32 + 4, lround(11 + satir * 22 - kay), 24, 20, 4);
    ekran.fillRect(0, 0, 128, 10, B); ekran.fillRect(0, 53, 128, 11, B);
    durumCubugu(); ekran.drawFastHLine(0, 53, 128, W); ortala(55, 1, ogeAd(menuListe[menuSecili]));
    return;
  }
  const int CY = 25, ARA = 40, MERCEK = 32;
  if (o1 == KISA) { menuSecili = (menuSecili + 1) % n; hedef++; }
  if (geriAdim) { menuSecili = (menuSecili - 1 + n) % n; hedef--; }
  hiz = hiz * 0.45 + (hedef - menuKay) * 0.38;       // yay: hafifçe taşar, geri oturur
  menuKay += hiz;
  if (fabs(hedef - menuKay) < 0.01 && fabs(hiz) < 0.01) { menuKay = hedef; hiz = 0; }
  if (o2 == KISA) { ogeAc(menuListe[menuSecili], 64 - MERCEK / 2, CY - MERCEK / 2, MERCEK, MERCEK); return; }
  durumCubugu();
  uint32_t t = millis();
  int taban = lround(menuKay), kayma = ((taban - hedef) % n + n) % n;   // şeridin gösterdiği öğe: menuSecili'den taşma kadar kaymış
  for (int d = -2; d <= 2; d++) {
    int i = ((menuSecili + kayma + d) % n + n) % n;
    float konum = (taban + d - menuKay) * ARA, yakin = max(0.0f, 1 - fabs(konum) / ARA);
    int s = lround(16 + 8 * yakin), x = lround(64 + konum - s / 2.0), y = lround(CY - s / 2.0);
    if (x > 128 || x + s < 0) continue;
    ogeSimge(menuListe[i], x, y, s, t);
    if (yakin < .9) ekran.drawRoundRect(x - 4, y - 4, s + 8, s + 8, 5, W);
  }
  tersKutu(64 - MERCEK / 2, CY - MERCEK / 2, MERCEK, MERCEK, 7);
  ekran.setTextWrap(false);
  for (int d = -1; d <= 1; d++) {
    int i = ((menuSecili + kayma + d) % n + n) % n;
    String ad = ogeAd(menuListe[i]); int boy = ayarBuyukYazi && ad.length() <= 10 && d == 0 && !hiz ? 2 : 1;
    yaz(lround(64 + (taban + d - menuKay) * 96 - ad.length() * 3.0 * boy), boy == 2 ? 38 : 44, boy, ad);
  }
  ekran.setTextWrap(true);
  for (int x = 0; x < 128; x += 2) ekran.drawPixel(x, 53, W);
  if (!hiz) { String ip = ogeIpucu(menuListe[menuSecili]); if (ip.length() > 21) ip = ip.substring(0, 20) + "."; ortala(55, 1, ip); }
}

void saatSayfasi() {
  baslik("SAAT");
  if (!saatVar()) { ortala(24, 2, "--:--"); ortala(46, 1, "telefon bekleniyor"); return; }
  const char *GUN[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
  const char *AY[] = {"Ocak", "Subat", "Mart", "Nisan", "Mayis", "Haziran", "Temmuz", "Agustos", "Eylul", "Ekim", "Kasim", "Aralik"};
  tm s = simdiTm();
  sSaat(4, 14, 40);
  yaz(52, 16, 2, ikiHane(s.tm_hour) + ((s.tm_sec % 2) ? ":" : " ") + ikiHane(s.tm_min));   // 60 piksel: analog saatin yanına sığar
  yaz(114, 23, 1, ikiHane(s.tm_sec));
  yaz(52, 38, 1, String(s.tm_mday) + " " + AY[s.tm_mon]);
  yaz(52, 48, 1, GUN[s.tm_wday]);
}

void havaSayfasi(int kare) {
  if (hava.kod < 0) { baslik("HAVA"); yaz(0, 20, 1, bagli ? "Kubik uygulamasini bir" : "telefon bekleniyor"); if (bagli) yaz(0, 30, 1, "kez ac, konum izni ver"); return; }
  baslik(hava.yer.length() ? hava.yer : "HAVA");
  havaSimge(24, 30, hava.kod, kare);
  yaz(52, 16, 3, String(hava.sic) + "C");
  yaz(52, 42, 1, havaAd(hava.kod));
  yaz(0, 55, 1, "En " + String(hava.max) + "/" + String(hava.min) + "C  yagis %" + String(hava.yagmur));
}

const String SORULAR[] = {"Klavye ile yaz", "Bir fikra anlat", "Bugun ne yapsam?", "Ilginc bir bilgi ver", "Beni motive et", "Bir bilmece sor"};
void klavyeAc(Hedef h) { klavyeHedef = h; yazi = ""; sayfa = KLAVYE; imlec = 3; klavyeKucuk = h == WIFI_SIFRE || h == WIFI_AD || h == WA_AD || h == WA_METIN; }
void soruGonder(const String &soru) {
  if (!bagli) { goster("telefon yok"); return; }
  cevap = ""; beklemede = true; sayac("c_soru", 1);
  telefonaGonder("SOR:" + soru.substring(0, 150), nullptr);
}
void sorSayfasi(Olay o1, Olay o2, int kare) {
  if (beklemede) {                                    // cevap bekleniyor
    kubit(DUSUNCELI, 0, 1, 0, kare);
    yaz(54, 16, 1, "Kubik"); yaz(54, 26, 1, "dusunuyor");
    for (int i = 0; i < 3; i++) if ((kare / 5) % 3 == i) ekran.fillCircle(58 + i * 8, 44, 2, W); else ekran.drawCircle(58 + i * 8, 44, 2, W);
    yaz(54, 55, 1, "2: vazgec");
    if (o2 == KISA) beklemede = false;
    return;
  }
  int s = liste("KUBIK'E SOR", SORULAR, 6, o1, o2);
  if (s == 0) klavyeAc(SORU);
  else if (s > 0) soruGonder(SORULAR[s]);
  altMesaj();
}

// klavye: Tuş 1 kaydırır (basılı tutunca hızlı), Tuş 2 harfi yazar, Tuş 2 uzun bitirir
const String TUSLAR[] = {"BITTI", "SIL", "BOS", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
                         "?", ".", "!", ",", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "/", ":", "-", "@", "#", "*", "+", "=", "&", "$", "%", "aA", "_", "(", ")", "[", "]", "{", "}", "\\", "\"", "'", ";", "<", ">", "~", "^", "`", "|", "GERI"};
const int TUS_N = sizeof(TUSLAR) / sizeof(TUSLAR[0]);
const String NUM_TUS[] = {"BITTI", "SIL", "+", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "GERI"};
int tusSayisi() { return klavyeHedef == WA_NO ? 14 : TUS_N; }
String tusAd(int i) { String t = klavyeHedef == WA_NO ? NUM_TUS[i] : TUSLAR[i]; if (klavyeKucuk && t.length() == 1 && t[0] >= 'A' && t[0] <= 'Z') t.toLowerCase(); return t; }
bool klavyeTekrarUygun() { String t = tusAd(imlec); return t.length() == 1 || t == "SIL" || t == "BOS"; }
void klavyeVazgec() {
  if (klavyeHedef == WEB) { webKlavyeMetin = ""; webKlavyeBitti = true; return; }
  if (klavyeHedef == WA_AD || klavyeHedef == WA_NO || klavyeHedef == WA_METIN) { waGeri(); return; }
  git(klavyeHedef == ISIM ? CARK : (klavyeHedef == WIFI_SIFRE || klavyeHedef == WIFI_AD) ? WIFI_S : SOR);
}
void klavyeBitir() {
  String metin = yazi; metin.trim();
  if (klavyeHedef == WA_AD || klavyeHedef == WA_NO || klavyeHedef == WA_METIN) { waKlavyeBitti(klavyeHedef, metin); return; }
  if (klavyeHedef == WEB) { webKlavyeMetin = metin; webKlavyeBitti = true; return; }   // tarayıcı kendi döngüsünde bekliyor
  if (klavyeHedef == WIFI_AD) { if (!yazi.length()) { git(WIFI_S); return; } wifiSecilen = yazi; klavyeAc(WIFI_SIFRE); return; }
  if (klavyeHedef == WIFI_SIFRE) { git(WIFI_S); wifiDene(wifiSecilen, yazi); return; }   // şifrede boşluk kırpılmaz
  if (klavyeHedef == ISIM) {
    if (metin.length()) { String isimler = kayit.getString("isimler", ""); kayit.putString("isimler", isimler.length() ? isimler + "|" + metin : metin); }
    git(CARK); return;
  }
  git(SOR);
  if (metin.length()) soruGonder(metin);
}
void klavyeSayfasi(Olay o1, Olay o2) {
  if (o1 == KISA) imlec = (imlec + 1) % tusSayisi();
  if (geriAdim) imlec = (imlec - 1 + tusSayisi()) % tusSayisi();
  if (o2 == UZUN) { klavyeBitir(); return; }
  if (o2 == KISA) {
    String t = tusAd(imlec);
    if (t == "GERI") { klavyeVazgec(); return; }
    if (t == "BITTI") { klavyeBitir(); return; }
    if (t == "aA") { klavyeKucuk = !klavyeKucuk; t = ""; }
    if (t == "SIL") { if (yazi.length()) yazi.remove(yazi.length() - 1); }
    else if (t.length() && (int)yazi.length() < (klavyeHedef == ISIM ? 10 : klavyeHedef == WIFI_SIFRE ? 63 : klavyeHedef == WIFI_AD ? 32 : klavyeHedef == WA_AD ? 12 : klavyeHedef == WA_NO ? 16 : klavyeHedef == WA_METIN ? 120 : 60)) yazi += t == "BOS" ? " " : t;
  }
  baslik(klavyeHedef == WA_AD ? String("KISI ADI (12)") : klavyeHedef == WA_NO ? String("ULKE KODUYLA NUMARA") : klavyeHedef == WA_METIN ? String("WHATSAPP MESAJ (120)") : klavyeHedef == ISIM ? String("ISIM YAZ") : klavyeHedef == WEB ? String(webKlavyeBaslik) : klavyeHedef == WIFI_AD ? String(dl("GIZLI AG ADI", "HIDDEN SSID")) : klavyeHedef == WIFI_SIFRE ? dl("SIFRE: ", "PASSWORD: ") + wifiSecilen.substring(0, 12) : String("SORUNU YAZ"));
  String son = yazi.length() > 20 ? yazi.substring(yazi.length() - 20) : yazi;
  if (klavyeHedef == WIFI_SIFRE) { son = ""; for (int i = 0; i < min(20, (int)yazi.length()); i++) son += "*"; }
  yaz(0, 13, 1, son + ((millis() / 400) % 2 ? "_" : ""));
  ekran.drawFastHLine(0, 23, 128, W);
  for (int d = -3; d <= 3; d++) {                     // ortada büyük seçili tuş, yanlarda komşular
    String t = tusAd((imlec + d + tusSayisi()) % tusSayisi());
    if (d == 0) { int boy = t.length() > 1 ? 2 : 3, g = t.length() * 6 * boy; ekran.drawRect(64 - g / 2 - 3, 27, g + 6, 24, W); yaz(64 - g / 2, boy == 3 ? 29 : 32, boy, t); }
    else { int x = 64 + d * 17 + (d > 0 ? 8 : -8) - (int)t.length() * 3; if (x >= 0 && x + (int)t.length() * 6 <= 128 && (t.length() == 1 || abs(d) == 1)) yaz(x, 36, 1, t); }
  }
  yaz(0, 56, 1, dl("R/L:gez 2t:tut/yaz", "hold R/L move 2t:type"));
}

void cevapSayfasi(Olay o1, Olay o2) {
  static uint32_t sonKay = 0;
  auto satirlar = sar(cevap.length() ? cevap : String("(bos cevap)"), 21);
  int fazla = max(0, (int)satirlar.size() - 5);
  if (o1 == KISA) { satirKay = satirKay >= fazla ? 0 : satirKay + 1; sonKay = millis() + 4000; }
  else if (fazla && millis() > sonKay + 1800) { sonKay = millis(); if (satirKay < fazla) satirKay++; }   // kendiliğinden kayar
  if (o2 == KISA) { git(SOR); return; }
  baslik("KUBIK");
  for (int i = 0; i < 5 && satirKay + i < (int)satirlar.size(); i++) yaz(0, 12 + i * 10, 1, satirlar[satirKay + i]);
  if (fazla) ekran.fillRect(126, 11 + 52 * satirKay / satirlar.size(), 2, max(3, 52 * 5 / (int)satirlar.size()), W);
}

String sure(long sn) { return sn < 3600 ? String(sn / 60) + " dk" : sn < 86400 ? String(sn / 3600) + " sa " + String(sn % 3600 / 60) + " dk" : String(sn / 86400) + " gun " + String(sn % 86400 / 3600) + " sa"; }
void ozetSayfasi() {
  baslik("KUBIK OZET");
  if (!ozet.var) { yaz(0, 16, 1, bagli ? "Kubit anahtari yok:" : "telefon bekleniyor"); if (bagli) yaz(0, 26, 1, "panel > Guvenlik"); return; }
  yaz(0, 13, 1, "Kup         " + String(ozet.kup));
  yaz(0, 23, 1, "Bugun mesaj " + String(ozet.mesaj));
  yaz(0, 33, 1, "Aktif       " + String(ozet.aktif));
  yaz(0, 43, 1, "Yeni kayit  " + String(ozet.yeni) + (ozet.isaret ? "  !" + String(ozet.isaret) : ""));
  yaz(0, 54, 1, "Pi acik " + (ozet.acik >= 0 ? sure(ozet.acik) : String("?")));
}

void bildirimSayfasi(Olay o1, Olay o2) {
  if (o2 == KISA && bildirimler.size()) bildirimler.erase(bildirimler.begin() + min(imlec, (int)bildirimler.size() - 1));
  if (bildirimler.empty()) { baslik("BILDIRIMLER"); yaz(0, 20, 1, "Bildirim yok"); yaz(0, 34, 1, "(uygulamada bildirim"); yaz(0, 44, 1, " erisimini ac)"); return; }
  if (o1 == KISA) imlec = (imlec + 1) % bildirimler.size();
  if (geriAdim) imlec = (imlec - 1 + bildirimler.size()) % bildirimler.size();
  imlec = min(imlec, (int)bildirimler.size() - 1);
  auto &b = bildirimler[imlec];
  baslik(b.app.substring(0, 14) + "  " + String(imlec + 1) + "/" + String(bildirimler.size()));
  yaz(0, 12, 1, b.baslik.substring(0, 21));
  int y = 23; for (auto &s : sar(b.metin, 21)) { if (y > 46) break; yaz(0, y, 1, s); y += 10; }
  yaz(0, 56, 1, "1:sonraki 2:sil");
}

void muzikSayfasi(Olay o1, Olay o2) {                // kayan şarkı adı, ilerleme, 7 düğme: önceki oynat sonraki beğen karıştır ses- ses+
  static int secim = 1;
  const char *K[7] = {"MUZ:onceki", "MUZ:oynat", "MUZ:sonraki", "MUZ:begen", "MUZ:karistir", "MUZ:ses-", "MUZ:ses+"};
  if (o1 == KISA) secim = (secim + 1) % 7;
  if (geriAdim) secim = (secim + 6) % 7;
  if (o2 == KISA) { telefonaGonder(K[secim], nullptr); if (secim == 1) { muzik.caliyor = !muzik.caliyor; muzik.zaman = millis(); } if (secim == 3) muzik.begen = !muzik.begen; }
  baslik(dl("MUZIK", "MUSIC"));
  if (!muzik.sarki.length()) { yaz(0, 16, 1, bagli ? dl("Calan muzik yok", "Nothing playing") : dl("telefon bekleniyor", "waiting for phone")); }
  else {
    String s = muzik.sarki; int kay = 0;
    if (s.length() > 21) { s = s + "   " + s; kay = (millis() / 180) % (muzik.sarki.length() + 3); }   // uzun ad kayar
    ekran.setTextWrap(false); yaz(-kay * 6, 13, 1, s); ekran.setTextWrap(true);
    yaz(0, 23, 1, muzik.sanatci.substring(0, 21));
    if (muzik.sure > 0) {
      int poz = min(muzik.sure, muzik.poz + (muzik.caliyor ? (int)((millis() - muzik.zaman) / 1000) : 0));
      ekran.drawFastHLine(0, 37, 128, W); ekran.fillRect(0, 36, 128 * poz / muzik.sure, 3, W); ekran.fillCircle(128 * poz / muzik.sure, 37, 2, W);
      yaz(0, 41, 1, String(poz / 60) + ":" + ikiHane(poz % 60)); String t2 = String(muzik.sure / 60) + ":" + ikiHane(muzik.sure % 60); yaz(128 - t2.length() * 6, 41, 1, t2);
    }
  }
  for (int i = 0; i < 7; i++) {                       // düğme şeridi
    int x = 9 + i * 18, y = 57;
    if (i == secim) ekran.fillRoundRect(x - 8, y - 7, 17, 14, 3, W);
    uint16_t r = i == secim ? B : W;
    switch (i) {
      case 0: ekran.fillTriangle(x + 3, y - 4, x + 3, y + 4, x - 3, y, r); ekran.fillRect(x - 5, y - 4, 2, 9, r); break;
      case 1: if (muzik.caliyor) { ekran.fillRect(x - 4, y - 4, 3, 9, r); ekran.fillRect(x + 1, y - 4, 3, 9, r); } else ekran.fillTriangle(x - 3, y - 5, x - 3, y + 5, x + 5, y, r); break;
      case 2: ekran.fillTriangle(x - 3, y - 4, x - 3, y + 4, x + 3, y, r); ekran.fillRect(x + 4, y - 4, 2, 9, r); break;
      case 3: if (muzik.begen) { ekran.fillCircle(x - 2, y - 1, 2, r); ekran.fillCircle(x + 2, y - 1, 2, r); ekran.fillTriangle(x - 4, y, x + 4, y, x, y + 4, r); }
              else { ekran.drawCircle(x - 2, y - 1, 2, r); ekran.drawCircle(x + 2, y - 1, 2, r); ekran.drawLine(x - 4, y, x, y + 4, r); ekran.drawLine(x + 4, y, x, y + 4, r); } break;
      case 4: ekran.drawLine(x - 5, y - 3, x + 4, y + 3, r); ekran.drawLine(x - 5, y + 3, x + 4, y - 3, r); if (muzik.karistir) ekran.fillCircle(x, y + 5, 1, r); break;
      case 5: ekran.fillRect(x - 5, y - 2, 3, 4, r); ekran.fillTriangle(x - 2, y - 2, x - 2, y + 1, x + 2, y - 5, r); ekran.fillTriangle(x - 2, y + 1, x + 2, y + 5, x + 2, y - 5, r); break;
      default: ekran.fillRect(x - 6, y - 2, 3, 4, r); ekran.fillTriangle(x - 3, y - 2, x - 3, y + 1, x + 1, y - 5, r); ekran.fillTriangle(x - 3, y + 1, x + 1, y + 5, x + 1, y - 5, r); ekran.drawLine(x + 3, y - 2, x + 3, y + 2, r); ekran.drawLine(x + 5, y - 3, x + 5, y + 3, r); break;
    }
  }
}

void bulSayfasi(Olay o2, int kare) {
  if (o2 == KISA) { buluyor = !buluyor; telefonaGonder(buluyor ? "BUL" : "BULDUR", nullptr); }
  baslik("TELEFONU BUL");
  int x = 64, y = 32;                                 // telefon simgesi ve dalgalar
  ekran.drawRoundRect(x - 8, y - 14, 16, 28, 3, W); ekran.drawFastHLine(x - 3, y + 10, 6, W);
  if (buluyor) for (int r = 0; r < 2; r++) { int rr = 14 + r * 6 + (kare % 6); ekran.drawCircleHelper(x, y, rr, 2 | 4, W); ekran.drawCircleHelper(x, y, rr, 1 | 8, W); }
  yaz(0, 56, 1, buluyor ? "caliyor  2:durdur" : "2: telefonu caldir");
}

#include "seviye.h"                                  // özel bölümler + editör (Geometry ve Süper Kübit kullanır)

// ---------------------------------------------------------------- Flappy
struct { float y, vy, hiz, boru[3]; int bosluk[3], skor, gecit; bool bitti, zor; } fb;
const int BORU_ARA = 48, BORU_EN = 10, KUS_X = 20;
const char *SKIN_AD[] = {"Kubit", "Kus", "Roket", "UFO"};
int fbSkin = 0;
const char *fbRekor() { return fb.zor ? "flappyz" : "flappy"; }
void flappyBasla() {                                 // zor mod: daha hızlı, geçit daha dar
  fb.y = 26; fb.vy = 0; fb.hiz = fb.zor ? 2.0 : 1.4; fb.gecit = fb.zor ? 18 : 22; fb.skor = 0; fb.bitti = false;
  for (int i = 0; i < 3; i++) { fb.boru[i] = 128 + i * BORU_ARA; fb.bosluk[i] = random(10, 64 - fb.gecit - 6); }
}
void miniKafa(int x, int y) {                        // 11x8 Kübit kafası
  int p[8][2] = {{2, 0}, {8, 0}, {10, 2}, {10, 5}, {8, 7}, {2, 7}, {0, 5}, {0, 2}};
  for (int i = 0; i < 8; i++) ekran.drawLine(x + p[i][0], y + p[i][1], x + p[(i + 1) % 8][0], y + p[(i + 1) % 8][1], W);
  ekran.drawPixel(x + 3, y + 3, W); ekran.drawPixel(x + 7, y + 3, W); ekran.drawLine(x + 4, y + 5, x + 6, y + 5, W);
}
void skinCiz(int x, int y) {                          // 11x8 alan, çarpışma kutusu hep aynı
  bool kanat = (millis() / 120) % 2;
  switch (fbSkin) {
    case 1: ekran.fillCircle(x + 5, y + 4, 3, W); ekran.drawPixel(x + 6, y + 3, B); ekran.fillTriangle(x + 8, y + 3, x + 10, y + 4, x + 8, y + 5, W);
            ekran.drawLine(x + 1, y + 4, x + 4, y + (kanat ? 1 : 7), W); ekran.drawLine(x, y + 4, x + 3, y + (kanat ? 1 : 7), W); break;
    case 2: ekran.fillRoundRect(x + 2, y + 2, 7, 4, 1, W); ekran.fillTriangle(x + 9, y + 2, x + 10, y + 4, x + 9, y + 5, W); ekran.drawPixel(x + 6, y + 3, B);
            ekran.drawLine(x + 2, y + 2, x, y, W); ekran.drawLine(x + 2, y + 5, x, y + 7, W); if (kanat) ekran.drawFastHLine(x - 2, y + 3, 2, W); else ekran.drawFastHLine(x - 3, y + 4, 3, W); break;
    case 3: ekran.drawCircleHelper(x + 5, y + 4, 3, 1 | 2, W); ekran.fillRoundRect(x, y + 4, 11, 3, 1, W);
            ekran.drawPixel(x + (kanat ? 2 : 8), y + 5, B); ekran.drawPixel(x + 5, y + 5, B); break;
    default: miniKafa(x, y);
  }
}
void flappySecSayfasi(Olay o1, Olay o2) {
  String og[3] = {"Normal   rekor " + String(kayit.getInt("flappy", 0)), "Zor      rekor " + String(kayit.getInt("flappyz", 0)), "Skin: " + String(SKIN_AD[fbSkin])};
  int s = liste("FLAPPY", og, 3, o1, o2);
  if (s == 2) { fbSkin = (fbSkin + 1) % 4; kayit.putInt("fbskin", fbSkin); }
  else if (s >= 0) { fb.zor = s == 1; flappyBasla(); sayfa = FLAPPY; }
  if (imlec == 2) skinCiz(110, 33);
}
void oyunBitti(const String &s1, const String &s2) {
  ekran.fillRect(8, 14, 112, 38, B); ekran.drawRect(8, 14, 112, 38, W);
  yaz(13, 18, 1, s1); yaz(13, 29, 1, s2); yaz(13, 41, 1, "1:tekrar  2:cik");
}
void flappyKare(bool bastiMi) {
  if (!fb.bitti) {
    if (bastiMi) fb.vy = -2.8;
    fb.vy += 0.28; fb.y += fb.vy;
    for (int i = 0; i < 3; i++) {
      float once = fb.boru[i];
      fb.boru[i] -= fb.hiz;
      if (once + BORU_EN >= KUS_X && fb.boru[i] + BORU_EN < KUS_X) { fb.skor++; fb.hiz += 0.04; }
      if (fb.boru[i] < -BORU_EN) { fb.boru[i] += 3 * BORU_ARA; fb.bosluk[i] = random(10, 64 - fb.gecit - 6); }
      bool hizada = fb.boru[i] < KUS_X + 10 && fb.boru[i] + BORU_EN > KUS_X;
      if (hizada && (fb.y < fb.bosluk[i] || fb.y + 7 > fb.bosluk[i] + fb.gecit)) fb.bitti = true;
    }
    if (fb.y < 0 || fb.y + 7 > 63) fb.bitti = true;
    if (fb.bitti && fb.skor > kayit.getInt(fbRekor(), 0)) kayit.putInt(fbRekor(), fb.skor);
  } else if (bastiMi) { flappyBasla(); return; }
  for (int i = 0; i < 3; i++) {
    int x = fb.boru[i], g = fb.bosluk[i];
    ekran.fillRect(x, 0, BORU_EN, g, W); ekran.fillRect(x - 1, g - 3, BORU_EN + 2, 3, W);
    ekran.fillRect(x, g + fb.gecit, BORU_EN, 64 - g - fb.gecit, W); ekran.fillRect(x - 1, g + fb.gecit, BORU_EN + 2, 3, W);
  }
  skinCiz(KUS_X, (int)fb.y);
  ekran.fillRect(58, 0, 14, 9, B); yaz(60, 1, 1, String(fb.skor));
  if (fb.bitti) oyunBitti("Skor " + String(fb.skor), "Rekor " + String(kayit.getInt(fbRekor(), 0)));
}

// ---------------------------------------------------------------- Geometry
// Harita karoları: ^ diken, # blok, H iki kat blok, p zıplama pedi (yüksek fırlatır),
// o alçak küre, O yüksek küre (havada küreye değerken Tuş 1 = bir zıplama daha), x dikenin üstünde alçak küre, E bitiş.
const Seviye SEVIYELER[] = {
  {"Baslangic", "..........^........^.........^^........#......#.....^.....##......^^......#.....^......##....^^......E", 2.3},
  {"Ziplama",   "........p.........p^^.......p..H.......^...p.^^^.......#..p.H.....p^^^^......##..p..HH......p^.p^.....E", 2.3},
  {"Kureler",   "........^^x^^........^^^x^^^.......o.....x......^^x^^x^^.......#^^x^^#.......^^^x^^^x^^^.......E", 2.4},
  {"Bloklar",   "........#.....##.....###.....#.#.#.....###^^###......##^^##.....#^#^#.....####......#..##..###......##p.H.....E", 2.4},
  {"Merdiven",  "........#H......#H#.....H..H.....#H^^H#......##HH##.....H^^^H.......#H#H#......p.HH^^HH.....#HO^^H......E", 2.5},
  {"Kabus",     ".....^^^....#^^^#....H^^^^H....p^^^^^....^^x^^x^^....##^^^##^^^##.....H#^^^#H....p.H^^^O^^^H.....^^^.^^^.^^^....E", 2.6}};
const int SEVIYE_N = 6, TILE = 8, ZEMIN = 56, KUP_X = 24;
struct { float x, y, vy, aci; bool yerde, bitti, kazandi; int sonKure; } gd;
const char *harita() { return gdSeviye == 6 ? ozelGd : SEVIYELER[gdSeviye].harita; }
float gdHiz() { return gdSeviye == 6 ? 2.4f : SEVIYELER[gdSeviye].hiz; }
int haritaBoyu() { return strlen(harita()); }
char karo(int i) { return i < 0 || i >= haritaBoyu() ? '.' : harita()[i]; }
int karoYuksek(char t) { return t == '#' ? TILE : t == 'H' ? 2 * TILE : 0; }
bool diken(char t) { return t == '^' || t == 'x'; }
int kureY(char t) { return t == 'o' || t == 'x' ? ZEMIN - 22 : t == 'O' ? ZEMIN - 34 : -1; }
String rekorAnahtar() { return "gd" + String(gdSeviye); }
void gdBasla() { gd.x = 0; gd.y = ZEMIN - TILE; gd.vy = 0; gd.aci = 0; gd.yerde = true; gd.bitti = gd.kazandi = false; gd.sonKure = -1; }

void editorAc(char tur);
void gdSecSayfasi(Olay o1, Olay o2) {
  String ogeler[SEVIYE_N + 3];
  int secili = gdSeviye;
  for (int i = 0; i < SEVIYE_N; i++) { gdSeviye = i; ogeler[i] = String(i + 1) + " " + SEVIYELER[i].ad + "  %" + String(kayit.getInt(rekorAnahtar().c_str(), 0)); }
  gdSeviye = secili;
  ogeler[SEVIYE_N] = dl("* Ozel bolum", "* Custom level"); ogeler[SEVIYE_N + 1] = dl("* Duzenleyici", "* Editor"); ogeler[SEVIYE_N + 2] = dl("* Telefona gonder", "* Send to phone");
  int s = liste("GEOMETRY  seviye sec", ogeler, SEVIYE_N + 3, o1, o2);
  if (s >= 0 && s <= SEVIYE_N) { gdSeviye = s; gdBasla(); sayfa = GD; }
  if (s == SEVIYE_N + 1) { editorAc('g'); sayfa = GD_ED; }
  if (s == SEVIYE_N + 2) seviyeGonder('g');
  altMesaj();
}

void gdKare(bool basili, bool bastiMi) {
  int boy = haritaBoyu() * TILE;
  if (gd.bitti) { if (bastiMi) { gdBasla(); return; } }
  else {
    float cx = gd.x + 3.5, cy = gd.y + 3.5;
    bool kureyeBasti = false;
    for (int i = (int)(gd.x / TILE) - 1; i <= (int)(gd.x / TILE) + 1; i++) {   // küre: havada değerken basış
      int ky = kureY(karo(i));
      if (ky >= 0 && i > gd.sonKure && bastiMi && abs(cx - (i * TILE + 3.5)) < 8 && abs(cy - ky) < 9) { gd.vy = -4.8; gd.yerde = false; gd.sonKure = i; kureyeBasti = true; }
    }
    if (!kureyeBasti && basili && gd.yerde) { gd.vy = -4.6; gd.yerde = false; }   // basılı tutunca her inişte zıplar
    float onceAlt = gd.y + TILE;
    gd.vy += 0.55; gd.y += gd.vy; gd.x += gdHiz();
    gd.yerde = false;
    if (gd.y + TILE >= ZEMIN) { gd.y = ZEMIN - TILE; gd.vy = 0; gd.yerde = true; }
    for (int i = (int)((gd.x + 1) / TILE); i <= (int)((gd.x + TILE - 1) / TILE); i++) {
      char t = karo(i);
      if (int h = karoYuksek(t)) {
        float ust = ZEMIN - h;
        if (onceAlt <= ust + 0.5 && gd.y + TILE >= ust && gd.vy >= 0) { gd.y = ust - TILE; gd.vy = 0; gd.yerde = true; }
        else if (gd.y + TILE > ust + 0.5) gd.bitti = true;
      } else if (diken(t)) {
        float sx = i * TILE;
        if (gd.x + TILE - 1 > sx + 2 && gd.x + 1 < sx + 6 && gd.y + TILE > ZEMIN - 5) gd.bitti = true;
      } else if (t == 'p' && gd.y + TILE >= ZEMIN - 0.5) { gd.vy = -6.3; gd.yerde = false; }   // ped: yüksek fırlatır
      else if (t == 'E') gd.bitti = gd.kazandi = true;
    }
    if (gd.yerde) gd.aci = round(gd.aci / HALF_PI) * HALF_PI; else gd.aci += 0.22;
    if (gd.bitti) { int y = gd.kazandi ? 100 : (int)(100 * gd.x / boy); if (y > kayit.getInt(rekorAnahtar().c_str(), 0)) kayit.putInt(rekorAnahtar().c_str(), y); }
  }
  ekran.drawFastHLine(0, ZEMIN, 128, W);
  for (int i = (int)((gd.x - KUP_X) / TILE) - 1; i <= (int)((gd.x + 128) / TILE); i++) {
    int sx = i * TILE - (int)gd.x + KUP_X;
    char t = karo(i);
    if (diken(t)) ekran.fillTriangle(sx, ZEMIN, sx + 4, ZEMIN - TILE, sx + 7, ZEMIN, W);
    else if (int h = karoYuksek(t)) for (int k = 0; k < h / TILE; k++) { int y = ZEMIN - (k + 1) * TILE; ekran.drawRect(sx, y, TILE, TILE, W); ekran.drawLine(sx, y, sx + 7, y + 7, W); }
    else if (t == 'p') { ekran.fillRoundRect(sx, ZEMIN - 2, 8, 3, 1, W); ekran.drawPixel(sx + 2, ZEMIN - 4, W); ekran.drawPixel(sx + 5, ZEMIN - 4, W); }
    else if (t == 'E') for (int y = 10; y < ZEMIN; y += 4) ekran.drawFastVLine(sx, y, 2, W);
    int ky = kureY(t);
    if (ky >= 0 && i > gd.sonKure) { ekran.drawCircle(sx + 3, ky, 3 + (millis() / 200) % 2, W); ekran.fillCircle(sx + 3, ky, 1, W); }
  }
  float cx = KUP_X + 3.5, cy = gd.y + 3.5;           // dönen küp
  int kx[4], ky[4];
  for (int k = 0; k < 4; k++) { float a = gd.aci + k * HALF_PI + PI / 4; kx[k] = cx + 5.0 * cos(a); ky[k] = cy + 5.0 * sin(a); }
  for (int k = 0; k < 4; k++) ekran.drawLine(kx[k], ky[k], kx[(k + 1) % 4], ky[(k + 1) % 4], W);
  ekran.drawPixel(cx - 1, cy, W); ekran.drawPixel(cx + 2, cy, W);
  int yuzde = min(100, (int)(100 * gd.x / boy));
  ekran.drawRect(0, 0, 100, 4, W); ekran.fillRect(0, 0, yuzde, 4, W);
  yaz(104, 0, 1, String(yuzde) + "%");
  if (gd.bitti) oyunBitti(gd.kazandi ? "TAMAMLANDI!" : "Oldun! %" + String(yuzde), String(gdSeviye + 1) + ". seviye rekor %" + String(kayit.getInt(rekorAnahtar().c_str(), 0)));
}

// ---------------------------------------------------------------- Kübit Racer: OutRun tarzı sahte 3B (arayuz.html'deki racer'ın aynısı)
// Kartta Tuş 2 solda, Tuş 1 sağda: Tuş 2 sola, Tuş 1 sağa, ikisi birden turbo (bar 20 sn'de dolar, 4 sn 3 kat hız).
// İki tuşu 4 sn basılı tutmak yarıştan çıkar.
const int YV = 96, UFUK = 24, RSEG = 200, RGENIS = 2000, KAMERA_Y = 1000, GORUS = 70, TUR_SAYISI = 3;
const float DERIN = 1 / tan(50 * PI / 180), OYUNCU_Z = KAMERA_Y * DERIN, TURBO_DOLMA = 20, TURBO_SURE = 4, TURBO_KAT = 3;
const Araba ARABALAR[3] = {{"KUBIT GT", 1.00, 1.00, 1.00}, {"ROKET", 1.10, 0.85, 0.75}, {"KUTU", 0.93, 1.25, 1.35}};
const char *PIST_AD[3] = {"Sahil", "Dag Yolu", "Gece Sehir"};
std::vector<RSeg> pist;
float pistSonY = 0;
struct { int pist = 0, araba = 0, mod = 0, tur, sira; float z, x, hiz, maks, ufuk, sure, sayim, turbo, turboKalan, uzun, turSure, enIyiTur; bool bitti; Bot bot[5]; uint32_t son; } rc;
// Hayalet: en iyi turda 0.2 sn'de bir konum (z/4, x*50). 450 örnek = 90 sn'lik tur; NVS'de pist başına 1350 bayt.
const int HAYALET_N = 450; const float HAYALET_ADIM = 0.2;
struct HOrnek { uint16_t z; int8_t x; } __attribute__((packed));
HOrnek *hyKayit = nullptr, *hyEski = nullptr; int hyKayitN = 0, hyEskiN = 0;
const char *RC_MOD_AD[3] = {"Yaris (5 bot)", "Zamana karsi", "Hayalet araba"};
String rcRekorAnahtar(int p) { return String(rc.mod == 0 ? "rc" : rc.mod == 1 ? "rt" : "rl") + String(p); }   // rl: en iyi tur

float kolayF(float a, float b, float p) { return a + (b - a) * p * p; }
float yumusakF(float a, float b, float p) { return a + (b - a) * ((-cos(p * PI) / 2) + 0.5); }
void yolEkle(int giris, int tut, int cikis, float viraj, float tepe) {   // viraj: + sağ, - sol; tepe: bölüm cinsinden yükseklik
  float bas = pistSonY, son = bas + tepe * RSEG; int top = giris + tut + cikis;
  for (int n = 0; n < top; n++) {
    float v = n < giris ? kolayF(0, viraj, (float)n / giris) : n < giris + tut ? viraj : yumusakF(viraj, 0, (float)(n - giris - tut) / cikis);
    float y = yumusakF(bas, son, (float)(n + 1) / top);
    RSeg s = {v, pistSonY, y, {}, 0}; pist.push_back(s); pistSonY = y;
  }
}
void pDuz(int n) { yolEkle(n, n, n, 0, 0); }
void pViraj(float v, int n, float t) { yolEkle(n, n, n, v, t); }
void pTepe(float t, int n) { yolEkle(n, n, n, 0, t); }
void susEkle(int i, SusTur tur, float x, int8_t yon) { RSeg &s = pist[i]; if (s.n < 2) s.sus[s.n++] = {tur, x, yon}; }

void pistKur(int k) {
  pist.clear(); pist.reserve(950); pistSonY = 0;
  if (k == 0) { pDuz(25); pViraj(2, 25, 0); pDuz(15); pTepe(20, 25); pViraj(-3, 30, 0); pDuz(25); pViraj(3, 20, -15); pViraj(-2, 25, 0); pDuz(20); pTepe(-20, 25); pViraj(2, 30, 0); pDuz(25); }
  if (k == 1) { pDuz(15); pTepe(40, 25); pViraj(-5, 20, 20); pViraj(5, 20, -30); pTepe(-30, 20); pViraj(-4, 15, 0); pViraj(4, 15, 30); pDuz(10); pViraj(-6, 25, -30); pTepe(20, 25); pDuz(15); }
  if (k == 2) { pDuz(30); pViraj(6, 15, 0); pDuz(20); pViraj(-6, 15, 0); pDuz(15); pViraj(6, 15, 0); pTepe(10, 15); pViraj(-6, 15, 0); pDuz(25); pViraj(4, 25, 0); pTepe(-10, 15); pDuz(20); }
  pist.back().y2 = pist[0].y1;                       // tur kapanır
  int N = pist.size();
  for (int i = 10; i < N - 10; i++) {                // kenar süsleri
    if (k == 2) { if (i % 6 == 0) { susEkle(i, BINA, -1.9 - (i % 12 ? .3 : 0), 0); susEkle(i, BINA, 1.9 + (i % 18 ? .2 : .5), 0); }
                  else if (i % 12 == 3) susEkle(i, LAMBA, i % 24 == 3 ? -1.15 : 1.15, 0); }
    else if (i % 5 == 0) susEkle(i, k == 1 ? CAM : PALMIYE, (i % 10 ? -1 : 1) * (1.4 + (i * 7 % 5) / 4.0), 0);
    if (i + 20 < N && fabs(pist[i + 20].viraj) > 2 && i % 40 == 0) susEkle(i, TABELA, pist[i + 20].viraj > 0 ? -1.3 : 1.3, pist[i + 20].viraj > 0 ? 1 : -1);
  }
  rc.uzun = N * (float)RSEG;
}

void yarisBasla() {
  pistKur(rc.pist);
  const Araba &a = ARABALAR[rc.araba];
  rc.maks = RSEG * 22 * a.hiz; rc.z = rc.x = rc.hiz = rc.ufuk = rc.sure = rc.turbo = rc.turboKalan = 0;
  rc.tur = 0; rc.sira = rc.mod ? 1 : 6; rc.sayim = 3; rc.bitti = false; rc.son = millis(); rc.turSure = 0; rc.enIyiTur = 0;
  if (rc.mod == 2) {
    if (!hyKayit) { hyKayit = new HOrnek[HAYALET_N]; hyEski = new HOrnek[HAYALET_N]; }
    hyKayitN = 0; String k = "rg" + String(rc.pist);
    hyEskiN = kayit.getBytes(k.c_str(), hyEski, sizeof(HOrnek) * HAYALET_N) / sizeof(HOrnek);
  }
  const float botX[5] = {-.5, .5, 0, -.6, .6};
  for (int i = 0; i < 5; i++) rc.bot[i] = {(i + 1) * RSEG * 6.0f, botX[i], RSEG * 22 * (0.80f + i * 0.03f), 0, (uint8_t)(i % 3)};
}

Proj proje(float wx, float wy, float wz, float kx, float ky, float kz) {
  Proj p; p.cz = wz - kz; p.s = DERIN / p.cz;
  p.x = YV / 2 + p.s * (wx - kx) * YV / 2; p.y = UFUK - p.s * (wy - ky) * 32; p.w = p.s * RGENIS * YV / 2;
  return p;
}
void serit(float yUst, float yAlt, float xUst, float xAlt, float wUst, float wAlt, uint8_t desen, float kirp) {   // desen: 0 sil, 1 dolu, 2 seyrek, 3 dama
  for (int y = max(0, (int)ceil(yUst)); y <= min(63, (int)floor(yAlt)) && y < kirp; y++) {
    float p = yAlt == yUst ? 1 : (y - yUst) / (yAlt - yUst), c = xUst + (xAlt - xUst) * p, w = wUst + (wAlt - wUst) * p;
    for (int x = max(0, (int)lround(c - w)); x <= min(YV - 1, (int)lround(c + w)); x++)
      ekran.drawPixel(x, y, desen == 1 || (desen == 2 && (x + y * 2) % 4 == 0) || (desen == 3 && (x + y) % 2) ? W : B);
  }
}
void rNokta(float x, float y, float kirp) { int xi = lround(x), yi = lround(y); if (yi < kirp && xi >= 0 && xi < YV) ekran.drawPixel(xi, yi, W); }
void rSil(float x, float y, float w, float h, float kirp) {
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) { int xx = lround(x + i), yy = lround(y + j); if (xx >= 0 && xx < YV && yy >= 0 && yy < 64 && yy < kirp) ekran.drawPixel(xx, yy, B); }
}
void susCiz(const Sus &sp, float sx, float sy, float olcek, float kirp) {   // olcek: dünya -> piksel
  float w = max(1.0f, olcek * 700), h = w * 1.3;
  switch (sp.tur) {
    case CAM: for (int j = 0; j < h; j++) { float g = (w / 2) * (j / h) * (1 - fmod(j / h * 3, 1.0f) * .35); for (float i = -g; i <= g; i++) rNokta(sx + i, sy - h + j, kirp); } break;
    case PALMIYE: for (int j = 0; j < h; j++) rNokta(sx + sin(j / h * 2) * w * .15, sy - j, kirp);
                  for (int k = -2; k <= 2; k++) for (int i = 0; i < w * .45; i++) rNokta(sx + k * i * .45, sy - h + i * i / (w * .45) * .6, kirp); break;
    case TABELA: { float tw = w * .9, th = w * .55; rSil(sx - tw / 2, sy - h, tw, th, kirp);
      for (int i = 0; i < tw; i++) { rNokta(sx - tw / 2 + i, sy - h, kirp); rNokta(sx - tw / 2 + i, sy - h + th, kirp); }
      for (int j = 0; j < th; j++) { rNokta(sx - tw / 2, sy - h + j, kirp); rNokta(sx + tw / 2, sy - h + j, kirp); }
      for (int j = 0; j < h - th; j++) rNokta(sx, sy - j, kirp);
      for (int i = 0; i < th * .35; i++) { float ax = sx + sp.yon * (tw * .2 - i); rNokta(ax, sy - h + th / 2 - i * .8, kirp); rNokta(ax, sy - h + th / 2 + i * .8, kirp); } } break;
    case BINA: { float bw = w * 1.4, bh = w * 2.6; rSil(sx - bw / 2, sy - bh, bw, bh, kirp);
      for (int i = 0; i < bw; i++) rNokta(sx - bw / 2 + i, sy - bh, kirp);
      for (int j = 0; j < bh; j++) { rNokta(sx - bw / 2, sy - bh + j, kirp); rNokta(sx + bw / 2, sy - bh + j, kirp); }
      if (w > 6) for (float j = bh * .15; j < bh * .9; j += max(2.0f, w * .35f)) for (float i = -bw * .3; i <= bw * .3; i += max(2.0f, w * .3f)) rNokta(sx + i, sy - bh + j, kirp); } break;
    case LAMBA: for (int j = 0; j < h * 1.2; j++) rNokta(sx, sy - j, kirp); for (int i = 0; i < w * .3; i++) rNokta(sx - (sp.x > 0 ? 1 : -1) * i, sy - h * 1.2, kirp); break;
  }
}
void arabaCiz(int tip, float sx, float sy, float w, float kirp, float egim) {   // arkadan görünüş: 0 GT, 1 Roket (alçak, kanatlı), 2 Kutu (yüksek)
  const float oran[3] = {0.45, 0.35, 0.6}, cati[3] = {0.55, 0.45, 0.85};
  float h = w * oran[tip];
  auto satir = [&](float y, float x0, float x1, bool beyaz) { int yi = lround(y); if (yi < 0 || yi >= 64 || yi >= kirp) return;
    for (int x = lround(x0); x <= lround(x1); x++) if (x >= 0 && x < YV) ekran.drawPixel(x, yi, beyaz ? W : B); };
  for (int j = 0; j < h; j++) { float k = egim * j / h; satir(sy - j, sx - w / 2 + k, sx + w / 2 + k, false); rNokta(sx - w / 2 + k, sy - j, kirp); rNokta(sx + w / 2 + k, sy - j, kirp); }
  float tavan = sy - h;
  for (float i = -w / 2; i <= w / 2; i++) rNokta(sx + i + egim, tavan, kirp);
  for (float i = -w * cati[tip] / 2; i <= w * cati[tip] / 2; i++) rNokta(sx + i + egim, tavan - h * .45, kirp);
  for (int j = 0; j < h * .45; j++) { rNokta(sx - w * cati[tip] / 2 + egim - j * .3, tavan - j, kirp); rNokta(sx + w * cati[tip] / 2 + egim + j * .3, tavan - j, kirp); }
  if (w >= 8) { satir(sy - h * .4, sx - w / 2 + 1, sx - w / 2 + w * .22, true); satir(sy - h * .4, sx + w / 2 - w * .22, sx + w / 2 - 1, true); }   // stop lambaları
  satir(sy, sx - w / 2 - 1, sx - w / 2 + w * .2, true); satir(sy, sx + w / 2 - w * .2, sx + w / 2 + 1, true);                                   // tekerlekler
  if (tip == 1 && w >= 8) satir(tavan - h * .5, sx - w / 2, sx + w / 2, true);                                                                    // roket: kanat
}

bool hayaletKonum(float &z, float &x) {             // bu turun süresinde hayaletin nerede olduğu (örnekler arası doğrusal)
  if (rc.mod != 2 || rc.sayim > 0) return false;
  float f = rc.turSure / HAYALET_ADIM; int i = f;
  if (i + 1 >= hyEskiN) return false;
  f -= i; z = (hyEski[i].z + (hyEski[i + 1].z - hyEski[i].z) * f) * 4; x = (hyEski[i].x + (hyEski[i + 1].x - hyEski[i].x) * f) / 50.0;
  return z > rc.z + OYUNCU_Z * .5;                    // hemen arkamızdakini çizme
}
void yarisCiz() {
  int N = pist.size();
  const RSeg &taban = pist[(int)(rc.z / RSEG) % N];
  int tabanI = (int)(rc.z / RSEG) % N;
  float yuzde = fmod(rc.z, (float)RSEG) / RSEG, oz = rc.z + OYUNCU_Z;
  const RSeg &os = pist[(int)(oz / RSEG) % N];
  float oy = os.y1 + (os.y2 - os.y1) * (fmod(oz, (float)RSEG) / RSEG);
  for (int x = 0; x < YV; x++) { int h = lround(5 + 3 * sin((x + rc.ufuk) / 9) + 2 * sin((x + rc.ufuk) / 4)); ekran.drawPixel(x, UFUK - h, W); if (x % 2 == 0) ekran.drawPixel(x, UFUK - h + 1, W); }   // dağ silueti
  if (rc.pist == 2) for (int i = 0; i < 12; i++) ekran.drawPixel((int)fmod(fmod(i * 37 + rc.ufuk * .5, YV) + YV, YV), (i * 13) % 14, W);   // yıldızlar
  struct Gorunen { int i; Proj p1, p2; float kirp; } g[GORUS];
  int gn = 0; float maksy = 64, x = 0, dx = -(taban.viraj * yuzde);
  for (int n = 0; n < GORUS; n++) {
    int i = (tabanI + n) % N; const RSeg &s = pist[i];
    float kz = rc.z - (i < tabanI ? rc.uzun : 0);
    Proj p1 = proje(0, s.y1, i * (float)RSEG, rc.x * RGENIS - x, oy + KAMERA_Y, kz), p2 = proje(0, s.y2, (i + 1) * (float)RSEG, rc.x * RGENIS - x - dx, oy + KAMERA_Y, kz);
    x += dx; dx += s.viraj;
    if (p1.cz <= DERIN || p2.y >= p1.y || p2.y >= maksy) continue;
    g[gn++] = {i, p1, p2, maksy}; maksy = p2.y;
  }
  for (int k = gn - 1; k >= 0; k--) {                // uzaktan yakına: yakın olan üstüne çizilir
    auto &v = g[k]; bool acik = (v.i / 3) % 2;
    serit(v.p2.y, v.p1.y, YV / 2, YV / 2, YV, YV, acik ? 3 : 2, v.kirp);                                       // çimen: dama/seyrek sırayla
    serit(v.p2.y, v.p1.y, v.p2.x, v.p1.x, v.p2.w * 1.15, v.p1.w * 1.15, acik ? 1 : 0, v.kirp);                  // bordür
    serit(v.p2.y, v.p1.y, v.p2.x, v.p1.x, v.p2.w, v.p1.w, 0, v.kirp);                                           // asfalt
    if (acik) serit(v.p2.y, v.p1.y, v.p2.x, v.p1.x, max(.4f, v.p2.w * .03f), max(.4f, v.p1.w * .03f), 1, v.kirp);   // orta çizgi
  }
  for (int k = gn - 1; k >= 0; k--) {                // süsler ve botlar
    auto &v = g[k]; const RSeg &s = pist[v.i]; float olcek = v.p1.s * YV / 2;
    for (int j = 0; j < s.n; j++) susCiz(s.sus[j], v.p1.x + olcek * s.sus[j].x * RGENIS, v.p1.y, olcek, v.kirp);
    if (rc.mod == 0) for (auto &b : rc.bot) if ((int)(b.z / RSEG) % N == v.i) arabaCiz(b.cizim, v.p1.x + olcek * b.x * RGENIS, v.p1.y, max(2.0f, olcek * 520), v.kirp, 0);
    float hz, hx;
    if (hayaletKonum(hz, hx) && (int)(hz / RSEG) % N == v.i && (millis() / 70) % 2) arabaCiz(rc.araba, v.p1.x + olcek * hx * RGENIS, v.p1.y, max(2.0f, olcek * 520), v.kirp, 0);   // yanıp sönen hayalet
  }
  float egim = (tus2.basili ? -2 : 0) + (tus1.basili ? 2 : 0);
  arabaCiz(rc.araba, YV / 2, 60 + (rc.hiz > 0 && (int)(rc.sure * 20) % 2 ? 1 : 0), 26, 64, egim);
  if (rc.turboKalan > 0) {                           // turbo: egzoz alevi + kenardan hız çizgileri
    int f = (int)(rc.sure * 30) % 2;
    for (int ex : {YV / 2 - 7, YV / 2 + 7}) ekran.fillTriangle(ex - 2, 61, ex + 2, 61, ex, 64 + (f ? 1 : 3), W);
    for (int i = 0; i < 6; i++) { int y = 30 + ((i * 11 + (int)(rc.sure * 90)) % 34), u = 4 + (y - 30) / 3; ekran.drawFastHLine(2, y, u, W); ekran.drawFastHLine(YV - 3 - u, y, u, W); }
  }
}

void yarisHud() {                                    // sağ panel: tur, sıra, süre, hız, turbo
  ekran.drawFastVLine(YV, 0, 64, W);
  yaz(99, 0, 1, "TUR"); yaz(99, 9, 1, String(min(rc.tur + 1, TUR_SAYISI)) + "/" + String(TUR_SAYISI));
  if (rc.mod == 0) { yaz(100, 21, 2, String(rc.sira) + "."); yaz(99, 38, 1, "/6"); }
  else { yaz(99, 20, 1, rc.mod == 2 ? "HAYAL" : "TUR"); yaz(99, 29, 1, String(rc.turSure, 1)); if (rc.enIyiTur) yaz(99, 38, 1, String(rc.enIyiTur, 1)); }
  yaz(99, 45, 1, String((int)rc.sure / 60) + ":" + ikiHane((int)rc.sure % 60));
  ekran.drawRect(99, 54, 28, 4, W); ekran.fillRect(100, 55, min(26, (int)lround(26 * rc.hiz / (rc.maks * TURBO_KAT))), 2, W);
  ekran.drawRect(99, 59, 28, 5, W);                  // turbo barı: dolarken dama, turboda dolu, hazırken yanıp söner
  if (rc.turboKalan > 0 || rc.turbo < 1) { int w = lround(26 * rc.turbo); for (int y = 60; y < 63; y++) for (int x = 100; x < 100 + w; x++) if (rc.turboKalan > 0 || (x + y) % 2) ekran.drawPixel(x, y, W); }
  else if ((millis() / 250) % 2) ekran.fillRect(100, 60, 26, 3, W);
}

void yarisGuncelle(float dt) {
  if (rc.sayim > 0) { rc.sayim -= dt; return; }
  if (rc.bitti) return;
  const Araba &a = ARABALAR[rc.araba]; int N = pist.size();
  bool ikisi = tus1.basili && tus2.basili, sol = tus2.basili && !ikisi, sag = tus1.basili && !ikisi;
  if (rc.turboKalan > 0) { rc.turboKalan -= dt; rc.turbo = max(0.0f, rc.turboKalan / TURBO_SURE); }
  else { rc.turbo = min(1.0f, rc.turbo + dt / TURBO_DOLMA); if (ikisi && rc.turbo >= 1) rc.turboKalan = TURBO_SURE; }
  float ust = rc.maks * (rc.turboKalan > 0 ? TURBO_KAT : 1);
  const RSeg &seg = pist[(int)((rc.z + OYUNCU_Z) / RSEG) % N];
  float oran = rc.hiz / rc.maks;
  rc.sure += dt; rc.turSure += dt;
  if (rc.mod == 2) while (hyKayitN < HAYALET_N && rc.turSure >= hyKayitN * HAYALET_ADIM) { hyKayit[hyKayitN++] = {(uint16_t)(rc.z / 4), (int8_t)constrain(lround(rc.x * 50), -127L, 127L)}; }
  rc.z += rc.hiz * dt;
  if (rc.z >= rc.uzun) {
    rc.z -= rc.uzun; rc.tur++;
    if (!rc.enIyiTur || rc.turSure < rc.enIyiTur) rc.enIyiTur = rc.turSure;
    if (rc.mod == 2) {                                 // tur rekoru: hayaleti bu turla değiştir
      String k = "rl" + String(rc.pist); int ds = lround(rc.turSure * 10), eski = kayit.getInt(k.c_str(), 0);
      if ((!eski || ds < eski) && hyKayitN < HAYALET_N) {
        kayit.putInt(k.c_str(), ds); kayit.putBytes(("rg" + String(rc.pist)).c_str(), hyKayit, sizeof(HOrnek) * hyKayitN);
        memcpy(hyEski, hyKayit, sizeof(HOrnek) * hyKayitN); hyEskiN = hyKayitN;
      }
      hyKayitN = 0;
    }
    rc.turSure = 0;
    if (rc.tur >= TUR_SAYISI) rc.bitti = true;
  }
  float dx = dt * 2.2 * min(1.0f, oran);
  if (sol) rc.x -= dx; if (sag) rc.x += dx;
  rc.x -= dx * min(1.0f, oran) * seg.viraj * 0.32 / a.tutus;   // virajda dışa savrulma
  rc.ufuk += seg.viraj * min(1.0f, oran) * 0.8;
  rc.hiz += (rc.turboKalan > 0 ? rc.maks * 2.5 : rc.maks / 4 * a.ivme) * dt;   // kendiliğinden hızlanır, turboda sertçe
  if (rc.hiz > ust) rc.hiz = max(ust, rc.hiz - rc.maks * 0.8f * dt);           // turbo bitince yavaşça normale iner
  if (fabs(rc.x) > 1 && rc.hiz > rc.maks / 4) rc.hiz -= rc.maks * 0.9 * dt;    // yol dışı yavaşlatır
  rc.x = constrain(rc.x, -2.2f, 2.2f); rc.hiz = max(0.0f, rc.hiz);
  if (fabs(rc.x) > 1.3) for (int j = 0; j < seg.n; j++) if (fabs(rc.x - seg.sus[j].x) < .45) { rc.hiz = rc.maks / 6; rc.x = seg.sus[j].x > 0 ? 1 : -1; }   // süse çarpma
  float benim = rc.tur * rc.uzun + rc.z + OYUNCU_Z;
  if (rc.mod == 0) for (auto &b : rc.bot) {
    b.z += b.hiz * dt; if (b.z >= rc.uzun) { b.z -= rc.uzun; b.tur++; }
    b.x -= pist[(int)(b.z / RSEG) % N].viraj * 0.0015;   // botlar virajı yumuşak alır
    float fark = b.tur * rc.uzun + b.z - benim;
    if (fark > 0 && fark < RSEG * 8 && fabs(b.x - rc.x) < .45) b.x += (b.x > rc.x ? 1 : -1) * dt * .8;   // önündeki bot kenara kaçar
    b.x = constrain(b.x, -.8f, .8f);
    if (fabs(fark) < RSEG * .8 && fabs(b.x - rc.x) < .35 && rc.hiz > b.hiz) rc.hiz = b.hiz * .8;          // arkadan çarpma
  }
  rc.sira = 1;
  if (rc.mod == 0) for (auto &b : rc.bot) if (b.tur * rc.uzun + b.z > rc.tur * rc.uzun + rc.z + OYUNCU_Z) rc.sira++;
  if (rc.bitti && rc.mod < 2) {                       // rekor: onda bir saniye (hayalet modunun rekoru tur başına yukarıda)
    String k = rcRekorAnahtar(rc.pist); int ds = lround(rc.sure * 10), eski = kayit.getInt(k.c_str(), 0);
    if (!eski || ds < eski) kayit.putInt(k.c_str(), ds);
  }
}

String sureYaz(int ds) { return ds ? String(ds / 600) + ":" + ikiHane(ds / 10 % 60) : String("--:--"); }
void racerMod(Olay o1, Olay o2) {
  String og[3] = {RC_MOD_AD[0], RC_MOD_AD[1], RC_MOD_AD[2]};
  int s = liste("KUBIT RACER  mod", og, 3, o1, o2);
  if (s >= 0) { rc.mod = s; git(RC_PIST); }
}
void racerPist(Olay o1, Olay o2) {                   // Tuş 1 (sağ) değiştirir, Tuş 2 (sol) seçer
  if (o1 == KISA) rc.pist = (rc.pist + 1) % 3;
  if (geriAdim) rc.pist = (rc.pist + 2) % 3;
  if (o2 == KISA) { sayfa = RC_ARABA; return; }
  baslik(RC_MOD_AD[rc.mod]); if (rc.mod == 2) yaz(98, 0, 1, "tur");
  for (int i = 0; i < 3; i++) {
    int y = 13 + i * 11;
    if (i == rc.pist) { ekran.fillRect(0, y - 1, 128, 10, W); ekran.setTextColor(B); }
    int ds = kayit.getInt(rcRekorAnahtar(i).c_str(), 0);
    yaz(3, y, 1, PIST_AD[i]); yaz(rc.mod == 2 ? 80 : 86, y, 1, rc.mod == 2 ? (ds ? String(ds / 10) + "." + String(ds % 10) + " sn" : String("--")) : sureYaz(ds));
    ekran.setTextColor(W);
  }
  yaz(0, 56, 1, "sag/sol  2 tus: sec");
}
void racerAraba(Olay o1, Olay o2) {
  if (o1 == KISA) rc.araba = (rc.araba + 1) % 3;
  if (geriAdim) rc.araba = (rc.araba + 2) % 3;
  if (o2 == KISA) { yarisBasla(); sayfa = RC_YARIS; return; }
  const Araba &a = ARABALAR[rc.araba];
  baslik("ARABA SEC"); yaz(0, 13, 1, a.ad);
  arabaCiz(rc.araba, 24, 50, 34, 64, 0);
  const char *ad[3] = {"HIZ", "IVME", "TUTUS"}; float deger[3] = {a.hiz, a.ivme, a.tutus}, mn[3] = {.9, .8, .7}, mx[3] = {1.15, 1.3, 1.4};
  for (int i = 0; i < 3; i++) { yaz(52, 22 + i * 11, 1, ad[i]); ekran.drawRect(88, 23 + i * 11, 38, 6, W); ekran.fillRect(89, 24 + i * 11, lround(36 * (deger[i] - mn[i]) / (mx[i] - mn[i])), 4, W); }
  yaz(0, 56, 1, "sag/sol  2 tus: basla");
}
void racerYaris(Olay o2) {
  uint32_t simdi = millis(); float dt = min(0.05f, (simdi - rc.son) / 1000.0f); rc.son = simdi;
  yarisGuncelle(dt); yarisCiz(); yarisHud();
  if (rc.sayim > 0) { ekran.fillRect(36, 18, 24, 20, W); ekran.setTextColor(B); yaz(42, 21, 2, String((int)ceil(rc.sayim))); ekran.setTextColor(W); }
  else if (rc.sure < 1) { ekran.fillRect(30, 18, 36, 18, W); ekran.setTextColor(B); yaz(36, 21, 2, "GO!"); ekran.setTextColor(W); }
  else if (rc.turbo >= 1 && !rc.turboKalan && !rc.bitti && (millis() / 400) % 2) { ekran.fillRect(24, 2, 48, 10, W); ekran.setTextColor(B); yaz(30, 3, 1, "TURBO!"); ekran.setTextColor(W); }
  if (rc.bitti) {
    ekran.fillRect(4, 14, 88, 36, W); ekran.fillRect(5, 15, 86, 34, B);
    yaz(10, 18, 1, rc.mod ? "BITTI!" : rc.sira == 1 ? "KAZANDIN!" : String(rc.sira) + ". oldun");
    yaz(10, 29, 1, "Sure " + sureYaz(lround(rc.sure * 10)));
    yaz(10, 39, 1, rc.mod ? "en iyi tur " + String(rc.enIyiTur, 1) : String("2: tamam"));
    if (o2 == KISA) { sayfa = RC_PIST; pist.clear(); pist.shrink_to_fit(); }
  }
  if (tus1.basili && tus2.basili && millis() - max(tus1.basla, tus2.basla) > 4000) { sayfa = RC_PIST; pist.clear(); pist.shrink_to_fit(); tusYut = true; }   // iki tuş 4 sn: çık
}
void sRacer(int x, int y, int s, uint32_t t) {        // simge: yol perspektifi + araba
  float k = s / 24.0;
  hat(x + 9 * k, y + 2 * k, x + 1 * k, y + 23 * k); hat(x + 15 * k, y + 2 * k, x + 23 * k, y + 23 * k);
  for (int j = 0; j < 3; j++) ekran.drawPixel(lround(x + 12 * k), lround(y + (3 + fmod(j * 7 + (t / 120 % 6) * 1.2, 20)) * k), W);
  ekran.fillRect(lround(x + 6 * k), lround(y + 15 * k), lround(12 * k), lround(5 * k), W); ekran.fillRect(lround(x + 8 * k), lround(y + 16 * k), lround(8 * k), max(1L, lround(2 * k)), B);
  ekran.fillRect(lround(x + 5 * k), lround(y + 20 * k), lround(3 * k), lround(3 * k), W); ekran.fillRect(lround(x + 16 * k), lround(y + 20 * k), lround(3 * k), lround(3 * k), W);
}

// ---------------------------------------------------------------- Oyunlar klasörü
#include "oyun_ortak.h"
#include "oyun_3d.h"
#include "oyun_beceri.h"
#include "oyun_zeka.h"
#include "oyun_mc.h"
#include "oyun_doom.h"
#include "oyun_mario.h"
#include "oyun_kubimon.h"
#include "oyun_zindan.h"
#include "oyun_metro.h"
#include "oyun_kasaba.h"
#include "oyun_simge.h"
const OyunTanim OYUN_LISTE[] = {
  {"Kubitcraft", "tut:yuru 2tik:kir 1tik:koy ikisi:zipla ikisi 1sn:menu", kubitcraft, 2, "", 0},
  {"Kubit Doom", "sol/sag:don ikisi tut:yuru ikisi tik:ates", kubitDoom, 1, "doom", 1},
  {"Super Kubit", "tut:kos ikisi:zipla kosarken ikinci tusa bas", superKubit, 0, "mario", 0},
  {"Kubimon", "sag:secenek sol:sec yakala, egit, evrimlestir", kubimon, 0, "kubimon", 0},
  {"Zindan", "sol:don sag:adim/vur ikisi:iksir merdivenden in", zindanOyun, 0, "zindan", 0},
  {"Metro Kosusu", "sol/sag:serit ikisi:zipla trenden kac", metroKosusu, 0, "metro", 0},
  {"Kubit Kasabasi", "sag:gez sol:yap bitkiler gercek saatle buyur", kubitKasabasi, 0, "", 0},
  {"Yilan", "sol/sag tus: yilani dondur", yilan, false, "yilan", 0},
  {"Dino", "bir tusa bas: zipla (basili tut: daha yuksek)", dino, false, "dino", 0},
  {"Pong", "sol: yukari  sag: asagi. 5 sayi alan kazanir", pong, false, "pong", 0},
  {"Breakout", "sol/sag: raket. bas: topu birak", breakout, false, "breakout", 0},
  {"Uzay Istilasi", "sol/sag: gemi, ates kendiliginden", istila, false, "istila", 0},
  {"Asteroid", "sol/sag: don, ates kendiliginden", asteroid, false, "asteroid", 0},
  {"3B Labirent", "sol/sag: don, iki tus birlikte: yuru. cikisi bul", labirent, true, "labirent", 1},
  {"Uzay Tuneli", "sol/sag: don, iki tus birlikte: ilerle. bosluktan gec", tunel, true, "tunel", 0},
  {"Tetris", "sol/sag: kaydir. 2 tus kisa: dondur, uzun: dusur", tetris, false, "tetris", 0},
  {"2048", "kisa bas: sola/saga, uzun bas: yukari/asagi", oyun2048, false, "2048", 0},
  {"Reaksiyon", "BAS! yazinca hemen bas. 5 tur", reaksiyon, false, "reaksiyon", 2},
  {"Simon", "diziyi izle, tekrarla: sol, sag ya da ikisi", simon, false, "simon", 0},
  {"Hafiza Kartlari", "sag: gez  sol: kart cevir", hafiza, false, "hafiza", 0},
  {"Yazi-Tura Serisi", "sol: YAZI  sag: TURA. ust uste bil", paraYaris, false, "para", 0},
  {"Okculuk", "sol tut: nisan, sag tut: guc, birak: at", okculuk, false, "okculuk", 0},
  {"Bowling", "sag: konumu sonra falsoyu kilitle", bowling, false, "bowling", 0},
  {"Mini Golf", "sol/sag: nisan, 2 tusu tut (guc) ve birak", miniGolf, false, "golf", 0},
  {"Satranc: 1'de Mat", "sag: hamle gez  sol: sec", satranc, false, "satranc", 0},
  {"XOX", "sag: kare gez  sol: X koy", xox, false, "xox", 0},
  {"Adam Asmaca", "sol/sag: harf sec, 2 tus: tahmin", adamAsmaca, false, "adam", 0},
  {"Wordle", "sol/sag: harf sec, 2 tus: yaz. < siler", wordle, false, "wordle", 0},
  {"Matematik Yarisi", "islem dogru mu? sol: yanlis sag: dogru", matYaris, false, "matyaris", 0},
  {"Kafadan Hesap", "dogru cevap hangi tarafta? sol ya da sag", kafadan, false, "kafadan", 0},
  {"Parasut", "sol/sag: suzul. kuslardan kac, piste in", parasut, false, "parasut", 0},
  {"Sonsuz Tirmanis", "sol/sag: yon. dusmeden tirman", tirmanis, false, "tirmanis", 0},
};
const int OYUN_N = sizeof(OYUN_LISTE) / sizeof(*OYUN_LISTE), OYUN_ILK = 3;   // listenin başında Flappy, Geometry, Racer
int oyunNo = 0, oyunImlec = 0; bool oyunGiris, oyunIlk; uint32_t oyunBas, oyunSon;
#include "hata.h"
#include "sistem.h"
#include "dolap.h"
#include "uzak.h"
#include "tarayici.h"
#include "kablosuz.h"
#include "kesif.h"
#include "whatsapp.h"
void oyunlaraDon() { sayfa = OYUNLAR; imlec = oyunImlec; }
int oyunSayisi() { return OYUN_N + OYUN_ILK; }
static_assert(sizeof(OYUN_SIMGE) / sizeof(*OYUN_SIMGE) == sizeof(OYUN_LISTE) / sizeof(*OYUN_LISTE) + 3, "her oyunun bir simgesi olmali (araclar/simge_uret.py)");
String rekorYazi(const char *k, uint8_t birim) {
  if (!*k) return "";
  int v = kayit.getInt(k, 0); if (!v) return "";
  return birim == 1 ? String(v / 10) + "." + String(v % 10) + " sn" : birim == 2 ? String(v) + " ms" : String(v);
}
void oyunlarSayfasi(Olay o1, Olay o2) {              // 5 sütunlu simge ızgarası: seçim kutusu kayarak gider, satırlar yumuşak kayar
  const int SUT = 5, HX = 24, HY = 21, X0 = 4, Y0 = 11, N = OYUN_N + OYUN_ILK, SATIR = (N + SUT - 1) / SUT;
  static float sx = X0, sy = Y0, kay = 0;
  imlec = min(imlec, N - 1);
  if (o1 == KISA) imlec = (imlec + 1) % N;
  if (geriAdim) imlec = (imlec - 1 + N) % N;
  oyunImlec = imlec;
  int satir = imlec / SUT, ust = constrain(satir - 1 + (satir == 0), 0, SATIR - 2);   // seçili satır hep görünsün
  kay += (ust * HY - kay) * .35; if (fabs(ust * HY - kay) < .3) kay = ust * HY;
  float hx = X0 + (imlec % SUT) * HX, hy = Y0 + satir * HY - kay;
  sx += (hx - sx) * .45; sy += (hy - sy) * .45; if (fabs(hx - sx) < .3) sx = hx; if (fabs(hy - sy) < .3) sy = hy;
  if (o2 == KISA) { oyunAc(imlec, hx - 1, hy); return; }
  if (o2 == UZUN) favDegistir(imlec);                 // sol uzun: favori (ana menüye)
  for (int i = 0; i < N; i++) {
    int y = lround(Y0 + (i / SUT) * HY - kay); if (y < -16 || y > 60) continue;
    bitSimge(OYUN_SIMGE[i], X0 + (i % SUT) * HX + 3, y + 2);
  }
  tersKutu(lround(sx) - 1, lround(sy), 22, 20, 5);
  ekran.fillRect(0, 0, 128, 10, B); ekran.fillRect(0, 53, 128, 11, B);   // ızgaradan taşanları kırp
  yaz(0, 0, 1, "OYUNLAR"); String say = String(imlec + 1) + "/" + String(N); yaz(128 - say.length() * 6, 0, 1, say);
  ekran.drawFastHLine(0, 9, 128, W);
  ekran.fillRect(126, 11 + 41 * ust / SATIR, 2, 41 * 2 / SATIR, W);       // kaydırma çubuğu
  String ad = gridAd(imlec); if (favMi(imlec)) ad = "*" + ad;
  String r = imlec == 0 ? rekorYazi("flappy", 0) : imlec == 1 ? String("6 seviye") : imlec == 2 ? String("3 mod") : rekorYazi(OYUN_LISTE[imlec - OYUN_ILK].rekor, OYUN_LISTE[imlec - OYUN_ILK].birim);
  ekran.drawFastHLine(0, 54, 128, W);
  yaz(0, 56, 1, ad);
  if (r.length() && (ad.length() + r.length() + 1) * 6 <= 128) { ekran.fillTriangle(127 - r.length() * 6 - 7, 57, 127 - r.length() * 6 - 3, 57, 127 - r.length() * 6 - 5, 61, W); yaz(128 - r.length() * 6, 56, 1, r); }   // kupa yerine küçük üçgen + rekor
}
String gridAd(int g) { return g == 0 ? "Flappy" : g == 1 ? "Geometry" : g == 2 ? "Racer" : OYUN_LISTE[g - OYUN_ILK].ad; }
const uint16_t *gridSimge(int g) { return OYUN_SIMGE[g]; }
void oyunAc(int g, float x, float y) {                // ızgaradan ya da ana menüdeki favori/son oynanandan
  if (oyunSuresiBitti()) { goster(dl("Bugunluk oyun suren doldu", "Daily game time is up")); return; }
  Sayfa h = g == 0 ? FB_SEC : g == 1 ? GD_SEC : g == 2 ? RC_MOD : OYUN;
  oyunImlec = g; imlec = h == GD_SEC ? gdSeviye : 0;
  if (h == OYUN) { oyunNo = g - OYUN_ILK; oyunGiris = true; oyunBas = millis() + GECIS_MS; }
  kayit.putInt("son", g); sayac(("oy" + String(g)).c_str(), 1); sayac("c_oyun", 1); menuListeKur();
  gecisBaslat(x, y, 22, 20, -1, OYUN_SIMGE[g], h);
}
void oyunKare(Olay o1, Olay o2) {
  static bool ikili; static uint32_t ikiliBas, ikiliBit;
  const OyunTanim &o = OYUN_LISTE[oyunNo];
  uint32_t t = millis();
  if (oyunGiris) {                                    // tanıtım kartı: 3 sn ya da bir tuş
    if ((int32_t)(t - oyunBas) > 3000 || o1 == KISA || o2 == KISA) { oyunGiris = false; oyunIlk = true; oyunSon = t; ikili = false; }
    baslik(o.ad); bitSimge(OYUN_SIMGE[oyunNo + OYUN_ILK], 0, 14, 2);
    int y = 13; for (auto &s : sar(o.nasil, 14)) { if (y < 50) yaz(40, y, 1, s); y += 9; }
    ekran.drawFastHLine(0, 54, 128, W); yaz(0, 56, 1, ayarOyunCikis ? "cikis: sol 5sn>sag 5sn" : "cikis:sol 10sn>sag 10");
    return;
  }
  Girdi g = {o1, o2, tus1.basili, tus2.basili, oyunIlk, false, 0, 0, min(0.05f, (t - oyunSon) / 1000.0f)};
  oyunIlk = false; oyunSon = t;
  if (g.b1 && g.b2) { if (!ikili) { ikili = true; ikiliBas = t; ikiliBit = 0; } g.ikiSure = t - ikiliBas; }
  else if (ikili && !ikiliBit) ikiliBit = t;
  if (ikili) {                                        // iki tuş birlikteyken tek tuş kısa/uzun olayları sayılmaz, ikisi de bırakılınca combo
    if (g.o1 == KISA || g.o1 == UZUN) g.o1 = YOK;
    if (g.o2 == KISA || g.o2 == UZUN) g.o2 = YOK;
    if (!g.b1 && !g.b2) { ikili = false; g.combo = true; g.comboSure = ikiliBit - ikiliBas; }
  }
  if (!o.f(g)) { oyunlaraDon(); tusYut = true; ikili = false; }            // çıkış: oyunCikisAdim (loop)
}

// ---------------------------------------------------------------- Şans: zar, yazı-tura, kim ödeyecek çarkı
const String SANS_OGE[] = {"Zar", "Yazi tura", "Kim odeyecek?"};
void sansSayfasi(Olay o1, Olay o2) {
  int s = liste("SANS", SANS_OGE, 3, o1, o2);
  if (s == 0) { git(ZAR); zarAnim = 0; } else if (s == 1) { git(YAZITURA); paraAnim = 0; paraSonuc = 0; } else if (s == 2) { git(CARK); carkSecili = -1; }
}
void zarCiz(int x, int y, int n) {
  ekran.drawRoundRect(x, y, 26, 26, 4, W);
  const int nok[7][6][2] = {{}, {{13, 13}}, {{6, 6}, {20, 20}}, {{6, 6}, {13, 13}, {20, 20}}, {{6, 6}, {20, 6}, {6, 20}, {20, 20}},
                            {{6, 6}, {20, 6}, {13, 13}, {6, 20}, {20, 20}}, {{6, 6}, {20, 6}, {6, 13}, {20, 13}, {6, 20}, {20, 20}}};
  for (int i = 0; i < n; i++) ekran.fillCircle(x + nok[n][i][0], y + nok[n][i][1], 2, W);
}
void zarSayfasi(Olay o1, Olay o2) {
  if (o1 == KISA) zarSayisi = zarSayisi == 1 ? 2 : 1;
  if (o2 == KISA) zarAnim = 14;
  if (zarAnim > 0) { zarAnim--; for (int i = 0; i < 2; i++) zarlar[i] = random(1, 7); }
  baslik("ZAR   1:sayi 2:at");
  if (zarSayisi == 1) zarCiz(51, 18, zarlar[0]); else { zarCiz(30, 18, zarlar[0]); zarCiz(72, 18, zarlar[1]); }
  if (!zarAnim && zarSayisi == 2) ortala(52, 1, "Toplam " + String(zarlar[0] + zarlar[1]));
}
void yaziTuraSayfasi(Olay o2) {
  if (o2 == KISA) { paraAnim = 20; paraSonuc = random(1, 3); }
  baslik("YAZI TURA   2:at");
  int en = paraAnim ? abs((int)(16 * cos(paraAnim * 0.6))) + 1 : 16;   // dönen para: eni daralıp genişler
  if (paraAnim) paraAnim--;
  ekran.fillRoundRect(64 - en, 18, 2 * en, 32, min(en, 16), W);
  if (!paraAnim && paraSonuc) {
    ekran.setTextColor(B); ortala(30, 1, paraSonuc == 1 ? "YAZI" : "TURA"); ekran.setTextColor(W);
    ortala(55, 1, paraSonuc == 1 ? "Yazi geldi!" : "Tura geldi!");
  }
}
std::vector<String> isimListesi() { String s = kayit.getString("isimler", ""); return s.length() ? parcala(s, '|') : std::vector<String>(); }
void carkSayfasi(Olay o1, Olay o2) {
  auto isimler = isimListesi();
  if (carkAnim > 0) {                                 // dönerken yavaşlayarak isimler arasında gezer
    carkAnim--;
    if (carkAnim % max(1, (40 - carkAnim) / 6) == 0) carkSecili = (carkSecili + 1) % isimler.size();
    baslik("KIM ODEYECEK?");
    for (int i = 0; i < (int)isimler.size(); i++) {
      float a = 2 * PI * i / isimler.size() - HALF_PI;
      int x = 64 + 40 * cos(a) - isimler[i].length() * 3, y = 36 + 17 * sin(a) - 3;
      x = constrain(x, 0, 128 - (int)isimler[i].length() * 6);
      if (i == carkSecili) { ekran.fillRect(x - 1, y - 1, isimler[i].length() * 6 + 2, 10, W); ekran.setTextColor(B); }
      yaz(x, y, 1, isimler[i]); ekran.setTextColor(W);
    }
    return;
  }
  if (carkSecili >= 0 && carkSecili < (int)isimler.size()) {
    baslik("KIM ODEYECEK?");
    sigdir(max(0, (128 - (int)isimler[carkSecili].length() * 12) / 2), 20, isimler[carkSecili]);
    ortala(42, 1, "oduyor! :)");
    yaz(0, 56, 1, "2: tamam");
    if (o2 == KISA) carkSecili = -1;
    return;
  }
  String ogeler[] = {"Cevir!", "Isim ekle", "Son ismi sil", "Hepsini sil"};
  int s = liste("CARK  " + String(isimler.size()) + " isim", ogeler, 4, o1, o2);
  if (s == 0) { if (isimler.size() < 2) goster("en az 2 isim ekle"); else { carkAnim = 40; carkSecili = random(isimler.size()); } }
  else if (s == 1) { if (isimler.size() >= 8) goster("en fazla 8 isim"); else klavyeAc(ISIM); }
  else if (s == 2 && isimler.size()) { isimler.pop_back(); String y; for (auto &i : isimler) y += (y.length() ? "|" : "") + i; kayit.putString("isimler", y); }
  else if (s == 3) kayit.putString("isimler", "");
  altMesaj();
}

// ---------------------------------------------------------------- kumanda ve ayarlar
const String KUMANDA_OGE[] = {"Kubik'i ac", "Modeli uyandir", "Modeli uyut", "Pi yeniden baslat", "Pi'yi kapat"};
void kumandaSayfasi(Olay o1, Olay o2) {
  int s = liste("KUMANDA", KUMANDA_OGE, 5, o1, o2);
  if (s == 0) { telefonaGonder("AC", "Kubik aciliyor"); git(ANA); }
  else if (s == 1) { telefonaGonder("CMD:uyandir", "gonderildi"); git(ANA); }
  else if (s == 2) { telefonaGonder("CMD:uyut", "gonderildi"); git(ANA); }
  else if (s == 3) { onayKomut = "yeniden"; sayfa = ONAY; }
  else if (s == 4) { onayKomut = "kapat"; sayfa = ONAY; }
}
void onaySayfasi(Olay o1, Olay o2) {
  bool kapat = String(onayKomut) == "kapat";
  if (o2 == KISA) { telefonaGonder(String("CMD:") + onayKomut, "gonderildi"); git(ANA); return; }
  if (o1 == KISA) { git(KUMANDA); return; }
  yaz(10, 4, 2, kapat ? "PI'YI" : "YENIDEN");
  yaz(10, 22, 2, kapat ? "KAPAT?" : "BASLAT?");
  yaz(10, 48, 1, "2: EVET   1: hayir");
}
bool geceMi() { if (!ayarGece || !saatVar()) return false; int s = simdiTm().tm_hour; return ayarGece == 1 ? (s >= 23 || s < 7) : (s < 8); }
void parlaklikUygula() { ekran.setContrast(ayarPilTas || geceMi() ? PARLAKLAR[0] : PARLAKLAR[ayarParlak]); }

// ---------------------------------------------------------------- kendi kendine güncelleme (GitHub sürümü "kubit")
void saatAyarla() {                                  // saat bilinmiyorsa internetten al (NTP, en çok 8 sn); telefon bağlanınca yerel saate döner
  if (saatVar()) return;
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  for (uint32_t t = millis(); !saatVar() && millis() - t < 8000;) delay(100);
}
void guncelleEkran(const String &s1, const String &s2, int yuzde = -1) {
  ekran.clearDisplay(); baslik("GUNCELLEME  s" + String(SURUM));
  yaz(0, 16, 1, s1); yaz(0, 28, 1, s2);
  if (yuzde >= 0) { ekran.drawRect(0, 44, 128, 10, W); ekran.fillRect(2, 46, 124 * yuzde / 100, 6, W); ortala(56, 1, String(yuzde) + "%"); }
  ekran.display();
}
void guncelle() {                                    // bitince her durumda yeniden başlar (Bluetooth belleği Wi-Fi/TLS için kapatılıyor)
  uzunIs = true;                                     // bekçi görevi indirme sürerken yeniden başlatmasın
  Preferences ag; ag.begin("kubit", true); String ad = ag.getString("wifi_ad", ""), sifre = ag.getString("wifi_sifre", ""); ag.end();   // Wi-Fi hep 1. profilde
  if (!ad.length()) { uzunIs = false; wifiDevam = 3; wifiHal = 0; git(WIFI_S); return; }
  guncelleEkran("Wi-Fi'a baglaniyor", ad);
  NimBLEDevice::deinit(true);
  WiFi.mode(WIFI_STA); WiFi.begin(ad.c_str(), sifre.c_str());
  for (uint32_t t = millis(); WiFi.status() != WL_CONNECTED && millis() - t < 15000;) delay(200);
  auto bitir = [](const String &s1, const String &s2) { guncelleEkran(s1, s2); delay(3000); ESP.restart(); };
  if (WiFi.status() != WL_CONNECTED) bitir("Wi-Fi'a baglanamadi", "2,4 GHz ag mi?");
  saatAyarla();                                       // saat yoksa (telefon bağlanmadan açıldıysa) sertifikalar "henüz geçerli değil" görünür
  WiFiClientSecure baglanti; baglanti.useBuiltinCACertBundle();   // ESP-IDF'in tam kök paketi: GitHub sertifika değiştirse de doğrular
  HTTPClient http; http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); http.setTimeout(10000);
  guncelleEkran("Yeni surum araniyor", "");
  int yeni = -1;
  int kod = 0;
  for (int deneme = 0; deneme < 3 && yeni < 0; deneme++) {   // geçici ağ/TLS hatalarına karşı 3 deneme
    if (deneme) { guncelleEkran("Tekrar deneniyor " + String(deneme + 1) + "/3", ""); delay(1500); }
    if (http.begin(baglanti, SURUM_URL)) { kod = http.GET(); if (kod == 200) yeni = http.getString().toInt(); http.end(); }
  }
  http.end();
  if (yeni < 0) {                                    // hatanın sebebini göster: HTTP kodu ya da TLS hata metni
    char tls[64] = "", h[32]; int tk = baglanti.lastError(tls, sizeof(tls));
    snprintf(h, sizeof h, "TLS -0x%X%s", (unsigned)-tk, saatVar() ? "" : " saat yok");   // -0x2700: sertifika doğrulanamadı
    bitir("Ulasilamadi kod " + String(kod), tk ? String(h) : "bos bellek " + String(ESP.getFreeHeap() / 1024) + "K");
  }
  if (yeni <= SURUM) bitir("Kubit guncel!", "surum " + String(SURUM));
  guncelleEkran("Yeni surum: " + String(yeni), "2: yukle   1: vazgec");
  while (digitalRead(TUS1) == HIGH && digitalRead(TUS2) == HIGH) delay(20);
  if (digitalRead(TUS2) == HIGH) ESP.restart();       // Tuş 1: vazgeç
#ifndef SIMULATOR
  { esp_task_wdt_config_t c = {60000, 0, true}; esp_task_wdt_reconfigure(&c); }   // flash yazarken 5 sn'lik bekçi Kübit'i yarıda yeniden başlatmasın
#endif
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.onProgress([](int bitti, int toplam) { static int son = -1; int y = toplam ? 100LL * bitti / toplam : 0; if (y != son) { son = y; guncelleEkran("Yukleniyor...", "kapatma!", y); } });
  t_httpUpdate_return sonuc = httpUpdate.update(baglanti, BIN_URL);
  if (sonuc == HTTP_UPDATE_OK) bitir("Tamam! Yeni surum " + String(yeni), "yeniden basliyor");
  bitir("Hata:", httpUpdate.getLastErrorString().substring(0, 21));
}
void parlaklikUygula();
void ayarSayfasi(Olay o1, Olay o2) {
  const char *ACILIS[3] = {"Kubit", "menu", "saat"}, *STIL[3] = {"mercek", "liste", "izgara"}, *KOR[4] = {"kapali", "akvaryum", "yildiz", "matrix"};
  const char *DUVAR[4] = {"yok", "yildiz", "hava", "kabarcik"}, *GECE[3] = {"kapali", "23-07", "00-08"}, *SINIR[4] = {"sinirsiz", "30 dk", "1 sa", "2 sa"};
  const char *ACILIS_EN[3] = {"Kubit", "menu", "clock"}, *STIL_EN[3] = {"lens", "list", "grid"}, *KOR_EN[4] = {"off", "aquarium", "stars", "matrix"};
  const char *DUVAR_EN[4] = {"none", "stars", "weather", "bubbles"}, *GECE_EN[3] = {"off", "23-07", "00-08"}, *SINIR_EN[4] = {"no limit", "30 min", "1 h", "2 h"};
  auto ac = [](bool v) { return String(v ? dl("acik", "on") : dl("kapali", "off")); };
  const int N = 25;
  String og[N] = {String(dl("Acilis: ", "Start: ")) + (dil ? ACILIS_EN : ACILIS)[ayarAcilis], String(dl("Uyku: ", "Sleep: ")) + UYKU_AD[ayarUyku],
                  String(dl("Parlaklik: ", "Brightness: ")) + PARLAK_AD[ayarParlak], dl("Dil: Turkce", "Language: English"),
                  String(dl("Menu: ", "Menu: ")) + (dil ? STIL_EN : STIL)[ayarMenuStil], dl("Ters ekran: ", "Invert: ") + ac(ayarTers),
                  dl("Buyuk yazi: ", "Large text: ") + ac(ayarBuyukYazi), String(dl("Koruyucu: ", "Saver: ")) + (dil ? KOR_EN : KOR)[ayarKoruyucu],
                  String(dl("Duvar: ", "Wallpaper: ")) + (dil ? DUVAR_EN : DUVAR)[ayarDuvar], dl("Pil tasarrufu: ", "Battery saver: ") + ac(ayarPilTas),
                  String(dl("Gece modu: ", "Night mode: ")) + (dil ? GECE_EN : GECE)[ayarGece], dl("Menuyu duzenle", "Edit menu"),
                  String(dl("Profil: ", "Profile: ")) + String(ayarProfil + 1), String(dl("Oyun suresi: ", "Game time: ")) + (dil ? SINIR_EN : SINIR)[ayarSinir],
                  ayarPin < 0 ? String(dl("PIN koy", "Set PIN")) : String(dl("PIN degistir", "Change PIN")), dl("Telefona yedekle", "Backup to phone"),
                  dl("Yedekten yukle", "Restore backup"), String(dl("Guncelleme ara (s", "Check update (v")) + String(SURUM) + ")", dl("Rekorlari sil", "Clear records"), String(dl("Uyku tipi: ", "Sleep type: ")) + (ayarUykuTipi ? dl("ekran", "screen") : dl("derin", "deep")), dl("Unutma uyarisi: ", "Forget alert: ") + ac(ayarUnutma), String(dl("Oyun cikisi: ", "Game exit: ")) + (ayarOyunCikis ? "5+5 sn" : "10+10 sn"), "Wi-Fi: " + (wifiAdi().length() ? wifiAdi().substring(0, 13) : String(dl("yok", "none"))),
                  String("BT: ") + (ayarBtGuc ? dl("guclu", "strong") : dl("normal", "normal")) + (btSinyal() ? "  " + String(btSinyal()) + "dBm" : String("")), dl("Hakkinda", "About")};
  int s = liste(dl("AYARLAR 2tus:degistir", "SETTINGS both:change"), og, N, o1, o2);
  auto kaydet = [](const char *k, int v) { kayit.putInt(k, v); };
  switch (s) {
    case 0: ayarAcilis = (ayarAcilis + 1) % 3; kaydet("acilis", ayarAcilis); break;
    case 1: ayarUyku = (ayarUyku + 1) % 4; kaydet("uyku", ayarUyku); break;
    case 2: ayarParlak = (ayarParlak + 1) % 3; kaydet("parlak", ayarParlak); parlaklikUygula(); break;
    case 3: dil = !dil; kaydet("dil", dil); break;
    case 4: ayarMenuStil = (ayarMenuStil + 1) % 3; kaydet("menustil", ayarMenuStil); break;
    case 5: ayarTers = !ayarTers; kaydet("ters", ayarTers); ekran.invertDisplay(ayarTers); break;
    case 6: ayarBuyukYazi = !ayarBuyukYazi; kaydet("buyuk", ayarBuyukYazi); break;
    case 7: ayarKoruyucu = (ayarKoruyucu + 1) % 4; kaydet("koruyucu", ayarKoruyucu); break;
    case 8: ayarDuvar = (ayarDuvar + 1) % 4; kaydet("duvar", ayarDuvar); break;
    case 9: ayarPilTas = !ayarPilTas; kaydet("piltas", ayarPilTas); parlaklikUygula(); break;
    case 10: ayarGece = (ayarGece + 1) % 3; kaydet("gece", ayarGece); break;
    case 11: git(MENU_DUZENLE); return;
    case 12: { Preferences sis; sis.begin("kubit_sis"); sis.putInt("profil", (ayarProfil + 1) % 3); sis.end();   // profil değişince yeniden başlar
               ekran.clearDisplay(); ortala(28, 1, String(dl("Profil ", "Profile ")) + String((ayarProfil + 1) % 3 + 1)); ekran.display(); delay(800); ESP.restart(); } break;
    case 13: pinIste(1); break;
    case 14: pinIste(2); break;
    case 15: yedekle(); break;
    case 16: if (bagli) { telefonaGonder("YEDEK:GERI", dl("yedek isteniyor", "requesting backup")); } else goster(dl("telefon bagli degil", "phone not connected")); break;
    case 17: guncelle(); break;
    case 18: kayit.remove("flappy"); for (int i = 0; i < SEVIYE_N; i++) kayit.remove(("gd" + String(i)).c_str());
             for (int i = 0; i < OYUN_N; i++) if (*OYUN_LISTE[i].rekor) kayit.remove(OYUN_LISTE[i].rekor); goster(dl("rekorlar silindi", "records cleared")); break;
    case 19: ayarUykuTipi = !ayarUykuTipi; kaydet("uykutip", ayarUykuTipi); break;
    case 20: ayarUnutma = !ayarUnutma; kaydet("unutma", ayarUnutma); break;
    case 21: ayarOyunCikis = !ayarOyunCikis; kaydet("oyuncik", ayarOyunCikis); break;
    case 22: wifiDevam = 0; wifiEkranAc(); git(WIFI_S); return;
    case 23: ayarBtGuc = !ayarBtGuc; kaydet("btguc", ayarBtGuc); btGucUygula(); break;
    case 24: git(HAKKINDA); return;
  }
  altMesaj();
}

// ---------------------------------------------------------------- bildirim açılır penceresi (oyun dışında her sayfada)
void popup() {
  if (millis() > popupBitis || bildirimler.empty()) return;
  auto &b = bildirimler[0];
  ekran.fillRect(2, 30, 124, 33, B); ekran.drawRoundRect(2, 30, 124, 33, 3, W);
  ekran.fillRect(2, 30, 124, 10, W); ekran.setTextColor(B); yaz(5, 31, 1, b.app.substring(0, 20)); ekran.setTextColor(W);
  yaz(5, 42, 1, b.baslik.substring(0, 20)); yaz(5, 52, 1, b.metin.substring(0, 20));
}

// ---------------------------------------------------------------- derin uyku ve tuşlar
void kapaliIsaretle(int v) {                         // elle kapatma (1): açmak için 5 sn; süre dolunca uyku (0): tek basış açar
  Preferences sis; sis.begin("kubit_sis"); if (sis.getInt("kapali", 0) != v) sis.putInt("kapali", v); sis.end();
}
void kapat() {
  ekran.clearDisplay(); ortala(28, 1, dl("Kapaniyor...", "Shutting down...")); ekran.display();
  uykuOncesiHaber(); kapaliIsaretle(1);
  delay(900);
  derinUyku();
}
void uykuyaGec() {                                   // ekran zaman aşımı
  if (ayarUykuTipi == 1 || ayarYakinlik) { ekranUyku = true; ekran.oled_command(SH110X_DISPLAYOFF); return; }   // yakınlık kilidi: PC Kübit'i görmeye devam etmeli
  uykuOncesiHaber(); kapaliIsaretle(0);
  derinUyku();
}

// ---------------------------------------------------------------- acil kapatma: sağ tuş 30 sn, bırakınca en geç 2 sn içinde sol tuş 30 sn.
// Ayrı bir FreeRTOS görevinde çalışır: ana döngü takılsa da (çökme, güncellemede kalma) çalışır ve cihazı derin uykuya alır.
void derinUyku() {                                   // iki tuştan biri uyandırır: Tuş 1 ext0, Tuş 2 ext1 (tek pin, LOW)
  ekran.oled_command(SH110X_DISPLAYOFF);
  while (digitalRead(TUS1) == LOW || digitalRead(TUS2) == LOW) delay(10);   // basılıyken uyursa hemen uyanır
  rtc_gpio_pullup_en(GPIO_NUM_25); rtc_gpio_pulldown_dis(GPIO_NUM_25);
  rtc_gpio_pullup_en(GPIO_NUM_26); rtc_gpio_pulldown_dis(GPIO_NUM_26);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
  esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_26, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start();
}
void acilKapat() { kapaliIsaretle(1); derinUyku(); }
bool acmaBasisi() {                                   // uyandıran tuş en az 5 sn basılı kalmalı; erken bırakılırsa yeniden uyu
  uint32_t bas = millis();
  while (digitalRead(TUS1) == LOW || digitalRead(TUS2) == LOW) {
    uint32_t g = millis() - bas;
    if (g >= 5000) return true;
    if (g > 600) { ekran.clearDisplay(); ekran.drawRoundRect(24, 28, 80, 8, 3, W); ekran.fillRect(26, 30, 76 * g / 5000, 4, W); ekran.display(); }   // dolan çubuk
    delay(20);
  }
  return false;
}
bool acilAdim(uint32_t simdi) {                      // her 50 ms çağrılır; true: kapat
  static uint8_t evre = 0; static uint32_t bas = 0, birak = 0;
  bool sag = digitalRead(TUS1) == LOW, sol = digitalRead(TUS2) == LOW;
  switch (evre) {
    case 0: if (sag && !sol) { bas = simdi; evre = 1; } break;                                  // sağ basıldı
    case 1: if (!sag || sol) { evre = 0; break; } if (simdi - bas >= 30000) evre = 2; break;   // 30 sn sağ
    case 2: if (sol) { evre = 0; break; } if (!sag) { birak = simdi; evre = 3; } break;          // sağ bırakılmayı bekle
    case 3: if (sag) { evre = 0; break; } if (sol) { bas = simdi; evre = 4; } else if (simdi - birak > 2000) evre = 0; break;   // 2 sn içinde sol
    case 4: if (!sol || sag) { evre = 0; break; } if (simdi - bas >= 30000) { evre = 0; return true; } break;               // 30 sn sol
  }
  return false;
}
#ifndef SIMULATOR
bool donmaVar(); void donmaKaydet();
void acilGorev(void *) { for (;;) { if (acilAdim(millis())) acilKapat(); if (donmaVar()) { donmaKaydet(); esp_restart(); } vTaskDelay(pdMS_TO_TICKS(50)); } }   // bekçi: döngü 20 sn durursa
void acilKapatmaBaslat() { xTaskCreatePinnedToCore(acilGorev, "acil", 3072, nullptr, 3, nullptr, 0); }
#else
void acilKapatmaBaslat() {}                          // simülatörde loop() içinden denetlenir
#endif

Olay oku(Tus &t) {
  bool simdi = digitalRead(t.pin) == LOW;
  if (simdi && !t.basili) { t.basili = true; t.basla = t.tekrar = millis(); sonDokunus = millis(); return BASTI; }
  if (!simdi && t.basili) {
    t.basili = false;
    uint32_t sure = millis() - t.basla;
    if (sure > 30) return sure >= UZUN_BASIS_MS ? UZUN : KISA;
  }
  return YOK;
}

// Menü düzeni: sağ = sonraki (o1 KISA), sol = önceki (onceki), iki tuş kısa = seç, iki tuş 1 sn = geri; basılı tutmak hızlı gezdirir.
// menu false ise yalnız iki tuş işlenir (sayfanın kendi tuşları kalır). solUzun: sol uzun basış sayfaya kalır (Oyunlar'da favori).
Kontrol yeniKontrol(Olay &o1, Olay &o2, bool menu, bool solUzun, bool klavye) {
  static bool ikili = false, geriOldu = false, yazdi = false; static uint32_t ikiBas = 0, sonYaz = 0;
  Kontrol k = {false, false, false};
  uint32_t t = millis();
  if (tus1.basili && tus2.basili) { if (!ikili) { ikili = true; ikiBas = sonYaz = t; geriOldu = yazdi = false; } if (!klavye && !geriOldu && t - ikiBas >= 1000) { geriOldu = true; k.geri = true; } }
  if (ikili) {
    if (klavye && tus1.basili && tus2.basili && klavyeTekrarUygun() && t - ikiBas >= 450 && t - sonYaz >= 160) { sonYaz = t; yazdi = true; k.sec = true; }
    if (o1 == KISA || o1 == UZUN) o1 = YOK; if (o2 == KISA || o2 == UZUN) o2 = YOK;
    if (!tus1.basili && !tus2.basili) { ikili = false; k.sec = !geriOldu && !yazdi; }
    return k;
  }
  if (!menu) return k;
  if (o1 == UZUN || (o1 == KISA && tus1.tekrar != tus1.basla)) o1 = YOK;     // basılı tutup gezdiyse bırakış ayrıca sayılmaz
  if (o2 == KISA) { o2 = YOK; if (tus2.tekrar == tus2.basla) k.onceki = true; } else if (o2 == UZUN && !solUzun) o2 = YOK;
  if (tus1.basili && !tus2.basili && t - tus1.basla > 400 && t - tus1.tekrar > tekrarMs) { tus1.tekrar = t; o1 = KISA; }
  if (!solUzun && tus2.basili && !tus1.basili && t - tus2.basla > 400 && t - tus2.tekrar > tekrarMs) { tus2.tekrar = t; k.onceki = true; }
  return k;
}
bool yeniDuzen(Sayfa s) {
  switch (s) { case MENU: case SOR: case KLAVYE: case BILDIRIM: case MUZIK: case FB_SEC: case GD_SEC: case RC_MOD: case RC_PIST: case RC_ARABA: case SANS:
               case KUMANDA: case AYAR: case OYUNLAR: case ARACLAR: case MENU_DUZENLE: case DOLAP: case PC_S: case WIFI_S: case RADAR_S: case UZAY_S: case WHATSAPP_S: return true; default: return false; }
}
// oyundan çıkış: sol N sn basılı tut, bırak, 2 sn içinde sağı N sn basılı tut (oyunda yanlışlıkla çıkılmasın)
uint8_t cikEvre = 0; uint32_t cikBas = 0, cikBirak = 0;
uint32_t cikisSure() { return ayarOyunCikis ? 5000 : 10000; }
bool oyunCikisAdim() {
  uint32_t t = millis(), n = cikisSure(); bool sag = tus1.basili, sol = tus2.basili;
  switch (cikEvre) {
    case 0: if (sol && !sag) { cikBas = tus2.basla; cikEvre = 1; } break;
    case 1: if (!sol || sag) { cikEvre = 0; break; } if (t - cikBas >= n) cikEvre = 2; break;
    case 2: if (sag) { cikEvre = 0; break; } if (!sol) { cikBirak = t; cikEvre = 3; } break;
    case 3: if (sol) { cikEvre = 0; break; } if (sag) { cikBas = tus1.basla; cikEvre = 4; } else if (t - cikBirak > 2000) cikEvre = 0; break;
    case 4: if (!sag || sol) { cikEvre = 0; break; } if (t - cikBas >= n) { cikEvre = 0; return true; } break;
  }
  return false;
}
void oyunCikisCiz() {                                // 1,5 sn sonra üstte küçük ilerleme kutusu
  uint32_t t = millis(), n = cikisSure();
  if (!cikEvre || (cikEvre == 1 && t - cikBas < 1500)) return;
  float p = cikEvre == 1 ? .5f * (t - cikBas) / n : cikEvre == 4 ? .5f + .5f * (t - cikBas) / n : .5f;
  const char *y = cikEvre == 1 ? "cikis: sol tut" : cikEvre == 4 ? "cikis: sag tut" : "simdi sag tusu tut";
  ekran.fillRect(10, 0, 108, 18, B); ekran.drawRect(10, 0, 108, 18, W); yaz(14, 2, 1, y);
  ekran.drawRect(14, 11, 100, 4, W); ekran.fillRect(14, 11, lround(100 * min(1.0f, p)), 4, W);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT); digitalWrite(LED, LOW);
  rtc_gpio_deinit(GPIO_NUM_25); rtc_gpio_deinit(GPIO_NUM_26);   // uykudan dönünce pinleri normal GPIO'ya geri al
  pinMode(TUS1, INPUT_PULLUP);
  pinMode(TUS2, INPUT_PULLUP);
  Wire.begin();
  if (!ekran.begin(0x3C, true) && !ekran.begin(0x3D, true)) {
    Serial.println("OLED bulunamadi: SDA/SCL/3V3/GND kablolarini kontrol et");
    for (;;) { digitalWrite(LED, !digitalRead(LED)); delay(200); }
  }
  ekran.setTextColor(W);                             // varsayılan renk (0xFFFF) bu sürücüde hiçbir piksel yakmıyor
  int neden = esp_sleep_get_wakeup_cause();
  bool elleKapandi; { Preferences sis; sis.begin("kubit_sis", true); elleKapandi = sis.getInt("kapali", 0); sis.end(); }
  if ((neden == ESP_SLEEP_WAKEUP_EXT0 || neden == ESP_SLEEP_WAKEUP_EXT1) && elleKapandi && !acmaBasisi()) derinUyku();   // elle kapatıldıysa açmak için 5 sn basılı tut
  { Preferences sis; sis.begin("kubit_sis", true); ayarProfil = sis.getInt("profil", 0); sis.end(); }
  kayit.begin(ayarProfil == 0 ? "kubit" : ayarProfil == 1 ? "kubit_p2" : "kubit_p3");   // her profilin kendi rekor/ayar alanı
#ifdef WIFI_ADI                                      // yerel USB sürümü Wi-Fi bilgisini bir kez hafızaya yazar (hep 1. profil)
  { Preferences ag; ag.begin("kubit"); if (ag.getString("wifi_ad", "") != WIFI_ADI) { ag.putString("wifi_ad", WIFI_ADI); ag.putString("wifi_sifre", WIFI_SIFRE); } ag.end(); }
#endif
  fbSkin = kayit.getInt("fbskin", 0);
  hataKontrol();                                     // çökme/donma/düşük pil sonrası: rapor + "CRASH REPORTER" ekranı
  dil = kayit.getInt("dil", 0);
  ayarMenuStil = kayit.getInt("menustil", 0); ayarTers = kayit.getInt("ters", 0); ayarBuyukYazi = kayit.getInt("buyuk", 0); ayarKoruyucu = kayit.getInt("koruyucu", 0);
  ayarDuvar = kayit.getInt("duvar", 0); ayarPilTas = kayit.getInt("piltas", 0); ayarGece = kayit.getInt("gece", 0); ayarGizli = kayit.getInt("gizli", 0);
  ayarUykuTipi = kayit.getInt("uykutip", 0); ayarUnutma = kayit.getInt("unutma", 1); ayarOyunCikis = kayit.getInt("oyuncik", 0); ayarBtGuc = kayit.getInt("btguc", 0); ayarYakinlik = kayit.getInt("yakinlik", 0); kiyafetYukle(); ozelYukle();
  ayarSinir = kayit.getInt("eb_sinir", 0); ayarPin = kayit.getInt("pin", -1);
  ekran.invertDisplay(ayarTers);
  ayarAcilis = kayit.getInt("acilis", 0); ayarUyku = kayit.getInt("uyku", 1); ayarParlak = kayit.getInt("parlak", 1);
  parlaklikUygula();
  sayfa = ayarAcilis == 2 ? BUYUK_SAAT : ayarAcilis ? MENU : ANA;
  bool webden = webDonus == 0x7A4A; webDonus = 0;       // tarayıcıdan çıkınca Araçlar'a dön, açılış animasyonu yok
  if (webden) sayfa = ARACLAR;
  bool uyandi = neden == ESP_SLEEP_WAKEUP_EXT0 || neden == ESP_SLEEP_WAKEUP_EXT1;
  for (uint32_t t = millis(); (digitalRead(TUS1) == LOW || digitalRead(TUS2) == LOW) && millis() - t < 3000;) delay(10);   // uyandıran basış bırakılsın, animasyon atlanmasın
  if (!webden) acilisAnimasyonu();
  acilKapatmaBaslat();

  NimBLEDevice::init("Kubit");
  NimBLEDevice::setMTU(185);
  btGucUygula();                         // uzun satırlar tek pakette gelsin (telefon da 185 ister)
  NimBLEServer *sunucu = NimBLEDevice::createServer();
  sunucu->setCallbacks(new Baglanti());
  NimBLEService *servis = sunucu->createService(SERVIS_UUID);
  servis->createCharacteristic(DURUM_UUID, NIMBLE_PROPERTY::WRITE)->setCallbacks(new GelenYaz());
  tusKar = servis->createCharacteristic(TUS_UUID, NIMBLE_PROPERTY::NOTIFY);   // 0x2902 tanımlayıcısını NimBLE kendisi ekler
  tusKar->setCallbacks(new TelefonAbone());
  ekranKar = servis->createCharacteristic(EKRAN_UUID, NIMBLE_PROPERTY::NOTIFY);
  ekranKar->setCallbacks(new EkranAbone());
  waKar = servis->createCharacteristic(WA_KOMUT_UUID, NIMBLE_PROPERTY::NOTIFY); waKar->setCallbacks(new WaAbone());
  servis->createCharacteristic(WA_SONUC_UUID, NIMBLE_PROPERTY::WRITE)->setCallbacks(new WaGelen());
  servis->start();
  NimBLEAdvertising *reklam = NimBLEDevice::getAdvertising();
  reklam->setName("Kubit");                          // telefonun eşleşme penceresi bu adla bulur
  reklam->addServiceUUID(SERVIS_UUID);
  reklam->enableScanResponse(true);
  reklam->start();

  randomSeed(esp_random());
  sonDokunus = millis();
  delay(1000);
  while (uyandi && (digitalRead(TUS1) == LOW || digitalRead(TUS2) == LOW)) delay(10);   // uyandıran basış "Kübik'i aç" sayılmasın
  menuListeKur(); rozetKontrol(nullptr);
  if (saatVar() && kayit.getInt("c_songun", 0) != (int)(time(nullptr) / 86400)) { kayit.putInt("c_songun", time(nullptr) / 86400); sayac("c_gun", 1); }
  tus1.basili = tus2.basili = false;
}

void loop() {
  izGuncelle();
#ifdef SIMULATOR
  { static uint32_t son = 0; if (millis() - son >= 50) { son = millis(); if (acilAdim(son)) acilKapat(); } }
#endif
  if (tara::aktif) { tara::kare(); return; }         // tarayıcı açıkken Bluetooth/uyku yok: kendi döngüsü
  ekranGonder();                                       // Bilgisayar > Ekrani paylas açıksa son kare PC'ye
  yakinlikReklam();                                    // yakınlık kilidi: bağlıyken de yayın yap ki PC Kübit'i görsün
  static uint32_t kare = 0;
  kare++;
  std::vector<String> yeni;
  portENTER_CRITICAL(&kilit); yeni.swap(gelenler); portEXIT_CRITICAL(&kilit);
  for (auto &v : yeni) isle(v);

  ekran.clearDisplay();
  Olay o1 = oku(tus1), o2 = oku(tus2);
  if (tusYut) { o1 = o2 = YOK; if (!tus1.basili && !tus2.basili) tusYut = false; }
  { static bool oncekiBagli = false; if (bagli && !oncekiBagli) sayac("c_bag", 1); oncekiBagli = bagli; }
  { static bool oncekiGece = false; bool g = geceMi(); if (g != oncekiGece) { oncekiGece = g; parlaklikUygula(); } }
  araclarGuncelle(); unutmaKontrol(); hataGonder();
  if (ekranUyku) {                                    // ekran uykusu: bir tuş, gelen arama ya da süre dolunca ekran açılır (basış yutulur)
    if (o1 != YOK || o2 != YOK || tel.araniyor || millis() < uyariBitis) {
      if (o1 != YOK || o2 != YOK) tusYut = true;
      ekranUyku = false; ekran.oled_command(SH110X_DISPLAYON); parlaklikUygula(); sonDokunus = millis(); o1 = o2 = YOK;
    } else { delay(60); return; }
  }
  bool oyunda = sayfa == FLAPPY || sayfa == GD || sayfa == RC_YARIS || sayfa == OYUN;   // oyunda uyku yok, tuşlar oyunun
  if (oyunda) {
    static uint32_t son = millis(); oyunSuresiEkle(min(.1f, (millis() - son) / 1000.0f)); son = millis();
    if (oyunSuresiBitti()) { goster(dl("Bugunluk oyun suren doldu", "Daily game time is up"), 4000); if (sayfa == OYUN) oyunlaraDon(); else { pist.clear(); git(OYUNLAR); } tusYut = true; oyunda = false; }
  }
  uint32_t uykuSure = geceMi() ? 15000 : UYKULAR[ayarUyku], bos = millis() - sonDokunus;
  if (!oyunda && !araclarMesgul() && bos > uykuSure && !ekranUyku) uykuyaGec();
  { static bool koruyor = false;                      // ekran koruyucu: uykudan önceki ikinci yarıda; bir tuş kapatır (tuş yutulur)
    if (!oyunda && ayarKoruyucu && sayfa != BUYUK_SAAT && !tel.araniyor && bos > uykuSure / 2) { koruyor = true; ekran.clearDisplay(); koruyucuCiz(millis()); ekran.display(); delay(40); return; }
    if (koruyor) { koruyor = false; tusYut = true; o1 = o2 = YOK; } }
  if (aramaCiz(o1, o2) || uyariCiz(o1, o2)) { ekran.display(); delay(40); return; }
  geriAdim = false; Kontrol k = {false, false, false};
  bool menuDuzen = yeniDuzen(sayfa);
  if (!oyunda && sayfa != GD_ED) { k = yeniKontrol(o1, o2, menuDuzen, sayfa == OYUNLAR, sayfa == KLAVYE); geriAdim = k.onceki; if (k.sec && menuDuzen) o2 = KISA; }
  if (oyunda && oyunCikisAdim()) {                    // sol N sn > sağ N sn: oyundan çık
    tusYut = true; o1 = o2 = YOK;
    if (sayfa == OYUN && OYUN_LISTE[oyunNo].f == kubitcraft && mc::dunyaAcik && !mc::kaydet()) { cikEvre = 0; return; }
    if (sayfa == OYUN) oyunlaraDon(); else if (sayfa == FLAPPY) { git(FB_SEC); imlec = fb.zor; } else if (sayfa == GD) { git(GD_SEC); imlec = gdSeviye; } else { pist.clear(); git(RC_MOD); }
    oyunda = false;
  }
  if (k.geri && sayfa == KLAVYE) { if (klavyeHedef == WEB) { webKlavyeMetin = ""; webKlavyeBitti = true; } else git(klavyeHedef == ISIM ? CARK : (klavyeHedef == WIFI_SIFRE || klavyeHedef == WIFI_AD) ? WIFI_S : SOR); k.geri = false; }
  // Tuş 1 uzun: geri (oyunlarda ve klavyede Tuş 1 basılı tutulur, oyunda geri Tuş 2)
  if ((k.geri || (o1 == UZUN && !menuDuzen)) && !oyunda && sayfa != KLAVYE && sayfa != GD_ED) {
    switch (sayfa) {
      case ANA: telefonaGonder("YENILE", "yenileniyor"); break;
      case MENU: git(ANA); break;
      case ZAR: case YAZITURA: case CARK: git(SANS); break;
      case ONAY: git(KUMANDA); break;
      case CEVAP: git(SOR); break;
      case FB_SEC: case GD_SEC: case RC_MOD: oyunlaraDon(); break;
      case TELEFON_S: case KRONOMETRE: case POMODORO: case BUYUK_SAAT: git(ARACLAR); break;
      case ZAMANLAYICI: case ALARM_KUR: if (k.geri) git(ARACLAR); break;   // bunlarda sağ basılı tutmak hızlı artırır: geri iki tuş
      case ROZETLER: if (k.geri) git(MENU); break;
      case PIN_GIR: case MENU_DUZENLE: case HAKKINDA: case GELISTIRICI: git(AYAR); break;
      case PC_S: case SUNUM_S: case YAZICI_S: git(ARACLAR); imlec = 6; break;
      case KAMERA_S: kameraKapat(); git(ARACLAR); imlec = 8; break;
      case CARK_GUNLUK: git(DOLAP); break;
      case WIFI_S: wifiVazgec(); git(AYAR); imlec = 22; break;
      case RADAR_S: if (radarDetay) radarDetay = false; else { git(ARACLAR); imlec = 12; } break;
      case UZAY_S: git(ARACLAR); imlec = 13; break;
      case WHATSAPP_S: waGeri(); break;
      case PCMON_S: git(PC_S); imlec = 10; break;
      case GD_ED: break;                                // editörde sağ basılı tutmak hızlı ilerler: çıkış menüsünden
      case RC_PIST: git(RC_MOD); imlec = rc.mod; break;
      case RC_ARABA: git(RC_PIST); break;
      default: git(MENU);
    }
    o1 = YOK;
  }
  if (sayfa == ANA && o2 == UZUN) kapat();
  if ((sayfa == ZAMANLAYICI || sayfa == ALARM_KUR) && o2 == UZUN) { git(ARACLAR); o2 = YOK; }
  if (sayfa == ROZETLER && o2 == UZUN) { git(MENU); o2 = YOK; }
  if ((sayfa == FLAPPY || sayfa == GD) && o2 == KISA) { bool gdMi = sayfa == GD; git(gdMi ? GD_SEC : FB_SEC); imlec = gdMi ? gdSeviye : fb.zor; oyunda = false; o2 = YOK; }

  if (gecisCiz()) { ekran.display(); delay(25); return; }
  switch (sayfa) {
    case ANA:      anaSayfa(o1, o2, kare); break;
    case MENU:     menuSayfasi(o1, o2); break;
    case SAAT:     saatSayfasi(); break;
    case HAVA:     havaSayfasi(kare); break;
    case SOR:      sorSayfasi(o1, o2, kare); break;
    case KLAVYE:   klavyeSayfasi(o1, o2); break;
    case CEVAP:    cevapSayfasi(o1, o2); break;
    case OZET:     ozetSayfasi(); break;
    case BILDIRIM: bildirimSayfasi(o1, o2); break;
    case MUZIK:    muzikSayfasi(o1, o2); break;
    case BUL:      bulSayfasi(o2, kare); break;
    case FB_SEC:   flappySecSayfasi(o1, o2); break;
    case FLAPPY:   flappyKare(o1 == BASTI); break;
    case GD_SEC:   gdSecSayfasi(o1, o2); break;
    case GD:       gdKare(tus1.basili, o1 == BASTI); break;
    case RC_MOD:   racerMod(o1, o2); break;
    case RC_PIST:  racerPist(o1, o2); break;
    case RC_ARABA: racerAraba(o1, o2); break;
    case RC_YARIS: racerYaris(o2); break;
    case SANS:     sansSayfasi(o1, o2); break;
    case ZAR:      zarSayfasi(o1, o2); break;
    case YAZITURA: yaziTuraSayfasi(o2); break;
    case CARK:     carkSayfasi(o1, o2); break;
    case KUMANDA:  kumandaSayfasi(o1, o2); break;
    case ONAY:     onaySayfasi(o1, o2); break;
    case AYAR:     ayarSayfasi(o1, o2); break;
    case OYUNLAR:  oyunlarSayfasi(o1, o2); break;
    case OYUN:     oyunKare(o1, o2); break;
    case ARACLAR:  araclarSayfasi(o1, o2); break;
    case TELEFON_S: telefonSayfasi(o2); break;
    case ZAMANLAYICI: zamanlayiciSayfasi(o1, o2); break;
    case KRONOMETRE: kronometreSayfasi(o1, o2); break;
    case POMODORO: pomodoroSayfasi(o1, o2); break;
    case ALARM_KUR: alarmSayfasi(o1, o2); break;
    case BUYUK_SAAT: if (o2 == KISA) git(MENU); buyukSaatSayfasi(); break;
    case ROZETLER: rozetSayfasi(o1, o2); break;
    case PIN_GIR:  pinSayfasi(o1, o2); break;
    case MENU_DUZENLE: menuDuzenleSayfasi(o1, o2); break;
    case HAKKINDA: hakkindaSayfasi(o1); break;
    case GELISTIRICI: gelistiriciSayfasi(); break;
    case DOLAP:    dolapSayfasi(o1, o2); break;
    case CARK_GUNLUK: gunlukCarkSayfasi(o1, o2); break;
    case PC_S:     pcSayfasi(o1, o2); break;
    case SUNUM_S:  sunumSayfasi(o1, o2); break;
    case KAMERA_S: kameraSayfasi(o1, o2); break;
    case YAZICI_S: yaziciSayfasi(o2); break;
    case WIFI_S:   wifiSayfasi(o1, o2); break;
    case RADAR_S:  radarSayfasi(o1, o2); break;
    case UZAY_S:   uzaySayfasi(o1, o2); break;
    case WHATSAPP_S: waSayfasi(o1, o2); break;
    case PCMON_S:  pcMonSayfasi(); break;
    case WEB_S:    tara::calistir(false); break;        // geri dönmez: çıkışta yeniden başlar
    case YT_S:     tara::calistir(true); break;
    case GD_ED:    { static uint32_t son = millis(); Girdi g = {o1, o2, tus1.basili, tus2.basili, false, false, 0, 0, min(.05f, (millis() - son) / 1000.0f)}; son = millis();
                     static bool ikili; static uint32_t ib, ibt;   // iki tuş: oyunlardaki gibi combo / basılı süre
                     if (g.b1 && g.b2) { if (!ikili) { ikili = true; ib = millis(); ibt = 0; } g.ikiSure = millis() - ib; } else if (ikili && !ibt) ibt = millis();
                     if (ikili) { if (g.o1 == KISA || g.o1 == UZUN) g.o1 = YOK; if (g.o2 == KISA || g.o2 == UZUN) g.o2 = YOK; if (!g.b1 && !g.b2) { ikili = false; g.combo = true; g.comboSure = ibt - ib; } }
                     bool oyna; if (!seviyeEditoru(g, oyna)) git(GD_SEC); else if (oyna) { gdSeviye = 6; gdBasla(); sayfa = GD; } } break;
  }
  if (!oyunda) popup(); else oyunCikisCiz();
  ekran.display();
  delay(oyunda ? 25 : ayarPilTas ? 90 : 40);
}
