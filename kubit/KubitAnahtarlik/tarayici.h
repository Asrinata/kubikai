// Kübit Tarayıcı ve YouTube: telefon/PC olmadan Kübit'in kendisi Wi-Fi'dan indirir. RAM ikisine yetmediği için açılınca Bluetooth
// kapanır (güncellemedeki gibi); çıkınca Kübit yeniden başlar ve Araçlar'a döner. Sayfa bütün tutulmaz: HTML akarken
// web_ayristir.h'deki durum makinesinden geçer, yalnız yazı ve linkler kalır. Resimler (yalnız JPEG) ROM'daki TJpgDec ile
// parça parça çözülüp 128x64'e ortalanır ve Floyd-Steinberg ile noktalanır.
// Tuşlar her yerde: sağ = ileri, sol = geri gider (basılı: hızlı), iki tuş kısa = tıkla, iki tuş 1 sn = bir üst ekran.
// Sayfa görünmez 11x6 kareye bölünür; fare oku kare kare kayar, altındaki link ters yazılır, tıklayınca açılır. Başlıktaki "≡" = menü.
#pragma once
#include "web_ayristir.h"
#ifndef SIMULATOR
#include "esp32/rom/tjpgd.h"
#include "qrcode.h"
#endif

RTC_NOINIT_ATTR uint32_t webDonus;                   // yeniden başlayınca Araçlar'a dön (0x7A4A)
const char *webKlavyeBaslik = ""; bool webKlavyeBitti = false; String webKlavyeMetin;

