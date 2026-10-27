// Kübimon: Pokémon tarzı. Sağ tık: seçenek gez, sol tık: seç / mesajı geç. Başlangıç Kübimon'unu seç, bölgelerde vahşi Kübimon'larla
// sıra tabanlı savaş, Kübi topuyla yakala, seviye atla, 12. seviyede evrimleş. 6 salon lideri (sırayla, rozet verir), Kübidex,
// takas kodu (5 rakam: Kübimon'unu koda çevir, arkadaşın kendi Kübit'ine girer). Takım, altın, top, rozet NVS'e kaydedilir.
#pragma once

namespace kmon {
enum Tip : uint8_t { ATES, SU, CIMEN, ELEKTRIK, KAYA, HAVA };
const char *HAMLE[6] = {"Alev Topu", "Su Topu", "Yaprak", "Simsek", "Kaya Atisi", "Kasirga"};
struct Tur { const char *ad; uint8_t tip, can, guc, def, hiz; };
const Tur TUR[16] = {
  {"Kivilcik", ATES, 39, 52, 43, 65}, {"Alevdag", ATES, 78, 94, 78, 90}, {"Damlacik", SU, 44, 48, 65, 43}, {"Dalgator", SU, 85, 83, 100, 78},
  {"Filizik", CIMEN, 45, 49, 49, 45}, {"Agacadam", CIMEN, 85, 82, 83, 70}, {"Voltik", ELEKTRIK, 35, 55, 40, 90}, {"Simsekbey", ELEKTRIK, 70, 90, 65, 110},
  {"Tascik", KAYA, 40, 80, 100, 20}, {"Kayadev", KAYA, 80, 110, 130, 45}, {"Kanatcik", HAVA, 40, 45, 40, 56}, {"Firtinakus", HAVA, 83, 80, 75, 101},
  {"Kubicik", ELEKTRIK, 50, 50, 50, 50}, {"Kubitron", ELEKTRIK, 95, 95, 95, 95}, {"Mantarcik", CIMEN, 60, 40, 55, 30}, {"Mantarkral", CIMEN, 110, 80, 90, 50}};
const char *BOLGE[3] = {"Cimenlik", "Magara", "Gol"};
const uint8_t BOLGE_TUR[3][4] = {{4, 10, 6, 14}, {8, 0, 12, 6}, {2, 10, 2, 8}};   // bölgede çıkan (temel) türler
float etki(uint8_t a, uint8_t d) {                   // saldıran tip -> savunan tip
  const int8_t T[6][6] = {{0, -1, 1, 0, -1, 0}, {1, 0, -1, 0, 1, 0}, {-1, 1, 0, 0, 1, -1}, {0, 1, -1, 0, 0, 1}, {1, 0, 0, 0, 0, 1}, {0, 0, 1, -1, -1, 0}};
  return T[a][d] > 0 ? 2 : T[a][d] < 0 ? .5f : 1;
}
struct Kmon { uint8_t tur, sv; uint16_t xp; int16_t can; };
struct KayitV1 { uint8_t surum, n, top, lider; uint16_t altin, dex; Kmon takim[6]; };
struct Kayit { uint8_t surum, n, top, lider; uint16_t altin, dex; Kmon takim[6]; uint8_t rozet; uint16_t gordu; };   // eski kayda ek alanlar sonda
struct Lider { const char *ad; uint8_t n, tur[3], sv[3]; };
const Lider LIDER[6] = {{"Ceren", 2, {4, 14}, {8, 10}}, {"Deniz", 2, {2, 3}, {13, 15}}, {"Kaan", 2, {8, 9}, {18, 20}},
                        {"Volkan", 2, {0, 1}, {23, 26}}, {"Simsek", 3, {6, 7, 13}, {28, 31, 33}}, {"Ruzgar", 3, {10, 11, 15}, {35, 38, 40}}};
Kayit k; Kmon vahsi;
int hal, imlec, alt; float gB, gO, mesajT, evrimFlas;   // hal: 0 seçim, 1 harita, 2 savaş, 3 takım, 4 Kübidex, 5 takas
int salon = -1, rakipSira; uint8_t takasRakam[5]; int takasMod, takasSec;   // salon: -1 vahşi savaş
std::vector<String> mesaj; bool savasBitti, yakalaAnim;

int stat(int taban, int sv) { return taban * sv / 50 + 5; }
int maxCan(const Kmon &m) { return TUR[m.tur].can * m.sv / 50 + m.sv + 10; }
Kmon yeni(int tur, int sv) { Kmon m = {(uint8_t)tur, (uint8_t)sv, 0, 0}; m.can = maxCan(m); return m; }
Kmon &lider() { return k.takim[k.lider]; }
void kaydet() { kayit.putBytes("kubimon", &k, sizeof(k)); rekorKaydet("kubimon", __builtin_popcount(k.dex)); rekorKaydet("kmrozet", __builtin_popcount(k.rozet)); }
void yaz2(const String &s) { mesaj.push_back(s); }

void yaratik(int tur, int x, int y, int o) {          // türden üretilen simetrik yaratık: gövde + göz + ağız + ayak + tipe özel süs; evrimde büyür
  bool evo = tur % 2; int n = evo ? 14 : 12, yarim = n / 2; uint32_t s = (tur / 2 + 1) * 2654435761u; uint8_t tip = TUR[tur].tip;
  bool iz[14][7] = {};                                 // yarım desen, aynada tamamlanır
  for (int r = 0; r < n; r++) for (int c = 0; c < yarim; c++) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    float dx = (yarim - .5f - c) / (yarim - .5f), dy = (r - n * .55f) / (n * .42f), d2 = dx * dx + dy * dy;
    iz[r][c] = tip == KAYA ? fabsf(dx) < .8f && fabsf(dy) < .85f : d2 < .55f || (d2 < 1 && s % 100 < 55u);   // kaya: köşeli
  }
  int kafa = n * 3 / 10;
  for (int r = n - 2; r < n; r++) for (int c = 0; c < yarim; c++) iz[r][c] = c == yarim - 3 || c == yarim - 4;   // iki ayak
  switch (tip) {                                       // tipe göre süs
    case ATES: iz[0][yarim - 1] = iz[1][yarim - 1] = iz[1][yarim - 2] = iz[0][yarim - 3] = true; break;           // alev perçemi
    case SU: for (int r = kafa; r < kafa + 4; r++) iz[r][0] = true; break;                                        // yan yüzgeçler
    case CIMEN: iz[0][yarim - 1] = iz[1][yarim - 1] = iz[0][yarim - 2] = true; break;                               // tepede yaprak
    case ELEKTRIK: iz[0][1] = iz[1][2] = iz[2][1] = true; break;                                                  // şimşek kulaklar
    case HAVA: for (int r = n / 2 - 2; r < n / 2 + 1; r++) { iz[r][0] = true; if (r > n / 2 - 2) iz[r][1] = true; } break;   // kanatlar
    default: iz[1][2] = true; break;                                                                              // kaya: boynuz
  }
  if (evo) { iz[0][1] = iz[0][2] = true; }             // evrim: taç gibi ek çıkıntı
  for (int r = 0; r < n; r++) for (int c = 0; c < yarim; c++) if (iz[r][c]) { ekran.fillRect(x + c * o, y + r * o, o, o, W); ekran.fillRect(x + (n - 1 - c) * o, y + r * o, o, o, W); }
  int gr = kafa + 1, gc = yarim - 2;                   // gözler (siyah) ve ağız
  ekran.fillRect(x + gc * o, y + gr * o, o, o + (evo ? o : 0), B); ekran.fillRect(x + (n - 1 - gc) * o, y + gr * o, o, o + (evo ? o : 0), B);
  ekran.fillRect(x + (yarim - 1) * o, y + (gr + 3) * o, 2 * o, max(1, o / 2), B);
}
void hpBar(int x, int y, int w, float oran) { ekran.drawRect(x, y, w, 4, W); ekran.fillRect(x + 1, y + 1, max(0, (int)((w - 2) * oran)), 2, W); }

