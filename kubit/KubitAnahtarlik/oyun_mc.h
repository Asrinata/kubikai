// Kübitcraft: yandan görünüşlü 2B Minecraft. Dünya 256x32 blok (8x8 piksel), 4 bölge halinde RLE ile NVS'e kaydedilir (yalnız değişen bölge yazılır).
// Sol/sağ basılı: yürü (1 blok basamağa kendi zıplar). Sol/sağ çift tık: o yandaki canavara vur ya da bloğu kır. Tek tık: o yana seçili şeyi koy
// (havadayken ayağının altına: kule; tohum ekilir, ekmek yenir). İki tuş tık: zıpla/yüz. İki tuş ~0.5 sn: ayağının altını kaz. İki tuş 1 sn: menü.
// Gece zombi ve creeper çıkar; mağaralar karanlık, meşale aydınlatır; su ve lav akar; buğday büyür; sandıklar ortak depo; üretim masasıyla alet yap.
#pragma once

namespace mc {
enum Esya : uint8_t { HAVA, CIMEN, TOPRAK, TAS, KOMUR, DEMIR, ELMAS, KUTUK, YAPRAK, TAHTA, KUM, ANAKAYA, MESALE, SU, LAV, MASA, SANDIK, EKIN1, EKIN2, EKIN3,
                      BLOK_N, CUBUK = BLOK_N, TOHUM, BUGDAY, EKMEK, KAZMA_A, KAZMA_T, KAZMA_D, KILIC, ESYA_N };
const char *AD[ESYA_N] = {"Hava", "Cimen", "Toprak", "Tas", "Komur", "Demir", "Elmas", "Kutuk", "Yaprak", "Tahta", "Kum", "Ana kaya", "Mesale", "Su", "Lav",
                          "Uretim masasi", "Sandik", "Ekin", "Ekin", "Bugday", "Cubuk", "Tohum", "Bugday", "Ekmek", "Ahsap kazma", "Tas kazma", "Demir kazma", "Kilic"};
const float SERTLIK[BLOK_N] = {0, .35, .3, .8, 1.0, 1.2, 1.5, .7, .15, .6, .3, -1, .05, -1, -1, .6, .6, .05, .05, .05};   // sn, -1 kırılmaz
const uint8_t DUSEN[BLOK_N] = {HAVA, TOPRAK, TOPRAK, TAS, KOMUR, DEMIR, ELMAS, KUTUK, HAVA, TAHTA, KUM, HAVA, MESALE, HAVA, HAVA, MASA, SANDIK, TOHUM, TOHUM, BUGDAY};
const uint8_t DOKU[ESYA_N][8] = {                    // 8x8, en soldaki piksel en yüksek bit
  {0}, {0xFF, 0xDD, 0x44, 0x00, 0x20, 0x02, 0x40, 0x08}, {0x00, 0x22, 0x00, 0x88, 0x00, 0x21, 0x00, 0x84}, {0xF7, 0x81, 0x89, 0xC1, 0x9D, 0x81, 0x91, 0xFF},
  {0x00, 0x66, 0x66, 0x00, 0x0C, 0x6C, 0x60, 0x00}, {0x00, 0x60, 0x50, 0x36, 0x05, 0x63, 0x50, 0x30}, {0x10, 0x28, 0x12, 0x85, 0x42, 0xA0, 0x44, 0x0A},
  {0x92, 0x92, 0xA2, 0xA4, 0x94, 0x92, 0xA2, 0xA4}, {0x00, 0x24, 0x5A, 0x24, 0x00, 0x42, 0xA5, 0x42}, {0xFF, 0x00, 0x10, 0xFF, 0x00, 0x01, 0xFF, 0x00},
  {0x80, 0x08, 0x21, 0x02, 0x88, 0x20, 0x04, 0x40}, {0xDB, 0xB6, 0x6D, 0xDB, 0xB6, 0x6D, 0xDB, 0xB6},
  {0x00, 0x18, 0x3C, 0x18, 0x10, 0x10, 0x10, 0x10},   // meşale
  {0x00, 0x00, 0x6C, 0x92, 0x00, 0x36, 0x49, 0x00},   // su: dalga
  {0x6E, 0xFB, 0xDF, 0x76, 0xEF, 0xBD, 0xF7, 0x5E},   // lav: kabarcık
  {0xFF, 0xDB, 0xFF, 0x81, 0xBD, 0xA5, 0xA5, 0xFF},   // üretim masası
  {0xFF, 0x81, 0x81, 0xFF, 0x99, 0x81, 0x81, 0xFF},   // sandık
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24}, {0x00, 0x00, 0x00, 0x24, 0x5A, 0x24, 0x24, 0x24}, {0x52, 0xA5, 0x5A, 0x24, 0x5A, 0x24, 0x24, 0x24},
  {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, {0x00, 0x24, 0x00, 0x90, 0x02, 0x40, 0x12, 0x00}, {0x24, 0x5A, 0x24, 0x5A, 0x24, 0x18, 0x18, 0x18},
  {0x00, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x00, 0x00}, {0x7E, 0x81, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, {0x7E, 0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
  {0xFF, 0xFF, 0x99, 0x18, 0x18, 0x18, 0x18, 0x18}, {0x01, 0x03, 0x06, 0x6C, 0x38, 0x30, 0x48, 0x80}};
const uint8_t KAPALI[8] = {0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00};
const uint8_t ZOMBI_S[2][14] = {{0x7E, 0x7E, 0x5A, 0x7E, 0x66, 0x3C, 0xFF, 0xBD, 0xBD, 0x3C, 0x24, 0x24, 0x24, 0x66}, {0x7E, 0x7E, 0x5A, 0x7E, 0x66, 0x3C, 0xFF, 0xBD, 0xBD, 0x3C, 0x24, 0x42, 0x42, 0xC3}};
const uint8_t CREEPER_S[14] = {0x7E, 0x42, 0x66, 0x7E, 0x5A, 0x5A, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x7E, 0x66, 0x66};
const int DW = 256, DH = 32, BOLGE = 64, CAN_MAX = 10;
const float OW = .6, OH = 1.75, YURU = 4.3, YERCEKIMI = 28, ZIPLA = 8.6;
struct Tarif { uint8_t cikti, adet, g1, n1, g2, n2; bool masa; };
const Tarif TARIF[] = {{TAHTA, 4, KUTUK, 1, 0, 0, false}, {CUBUK, 4, TAHTA, 2, 0, 0, false}, {MESALE, 4, CUBUK, 1, KOMUR, 1, false}, {MASA, 1, TAHTA, 4, 0, 0, false},
                       {SANDIK, 1, TAHTA, 8, 0, 0, true}, {KAZMA_A, 1, TAHTA, 3, CUBUK, 2, true}, {KAZMA_T, 1, TAS, 3, CUBUK, 2, true}, {KAZMA_D, 1, DEMIR, 3, CUBUK, 2, true},
                       {KILIC, 1, DEMIR, 2, CUBUK, 1, true}, {EKMEK, 1, BUGDAY, 3, 0, 0, false}};
const int TARIF_N = sizeof(TARIF) / sizeof(*TARIF);
struct Varlik { float x, y, vy; int yon; bool yerde; };
struct Canavar { Varlik v; uint8_t tur; int8_t can; float t, vurus; bool var; };   // tur 0 zombi, 1 creeper; t: creeper fitili
uint8_t d[DH][DW];
Varlik o; float dogX, dogY;
uint16_t env[ESYA_N], sandik[ESYA_N]; uint8_t secili; int can; float sonHasar, yenilenme, lavT;
float gun, kamX, kamY, akisT, ekinT, dogurT, otoKayitT;
bool kirli[DW / BOLGE];
Canavar cv[6];
struct { int x, y; float t, sure; bool aktif; } kir;
struct Taraf { bool yuruyor, bekliyor; uint32_t sonTik; } T[2];
String bilgi; uint32_t bilgiBitis;
bool yaratici = false, bitirdi = false, menuYut = false, dunyaAcik = false; uint32_t gorevler = 0, dunyaTohum = 0; int gunSayisi = 1, yardimDonus = 0; bool yeniYaratici = false;
int hal, imlec; bool menuAcildi;                     // hal: 0 başlık, 1 oyun, 2 menü, 3 üretim, 4 sandık
const int LX = 26, LY = 13; uint8_t isik[LY][LX]; int isikX0, isikY0;

void soyle(const String &s) { bilgi = s; bilgiBitis = millis() + 1300; }
bool icinde(int x, int y) { return x >= 0 && x < DW && y >= 0 && y < DH; }
bool gecirgen(uint8_t b) { return b == HAVA || b == MESALE || b == SU || b == LAV || b == EKIN1 || b == EKIN2 || b == EKIN3; }
bool kati(int x, int y) { if (x < 0 || x >= DW || y >= DH) return true; if (y < 0) return false; return !gecirgen(d[y][x]); }
bool opak(int x, int y) { return kati(x, y) && !(icinde(x, y) && d[y][x] == YAPRAK); }
void koyBlok(int x, int y, uint8_t b) { if (icinde(x, y)) { d[y][x] = b; kirli[x / BOLGE] = true; } }
bool carpar(float x, float y) {
  for (int yy = (int)floorf(y); yy <= (int)floorf(y + OH - .001f); yy++)
    for (int xx = (int)floorf(x); xx <= (int)floorf(x + OW - .001f); xx++) if (kati(xx, yy)) return true;
  return false;
}
uint8_t icindeki(const Varlik &v) { int x = (int)floorf(v.x + OW / 2), y = (int)floorf(v.y + OH - .3f); return icinde(x, y) ? d[y][x] : HAVA; }
bool oyuncuyaDeger(int x, int y) { return x + 1 > o.x && x < o.x + OW && y + 1 > o.y && y < o.y + OH; }
int merkezX() { return (int)floorf(o.x + OW / 2); }
int kazma() { return env[KAZMA_D] ? 3 : env[KAZMA_T] ? 2 : env[KAZMA_A] ? 1 : 0; }
bool masaYakin() { for (int y = (int)o.y - 3; y <= (int)o.y + 4; y++) for (int x = merkezX() - 4; x <= merkezX() + 4; x++) if (icinde(x, y) && d[y][x] == MASA) return true; return false; }
bool sandikYakin() { for (int y = (int)o.y - 2; y <= (int)o.y + 3; y++) for (int x = merkezX() - 3; x <= merkezX() + 3; x++) if (icinde(x, y) && d[y][x] == SANDIK) return true; return false; }
bool gece() { return gun > .6f; }

void dog() { o = {dogX, dogY, 0, 1, false}; while (carpar(o.x, o.y) && o.y > 0) o.y -= 1; }
void uret(uint32_t tohum) {                          // yüzey: üç sinüs; toprak, taş, maden, mağara, göl, lav cebi, ağaç
  dunyaTohum = tohum; gorevler = 0; gunSayisi = 1; yaratici = bitirdi = false;
  sonHasar = 10; yenilenme = lavT = akisT = ekinT = dogurT = otoKayitT = 0;
  randomSeed(tohum);
  float a = random(628) / 100.0, b = random(628) / 100.0, c = random(628) / 100.0;
  static int yuzey[DW];
  for (int x = 0; x < DW; x++) yuzey[x] = 15 + lroundf(3 * sinf(x * .09f + a) + 2 * sinf(x * .23f + b) + 1.2f * sinf(x * .5f + c));
  for (int x = 0; x < DW; x++) for (int y = 0; y < DH; y++) {
    int s = yuzey[x]; bool kumsal = s >= 18;
    d[y][x] = y < s ? HAVA : y == s ? (kumsal ? KUM : CIMEN) : y <= s + 3 ? (kumsal && y <= s + 2 ? KUM : TOPRAK) : TAS;
    if (y == DH - 1 || (y == DH - 2 && random(2))) d[y][x] = ANAKAYA;
  }
  for (int x = 1; x < DW - 1; x++) if (yuzey[x] >= 18) for (int y = 16; y < yuzey[x]; y++) d[y][x] = SU;   // çukurlarda göl
  for (int i = 0; i < 8; i++) {                      // mağaralar
    float x = random(10, DW - 10), y = random(19, DH - 4), aci = random(628) / 100.0;
    for (int k = 0; k < 45; k++) {
      for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) { int xx = x + dx, yy = y + dy; if (icinde(xx, yy) && yy < DH - 2 && yy > yuzey[xx] + 2 && abs(dx) + abs(dy) < 2) d[yy][xx] = HAVA; }
      aci += random(-60, 61) / 100.0; x += cosf(aci); y += sinf(aci) * .6f; y = constrain(y, 18.0f, DH - 4.0f);
    }
  }
  for (int x = 0; x < DW; x++) for (int y = 0; y < DH - 1; y++) if (d[y][x] == TAS) {
    int r = random(1000);
    uint8_t m = y > 25 && r < 12 ? ELMAS : y > 19 && r < 40 ? DEMIR : r < 75 ? KOMUR : TAS;
    if (m != TAS) { d[y][x] = m; if (x + 1 < DW && d[y][x + 1] == TAS && random(2)) d[y][x + 1] = m; }
  }
  for (int i = 0; i < 6; i++) { int x = random(4, DW - 4), y = random(26, DH - 2); for (int k = 0; k < 4; k++) if (icinde(x + k, y) && d[y][x + k] != ANAKAYA) d[y][x + k] = LAV; }   // derinde lav
  int son = -9;
  for (int x = 3; x < DW - 3; x++) {
    int s = yuzey[x];
    if (d[s][x] != CIMEN || x - son < 5 || random(5)) continue;
    son = x; int boy = random(3, 5);
    for (int k = 1; k <= boy; k++) d[s - k][x] = KUTUK;
    for (int dy = -1; dy <= 1; dy++) for (int dx = -2; dx <= 2; dx++) { int xx = x + dx, yy = s - boy + dy; if (icinde(xx, yy) && d[yy][xx] == HAVA && !(abs(dx) == 2 && dy == -1)) d[yy][xx] = YAPRAK; }
    if (s - boy - 2 >= 0) d[s - boy - 2][x] = YAPRAK;
  }
  dogX = DW / 2.0f; dogY = 0; o = {dogX, 0, 0, 1, false};
  while (!carpar(o.x, o.y + .5f) && o.y < DH) o.y += .5f;
  dogY = o.y;
  memset(env, 0, sizeof(env)); memset(sandik, 0, sizeof(sandik)); for (auto &c : cv) c.var = false;
  secili = TOPRAK; gun = .1; can = CAN_MAX; kir.aktif = false;
  for (auto &k : kirli) k = true;
  randomSeed(esp_random());
}

// ---- kayıt: her bölge (64 sütun) ayrı RLE (adet, blok) çiftleri; durum ayrı
struct Kayit { uint8_t surum; float x, y, gun, dogX, dogY; int8_t can; uint8_t secili; uint16_t env[ESYA_N], sandik[ESYA_N]; };
bool kayitVar() { return kayit.getInt("mc_var", 0); }
bool kaydet() {
  static uint8_t paket[BOLGE * DH * 2];
  bool tamam = true;
  for (int b = 0; b < DW / BOLGE; b++) {
    if (!kirli[b]) continue;
    int n = 0; uint8_t son = 255, adet = 0;
    for (int x = b * BOLGE; x < (b + 1) * BOLGE; x++) for (int y = 0; y < DH; y++) {
      uint8_t k = d[y][x];
      if (k == son && adet < 255) adet++; else { if (adet) { paket[n++] = adet; paket[n++] = son; } son = k; adet = 1; }
    }
    paket[n++] = adet; paket[n++] = son;
    char ad[8]; snprintf(ad, 8, "mc_b%d", b);
    if (kayit.putBytes(ad, paket, n) == (size_t)n) kirli[b] = false; else tamam = false;
  }
  Kayit k = {2, o.x, o.y, gun, dogX, dogY, (int8_t)can, secili, {}, {}};
  memcpy(k.env, env, sizeof(env)); memcpy(k.sandik, sandik, sizeof(sandik));
  tamam &= kayit.putBytes("mc_s2", &k, sizeof(k)) == sizeof(k);
  struct Ek { uint32_t sihir, gorev, tohum, gun; uint8_t mod, bitti; } ek = {0x4D433234, gorevler, dunyaTohum, (uint32_t)gunSayisi, (uint8_t)yaratici, (uint8_t)bitirdi};
  tamam &= kayit.putBytes("mc_ek", &ek, sizeof(ek)) == sizeof(ek);
  kayit.putInt("mc_var", tamam ? 2 : 0);
  if (!tamam) soyle("Kayit yeri dolu!"); return tamam;
}
bool yukle() {
  static uint8_t paket[BOLGE * DH * 2];
  int surum = kayit.getInt("mc_var", 0);
  if (surum == 1) {                                  // eski 128 sütunluk dünya: sola yerleşir, sağı yeniden üretilir
    struct Eski { float x, y, gun; uint16_t env[12]; uint8_t secili; } e;
    if (kayit.getBytes("mc_d", paket, DH * 128 / 2) != DH * 128 / 2 || kayit.getBytes("mc_s", &e, sizeof(e)) != sizeof(e)) return false;
    uret(esp_random());
    for (int i = 0; i < 128 * DH; i += 2) { d[i / 128][i % 128] = paket[i / 2] >> 4; d[(i + 1) / 128][(i + 1) % 128] = paket[i / 2] & 15; }
    o = {e.x, e.y, 0, 1, false}; dogX = e.x; dogY = e.y; gun = e.gun; memcpy(env, e.env, sizeof(e.env)); secili = e.secili;
    return true;
  }
  Kayit k;
  if (surum != 2 || kayit.getBytes("mc_s2", &k, sizeof(k)) != sizeof(k)) return false;
  if (k.surum != 2 || !isfinite(k.x) || !isfinite(k.y) || !isfinite(k.gun) || !isfinite(k.dogX) || !isfinite(k.dogY) || k.x < 0 || k.x > DW - OW || k.y < -4 || k.y > DH || k.dogX < 0 || k.dogX > DW - OW || k.dogY < -4 || k.dogY > DH || k.secili >= ESYA_N || k.gun < 0 || k.gun >= 1) return false;
  // Once tum bolgeleri dogrula; bozuk kaydi canli dunyaya yari yukleme.
  for (int b = 0; b < DW / BOLGE; b++) {
    char ad[8]; snprintf(ad, 8, "mc_b%d", b); int n = kayit.getBytes(ad, paket, sizeof(paket)), toplam = 0;
    if (!n || n % 2) return false;
    for (int p = 0; p < n; p += 2) { if (!paket[p] || paket[p + 1] >= BLOK_N) return false; toplam += paket[p]; }
    if (toplam != BOLGE * DH) return false;
  }
  for (int b = 0; b < DW / BOLGE; b++) {
    char ad[8]; snprintf(ad, 8, "mc_b%d", b);
    int n = kayit.getBytes(ad, paket, sizeof(paket)), i = 0;
    for (int p = 0; p + 1 < n; p += 2) for (int a = 0; a < paket[p]; a++, i++) { int x = b * BOLGE + i / DH, y = i % DH; if (x < (b + 1) * BOLGE) d[y][x] = paket[p + 1] < BLOK_N ? paket[p + 1] : HAVA; }
    kirli[b] = false;
  }
  o = {k.x, k.y, 0, 1, false}; gun = k.gun; dogX = k.dogX; dogY = k.dogY; can = k.can > 0 ? k.can : CAN_MAX; secili = k.secili;
  memcpy(env, k.env, sizeof(env)); memcpy(sandik, k.sandik, sizeof(sandik)); kir.aktif = false; for (auto &c : cv) c.var = false;
  yaratici = bitirdi = false; gorevler = dunyaTohum = 0; gunSayisi = 1;
  struct Ek { uint32_t sihir, gorev, tohum, gun; uint8_t mod, bitti; } ek;
  if (kayit.getBytes("mc_ek", &ek, sizeof(ek)) == sizeof(ek) && ek.sihir == 0x4D433234 && ek.mod <= 1 && ek.bitti <= 1) {
    gorevler = ek.gorev & 255; dunyaTohum = ek.tohum; gunSayisi = constrain((int)ek.gun, 1, 99999); yaratici = ek.mod; bitirdi = ek.bitti;
  }
  sonHasar = 10; yenilenme = lavT = akisT = ekinT = dogurT = otoKayitT = 0;
  return true;
}

// ---- ışık: gökyüzü (sütunda ilk opak bloğa kadar) + meşale + lav + oyuncunun yanı, havadan yayılır
void isikHesapla(int bx0, int by0) {
  isikX0 = bx0 - 5; isikY0 = by0 - 2;
  int gok = gece() ? 3 : 6;
  for (int i = 0; i < LX; i++) {
    int x = isikX0 + i, tepe = 0;
    while (tepe < DH && !opak(x, tepe)) tepe++;
    for (int j = 0; j < LY; j++) {
      int y = isikY0 + j; uint8_t v = y <= tepe ? gok : 0;
      if (icinde(x, y)) { if (d[y][x] == MESALE) v = max(v, (uint8_t)7); if (d[y][x] == LAV) v = max(v, (uint8_t)5); }
      isik[j][i] = v;
    }
  }
  int pi = merkezX() - isikX0, pj = (int)floorf(o.y + 1) - isikY0;
  if (pi >= 0 && pi < LX && pj >= 0 && pj < LY) isik[pj][pi] = max(isik[pj][pi], (uint8_t)3);
  for (int tur = 0; tur < 6; tur++) for (int j = 0; j < LY; j++) for (int i = 0; i < LX; i++) {
    uint8_t en = isik[j][i];
    const int8_t K[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto &k : K) { int a = i + k[0], b = j + k[1]; if (a < 0 || b < 0 || a >= LX || b >= LY || opak(isikX0 + a, isikY0 + b)) continue; if (isik[b][a] > 0) en = max(en, (uint8_t)(isik[b][a] - 1)); }
    isik[j][i] = en;
  }
}
uint8_t isikAl(int x, int y) { int i = x - isikX0, j = y - isikY0; return i < 0 || j < 0 || i >= LX || j >= LY ? 0 : isik[j][i]; }

// ---- su ve lav: hacim korunur, aşağı düşer, yoksa yana kayar; lav suya değince taş olur
void akis(int x0, int x1) {
  for (int y = DH - 2; y >= 0; y--) for (int x = max(1, x0); x < min(DW - 1, x1); x++) {
    uint8_t s = d[y][x]; if (s != SU && s != LAV) continue;
    if (s == LAV) for (int k = -1; k <= 1; k += 2) if (d[y][x + k] == SU || d[y + 1][x] == SU || (y > 0 && d[y - 1][x] == SU)) { koyBlok(x, y, TAS); s = TAS; break; }
    if (s == TAS) continue;
    if (s == LAV && random(3)) continue;             // lav yavaş akar
    if (d[y + 1][x] == HAVA) { koyBlok(x, y + 1, s); koyBlok(x, y, HAVA); continue; }
    int yon = random(2) ? 1 : -1;
    for (int k = 0; k < 2; k++, yon = -yon) if (d[y][x + yon] == HAVA && (d[y + 1][x + yon] == HAVA || random(8) == 0)) { koyBlok(x + yon, y, s); koyBlok(x, y, HAVA); break; }
  }
}

// ---- eylemler
Canavar *canavarYanda(int yon) {
  for (auto &c : cv) if (c.var) { float dx = (c.v.x - o.x) * yon; if (dx > -.2f && dx < 1.7f && fabsf(c.v.y - o.y) < 1.5f) return &c; }
  return nullptr;
}
bool hedefKir(int yon, int &hx, int &hy) {
  int cx = merkezX(), kafa = (int)floorf(o.y + .1f), ayak = (int)floorf(o.y + OH - .01f), sx = cx + yon;
  const int aday[4][2] = {{sx, kafa}, {sx, ayak}, {sx, ayak + 1}, {cx, kafa - 1}};
  for (auto &a : aday) if (icinde(a[0], a[1]) && d[a[1]][a[0]] != HAVA && d[a[1]][a[0]] != SU && d[a[1]][a[0]] != LAV) { hx = a[0]; hy = a[1]; return true; }
  return false;
}
void kirBasla(int x, int y) {
  if (!icinde(x, y) || d[y][x] == HAVA) return;
  uint8_t b = d[y][x];
  if (SERTLIK[b] < 0) { if (b == ANAKAYA) soyle("Ana kaya kirilmaz"); return; }
  bool tas = b == TAS || b == KOMUR || b == DEMIR || b == ELMAS;
  kir = {x, y, 0, yaratici ? .06f : SERTLIK[b] / (tas ? 1 + kazma() : 1), true};
}
void kirBitir() {                                     // düşen eşya: kazma yetmiyorsa taş/maden bir şey vermez
  uint8_t b = d[kir.y][kir.x], dus = DUSEN[b]; int gerek = b == ELMAS ? 3 : b == DEMIR ? 2 : (b == TAS || b == KOMUR) ? 1 : 0;
  koyBlok(kir.x, kir.y, HAVA); kir.aktif = false;
  if (!yaratici && kazma() < gerek) { soyle(gerek == 1 ? "Kazma lazim" : gerek == 2 ? "Tas kazma lazim" : "Demir kazma lazim"); return; }
  if (b == CIMEN && random(10) < 3) { env[TOHUM]++; }
  if (b == EKIN3) env[TOHUM] += 1 + random(2);
  if (b == ELMAS) sayac("b_elmas", 1);
  if (dus != HAVA) { if (!env[dus] && !env[secili]) secili = dus; env[dus]++; soyle("+1 " + String(AD[dus])); }
}
void blokKoy(int x, int y) {
  if (!env[secili]) { for (int i = 1; i < ESYA_N; i++) if (env[i] && (i < BLOK_N || i == TOHUM || i == EKMEK)) { secili = i; break; } }
  if (!env[secili]) { soyle("Elin bos: once kir"); return; }
  if (secili == EKMEK) { if (can < CAN_MAX) { env[EKMEK]--; can = min(CAN_MAX, can + 4); soyle("Ekmek: +4 can"); } else soyle("Karnin tok"); return; }
  if (!icinde(x, y) || !(d[y][x] == HAVA || d[y][x] == SU || d[y][x] == LAV) || oyuncuyaDeger(x, y)) return;
  if (secili == TOHUM) { if (y + 1 < DH && (d[y + 1][x] == TOPRAK || d[y + 1][x] == CIMEN) && d[y][x] == HAVA) { koyBlok(x, y, EKIN1); env[TOHUM]--; } else soyle("Tohum topraga ekilir"); return; }
  if (secili >= BLOK_N) { soyle("Blok/tohum/ekmek sec"); return; }
  koyBlok(x, y, secili); env[secili]--;
}
void koy(int yon) {
  if (!o.yerde && icindeki(o) != SU) { blokKoy(merkezX(), (int)ceilf(o.y + OH)); return; }
  int cx = merkezX(), sx = cx + yon, ayak = (int)floorf(o.y + OH - .01f), kafa = (int)floorf(o.y + .1f);
  auto bos = [&](int x, int y) { return icinde(x, y) && (d[y][x] == HAVA || d[y][x] == SU) && !oyuncuyaDeger(x, y); };
  if (secili == EKMEK) blokKoy(0, 0);
  else if (bos(sx, ayak)) blokKoy(sx, ayak); else if (bos(sx, kafa)) blokKoy(sx, kafa);
}
void vur(Canavar &c, int yon) {
  c.can -= env[KILIC] ? 5 : 2; c.v.vy = -5; c.v.x += yon * .6f; c.t = 0;
  if (c.can <= 0) { c.var = false; soyle(c.tur ? "Creeper yok edildi" : "Zombi yok edildi"); }
}
void tik(int i) {
  int yon = i ? 1 : -1; o.yon = yon;
  if (!o.yerde && icindeki(o) != SU) { koy(yon); return; }
  uint32_t t = millis();
  if (T[i].bekliyor && t - T[i].sonTik < 320) {
    T[i].bekliyor = false;
    if (Canavar *c = canavarYanda(yon)) { vur(*c, yon); return; }
    int hx, hy; if (hedefKir(yon, hx, hy)) kirBasla(hx, hy);
  } else { T[i].bekliyor = true; T[i].sonTik = t; }
}
void hasar(int h, const char *neden) {
  if (h <= 0 || yaratici) return;
  can -= h; sonHasar = 0;
  if (can <= 0) { can = CAN_MAX; dog(); soyle(String(neden) + ": oldun, dogdun"); for (auto &c : cv) c.var = false; }
}

void fizik(Varlik &v, float dt, int yuru, float hiz, bool oyuncu) {
  if (yuru) v.yon = yuru;
  bool suda = icindeki(v) == SU;
  float nx = constrain(v.x + yuru * hiz * (suda ? .55f : 1) * dt, 0.0f, DW - OW);
  if (!carpar(nx, v.y)) v.x = nx;
  else if (yuru) {
    int sx = yuru > 0 ? (int)floorf(nx + OW - .001f) : (int)floorf(nx), ayak = (int)floorf(v.y + OH - .01f), mx = (int)floorf(v.x + OW / 2);
    if (v.yerde && kati(sx, ayak) && !kati(sx, ayak - 1) && !kati(sx, ayak - 2) && !kati(mx, ayak - 2)) { v.vy = -ZIPLA * .97f; v.yerde = false; }
    v.x = yuru > 0 ? sx - OW - .0005f : sx + 1 + .0005f;
  }
  float ilk = v.vy;
  v.vy = min(v.vy + YERCEKIMI * (suda ? .3f : 1) * dt, suda ? 3.0f : 18.0f);
  float ny = v.y + v.vy * dt;
  v.yerde = false;
  if (!carpar(v.x, ny)) v.y = ny;
  else {
    if (v.vy > 0) { v.y = floorf(ny + OH - .001f) - OH; v.yerde = true; if (oyuncu && ilk > 14 && !suda) hasar((int)(ilk - 13), "Dustun"); }
    else v.y = floorf(ny) + 1;
    v.vy = 0;
  }
  if (v.y > DH) { v.y = 0; v.vy = 0; }
}

void canavarlar(float dt) {
  if (yaratici) { for (auto &c : cv) c.var = false; return; }
  dogurT -= dt;
  bool karanlik = gece();
  if (dogurT <= 0) {                                  // gece yüzeyde, her zaman karanlık mağarada doğar
    dogurT = 3.5f; int say = 0; for (auto &c : cv) say += c.var;
    if (say < 4) for (auto &c : cv) if (!c.var) {
      int x = constrain(merkezX() + (random(2) ? 1 : -1) * (int)random(9, 15), 1, DW - 2), y = 0;
      while (y < DH - 1 && !kati(x, y + 1)) y++;
      bool yuzey = y < 20, isikli = isikAl(x, y) > 2;
      if (yuzey ? !karanlik : isikli) break;
      if (y < 2 || d[y][x] != HAVA || d[y - 1][x] != HAVA) break;
      c = {{(float)x, y + 1 - OH, 0, 1, false}, (uint8_t)(random(3) == 0), (int8_t)(random(3) == 0 ? 6 : 8), 0, 0, true};
      if (c.tur) c.can = 6;
      break;
    }
  }
  for (auto &c : cv) {
    if (!c.var) continue;
    float dx = o.x - c.v.x, dy = o.y - c.v.y, uz = sqrtf(dx * dx + dy * dy);
    if (uz > 26) { c.var = false; continue; }
    if (!karanlik && c.tur == 0 && c.v.y < 18 && isikAl((int)c.v.x, (int)c.v.y) >= 5 && (c.vurus -= dt) <= 0) { c.vurus = 1; if (--c.can <= 0) c.var = false; }   // zombi güneşte yanar
    int yuru = fabsf(dx) > .4f ? (dx > 0 ? 1 : -1) : 0;
    if (c.tur == 1 && uz < 1.6f) yuru = 0;
    fizik(c.v, dt, yuru, c.tur ? 2.4f : 2.0f, false);
    if (c.tur == 0) {
      if ((c.vurus -= dt) <= 0 && fabsf(dx) < .7f && fabsf(dy) < 1.2f) { c.vurus = 1; hasar(2, "Zombi"); o.vy = -4; o.x += (dx > 0 ? .5f : -.5f); }
    } else {
      if (uz < 2) c.t += dt; else c.t = max(0.0f, c.t - dt);
      if (c.t > 1.5f) {                                // patlama: çevredeki blokları siler, mesafeye göre hasar
        int cx = (int)(c.v.x + OW / 2), cy = (int)(c.v.y + 1);
        for (int y = cy - 2; y <= cy + 2; y++) for (int x = cx - 2; x <= cx + 2; x++) if (icinde(x, y) && (x - cx) * (x - cx) + (y - cy) * (y - cy) <= 4 && d[y][x] != ANAKAYA) koyBlok(x, y, HAVA);
        hasar(max(0, (int)(7 - uz * 2.5f)), "Creeper"); c.var = false; soyle("BOOM!");
      }
    }
  }
}

// ---- çizim
void oyuncuCiz(int sx, int sy, uint32_t t, bool yuruyor) {
  ekran.fillRect(sx, sy, 6, 6, W); ekran.drawPixel(sx + (o.yon > 0 ? 4 : 1), sy + 2, B); ekran.drawPixel(sx + (o.yon > 0 ? 5 : 0), sy + 2, B);
  ekran.drawRect(sx, sy + 6, 6, 5, W);
  int kol = kir.aktif ? ((t / 90) % 2 ? 2 : 0) : 0;
  ekran.drawLine(sx + (o.yon > 0 ? 5 : 0), sy + 7, sx + (o.yon > 0 ? 8 : -3), sy + 7 + kol, W);
  if (env[KILIC] || kazma()) ekran.drawPixel(sx + (o.yon > 0 ? 9 : -4), sy + 6 + kol, W);
  bool adim = yuruyor && (t / 120) % 2;
  ekran.drawLine(sx + 1, sy + 11, sx + (adim ? 0 : 1), sy + 13, W); ekran.drawLine(sx + 4, sy + 11, sx + (adim ? 5 : 4), sy + 13, W);
}
void ciz(int yuru) {
  uint32_t t = millis();
  float hx = (o.x + OW / 2) * 8 - 64, hy = (o.y + OH / 2) * 8 - 34;
  kamX += (constrain(hx, 0.0f, DW * 8 - 128.0f) - kamX) * .25f; kamY += (constrain(hy, -8.0f, DH * 8 - 64.0f) - kamY) * .25f;
  int bx0 = (int)floorf(kamX / 8), by0 = (int)floorf(kamY / 8);
  isikHesapla(bx0, by0);
  float gp = gece() ? (gun - .6f) / .4f : gun / .6f; int gx = 8 + gp * 112 - kamX * .03f, gy = 14 - sinf(gp * PI) * 10 - kamY * .05f;
  if (gece()) { ekran.fillCircle(gx, gy, 4, W); ekran.fillCircle(gx + 2, gy - 1, 4, B); for (int i = 0; i < 14; i++) ekran.drawPixel(((i * 53 - (int)(kamX * .1f)) % 128 + 128) % 128, (i * 23) % 26 - (int)(kamY * .1f), W); }
  else { ekran.drawCircle(gx, gy, 4, W); for (int i = 0; i < 8; i++) ekran.drawPixel(gx + cosf(i * PI / 4) * 6, gy + sinf(i * PI / 4) * 6, W); }
  for (int by = by0; by <= by0 + 8; by++) for (int bx = bx0; bx <= bx0 + 16; bx++) {
    if (!icinde(bx, by) || d[by][bx] == HAVA) continue;
    uint8_t l = isikAl(bx, by); if (!l) continue;     // karanlıkta görünmez
    int sx = lroundf(bx * 8 - kamX), sy = lroundf(by * 8 - kamY); uint8_t b = d[by][bx];
    bool ust = by == 0 || !kati(bx, by - 1), alt = !kati(bx, by + 1), sol = !kati(bx - 1, by), sag = !kati(bx + 1, by);
    if (b == SU) { int f = (t / 300 + bx) % 2; ekran.drawBitmap(sx + f, sy, DOKU[SU], 8, 8, W); if (by == 0 || d[by - 1][bx] != SU) ekran.drawFastHLine(sx, sy, 8, W); continue; }
    if (gecirgen(b)) { ekran.drawBitmap(sx, sy, DOKU[b], 8, 8, W); continue; }
    if (l <= 2) { for (int r = 0; r < 8; r++) { uint8_t m = DOKU[b][r] & ((r % 2) ? 0xAA : 0x55); for (int c = 0; c < 8; c++) if (m >> (7 - c) & 1) ekran.drawPixel(sx + c, sy + r, W); } continue; }   // loş: yarı nokta
    ekran.drawBitmap(sx, sy, ust || alt || sol || sag ? DOKU[b] : KAPALI, 8, 8, W);
    if (b != YAPRAK) { if (ust) ekran.drawFastHLine(sx, sy, 8, W); if (alt) ekran.drawFastHLine(sx, sy + 7, 8, W); if (sol) ekran.drawFastVLine(sx, sy, 8, W); if (sag) ekran.drawFastVLine(sx + 7, sy, 8, W); }
  }
  for (auto &c : cv) if (c.var) {                     // canavarlar: siyah zemin üstüne
    if (isikAl((int)(c.v.x + .3f), (int)(c.v.y + 1)) == 0 && fabsf(c.v.x - o.x) > 3) continue;
    int sx = lroundf((c.v.x + OW / 2) * 8 - kamX) - 4, sy = lroundf(c.v.y * 8 - kamY);
    ekran.fillRect(sx, sy, 8, 14, B);
    if (c.tur == 0) ekran.drawBitmap(sx, sy, ZOMBI_S[(t / 200) % 2], 8, 14, W);
    else if (c.t > 0 && (t / (c.t > 1 ? 60 : 150)) % 2) ekran.fillRect(sx, sy, 8, 14, W);   // fitil: yanıp söner
    else ekran.drawBitmap(sx, sy, CREEPER_S, 8, 14, W);
  }
  int px = lroundf((o.x + OW / 2) * 8 - kamX) - 3, py = lroundf(o.y * 8 - kamY);
  if (kir.aktif) {
    int sx = lroundf(kir.x * 8 - kamX), sy = lroundf(kir.y * 8 - kamY), n = 1 + 5 * kir.t / kir.sure;
    ekran.fillRect(sx, sy, 8, 8, B); ekran.drawBitmap(sx, sy, DOKU[d[kir.y][kir.x]], 8, 8, W);
    const int8_t C[6][4] = {{3, 3, 0, 1}, {3, 3, 6, 0}, {4, 4, 7, 6}, {3, 4, 1, 7}, {4, 3, 7, 2}, {3, 4, 4, 7}};
    for (int i = 0; i < n && i < 6; i++) ekran.drawLine(sx + C[i][0], sy + C[i][1], sx + C[i][2], sy + C[i][3], B);
    ekran.drawRect(sx, sy, 8, 8, W);
  } else if (!yuru && o.yerde && (t / 300) % 2) {
    int kx, ky; if (hedefKir(o.yon, kx, ky)) { int sx = lroundf(kx * 8 - kamX), sy = lroundf(ky * 8 - kamY); for (int i = 0; i < 8; i += 2) { ekran.drawPixel(sx + i, sy, W); ekran.drawPixel(sx + i + 1, sy + 7, W); ekran.drawPixel(sx, sy + i + 1, W); ekran.drawPixel(sx + 7, sy + i, W); } }
  }
  ekran.fillRect(px - 1, py, 8, 14, B);
  if (sonHasar > .4f || (t / 70) % 2) oyuncuCiz(px, py, t, yuru && o.yerde);
  ekran.fillRect(0, 0, 40, 11, B); ekran.drawRect(0, 0, 11, 11, W);
  if (env[secili]) { ekran.drawBitmap(1, 1, DOKU[secili], 8, 8, W); yaz(13, 2, 1, String(env[secili])); } else yaz(13, 2, 1, "-");
  ekran.fillRect(96, 0, 32, 8, B);
  for (int i = 0; i < CAN_MAX / 2; i++) {             // kalpler
    int x = 97 + i * 6, dolu = can - i * 2;
    if (dolu >= 2) { ekran.fillCircle(x + 1, 2, 1, W); ekran.fillCircle(x + 3, 2, 1, W); ekran.fillTriangle(x, 3, x + 4, 3, x + 2, 5, W); }
    else if (dolu == 1) { ekran.fillCircle(x + 1, 2, 1, W); ekran.drawPixel(x + 3, 2, W); ekran.drawLine(x, 3, x + 2, 5, W); }
    else ekran.drawPixel(x + 2, 3, W);
  }
  ekran.fillRect(42, 0, 52, 8, B); yaz(43, 0, 1, yaratici ? String("YARATICI") : String("G") + gunSayisi + (bitirdi ? " +" : ""));
  if (millis() < bilgiBitis) { int w = min(128, (int)bilgi.length() * 6 + 6); ekran.fillRect(64 - w / 2, 54, w, 10, B); ekran.drawRect(64 - w / 2, 54, w, 10, W); ortala(55, 1, bilgi.substring(0, 20)); }
}
bool yapabilir(const Tarif &r) { return env[r.g1] >= r.n1 && (!r.n2 || env[r.g2] >= r.n2) && (!r.masa || masaYakin()); }
void kutu(const char *bas, String *og, int n, int satirSayisi = 5) {   // menü kutusu: kaydırmalı liste
  ekran.fillRect(6, 2, 116, 60, B); ekran.drawRoundRect(6, 2, 116, 60, 4, W);
  yaz(11, 5, 1, String(bas).substring(0, 18)); ekran.drawFastHLine(9, 14, 110, W);
  int ilk = constrain(imlec - 2, 0, max(0, n - satirSayisi));
  for (int i = ilk; i < min(n, ilk + satirSayisi); i++) { int y = 17 + (i - ilk) * 9; if (i == imlec) { ekran.fillRect(9, y - 1, 110, 9, W); ekran.setTextColor(B); } yaz(12, y, 1, og[i].substring(0, 17)); ekran.setTextColor(W); }
}

const char *GOREV[] = {"Kutuk topla", "Tahta uret", "Masa kur", "Kazma yap", "Demir bul", "Ekmek pisir", "Elmas bul", "Sinyali yak"};
void gorevKontrol() {
  if (yaratici) return;
  bool k[] = {env[KUTUK] || env[TAHTA] || env[MASA] || kazma(), env[TAHTA] || env[MASA] || kazma(), env[MASA] || masaYakin() || kazma(), kazma() > 0,
              env[DEMIR] || env[KAZMA_D] || env[KILIC], env[EKMEK] > 0, env[ELMAS] > 0, bitirdi};
  for (int i = 0; i < 8; i++) if (k[i]) gorevler |= 1u << i;
}
void oyunaDon() { hal = 1; menuAcildi = true; menuYut = true; T[0] = T[1] = {}; kir.aktif = false; }
void baslat(bool devam, bool serbest) {
  if (devam && !yukle()) { soyle("Kayit bozuk/eksik!"); hal = 0; return; }
  if (!devam) {
    uret(esp_random()); yaratici = serbest;
    if (yaratici) { for (int i = 1; i < ESYA_N; i++) if (i != ANAKAYA && i != SU && i != LAV && i != EKIN1 && i != EKIN2 && i != EKIN3) env[i] = 999; secili = TAHTA; }
  }
  kamX = constrain((o.x + OW / 2) * 8 - 64, 0.0f, DW * 8 - 128.0f); kamY = (o.y + OH / 2) * 8 - 34;
  dunyaAcik = true;
  gorevKontrol(); oyunaDon(); soyle(devam ? "Tekrar hos geldin!" : yaratici ? "Sinirsiz insa et!" : "Gorev: kutuk topla");
}
bool gokAcik() { for (int y = 0; y < (int)floorf(o.y); y++) if (opak(merkezX(), y)) return false; return true; }
bool sinyalHazir() { return !yaratici && !bitirdi && env[ELMAS] >= 3 && env[DEMIR] >= 8 && env[MESALE] >= 4; }
void sinyalYak() {
  if (yaratici) { soyle("Serbest insa modu"); return; }
  if (bitirdi) { soyle("Sinyal zaten yandi"); return; }
  if (!sinyalHazir()) { soyle("3 elmas 8 demir 4 mes"); return; }
  if (fabsf(o.x - dogX) > 8 || !gokAcik() || !o.yerde) { soyle("Baslangic yuzeyine don"); return; }
  env[ELMAS] -= 3; env[DEMIR] -= 8; env[MESALE] -= 4; bitirdi = true; gorevler |= 128;
  kaydet(); hal = 9; menuYut = true; imlec = 0;
}
void yardimCiz() {
  const char *ad[] = {"YURUME VE ZIPLAMA", "KAZMA VE SAVAS", "INSA VE ESYA", "MENU VE KAYIT", "OYUNUN HEDEFI"};
  const char *sat[5][4] = {
    {"Sol/sag tut: yuru", "Iki tus kisa: zipla", "Suda 2 tus: yuz", "1 blok: oto zipla"},
    {"Ayni tus 2 tik: kir", "Dusman varsa: vur", "2 tus 0.35-1sn:", "Ayaginin altini kaz"},
    {"Tek tik: o yana koy", "Havada: alta koy", "Esya: menu/envanter", "Tohum topraga ekilir"},
    {"2 tus 1sn: menu", "Menude sag: gez", "Sol veya 2t: sec", "Kaydet ve cik:sakla"},
    {"Gorevleri tamamla", "3 elmas, 8 demir,", "4 mesale ile don", "Yuzeyde sinyali yak"}};
  ekran.clearDisplay(); baslik(String(imlec + 1) + "/5 " + ad[imlec]);
  for (int i = 0; i < 4; i++) yaz(0, 13 + i * 10, 1, sat[imlec][i]);
  yaz(0, 56, 1, "sag:sonraki sol:geri");
}
bool oyna(Girdi &g) {
  if (g.ilk) { hal = 0; dunyaAcik = false; imlec = kayitVar() ? 0 : 1; menuAcildi = false; menuYut = false; bilgiBitis = 0; T[0] = T[1] = {}; }
  if (menuYut) { g.o1 = g.o2 = YOK; g.combo = false; if (!g.b1 && !g.b2) menuYut = false; }
  bool sec = g.o2 == KISA || (g.combo && g.comboSure < 350);
  if (hal == 0) {
    bool var = kayitVar();
    if (g.o1 == KISA) { imlec = (imlec + 1) % 5; if (!var && imlec == 0) imlec = 1; }
    if (sec) {
      if (imlec == 0) { baslat(true, false); return true; }
      if (imlec == 1 || imlec == 2) { yeniYaratici = imlec == 2; if (var) { hal = 8; imlec = 0; menuYut = true; } else baslat(false, yeniYaratici); return true; }
      if (imlec == 3) { yardimDonus = 0; hal = 7; imlec = 0; return true; }
      return false;
    }
    ortala(2, 1, "KUBITCRAFT"); ekran.drawFastHLine(0, 11, 128, W);
    const char *og[] = {"Devam et", "Hayatta kal", "Yaratici dunya", "Kontroller", "Cik"};
    for (int i = 0; i < 5; i++) { int y = 14 + i * 9; if (i == imlec) { ekran.fillRect(0, y - 1, 128, 9, W); ekran.setTextColor(B); } yaz(3, y, 1, i == 0 && !var ? "(Kayit yok)" : og[i]); ekran.setTextColor(W); }
    if (millis() < bilgiBitis) { ekran.fillRect(0, 54, 128, 10, B); yaz(0, 55, 1, bilgi.substring(0, 21)); }
    return true;
  }
  if (hal == 8) {
    String og[] = {"Vazgec", "Evet, yeni dunya"}; if (g.o1 == KISA) imlec = !imlec;
    if (sec) { if (!imlec) { hal = 0; imlec = 0; } else baslat(false, yeniYaratici); return true; }
    kutu("ESKI DUNYA DEGISECEK", og, 2); return true;
  }
  if (hal == 7) {
    if (g.o1 == KISA) imlec = (imlec + 1) % 5;
    if (sec || (g.combo && g.comboSure >= 1000)) { hal = yardimDonus; imlec = hal == 0 ? 3 : 6; menuYut = true; return true; }
    yardimCiz(); return true;
  }
  if (hal == 9) {
    ciz(0); ekran.fillRect(0, 10, 128, 44, B); ortala(12, 2, "BASARDIN!");
    ortala(31, 1, "Kubit'e sinyal ulasti"); ortala(42, 1, String(gunSayisi) + ". gun / dunya senin");
    ekran.fillRect(0, 54, 128, 10, B); yaz(0, 56, 1, "sol/2t:devam");
    if (sec) oyunaDon(); return true;
  }
  gorevKontrol();
  if (hal == 2) {
    String og[] = {"Devam", "Envanter...", "Uret...", sandikYakin() ? "Sandik..." : "Sandik (uzakta)", "Gorevler / sinyal", "Ekmek ye (" + String(env[EKMEK]) + ")", "Kontroller", "Dogma noktasi koy", "Kaydet ve cik", "Ana menu"};
    if (g.o1 == KISA) imlec = (imlec + 1) % 10;
    if (sec) switch (imlec) {
      case 0: oyunaDon(); break;
      case 1: hal = 5; imlec = 0; return true;
      case 2: hal = 3; imlec = 0; return true;
      case 3: if (sandikYakin()) { hal = 4; imlec = 0; return true; } else soyle("Once sandiga yaklas"); break;
      case 4: hal = 6; imlec = 0; return true;
      case 5: if (env[EKMEK] && can < CAN_MAX) { env[EKMEK]--; can = min(CAN_MAX, can + 4); } break;
      case 6: yardimDonus = 2; hal = 7; imlec = 0; return true;
      case 7: if (o.yerde && gokAcik()) { dogX = o.x; dogY = o.y; soyle("Dogma noktasi tamam"); } else soyle("Acik yuzeyde dur"); break;
      case 8: if (kaydet()) return false; break;
      case 9: if (kaydet()) { hal = 0; dunyaAcik = false; imlec = 0; menuYut = true; bilgiBitis = 0; } return true;
    }
    ciz(0); kutu("KUBITCRAFT", og, 10); return true;
  }
  if (hal == 5) {
    int esyalar[ESYA_N], n = 0; String og[ESYA_N + 1];
    for (int i = 1; i < ESYA_N; i++) if (env[i]) { esyalar[n] = i; og[n++] = String(secili == i ? ">" : " ") + AD[i] + " " + String(env[i]); }
    og[n] = "Geri"; if (g.o1 == KISA) imlec = (imlec + 1) % (n + 1); imlec = min(imlec, n);
    if (sec) { if (imlec == n) { hal = 2; imlec = 1; return true; } secili = esyalar[imlec]; soyle("Secildi: " + String(AD[secili])); oyunaDon(); return true; }
    ciz(0); kutu("ENVANTER / SOL:SEC", og, n + 1); return true;
  }
  if (hal == 6) {
    String og[10]; for (int i = 0; i < 8; i++) og[i] = String(gorevler & (1u << i) ? "+ " : "- ") + GOREV[i];
    og[8] = bitirdi ? "Sinyal tamamlandi" : "Sinyali yak"; og[9] = "Geri";
    if (g.o1 == KISA) imlec = (imlec + 1) % 10;
    if (sec) { if (imlec == 9) { hal = 2; imlec = 4; return true; } else if (imlec == 8) { sinyalYak(); if (hal == 9) return true; } }
    ciz(0); kutu(yaratici ? "YARATICI: SERBEST" : "KESIF GOREVLERI", og, 10);
    if (imlec == 8) { ekran.fillRect(7, 54, 114, 8, B); yaz(9, 55, 1, "3elm 8dem 4mes"); }
    return true;
  }
  if (hal == 3) {
    String og[TARIF_N + 1];
    for (int i = 0; i < TARIF_N; i++) { const Tarif &r = TARIF[i]; og[i] = String(yapabilir(r) ? "+" : " ") + AD[r.cikti] + (r.adet > 1 ? " x" + String(r.adet) : ""); }
    og[TARIF_N] = "Geri";
    if (g.o1 == KISA) imlec = (imlec + 1) % (TARIF_N + 1);
    if (sec) {
      if (imlec == TARIF_N) { hal = 2; imlec = 2; return true; }
      const Tarif &r = TARIF[imlec];
      if (yapabilir(r)) { env[r.g1] -= r.n1; if (r.n2) env[r.g2] -= r.n2; env[r.cikti] = min(65535, (int)env[r.cikti] + r.adet); if (r.cikti < BLOK_N || r.cikti == EKMEK) secili = r.cikti; gorevKontrol(); soyle("+" + String(r.adet) + " " + AD[r.cikti]); }
      else soyle(r.masa && !masaYakin() ? "Masa kur ve yaklas" : "Malzeme yetmiyor");
    }
    ciz(0); kutu("URETIM: + HAZIR", og, TARIF_N + 1, 4);
    if (imlec < TARIF_N) { const Tarif &r = TARIF[imlec]; String s = String(r.n1) + " " + AD[r.g1] + (r.n2 ? "+" + String(r.n2) + " " + AD[r.g2] : ""); ekran.fillRect(7, 54, 114, 8, B); yaz(10, 55, 1, s.substring(0, 18)); }
    return true;
  }
  if (hal == 4) {
    int toplam = 0; for (int i = 0; i < ESYA_N; i++) toplam += sandik[i];
    String og[] = {"Koy: " + String(AD[secili]) + " " + String(env[secili]), "Hepsini al (" + String(toplam) + ")", "Geri"};
    if (g.o1 == KISA) imlec = (imlec + 1) % 3;
    if (sec) {
      if (imlec == 0 && env[secili]) { int aktar = min((int)env[secili], 65535 - (int)sandik[secili]); sandik[secili] += aktar; env[secili] -= aktar; soyle("Sandiga kondu"); }
      if (imlec == 1) { for (int i = 0; i < ESYA_N; i++) { int aktar = min((int)sandik[i], 65535 - (int)env[i]); env[i] += aktar; sandik[i] -= aktar; } soyle("Alindi"); }
      if (imlec == 2) { hal = 2; imlec = 3; }
    }
    ciz(0); kutu("SANDIK", og, 3); return true;
  }
  // ---- oyun
  uint32_t t = millis(); bool ikisi = g.b1 && g.b2; int yuru = 0;
  if (ikisi && g.ikiSure >= 1000 && !menuAcildi) { menuAcildi = true; hal = 2; imlec = 0; menuYut = true; T[0] = T[1] = {}; kir.aktif = false; return true; }
  bool suda = icindeki(o) == SU;
  if (g.combo) {
    T[0].yuruyor = T[1].yuruyor = false;
    if (menuAcildi) menuAcildi = false;
    else if (g.comboSure < 350) { if (o.yerde || suda) { o.vy = suda ? -5 : -ZIPLA; o.yerde = false; kir.aktif = false; } }   // suda her tıkta yüzer
    else kirBasla(merkezX(), (int)floorf(o.y + OH + .01f));
  }
  if (!ikisi) {
    if (g.b2 && t - tus2.basla > 160) { yuru = -1; T[0].yuruyor = true; }
    if (g.b1 && t - tus1.basla > 160) { yuru = 1; T[1].yuruyor = true; }
  }
  for (int i = 0; i < 2; i++) {
    Olay ev = i ? g.o1 : g.o2;
    if (ev == KISA || ev == UZUN) { if (T[i].yuruyor) T[i].yuruyor = false; else if (ev == KISA) tik(i); }
    if (T[i].bekliyor && t - T[i].sonTik >= 320) { T[i].bekliyor = false; koy(i ? 1 : -1); }
  }
  if (!g.b1 && !g.b2 && !g.combo) menuAcildi = false;
  if (yuru || (kir.aktif && (fabsf(kir.x - o.x) > 3 || fabsf(kir.y - o.y) > 4))) kir.aktif = false;
  if (yaratici) { for (int i = 1; i < ESYA_N; i++) if (i != ANAKAYA && i != SU && i != LAV && i != EKIN1 && i != EKIN2 && i != EKIN3) env[i] = 999; }
  if (kir.aktif && (kir.t += g.dt) >= kir.sure) kirBitir();
  fizik(o, g.dt, yuru, YURU, true);
  if (icindeki(o) == LAV && (lavT -= g.dt) <= 0) { lavT = .5f; hasar(2, "Lav"); }
  sonHasar += g.dt;
  if (sonHasar > 6 && can < CAN_MAX && (yenilenme += g.dt) > 5) { yenilenme = 0; can++; }   // dinlenince can dolar
  if ((akisT -= g.dt) <= 0) { akisT = .2f; akis((int)(kamX / 8) - 24, (int)(kamX / 8) + 40); }
  if ((ekinT -= g.dt) <= 0) { ekinT = 2; for (int y = 0; y < DH; y++) for (int x = 0; x < DW; x++) if ((d[y][x] == EKIN1 || d[y][x] == EKIN2) && random(15) == 0) koyBlok(x, y, d[y][x] + 1); }
  canavarlar(g.dt);
  float yeniGun = gun + g.dt / 240; if (yeniGun >= 1) gunSayisi++; gun = fmodf(yeniGun, 1); gorevKontrol();
  if ((otoKayitT += g.dt) > 120) { otoKayitT = 0; kaydet(); }   // 2 dakikada bir yalnız değişen bölgeler
  ciz(yuru);
  return true;
}
}  // namespace mc
bool kubitcraft(Girdi &g) { return mc::oyna(g); }