namespace tara {
using web::Sayfa;
Sayfa sayfa;
web::Video *vid = nullptr; int vidN = 0, vidSec = 0;
char adres[300] = "", sorgu[64] = "";
char gecmis[6][300]; int gecmisUst[6] = {}; int gecmisN = 0;
int gezinme = 0; // 0: okuma, 1: link secimi, 2: fare
int ust = 0, sec = -1;                               // ilk görünen satır, açılacak link
// sayfa görünmez 11x6 kareye bölünür (12x9 px: 2 harf, 1 satır); fare oku kare kare kayarak gider, başlıktaki "≡" menüyü açar
const int IZ_SUT = 11, IZ_SAT = 5, IZ_W = 12, IZ_H = 9, IZ_Y = 10;
int imx = 0, imy = 0; bool imBaslik = false; float okX = 4, okY = 12;   // ok ekranda hedef kareye yumuşakça kayar
enum Mod { SAYFA, YT_LISTE, YT_DETAY, RESIM, KLAVYE_MOD, MENU_MOD, YER_MOD } mod = SAYFA;
bool aktif = false;
uint8_t resim[1024]; int rw = 0, rh = 0; char resimId[12] = "";   // 1 bit görüntü (en çok 128x64)
String hata;
uint32_t sonTus = 0;

const char *MASAUSTU_UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0 Safari/537.36";
const char *MOBIL_UA = "Mozilla/5.0 (Linux; Android 14) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0 Mobile Safari/537.36";

// ---------------------------------------------------------------- girdi: menü düzeni (sağ ileri, sol geri, iki tuş seç, iki tuş 1 sn geri)
struct G { bool ileri, onceki, sec, geri; Olay o1, o2; };   // o1/o2: liste()/klavye için (o1 KISA ileri, o2 KISA seç)
G girdi() {
  Olay o1 = oku(tus1), o2 = oku(tus2);
  tekrarMs = mod == SAYFA ? 55 : 110;               // sayfada fare oku basılı tutunca hızlı gider
  Kontrol k = yeniKontrol(o1, o2, true, false, mod == KLAVYE_MOD);
  if (k.sec) o2 = KISA;
  geriAdim = k.onceki;
  if (o1 != YOK || o2 != YOK || k.onceki || k.geri || tus1.basili || tus2.basili) sonTus = millis();
  return {o1 == KISA, k.onceki, k.sec, k.geri, o1, o2};
}

void yukleniyor(const char *url, const char *ne, uint32_t bayt) {
  ekran.clearDisplay(); baslik(dl("YUKLENIYOR", "LOADING"));
  String a = url; a.replace("https://", ""); a.replace("http://", "");
  yaz(0, 14, 1, a.substring(0, 21)); yaz(0, 24, 1, a.substring(21, 42));
  yaz(0, 38, 1, ne); if (bayt) yaz(0, 48, 1, String(bayt / 1024) + " KB");
  int k = (millis() / 120) % 16; ekran.fillRect(k * 8, 60, 8, 3, W);
  yaz(80, 48, 1, dl("sol:iptal", "L:cancel"));
  ekran.display();
}

// URL'deki boşluk ve UTF-8 baytlarını %XX yapar (Wikipedia linkleri Türkçe harf içerir)
void guvenli(const char *a, char *o, int maks) {
  int k = 0; const char *H = "0123456789ABCDEF";
  for (; *a && k < maks - 4; a++) { uint8_t c = *a; if (c > 127 || c == ' ' || c == '"' || c == '<' || c == '>') { o[k++] = '%'; o[k++] = H[c >> 4]; o[k++] = H[c & 15]; } else o[k++] = c; }
  o[k] = 0;
}

#ifndef SIMULATOR
// ---------------------------------------------------------------- ağ
bool wifiAc() {
  if (WiFi.status() == WL_CONNECTED) { saatAyarla(); return true; }
  Preferences ag; ag.begin("kubit", true); String ad = ag.getString("wifi_ad", ""), sifre = ag.getString("wifi_sifre", ""); ag.end();
  if (!ad.length()) { hata = dl("Wi-Fi kayitli degil: Ayarlar > Wi-Fi", "No saved network: Settings > Wi-Fi"); return false; }
  ekran.clearDisplay(); baslik("Wi-Fi"); yaz(0, 16, 1, dl("baglaniyor:", "connecting:")); yaz(0, 28, 1, ad.substring(0, 21)); ekran.display();
  WiFi.mode(WIFI_STA); WiFi.begin(ad.c_str(), sifre.c_str());
  for (uint32_t t = millis(); WiFi.status() != WL_CONNECTED && millis() - t < 15000;) delay(100);
  if (WiFi.status() != WL_CONNECTED) { hata = dl("Wi-Fi'a baglanamadi (2,4 GHz?)", "Wi-Fi failed (2.4 GHz?)"); return false; }
  saatAyarla();                                      // HTTPS için saat şart
  return true;
}

// besle(veri, n) false dönerse indirme durur. Yönlendirmeleri kendimiz izleriz ki göreli linkler son adrese göre çözülsün.
// u: indirilecek adres; yönlendirilirse son adres buraya yazılır (300 bayt)
template <typename F> bool indir(char *u, const char *ua, uint32_t maks, F besle, String *tur = nullptr) {
  if (!wifiAc()) return false;
  for (int yon = 0; yon < 6; yon++) {
    char url[340]; guvenli(u, url, sizeof url);
    bool tls = strncmp(url, "https://", 8) == 0;
    WiFiClientSecure *gc = nullptr; WiFiClient *c;
    if (tls) { gc = new WiFiClientSecure; gc->useBuiltinCACertBundle(); c = gc; } else c = new WiFiClient;
    HTTPClient h; h.useHTTP10(true); h.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS); h.setTimeout(10000); h.setUserAgent(ua);
    const char *basliklar[] = {"Location", "Content-Type"}; h.collectHeaders(basliklar, 2);
    yukleniyor(u, dl("baglaniyor...", "connecting..."), 0);
    bool tamam = false;
    if (h.begin(*c, url)) {
      h.addHeader("Accept-Language", "tr-TR,tr;q=0.9,en;q=0.6"); h.addHeader("Accept-Encoding", "identity"); h.addHeader("Cookie", "SOCS=CAI; CONSENT=YES+");
      int kod = h.GET();
      if (kod >= 300 && kod < 400 && h.header("Location").length()) {
        char yeni[300]; web::cozumle(u, h.header("Location").c_str(), yeni, sizeof yeni); strcpy(u, yeni);
        h.end(); delete c; continue;
      }
      if (kod == 200) {
        if (tur) *tur = h.header("Content-Type");
        if (tur && tur->startsWith("image/jpeg")) { h.end(); delete c; return true; }
        if (tur && !(tur->startsWith("text/") || tur->startsWith("application/xhtml") || !tur->length())) { hata = dl("Bu dosya turu acilamaz", "Unsupported file type"); h.end(); delete c; return false; }
        WiFiClient *s = h.getStreamPtr(); static uint8_t tampon[1024]; uint32_t toplam = 0, son = millis(), ciz = 0; int beklenen = h.getSize(); bool durdu = false, sorun = false;
        while (h.connected() || s->available()) {
          int a = s->available();
          if (a > 0) { int n = s->read(tampon, min(a, (int)sizeof tampon)); if (n <= 0) continue; toplam += n; son = millis(); if (!besle(tampon, n) || toplam >= maks) { durdu = true; break; } if (beklenen >= 0 && toplam >= (uint32_t)beklenen) break; }
          else { if (millis() - son > 10000) { hata = dl("Indirme zaman asimi", "Download timed out"); sorun = true; break; } delay(2); }
          if (millis() - ciz > 250) { ciz = millis(); yukleniyor(u, dl("indiriliyor", "downloading"), toplam); }
          if (digitalRead(TUS2) == LOW) { hata = dl("iptal edildi", "cancelled"); sorun = true; break; }
        }
        if (!durdu && !sorun && beklenen >= 0 && toplam < (uint32_t)beklenen) { hata = dl("Baglanti kesildi: eksik sayfa", "Connection lost: partial page"); sorun = true; }
        tamam = !sorun;
      } else {
        char t[48] = ""; if (gc) gc->lastError(t, sizeof t);
        hata = kod > 0 ? String("HTTP ") + kod : (*t ? String(t).substring(0, 40) : String(dl("baglanilamadi ", "connect failed ")) + kod);
      }
    } else hata = dl("adres hatali", "bad address");
    h.end(); delete c;
    return tamam;
  }
  hata = dl("cok fazla yonlendirme", "too many redirects"); return false;
}