int hasar(const Kmon &a, const Kmon &d, int hamle, float &e) {   // hamle 0: Vurus (normal 40), 1: tip hamlesi (60, evrimde 80)
  int guc = hamle ? (a.tur % 2 ? 80 : 60) : 40; uint8_t tip = TUR[a.tur].tip;
  e = hamle ? etki(tip, TUR[d.tur].tip) : 1;
  float h = ((2 * a.sv / 5 + 2) * guc * (float)stat(TUR[a.tur].guc, a.sv) / stat(TUR[d.tur].def, d.sv)) / 50 + 2;
  return max(1, (int)(h * e * (hamle ? 1.2f : 1) * random(85, 101) / 100));
}
void vur(Kmon &a, Kmon &d, int hamle, bool benim) {
  float e; int h = hasar(a, d, hamle, e);
  d.can = max(0, d.can - h);
  yaz2(String(benim ? "" : "Vahsi ") + TUR[a.tur].ad + ": " + (hamle ? HAMLE[TUR[a.tur].tip] : "Vurus"));
  if (e > 1) yaz2("Super etkili!"); else if (e < 1) yaz2("Pek etkili degil...");
}
void kazandi() {                                      // xp, seviye, evrim
  Kmon &m = lider(); int xp = vahsi.sv * (salon >= 0 ? 12 : 9); k.altin += vahsi.sv * 6;
  yaz2(String(TUR[m.tur].ad) + " +" + String(xp) + " XP");
  m.xp += xp;
  while (m.xp >= m.sv * 12 && m.sv < 60) {
    m.xp -= m.sv * 12; int eski = maxCan(m); m.sv++; m.can += maxCan(m) - eski;
    yaz2(String(TUR[m.tur].ad) + " sv " + String(m.sv) + "!");
    if (m.sv == 12 && m.tur % 2 == 0) k.gordu |= 1 << (m.tur + 1);
    if (m.sv == 12 && m.tur % 2 == 0) { yaz2(String(TUR[m.tur].ad) + " evrimlesiyor!"); m.tur++; m.can = maxCan(m); yaz2(String("-> ") + TUR[m.tur].ad + "!"); evrimFlas = 2; k.dex |= 1 << m.tur; }
  }
  if (salon >= 0 && ++rakipSira < LIDER[salon].n) {   // salon: liderin sıradaki Kübimon'u
    vahsi = yeni(LIDER[salon].tur[rakipSira], LIDER[salon].sv[rakipSira]); gO = vahsi.can; k.gordu |= 1 << vahsi.tur;
    yaz2(String(LIDER[salon].ad) + ": " + TUR[vahsi.tur].ad + "!"); kaydet(); return;
  }
  if (salon >= 0) { k.rozet |= 1 << salon; k.altin += 100 + salon * 60; yaz2(String(LIDER[salon].ad) + " yenildi!"); yaz2("Rozet kazandin! (" + String(__builtin_popcount(k.rozet)) + "/6)"); }
  savasBitti = true; kaydet();
}
bool sonrakiCanli() { for (int i = 0; i < k.n; i++) if (k.takim[i].can > 0) { k.lider = i; return true; } return false; }
void dusmanSirasi() {
  if (vahsi.can <= 0 || savasBitti) return;
  vur(vahsi, lider(), random(2), false);
  if (lider().can <= 0) {
    yaz2(String(TUR[lider().tur].ad) + " bayildi!");
    if (sonrakiCanli()) yaz2(String(TUR[lider().tur].ad) + ", sira sende!");
    else { yaz2("Takimin bayildi..."); yaz2("Merkezde iyilestin."); k.altin /= 2; for (int i = 0; i < k.n; i++) k.takim[i].can = maxCan(k.takim[i]); savasBitti = true; kaydet(); }
  }
}
void saldir(int hamle) {
  bool once = stat(TUR[lider().tur].hiz, lider().sv) >= stat(TUR[vahsi.tur].hiz, vahsi.sv);
  if (!once) { dusmanSirasi(); if (savasBitti || lider().can <= 0) return; }
  vur(lider(), vahsi, hamle, true);
  if (vahsi.can <= 0) { yaz2(String("Vahsi ") + TUR[vahsi.tur].ad + " bayildi!"); kazandi(); return; }
  if (once) dusmanSirasi();
}
void savasBasla(int bolge) {
  int tur = BOLGE_TUR[bolge][random(4)], sv = constrain((int)lider().sv + (int)random(-2, 2), 2, 58);
  if (sv >= 12 && random(3) == 0) tur++;                // yüksek seviyede evrimleşmiş de çıkar
  vahsi = yeni(tur, sv); hal = 2; alt = 0; imlec = 0; savasBitti = false; mesaj.clear(); salon = -1;
  gB = lider().can; gO = vahsi.can; k.gordu |= 1 << tur;
  yaz2(String("Vahsi ") + TUR[tur].ad + " cikti!");
}
void salonBasla(int i) {
  salon = i; rakipSira = 0; vahsi = yeni(LIDER[i].tur[0], LIDER[i].sv[0]); hal = 2; alt = 0; imlec = 0; savasBitti = false; mesaj.clear();
  gB = lider().can; gO = vahsi.can; k.gordu |= 1 << vahsi.tur;
  yaz2(String("Lider ") + LIDER[i].ad + " meydan okudu!"); yaz2(String(LIDER[i].ad) + ": " + TUR[vahsi.tur].ad + "!");
}
// takas kodu: (tür*64 + sv) karışık sayıya çevrilir, 2 basamak sağlama; 5 rakam
long takasKod(const Kmon &m) { long y = m.tur * 64 + m.sv; long x = (y * 7919 + 1234) % 1024; return x * 97 + (y * 31 + 7) % 97; }
bool takasCoz(long kod, Kmon &m) {
  long x = kod / 97, c = kod % 97;
  for (long y = 0; y < 1024; y++) if ((y * 7919 + 1234) % 1024 == x && (y * 31 + 7) % 97 == c) { int tur = y / 64, sv = y % 64; if (tur > 15 || sv < 2) return false; m = yeni(tur, sv); return true; }
  return false;
}