// ---------------------------------------------------------------- JPEG → 1 bit (en çok mw x mh, oran korunur)
struct Jpg { WiFiClient *s; uint16_t *top; uint8_t *say; int tw, th, sw, sh; };
UINT jpgOku(JDEC *jd, BYTE *b, UINT n) {
  Jpg *j = (Jpg *)jd->device; UINT k = 0; uint32_t son = millis();
  while (k < n && millis() - son < 6000) {
    int a = j->s->available();
    if (a > 0) { int r = b ? j->s->read(b + k, min((int)(n - k), a)) : (j->s->read() >= 0 ? 1 : 0); if (r > 0) { k += r; son = millis(); } }
    else if (!j->s->connected()) break; else delay(1);
  }
  return k;
}
UINT jpgYaz(JDEC *jd, void *v, JRECT *r) {
  Jpg *j = (Jpg *)jd->device; BYTE *p = (BYTE *)v;
  for (int y = r->top; y <= r->bottom; y++) for (int x = r->left; x <= r->right; x++, p += 3) {
    if (x >= j->sw || y >= j->sh) continue;
    int tx = x * j->tw / j->sw, ty = y * j->th / j->sh, i = ty * j->tw + tx;
    j->top[i] += (p[0] * 77 + p[1] * 150 + p[2] * 29) >> 8; if (j->say[i] < 255) j->say[i]++;
  }
  return 1;
}
bool resimIndir(const char *u, int mw, int mh) {     // u'daki JPEG'i resim[]'e çizer
  if (!wifiAc()) return false;
  char url[340]; guvenli(u, url, sizeof url);
  WiFiClientSecure gc; gc.useBuiltinCACertBundle(); WiFiClient duz;
  bool tls = strncmp(url, "https://", 8) == 0;
  HTTPClient h; h.useHTTP10(true); h.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); h.setTimeout(10000); h.setUserAgent(MOBIL_UA);
  yukleniyor(u, dl("resim", "image"), 0);
  if (!h.begin(tls ? (WiFiClient &)gc : duz, url)) { hata = dl("adres hatali", "bad address"); return false; }
  int kod = h.GET();
  if (kod != 200) { hata = String("HTTP ") + kod; h.end(); return false; }
  Jpg j; j.s = h.getStreamPtr();
  void *is = malloc(4000); JDEC jd; bool ok = false;
  if (is && jd_prepare(&jd, jpgOku, is, 4000, &j) == JDR_OK) {
    int s = 0; while (s < 3 && ((int)(jd.width >> s) > mw * 2 || (int)(jd.height >> s) > mh * 2)) s++;
    j.sw = (jd.width + (1 << s) - 1) >> s; j.sh = (jd.height + (1 << s) - 1) >> s;
    if (j.sw * mh > j.sh * mw) { j.tw = mw; j.th = max(1, j.sh * mw / j.sw); } else { j.th = mh; j.tw = max(1, j.sw * mh / j.sh); }
    j.top = (uint16_t *)calloc(j.tw * j.th, 2); j.say = (uint8_t *)calloc(j.tw * j.th, 1);
    if (j.top && j.say && jd_decomp(&jd, jpgYaz, s) == JDR_OK) {
      // gri tonlar: oto seviye + Floyd-Steinberg
      int n = j.tw * j.th, en = 255, ek = 0;
      for (int i = 0; i < n; i++) { j.top[i] = j.say[i] ? j.top[i] / j.say[i] : 0; en = min(en, (int)j.top[i]); ek = max(ek, (int)j.top[i]); }
      int16_t *g = (int16_t *)j.top; int fark = max(1, ek - en);
      for (int i = 0; i < n; i++) g[i] = (j.top[i] - en) * 255 / fark;
      memset(resim, 0, sizeof resim); rw = j.tw; rh = j.th; int bg = (rw + 7) / 8;
      for (int y = 0; y < rh; y++) for (int x = 0; x < rw; x++) {
        int i = y * rw + x, e = g[i], o = e > 127 ? 255 : 0, h2 = e - o;
        if (o) resim[y * bg + x / 8] |= 0x80 >> (x & 7);
        if (x + 1 < rw) g[i + 1] += h2 * 7 / 16;
        if (y + 1 < rh) { if (x) g[i + rw - 1] += h2 * 3 / 16; g[i + rw] += h2 * 5 / 16; if (x + 1 < rw) g[i + rw + 1] += h2 / 16; }
      }
      ok = true;
    } else hata = dl("resim cozulemedi", "decode failed");
    free(j.top); free(j.say);
  } else hata = dl("JPEG desteklenmiyor (progressive?)", "unsupported JPEG");
  free(is); h.end();
  return ok;
}
#else
// ---------------------------------------------------------------- simülatör: ağ yok, örnek sayfalar
const char *DEMO = "<title>Kubit Tarayici (demo)</title><h1>Merhaba!</h1><p>Bu simulatordeki <b>ornek sayfa</b>. Gercek Kubit bu sayfalari Wi-Fi'dan indirir: "
  "<a href=\"/2\">ikinci sayfa</a>, <a href=\"https://tr.wikipedia.org/wiki/Kedi\">Kedi (Vikipedi)</a> ve <a href=\"/3\">uzun yazi</a>.</p>"
  "<ul><li>Sag tus: sonraki link</li><li>Sol tus: linki ac</li><li>Sol uzun: geri</li><li>Iki tus: menu</li></ul><p>&Ccedil;ay &amp; simit &ndash; g\xC3\xBCzel bir g\xC3\xBCn.</p>";
const char *DEMO_YT = "\"videoRenderer\":{\"videoId\":\"abcdefghijk\",\"title\":{\"runs\":[{\"text\":\"Lille - Galatasaray (Ozet)\"}]},\"longBylineText\":{\"runs\":[{\"text\":\"TRT SPOR\"}]},"
  "\"publishedTimeText\":{\"simpleText\":\"4 gun once\"},\"lengthText\":{\"accessibility\":{},\"simpleText\":\"8:05\"},\"viewCountText\":{\"simpleText\":\"4.155.615 goruntuleme\"}"
  "\"videoRenderer\":{\"videoId\":\"bcdefghijkl\",\"title\":{\"runs\":[{\"text\":\"Kubit nasil yapilir? Uzun bir baslik ki ekrana sigmasin\"}]},\"longBylineText\":{\"runs\":[{\"text\":\"Asrin\"}]},"
  "\"lengthText\":{\"simpleText\":\"12:34\"},\"viewCountText\":{\"simpleText\":\"1.234 goruntuleme\"}";
bool wifiAc() { return true; }
template <typename F> bool indir(char *u, const char *ua, uint32_t, F besle, String * = nullptr) {
  const char *v = strstr(u, "youtube") ? DEMO_YT : DEMO;
  if (strstr(u, "/3")) { static String uzun; uzun = "<title>Uzun</title>"; for (int i = 0; i < 40; i++) uzun += "<p>Paragraf " + String(i) + ": lorem ipsum dolor sit amet, kubit kubit.</p>"; v = uzun.c_str(); }
  besle((uint8_t *)v, strlen(v)); return true;
}
bool resimIndir(const char *, int mw, int mh) { rw = min(mw, 64); rh = min(mh, 32); for (int i = 0; i < (int)sizeof resim; i++) resim[i] = (i / 8) % 2 ? 0xAA : 0x55; return true; }
#endif

// ---------------------------------------------------------------- sayfa yükleme
void sayfaYukle(const char *url, bool geriEkle = true) {
  if (geriEkle && adres[0] && sayfa.satirN) { if (gecmisN == 6) { memmove(gecmis[0], gecmis[1], 5 * sizeof gecmis[0]); memmove(gecmisUst, gecmisUst + 1, 5 * sizeof(int)); gecmisN = 5; } strcpy(gecmis[gecmisN], adres); gecmisUst[gecmisN++] = ust; }
  if (url != adres) strncpy(adres, url, sizeof adres - 1); adres[sizeof adres - 1] = 0;
  hata = ""; mod = SAYFA; ust = 0; sec = -1; imx = imy = 0; imBaslik = false;
  web::Html h(sayfa); String tur;
  indir(adres, MOBIL_UA, 600000, [&](uint8_t *v, int n) { for (int i = 0; i < n; i++) h.bayt(v[i]); return !sayfa.dolu(); }, &tur);
  h.bitir();
  if (!sayfa.satirN && !hata.length() && !tur.startsWith("image/jpeg")) hata = dl("Okunabilir metin yok. Site JavaScript gerektirebilir.", "No readable text. Site may require JavaScript.");
  if (tur.startsWith("image/jpeg")) { mod = RESIM; if (!resimIndir(adres, 128, 64)) mod = SAYFA; }
}
void geri() { if (gecmisN) { int i = --gecmisN, kay = gecmisUst[i]; sayfaYukle(gecmis[i], false); ust = constrain(kay, 0, max(0, sayfa.satirN - IZ_SAT)); } else goster(dl("geri yok", "no back"), 1200); }

void ytAra(const char *q) {
  strncpy(sorgu, q, sizeof sorgu - 1);
  char k[200]; web::kodla(q, k, sizeof k);
  static char u[300]; snprintf(u, sizeof u, "https://www.youtube.com/results?search_query=%s&hl=tr&gl=TR", k);
  if (!vid) vid = (web::Video *)malloc(sizeof(web::Video) * web::VIDEO_MAKS);
  if (!vid) { hata = dl("Video listesi icin bellek yok", "Not enough memory"); vidN = 0; mod = YT_LISTE; return; }
  web::YouTube y(vid); hata = "";
  indir(u, MASAUSTU_UA, 2800000, [&](uint8_t *v, int n) { for (int i = 0; i < n; i++) y.bayt(v[i]); return !y.dolu(); });
  vidN = y.n; vidSec = 0; mod = YT_LISTE; resimId[0] = 0;
  if (!vidN && !hata.length()) hata = dl("video bulunamadi", "no videos found");
}
void ytResim() {
  if (!vidN || strcmp(resimId, vid[vidSec].id) == 0) return;
  char u[80]; snprintf(u, sizeof u, "https://i.ytimg.com/vi/%s/mqdefault.jpg", vid[vidSec].id);
  rw = 0; if (resimIndir(u, 80, 45)) strcpy(resimId, vid[vidSec].id);
}