void savasCiz() {
  gB += (lider().can - gB) * .2f; gO += (vahsi.can - gO) * .2f;   // can barları yumuşakça azalır
  yaz(0, 0, 1, TUR[vahsi.tur].ad); yaz(0, 9, 1, "sv" + String(vahsi.sv)); hpBar(26, 10, 50, gO / maxCan(vahsi));
  if (!(yakalaAnim && mesaj.size() > 1)) yaratik(vahsi.tur, 96, 0, 2);
  else { int s = (millis() / 150) % 4; ekran.fillCircle(108 + (s == 1) - (s == 3), 14, 6, W); ekran.drawFastHLine(102, 14, 13, B); ekran.fillCircle(108 + (s == 1) - (s == 3), 14, 2, B); }
  yaratik(lider().tur, 4, 20, 2);
  yaz(40, 24, 1, TUR[lider().tur].ad); yaz(40, 33, 1, "sv" + String(lider().sv)); hpBar(66, 34, 50, gB / maxCan(lider()));
  yaz(122 - String(lider().can).length() * 6, 24, 1, String(lider().can));
  ekran.fillRect(0, 45, 128, 19, B); ekran.drawRoundRect(0, 45, 128, 19, 3, W);
  if (!mesaj.empty()) { auto s = sar(mesaj[0], 20); yaz(4, 47, 1, s[0]); if (s.size() > 1) yaz(4, 55, 1, s[1]); if ((millis() / 400) % 2) ekran.fillTriangle(120, 56, 124, 56, 122, 59, W); return; }
  const char *ana[4] = {"Saldir", "Top", "Degistir", "Kac"};
  if (alt == 0) for (int i = 0; i < 4; i++) { int x = 4 + (i % 2) * 62, y = 48 + (i / 2) * 8; if (i == imlec) { ekran.fillRect(x - 2, y - 1, 58, 8, W); ekran.setTextColor(B); } yaz(x, y, 1, i == 1 ? "Top x" + String(k.top) : ana[i]); ekran.setTextColor(W); }
  else { const char *hm[3] = {"Vurus", HAMLE[TUR[lider().tur].tip], "Geri"};
    for (int i = 0; i < 3; i++) { int x = 4 + (i % 2) * 62, y = 48 + (i / 2) * 8; if (i == imlec) { ekran.fillRect(x - 2, y - 1, 58, 8, W); ekran.setTextColor(B); } yaz(x, y, 1, hm[i]); ekran.setTextColor(W); } }
}