// ---------------------------------------------------------------- klavye (KubitAnahtarlik.ino'daki klavyeSayfasi'nı kullanır; bitince klavyeBitti)
enum Istek { I_YT, I_WEB, I_VIKI, I_ADRES } istek;
Mod klavyeOncesi = SAYFA;
void metinIste(const char *bas, Istek i) {
  webKlavyeBaslik = bas; webKlavyeBitti = false; webKlavyeMetin = ""; klavyeHedef = WEB; yazi = ""; imlec = 3; klavyeKucuk = true;
  istek = i; if (mod != KLAVYE_MOD && mod != MENU_MOD && mod != YER_MOD) klavyeOncesi = mod; mod = KLAVYE_MOD;
}
void ddgAra(const String &q) { char k[200], u[260]; web::kodla(q.c_str(), k, sizeof k); snprintf(u, sizeof u, "https://lite.duckduckgo.com/lite/?q=%s", k); sayfaYukle(u); }
void vikiAra(const String &q) { char k[200], u[260]; web::kodla(q.c_str(), k, sizeof k); snprintf(u, sizeof u, "https://tr.m.wikipedia.org/w/index.php?search=%s", k); sayfaYukle(u); }

// ---------------------------------------------------------------- çizim
int hucreLinki() {                                   // imlecin karesine değen ilk link (-1: yok)
  if (imBaslik) return -1;
  int x0 = imx * IZ_W, x1 = x0 + IZ_W;
  for (int l = 0; l < sayfa.linkN; l++) {
    const web::Link &k = sayfa.link[l];
    for (int s = ust + imy; s < ust + imy + 1 && s < sayfa.satirN; s++) {
      int a = sayfa.satir[s], e = sayfa.satirSonu(s), b = max((int)k.bas, a), son = min((int)k.son, e);
      if (b < son && (b - a) * 6 < x1 && (son - a) * 6 > x0) return l;
    }
  }
  return -1;
}
void satirCiz(int s, int y, int vurgu) {             // vurgu: imlecin altındaki link (ters yazılır), diğer linklerin altı noktalı
  int a = sayfa.satir[s], e = sayfa.satirSonu(s);
  for (int i = a; i < e; i++) ekran.drawChar((i - a) * 6, y, sayfa.metin[i], W, B, 1);
  for (int l = 0; l < sayfa.linkN; l++) {
    const web::Link &k = sayfa.link[l];
    int b = max((int)k.bas, a), son = min((int)k.son, e); if (b >= son) continue;
    if (l == vurgu) { ekran.fillRect((b - a) * 6 - 1, y - 1, (son - b) * 6 + 1, 9, W); for (int i = b; i < son; i++) ekran.drawChar((i - a) * 6, y, sayfa.metin[i], B, W, 1); }
    else for (int x = (b - a) * 6; x < (son - a) * 6 - 1; x += 2) ekran.drawPixel(x, y + 8, W);
  }
}
void imlecCiz(int x, int y) {                        // fare oku: siyah kenarlı beyaz, her zeminde görünür
  static const char *OK[] = {"2       ", "22      ", "212     ", "2112    ", "21112   ", "211112  ", "2111112 ", "2112222 ", "212     ", "22      "};
  for (int j = 0; j < 10; j++) for (int i = 0; i < 8; i++) { char c = OK[j][i]; if (c != ' ') ekran.drawPixel(x + i, y + j, c == '1' ? W : B); }
  ekran.drawPixel(x - 1, y, W);                        // ucu belli olsun
}
void sayfaCiz() {
  String b = sayfa.baslik[0] ? sayfa.baslik : adres; b.trim();
  ekran.fillRect(0, 0, 128, 9, W); ekran.setTextColor(B); yaz(12, 1, 1, b.substring(0, 15));
  int yuzde = sayfa.satirN <= IZ_SAT ? 100 : ust * 100 / max(1, sayfa.satirN - IZ_SAT);
  yaz(104, 1, 1, String(yuzde) + "%"); ekran.setTextColor(W);
  bool menuSecili = gezinme == 1 ? sec < 0 : gezinme == 2 && imBaslik;
  if (menuSecili) ekran.fillRect(0, 0, 11, 9, B);
  for (int i = 0; i < 3; i++) ekran.drawFastHLine(2, 2 + i * 2, 7, menuSecili ? W : B);
  if (hata.length() && !sayfa.satirN) {
    auto s = sar(hata, 21); for (int i = 0; i < (int)s.size() && i < 4; i++) yaz(0, 12 + i * 10, 1, s[i]);
  } else {
    int v = gezinme == 2 ? hucreLinki() : gezinme == 1 ? sec : -1;
    for (int i = 0; i < IZ_SAT && ust + i < sayfa.satirN; i++) satirCiz(ust + i, 11 + i * 9, v);
    if (sayfa.satirN > IZ_SAT) { int h = max(3, 44 * IZ_SAT / sayfa.satirN); ekran.fillRect(127, 10 + (44 - h) * ust / max(1, sayfa.satirN - IZ_SAT), 1, h, W); }
  }
  ekran.fillRect(0, 55, 128, 9, B); ekran.drawFastHLine(0, 54, 128, W);
  String alt = hata.length() ? String(dl("Eksik/hata 2t:menu", "Error/partial 2t:menu")) : gezinme == 0 ? String(dl("sag/sol:oku 2t:menu", "R/L:read both:menu")) : gezinme == 1 ? (sec < 0 ? String(dl("2t:menu sag:link", "both:menu R:link")) : String(sec + 1) + "/" + String(sayfa.linkN) + dl(" 2t:ac", " both:open")) : String(dl("Fare 2t:tikla", "Pointer both:click"));
  yaz(0, 56, 1, alt.substring(0, 21));
  if (gezinme == 2) {
    float hx = imBaslik ? 5 : imx * IZ_W + 4, hy = imBaslik ? 3 : IZ_Y + imy * IZ_H + 3;
    okX += (hx - okX) * .5f; okY += (hy - okY) * .5f;
    imlecCiz(lround(okX), lround(okY));
  }
}
void ytListeCiz() {
  ekran.fillRect(0, 0, 128, 9, W); ekran.setTextColor(B); yaz(1, 1, 1, ("YT: " + String(sorgu)).substring(0, 21)); ekran.setTextColor(W);
  if (!vidN) { auto s = sar(hata, 21); for (int i = 0; i < (int)s.size() && i < 4; i++) yaz(0, 14 + i * 10, 1, s[i]); return; }
  int ilk = constrain(vidSec - 1, 0, max(0, vidN - 3));
  for (int i = ilk; i < ilk + 3 && i < vidN; i++) {
    int y = 11 + (i - ilk) * 18; web::Video &v = vid[i];
    if (i == vidSec) { ekran.fillRect(0, y - 1, 126, 18, W); ekran.setTextColor(B); }
    yaz(1, y, 1, String(v.baslik).substring(0, 21));
    String alt = String(v.kanal).substring(0, v.sure[0] ? 14 : 21); if (v.sure[0]) { while (alt.length() < 15) alt += ' '; alt += v.sure; }
    yaz(1, y + 8, 1, alt.substring(0, 21)); ekran.setTextColor(W);
  }
  ekran.fillRect(127, 10 + 53 * ilk / vidN, 1, max(3, 53 * 3 / vidN), W);
}
void qrCiz(int x0, int y0) {                         // youtu.be/ID: telefon kamerasıyla okut, videoyu aç
#ifndef SIMULATOR
  static int ox, oy;
  ox = x0; oy = y0;
  esp_qrcode_config_t c = ESP_QRCODE_CONFIG_DEFAULT();
  c.max_qrcode_version = 3;
  c.display_func = [](esp_qrcode_handle_t q) {
    int n = esp_qrcode_get_size(q), p = n * 2 + 4 <= 64 ? 2 : 1;
    ekran.fillRect(ox, oy, n * p + 4, n * p + 4, W);
    for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) if (esp_qrcode_get_module(q, x, y)) ekran.fillRect(ox + 2 + x * p, oy + 2 + y * p, p, p, B);
  };
  esp_qrcode_generate(&c, (String("https://youtu.be/") + vid[vidSec].id).c_str());
#else
  ekran.drawRect(x0, y0, 54, 54, W); yaz(x0 + 15, y0 + 23, 1, "QR");
#endif
}
void ytDetayCiz(bool qr) {
  web::Video &v = vid[vidSec];
  if (qr) {
    qrCiz(0, 4);
    auto s = sar(v.baslik, 10); for (int i = 0; i < (int)s.size() && i < 4; i++) yaz(60, 4 + i * 9, 1, s[i]);
    yaz(60, 44, 1, dl("telefonla", "scan with")); yaz(60, 53, 1, dl("okut", "phone"));
    return;
  }
  yaz(0, 0, 1, String(v.kanal).substring(0, 21));
  if (rw) ekran.drawBitmap(0, 10, resim, rw, rh, W); else { ekran.drawRect(0, 10, 80, 45, W); yaz(22, 29, 1, "..."); }
  yaz(84, 11, 1, v.sure[0] ? v.sure : "CANLI");
  String iz = v.izlenme; int b = iz.indexOf(' '); if (b > 0) iz = iz.substring(0, b);
  yaz(84, 21, 1, iz.substring(0, 7)); yaz(84, 30, 1, iz.length() > 7 ? iz.substring(7, 14) : String(dl("izl.", "views")));
  auto t = sar(v.tarih, 7); for (int i = 0; i < (int)t.size() && i < 2; i++) yaz(84, 39 + i * 8, 1, t[i]);
  String bas = String(v.baslik) + "     ";             // kayan başlık
  int kay = (millis() / 180) % bas.length(); String g = (bas + bas).substring(kay, kay + 21);
  yaz(0, 57, 1, g);
}