bool oyna(Girdi &g) {
  if (g.ilk) {
    memset(&k, 0, sizeof(k));
    size_t n = kayit.getBytes("kubimon", &k, sizeof(k));
    if (n == sizeof(KayitV1) && k.surum == 1) { k.surum = 2; k.rozet = 0; k.gordu = k.dex; }   // eski kayıt: rozetsiz, görülen = yakalanan
    if (k.surum != 2 || !k.n) { memset(&k, 0, sizeof(k)); k.surum = 2; k.top = 5; k.altin = 100; hal = 0; }
    else hal = 1;
    imlec = 0; mesaj.clear(); evrimFlas = 0;
  }
  evrimFlas -= g.dt;
  bool sec = g.o2 == KISA, gez = g.o1 == KISA;
  if (hal == 0) {                                     // başlangıç seçimi
    if (gez) imlec = (imlec + 1) % 3;
    const int ilk3[3] = {0, 2, 4};
    if (sec) { k.n = 1; k.takim[0] = yeni(ilk3[imlec], 5); k.dex = k.gordu = 1 << ilk3[imlec]; k.lider = 0; kaydet(); hal = 1; imlec = 0; return true; }
    ortala(0, 1, "Ilk Kubimon'unu sec!");
    for (int i = 0; i < 3; i++) { yaratik(ilk3[i], 4 + i * 43, 12, 3); if (i == imlec) ekran.drawRoundRect(1 + i * 43, 10, 42, 40, 4, W); }
    ortala(54, 1, String(TUR[ilk3[imlec]].ad) + " (" + (imlec == 0 ? "Ates" : imlec == 1 ? "Su" : "Cimen") + ")");
    return true;
  }
  if (hal == 3) {                                     // takım: sağ gez, sol lider yap
    if (gez) imlec = (imlec + 1) % (k.n + 1);
    if (sec) { if (imlec == k.n) { hal = 1; imlec = 4; } else if (k.takim[imlec].can > 0) { k.lider = imlec; kaydet(); } }
    baslik("TAKIM  " + String(__builtin_popcount(k.dex)) + "/16 tur");
    for (int i = 0; i <= k.n; i++) {
      int y = 11 + i * 9; if (i == imlec) { ekran.fillRect(0, y - 1, 128, 9, W); ekran.setTextColor(B); }
      if (i == k.n) yaz(2, y, 1, "Geri");
      else { const Kmon &m = k.takim[i]; yaz(2, y, 1, String(i == k.lider ? ">" : " ") + TUR[m.tur].ad); yaz(76, y, 1, "sv" + String(m.sv)); yaz(100, y, 1, String(m.can * 100 / maxCan(m)) + "%"); }
      ekran.setTextColor(W);
    }
    return true;
  }
  if (hal == 4) {                                     // Kübidex: sağ gez, sol çık
    if (gez) imlec = (imlec + 1) % 16;
    if (sec) { hal = 1; imlec = 7; return true; }
    bool gor = k.gordu >> imlec & 1, yak = k.dex >> imlec & 1;
    baslik("KUBIDEX " + String(__builtin_popcount(k.dex)) + "/16");
    for (int i = 0; i < 16; i++) { int x = 2 + (i % 8) * 8, y = 14 + (i / 8) * 8; if (k.dex >> i & 1) ekran.fillRect(x, y, 6, 6, W); else if (k.gordu >> i & 1) ekran.drawRect(x, y, 6, 6, W); else ekran.drawPixel(x + 2, y + 2, W); if (i == imlec) ekran.drawRect(x - 1, y - 1, 8, 8, W); }
    if (gor) yaratik(imlec, 96, 12, 2); else { ekran.drawRoundRect(98, 14, 24, 24, 4, W); yaz(107, 22, 1, "?"); }
    yaz(0, 34, 1, String(imlec < 9 ? "#0" : "#") + String(imlec + 1) + " " + (gor ? TUR[imlec].ad : "???"));
    const char *TIP[6] = {"Ates", "Su", "Cimen", "Elektrik", "Kaya", "Hava"};
    yaz(0, 44, 1, gor ? String(TIP[TUR[imlec].tip]) + (imlec % 2 ? " evrim" : "") : String("gorulmedi"));
    yaz(0, 55, 1, yak ? "yakalandi" : gor ? "goruldu" : ""); yaz(86, 55, 1, "sol:cik");
    return true;
  }
  if (hal == 5) {                                     // takas: 0 seçim, 1 kod göster (gönder), 2 kod gir
    if (takasMod == 0) {
      if (gez) imlec = (imlec + 1) % 3;
      if (sec) { if (imlec == 0) { if (k.n < 2) { mesaj.push_back("Takimda 2+ Kubimon lazim"); mesajT = 1.5f; hal = 1; } else { takasMod = 1; takasSec = (k.lider + 1) % k.n; } }
                 else if (imlec == 1) { takasMod = 2; memset(takasRakam, 0, 5); imlec = 0; } else { hal = 1; imlec = 8; } return true; }
      baslik("TAKAS");
      const char *og[3] = {"Gonder (kod olustur)", "Al (kod gir)", "Geri"};
      for (int i = 0; i < 3; i++) { int y = 14 + i * 10; if (i == imlec) { ekran.fillRect(0, y - 1, 128, 9, W); ekran.setTextColor(B); } yaz(2, y, 1, og[i]); ekran.setTextColor(W); }
      yaz(0, 50, 1, "Kodu arkadasin kendi"); yaz(0, 57, 1, "Kubit'ine girer.");
      return true;
    }
    if (takasMod == 1) {                              // sağ: başka Kübimon, sol: gönderdim (takımdan çıkar)
      if (gez) takasSec = (takasSec + 1) % k.n;
      Kmon &m = k.takim[takasSec];
      if (sec) {
        if (k.n < 2) { takasMod = 0; return true; }
        for (int i = takasSec; i < k.n - 1; i++) k.takim[i] = k.takim[i + 1];
        k.n--; k.lider = min((int)k.lider, k.n - 1); kaydet(); takasMod = 0; hal = 1; mesaj.push_back("Gonderildi, gule gule!"); mesajT = 1.5f; return true;
      }
      yaratik(m.tur, 2, 2, 2); yaz(34, 2, 1, TUR[m.tur].ad); yaz(34, 11, 1, "sv" + String(m.sv));
      char kod[8]; snprintf(kod, 8, "%05ld", takasKod(m)); yaz(34, 24, 2, kod);
      yaz(0, 46, 1, "sag: baska Kubimon"); yaz(0, 55, 1, "sol: gonderdim");
      return true;
    }
    static float tekrar = 0;                          // kod gir: sağ rakamı artırır, sol sonraki basamak
    if (g.o1 == BASTI) { takasRakam[imlec] = (takasRakam[imlec] + 1) % 10; tekrar = .4f; }
    else if (g.b1 && !g.b2 && (tekrar -= g.dt) <= 0) { tekrar = .15f; takasRakam[imlec] = (takasRakam[imlec] + 1) % 10; }
    if (sec) {
      if (++imlec >= 5) {
        long kod = 0; for (int i = 0; i < 5; i++) kod = kod * 10 + takasRakam[i];
        Kmon m;
        if (k.n >= 6) mesaj.push_back("Takim dolu");
        else if (takasCoz(kod, m)) { k.takim[k.n++] = m; k.dex |= 1 << m.tur; k.gordu |= 1 << m.tur; kaydet(); mesaj.push_back(String(TUR[m.tur].ad) + " geldi!"); }
        else mesaj.push_back("Kod hatali");
        mesajT = 1.8f; takasMod = 0; hal = 1; imlec = 8; return true;
      }
    }
    baslik("KOD GIR");
    for (int i = 0; i < 5; i++) { int x = 14 + i * 21; ekran.drawRoundRect(x, 18, 16, 20, 3, W); if (i == imlec) ekran.fillRoundRect(x, 18, 16, 20, 3, W); ekran.setTextColor(i == imlec ? B : W); yaz(x + 3, 21, 2, String(takasRakam[i])); ekran.setTextColor(W); }
    yaz(0, 46, 1, "sag: rakam +1"); yaz(0, 55, 1, "sol: sonraki basamak");
    return true;
  }
  if (hal == 1) {                                     // harita: bölgeler, merkez, market, takım, salon, Kübidex, takas
    const int N = 9;
    if (gez) imlec = (imlec + 1) % N;
    if (sec) {
      if (imlec < 3) { if (lider().can > 0 || sonrakiCanli()) savasBasla(imlec); else mesaj.push_back("Once iyiles!"); }
      if (imlec == 3) { for (int i = 0; i < k.n; i++) k.takim[i].can = maxCan(k.takim[i]); kaydet(); mesaj.push_back("Takim iyilesti!"); }
      if (imlec == 4) { hal = 3; imlec = 0; return true; }
      if (imlec == 5) { if (k.altin >= 40) { k.altin -= 40; k.top++; kaydet(); mesaj.push_back("Kubi topu alindi"); } else mesaj.push_back("40 altin lazim"); }
      if (imlec == 6) { int sira = __builtin_popcount(k.rozet); if (sira >= 6) mesaj.push_back("Tum rozetler senin!"); else if (lider().can > 0 || sonrakiCanli()) { salonBasla(sira); return true; } else mesaj.push_back("Once iyiles!"); }
      if (imlec == 7) { hal = 4; imlec = 0; return true; }
      if (imlec == 8) { hal = 5; takasMod = 0; imlec = 0; return true; }
      mesajT = 1.2f;
    }
    if (!mesaj.empty() && hal == 1 && (mesajT -= g.dt) <= 0) mesaj.clear();
    String salonAd = __builtin_popcount(k.rozet) >= 6 ? String("Salon (bitti)") : "Salon: " + String(LIDER[__builtin_popcount(k.rozet)].ad);
    String og[N] = {"Cimenlik", "Magara", "Gol", "Merkez", "Takim", "Market", salonAd, "Kubidex", "Takas"};
    yaz(0, 0, 1, String(k.altin) + "a"); yaz(42, 0, 1, "top" + String(k.top)); yaz(84, 0, 1, "rozet" + String(__builtin_popcount(k.rozet))); ekran.drawFastHLine(0, 9, 128, W);
    int ilk = constrain(imlec - 1, 0, N - 4);
    for (int i = ilk; i < ilk + 4; i++) { int y = 12 + (i - ilk) * 9; if (i == imlec) { ekran.fillRoundRect(0, y - 1, 90, 9, 2, W); ekran.setTextColor(B); } yaz(2, y, 1, og[i]); ekran.setTextColor(W); }
    ekran.fillRect(91, 11 + 34 * ilk / N, 2, 34 * 4 / N, W);
    yaratik(lider().tur, 96, 14, 2);
    ekran.drawFastHLine(0, 46, 128, W);
    if (!mesaj.empty()) ortala(52, 1, mesaj[0]);
    else { yaz(0, 49, 1, String(TUR[lider().tur].ad) + " sv" + String(lider().sv)); hpBar(0, 58, 88, (float)lider().can / maxCan(lider())); }
    return true;
  }
  // ---- savaş
  if (!mesaj.empty()) {
    if (sec || gez) { mesaj.erase(mesaj.begin()); if (mesaj.empty() && savasBitti) { hal = 1; imlec = 0; yakalaAnim = false; } }
    savasCiz(); return true;
  }
  if (savasBitti) { hal = 1; imlec = 0; return true; }
  if (alt == 0) {
    if (gez) imlec = (imlec + 1) % 4;
    if (sec) switch (imlec) {
      case 0: alt = 1; imlec = 0; break;
      case 1:
        if (salon >= 0) { yaz2("Liderin Kubimon'u yakalanmaz!"); break; }
        if (!k.top) { yaz2("Topun kalmadi!"); break; }
        k.top--; yakalaAnim = true; yaz2("Kubi topu atildi!"); yaz2("...");
        if (random(100) < (int)((1 - (float)vahsi.can / maxCan(vahsi)) * 70 + 15)) {
          yaz2(String(TUR[vahsi.tur].ad) + " yakalandi!");
          k.dex |= 1 << vahsi.tur;
          if (k.n < 6) { k.takim[k.n++] = vahsi; } else yaz2("Takim dolu, saliverdin");
          savasBitti = true; kaydet();
        } else { yaz2("Kacti!"); yakalaAnim = false; dusmanSirasi(); }
        break;
      case 2: { int bas = k.lider; for (int i = 1; i <= k.n; i++) { int j = (bas + i) % k.n; if (j != bas && k.takim[j].can > 0) { k.lider = j; yaz2(String(TUR[lider().tur].ad) + ", sira sende!"); dusmanSirasi(); break; } } if (k.lider == bas) yaz2("Baska Kubimon yok"); } break;
      case 3: if (salon >= 0) { yaz2("Salon savasindan kacilmaz!"); break; }
        if (random(100) < 75) { yaz2("Kactin!"); savasBitti = true; kaydet(); } else { yaz2("Kacamadin!"); dusmanSirasi(); } break;
    }
  } else {
    if (gez) imlec = (imlec + 1) % 3;
    if (sec) { if (imlec == 2) { alt = 0; imlec = 0; } else { int h = imlec; alt = 0; imlec = 0; saldir(h); } }
  }
  savasCiz();
  if (evrimFlas > 0 && (millis() / 100) % 2) ekran.fillRect(0, 0, 128, 45, W);
  return true;
}
}  // namespace kmon
bool kubimon(Girdi &g) { return kmon::oyna(g); }