// ---------------------------------------------------------------- menü
const char *YER_AD[] = {"68k Haber (TR)", "Vikipedi", "Wikipedia (EN)", "Hacker News", "CNN Lite", "NPR Text"};
const char *YER_URL[] = {"http://68k.news/index.php?loc=TR", "https://tr.m.wikipedia.org/", "https://en.m.wikipedia.org/", "https://news.ycombinator.com/", "https://lite.cnn.com/", "https://text.npr.org/"};
void cikis() { webDonus = 0x7A4A; ekran.clearDisplay(); ortala(28, 1, dl("Bluetooth'a donuluyor", "Back to Bluetooth")); ekran.display(); delay(400); ESP.restart(); }
Mod menuOncesi = SAYFA;
void menuAc() { menuOncesi = mod; mod = MENU_MOD; imlec = 0; }
void menuCiz(Olay o1, Olay o2) {
  String og[] = {dl("Onceki sayfa", "Previous page"), dl("Web'de ara", "Search web"), dl("Vikipedi'de ara", "Search Wikipedia"), dl("YouTube'da ara", "Search YouTube"),
    dl("Yer imleri", "Bookmarks"), dl("Adres yaz", "Type address"), dl("Okuma modu", "Reading mode"), dl("Baglantilari sec", "Select links"), dl("Fare modu", "Pointer mode"), dl("Sayfayi yenile", "Reload page"), dl("Sayfanin basi", "Top of page"), dl("Tarayicidan cik", "Exit browser")};
  int s = liste(dl("TARAYICI", "BROWSER"), og, 12, o1, o2);
  if (s < 0) return;
  mod = menuOncesi == YT_DETAY ? YT_LISTE : menuOncesi;
  switch (s) {
    case 0: if (mod == SAYFA) geri(); break;
    case 1: metinIste(dl("WEB'DE ARA", "SEARCH WEB"), I_WEB); break;
    case 2: metinIste(dl("VIKIPEDI'DE ARA", "SEARCH WIKIPEDIA"), I_VIKI); break;
    case 3: metinIste(dl("YOUTUBE'DA ARA", "SEARCH YOUTUBE"), I_YT); break;
    case 4: mod = YER_MOD; imlec = 0; break;
    case 5: metinIste(dl("ADRES: orn 68k.news", "ADDRESS"), I_ADRES); break;
    case 6: mod = SAYFA; gezinme = 0; break;
    case 7: mod = SAYFA; gezinme = 1; sec = -1; if (!sayfa.linkN) goster(dl("Bu sayfada link yok", "No links on page")); break;
    case 8: mod = SAYFA; gezinme = 2; imx = imy = 0; imBaslik = true; break;
    case 9: if (adres[0]) { int kay = ust; sayfaYukle(adres, false); ust = min(kay, max(0, sayfa.satirN - IZ_SAT)); } break;
    case 10: mod = SAYFA; ust = 0; sec = -1; break;
    case 11: cikis();
  }
}
void yerCiz(Olay o1, Olay o2) {
  String y[6]; for (int i = 0; i < 6; i++) y[i] = YER_AD[i];
  int s = liste(dl("YER IMLERI", "BOOKMARKS"), y, 6, o1, o2);
  if (s >= 0) sayfaYukle(YER_URL[s]);
}
void klavyeBitti() {
  String q = webKlavyeMetin; q.trim(); // URL yolu ve arama metninin buyuk/kucuk harflerini koru
  mod = klavyeOncesi;
  if (!q.length()) { if (istek == I_YT && !vidN && !sayfa.satirN) mod = YT_LISTE; return; }
  switch (istek) {
    case I_YT: ytAra(q.c_str()); break;
    case I_WEB: ddgAra(q); break;
    case I_VIKI: vikiAra(q); break;
    case I_ADRES: if (q.indexOf("://") < 0) q = "https://" + q; sayfaYukle(q.c_str()); break;
  }
}

// ---------------------------------------------------------------- sayfa tuşları: imleç kare kare (okuma sırası), sona gelince sayfa kayar
int sonUst() { return max(0, sayfa.satirN - IZ_SAT); }
void imlecIleri() {
  if (imBaslik) { imBaslik = false; imx = imy = 0; return; }
  if (++imx < IZ_SUT) return;
  imx = 0;
  if (++imy < IZ_SAT) return;
  imy = IZ_SAT - 1;
  if (ust < sonUst()) { ust++; okY -= IZ_H; } else imx = IZ_SUT - 1;   // alt kenar: sayfa bir satır kayar (sayfa sonunda son karede kal)
}
void imlecGeri() {
  if (imBaslik) return;
  if (--imx >= 0) return;
  imx = IZ_SUT - 1;
  if (--imy >= 0) return;
  imy = 0;
  if (ust > 0) { ust--; okY += IZ_H; } else { imBaslik = true; imx = 0; }   // üst kenar: sayfa kayar, en üstte başlıktaki menü
}
void linkAc() {
  if (sec < 0 || sec >= sayfa.linkN) return;
  const char *h = sayfa.href(sec); bool res = h[0] == '!'; char tam[300];
  web::cozumle(adres, res ? h + 1 : h, tam, sizeof tam);
  if (res) { hata = ""; mod = resimIndir(tam, 128, 64) ? RESIM : SAYFA; if (mod == SAYFA) goster(hata, 2500); return; }
  if (strstr(tam, "youtube.com/watch") || strstr(tam, "youtu.be/")) goster(dl("video Kubit'te oynamaz", "videos don't play here"), 2000);
  sayfaYukle(tam);
}

// ---------------------------------------------------------------- giriş ve kare (loop() tarayıcı açıkken yalnız kare()'yi çağırır)
void calistir(bool youtube) {                        // Araçlar'dan: Bluetooth kapanır, çıkışta yeniden başlanır
  if (!wifiAdi().length() && WiFi.status() != WL_CONNECTED) { wifiDevam = youtube ? 2 : 1; wifiHal = 0; git(WIFI_S); return; }
  uzunIs = true;                                     // bekçi görevi indirme sürerken "dondu" sanmasın
  ekran.clearDisplay(); ortala(20, 1, dl("Bluetooth kapaniyor", "Bluetooth off")); ortala(32, 1, dl("Wi-Fi aciliyor...", "Wi-Fi on...")); ekran.display();
  NimBLEDevice::deinit(true); bagli = false;
  aktif = true; gezinme = 0; sonTus = millis(); gecmisN = 0; adres[0] = 0;
  if (!sayfa.hazirla()) { hata = "bellek yok"; mod = SAYFA; return; }
  if (!wifiAc()) { ekran.clearDisplay(); baslik("Wi-Fi"); auto s = sar(hata, 21); for (int i = 0; i < (int)s.size() && i < 3; i++) yaz(0, 14 + i * 10, 1, s[i]); ekran.display(); delay(3500); cikis(); }
  if (youtube) { mod = YT_LISTE; metinIste(dl("YOUTUBE'DA ARA", "SEARCH YOUTUBE"), I_YT); }
  else sayfaYukle(YER_URL[0]);
}
bool qr = false;
void kare() {
  if (millis() - sonTus > 300000) cikis();           // 5 dk dokunulmazsa Bluetooth'a dön
  ekran.clearDisplay();
  G g = girdi();
  if (mod == KLAVYE_MOD) {                           // sağ/sol harf gezer, iki tuş yazar, iki tuş 1 sn vazgeçer
    if (g.geri) { webKlavyeMetin = ""; webKlavyeBitti = true; }
    else klavyeSayfasi(g.o1, g.o2);
    ekran.display();
    if (webKlavyeBitti) { webKlavyeBitti = false; klavyeBitti(); }
    delay(15); return;
  }
  bool yukari = false; Mod basta = mod;              // iki tuş 1 sn: bir üst ekrana
  if (g.geri) {
    yukari = true;
    if (mod == YT_DETAY) mod = YT_LISTE;
    else if (mod == RESIM) mod = SAYFA;
    else if (mod == MENU_MOD || mod == YER_MOD) mod = menuOncesi == YT_DETAY ? YT_LISTE : menuOncesi;
    else if (mod == SAYFA && gecmisN) geri();
    else menuAc();
  }
  if (!yukari) switch (mod) {
    case SAYFA:
      if (gezinme == 0) {
        if (g.ileri) ust = min(sonUst(), ust + 1); if (g.onceki) ust = max(0, ust - 1); if (g.sec) menuAc();
      } else if (gezinme == 1) {
        if (g.ileri) sec = sec + 1 >= sayfa.linkN ? -1 : sec + 1;
        if (g.onceki) sec = sec < 0 ? sayfa.linkN - 1 : sec - 1;
        if ((g.ileri || g.onceki) && sec >= 0) { int sat = sayfa.satirBul(sayfa.link[sec].bas); ust = constrain(sat - 1, 0, sonUst()); }
        if (g.sec) { if (sec < 0) menuAc(); else linkAc(); }
      } else {
        if (g.ileri) imlecIleri(); if (g.onceki) imlecGeri();
        if (g.sec) { if (imBaslik) menuAc(); else { sec = hucreLinki(); if (sec >= 0) linkAc(); else goster(dl("burada link yok", "no link here"), 900); } }
      }
      break;
    case RESIM: if (g.sec) mod = SAYFA; break;
    case YT_LISTE:
      if (g.ileri && vidN) vidSec = (vidSec + 1) % vidN;
      if (g.onceki && vidN) vidSec = (vidSec + vidN - 1) % vidN;
      if (g.sec && vidN) { mod = YT_DETAY; qr = false; ytResim(); }
      break;
    case YT_DETAY:
      if (g.ileri) { vidSec = (vidSec + 1) % vidN; qr = false; ytResim(); }
      if (g.onceki) { vidSec = (vidSec + vidN - 1) % vidN; qr = false; ytResim(); }
      if (g.sec) qr = !qr;
      break;
    default: break;
  }
  if (mod == KLAVYE_MOD) return;                     // menüden klavyeye geçildi: sonraki karede çizilir
  ekran.clearDisplay();
  bool tazeMod = yukari || mod != basta;             // bu karede açılan menü aynı tıklamayı "seç" saymasın
  Olay o1 = tazeMod ? YOK : g.o1, o2 = tazeMod ? YOK : g.o2; if (tazeMod) geriAdim = false;
  switch (mod) {
    case SAYFA: sayfaCiz(); break;
    case RESIM: if (rw) ekran.drawBitmap((128 - rw) / 2, (64 - rh) / 2, resim, rw, rh, W); else ortala(28, 1, hata); break;
    case YT_LISTE: ytListeCiz(); break;
    case YT_DETAY: ytDetayCiz(qr); break;
    case MENU_MOD: menuCiz(o1, o2); break;
    case YER_MOD: yerCiz(o1, o2); break;
    default: break;
  }
  if (mod == KLAVYE_MOD) return;
  altMesaj();
  ekran.display();
  delay(15);
}
}  // namespace tara
