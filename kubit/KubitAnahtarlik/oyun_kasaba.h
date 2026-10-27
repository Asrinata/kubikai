// Kübit Kasabası: Stardew tarzı çiftlik. Bitkiler gerçek saatle büyür (Kübit kapalıyken de: telefon saati varsa aradaki süre sayılır).
// Sağ tık: tarla/pazar/kümes/ahır/komşu arasında gez. Sol tık: ek, sula, hasat et (ambara); pazarda sat, tohum seç, tarla ve hayvan al;
// ahırda besle ve süt/yün topla; komşu Ayşe'ye hediye ver (dostluk: para, çilek tohumu). Mevsim gerçek tarihten: kışın sadece turp ve havuç.
#pragma once

namespace kasaba {
struct Bitki { const char *ad; int tohum, satis; uint32_t sure; };   // süre saniye
const Bitki BITKI[6] = {{"Turp", 5, 12, 300}, {"Havuc", 15, 40, 1800}, {"Domates", 40, 120, 7200}, {"Misir", 80, 260, 21600}, {"Karpuz", 150, 600, 43200}, {"Cilek", 60, 320, 10800}};
const char *URUN_AD[9] = {"Turp", "Havuc", "Domates", "Misir", "Karpuz", "Cilek", "Yumurta", "Sut", "Yun"};
const int URUN_FIYAT[9] = {12, 40, 120, 260, 600, 320, 50, 120, 200};
const char *MEVSIM[4] = {"Ilkbahar", "Yaz", "Sonbahar", "Kis"};
const uint8_t SEVDIGI[4] = {5, 4, 3, 7};             // Ayşe'nin mevsime göre en sevdiği (iki kat dostluk)
struct Tarla { uint8_t bitki; uint8_t sulu; uint32_t ekim; };   // bitki 0 boş, 1..6
struct DurumV1 { int32_t para; uint8_t tarlaN, tohum, tavuk, surum; Tarla t[8]; uint32_t saat, sonEpoch, yumurtaSaat; };
struct Durum { int32_t para; uint8_t tarlaN, tohum, tavuk, surum; Tarla t[8]; uint32_t saat, sonEpoch, yumurtaSaat;
               uint8_t inek, koyun, kalp, hediyeGun; uint32_t besleme, sutSaat, yunSaat; uint16_t ambar[9]; uint8_t odul; };   // eski kayda ekler sonda
Durum d;
int imlec, menu, menuImlec, hediyeSec; float kx = 10, kayitSay, sulamaAnim; String bilgi; uint32_t bilgiBitis;   // menu: 0 yok, 1 pazar, 2 ahır, 3 komşu
const int YER_N = 12;                                // 0-7 tarla, 8 pazar, 9 kümes, 10 ahır, 11 komşu

void soyle(const String &s) { bilgi = s; bilgiBitis = millis() + 1500; }
void kaydet() { if (saatVar()) d.sonEpoch = time(nullptr); kayit.putBytes("kasaba", &d, sizeof(d)); }
void yukle() {
  memset(&d, 0, sizeof(d));
  size_t n = kayit.getBytes("kasaba", &d, sizeof(d));
  if (n == sizeof(DurumV1) && d.surum == 1) { d.surum = 2; d.besleme = d.sutSaat = d.yunSaat = d.saat; }   // eski kayıt: yeni alanlar sıfır
  if (d.surum != 2) { memset(&d, 0, sizeof(d)); d.para = 30; d.tarlaN = 4; d.surum = 2; }
  if (saatVar()) {
    uint32_t e = time(nullptr);
    if (d.sonEpoch && e > d.sonEpoch) d.saat += min(e - d.sonEpoch, (uint32_t)7 * 86400);
    d.sonEpoch = e;
  }
}
int mevsim() {                                        // gerçek takvimden; saat yoksa oyun gününden (haftada bir döner)
  if (saatVar()) { int ay = simdiTm().tm_mon; return ay >= 2 && ay <= 4 ? 0 : ay >= 5 && ay <= 7 ? 1 : ay >= 8 && ay <= 10 ? 2 : 3; }
  return (d.saat / 86400 / 7) % 4;
}
float hiz(int bitki) {                                // mevsim çarpanı (büyüme süresine)
  int m = mevsim();
  if (m == 3) return 1.5f;
  if (m == 1 && (bitki == 4 || bitki == 5)) return .75f;
  return 1;
}
bool kisinOlur(int bitki) { return bitki <= 2; }      // turp, havuç
float olgunluk(const Tarla &t) { if (!t.bitki) return 0; float s = BITKI[t.bitki - 1].sure * (t.sulu ? .7f : 1) * hiz(t.bitki); return min(1.0f, (d.saat - t.ekim) / s); }
String sureYazi(uint32_t s) { return s >= 3600 ? String(s / 3600) + "sa " + String(s % 3600 / 60) + "dk" : s >= 60 ? String(s / 60) + "dk " + String(s % 60) + "sn" : String(s) + "sn"; }
int yumurta() { return d.tavuk ? min(9, (int)((d.saat - d.yumurtaSaat) / 14400) * d.tavuk) : 0; }   // tavuk başına 4 saatte bir
uint32_t tok() { return min(d.saat, d.besleme + 86400); }   // hayvanlar beslendikten sonra 1 gün üretir
int sut() { return d.inek && tok() > d.sutSaat ? min(4 * d.inek, (int)((tok() - d.sutSaat) / 21600) * d.inek) : 0; }   // 6 saatte bir
int yun() { return d.koyun && tok() > d.yunSaat ? min(2 * d.koyun, (int)((tok() - d.yunSaat) / 43200) * d.koyun) : 0; }   // 12 saatte bir
int ambarDeger() { int t = 0; for (int i = 0; i < 9; i++) t += d.ambar[i] * URUN_FIYAT[i]; return t; }
int ambarToplam() { int t = 0; for (int i = 0; i < 9; i++) t += d.ambar[i]; return t; }

void bitkiCiz(int cx, int alt, int tur, float o) {   // o: 0..1 büyüme
  if (o < .25f) { ekran.drawPixel(cx - 2, alt - 1, W); ekran.drawPixel(cx + 2, alt - 1, W); return; }
  if (o < .6f) { ekran.drawLine(cx, alt, cx, alt - 3, W); ekran.drawLine(cx, alt - 3, cx - 2, alt - 5, W); ekran.drawLine(cx, alt - 3, cx + 2, alt - 5, W); return; }
  if (o < 1) { ekran.drawLine(cx, alt, cx, alt - 7, W); for (int k = 0; k < 3; k++) { ekran.drawLine(cx, alt - 2 - k * 2, cx - 3, alt - 4 - k * 2, W); ekran.drawLine(cx, alt - 2 - k * 2, cx + 3, alt - 4 - k * 2, W); } return; }
  switch (tur) {
    case 1: ekran.fillCircle(cx, alt - 3, 3, W); ekran.drawLine(cx, alt - 6, cx - 2, alt - 10, W); ekran.drawLine(cx, alt - 6, cx + 2, alt - 10, W); break;
    case 2: ekran.fillTriangle(cx - 3, alt - 7, cx + 3, alt - 7, cx, alt, W); ekran.drawLine(cx, alt - 7, cx - 3, alt - 11, W); ekran.drawLine(cx, alt - 7, cx + 3, alt - 11, W); break;
    case 3: ekran.drawLine(cx, alt, cx, alt - 10, W); ekran.fillCircle(cx - 3, alt - 4, 2, W); ekran.fillCircle(cx + 3, alt - 7, 2, W); ekran.fillCircle(cx - 2, alt - 9, 1, W); break;
    case 4: ekran.drawLine(cx, alt, cx, alt - 13, W); ekran.fillRoundRect(cx + 1, alt - 11, 3, 7, 1, W); ekran.drawLine(cx, alt - 5, cx - 4, alt - 8, W); break;
    case 6: for (int k = -1; k <= 1; k += 2) { ekran.fillTriangle(cx + k * 3 - 2, alt - 6, cx + k * 3 + 2, alt - 6, cx + k * 3, alt - 2, W); ekran.drawPixel(cx + k * 3, alt - 7, W); } ekran.drawLine(cx, alt, cx, alt - 9, W); break;   // çilek
    default: ekran.fillCircle(cx, alt - 4, 5, W); for (int k = -3; k <= 3; k += 3) ekran.drawFastVLine(cx + k, alt - 8, 8, B); break;
  }
}
void mevsimCiz(uint32_t t) {                          // kar, düşen yaprak, çiçek, güneş ışını
  int m = mevsim();
  for (int i = 0; i < 9; i++) {
    int x = (i * 37 + (m == 3 ? (int)(sinf(t / 900.0f + i) * 3) : (int)(t / 60) % 128 * (m == 2))) % 92, y = 11 + (int)((t / (m == 3 ? 70 : 110) + i * 13) % 43);
    if (m == 3) ekran.drawPixel(x, y, W);
    else if (m == 2 && i < 4) { ekran.drawPixel(x, y, W); ekran.drawPixel(x + 1, y + 1, W); }
  }
  if (m == 0) for (int i = 0; i < 6; i++) { int x = 5 + i * 15, y = 53; ekran.drawPixel(x, y - 1, W); ekran.drawPixel(x - 1, y - 2, W); ekran.drawPixel(x + 1, y - 2, W); }
}
void ciz() {
  uint32_t t = millis();
  mevsimCiz(t);
  ekran.fillRect(0, 0, 128, 10, B);
  yaz(0, 1, 1, String(d.para) + "TL"); yaz(46, 1, 1, String(MEVSIM[mevsim()]).substring(0, 3));
  if (saatVar()) { tm z = simdiTm(); yaz(128 - 30, 1, 1, ikiHane(z.tm_hour) + ":" + ikiHane(z.tm_min)); bool gece = z.tm_hour < 7 || z.tm_hour >= 20;
    if (gece) { ekran.fillCircle(84, 4, 3, W); ekran.fillCircle(86, 3, 3, B); } else ekran.drawCircle(85, 4, 3, W); }
  else yaz(70, 1, 1, "saat yok");
  ekran.drawFastHLine(0, 10, 128, W);
  for (int i = 0; i < 8; i++) {
    int x = 3 + (i % 4) * 23, y = 15 + (i / 4) * 20;
    if (i >= d.tarlaN) { for (int k = 0; k < 20; k += 3) ekran.drawPixel(x + k, y + 14, W); continue; }
    ekran.drawRect(x, y + 10, 20, 5, W); for (int k = 2; k < 19; k += 4) ekran.drawPixel(x + k, y + 12, W);
    if (mevsim() == 3) ekran.drawFastHLine(x, y + 10, 20, W);   // karlı toprak
    if (d.t[i].bitki) bitkiCiz(x + 10, y + 10, d.t[i].bitki, olgunluk(d.t[i]));
    if (d.t[i].sulu && olgunluk(d.t[i]) < 1) ekran.drawPixel(x + 18, y + 8, W);
    if (d.t[i].bitki && olgunluk(d.t[i]) >= 1 && (t / 300) % 2) ekran.drawPixel(x + 10, y - 2, W);
  }
  const char *YER[4] = {"PAZ", "KUM", "AHR", "AYS"};  // sağ sütun: pazar, kümes, ahır, komşu Ayşe
  for (int i = 0; i < 4; i++) {
    int y = 12 + i * 10; ekran.drawRoundRect(94, y, 34, 9, 2, W); yaz(97, y + 1, 1, YER[i]);
    int adet = i == 1 ? yumurta() : i == 2 ? sut() + yun() : 0;
    if (adet) yaz(119, y + 1, 1, String(min(adet, 9)));
    if (i == 3) for (int k = 0; k < min(5, d.kalp / 2 + 1) && d.kalp; k++) ekran.drawPixel(118 + k * 2, y + 6, W);
  }
  int hedefX = imlec < 8 ? 3 + (imlec % 4) * 23 + 6 : 86, hedefY = imlec < 8 ? 15 + (imlec / 4) * 20 + 4 : 10 + (imlec - 8) * 10;
  kx += (hedefX - kx) * .25f;
  if ((t / 400) % 2) { if (imlec < 8) ekran.drawRoundRect(2 + (imlec % 4) * 23, 14 + (imlec / 4) * 20, 22, 18, 3, W); else ekran.fillRect(94, 12 + (imlec - 8) * 10 + 8, 34, 1, W); }
  kucukKubit((int)kx + (imlec < 8 ? 10 : 0), hedefY, fabsf(hedefX - kx) > 1 ? t / 40 : 0);
  if (sulamaAnim > 0) for (int k = 0; k < 4; k++) ekran.drawPixel((int)kx + 9 + k * 2, hedefY + 2 + (int)((1 - sulamaAnim) * 8 + k) % 8, W);
  ekran.fillRect(0, 54, 128, 10, B); ekran.drawFastHLine(0, 54, 128, W);
  String alt;
  if (millis() < bilgiBitis) alt = bilgi;
  else if (imlec < d.tarlaN) {
    const Tarla &tr = d.t[imlec];
    if (!tr.bitki) alt = "Bos: " + String(BITKI[d.tohum].ad) + " ek " + String(BITKI[d.tohum].tohum) + "TL";
    else { float o = olgunluk(tr); const Bitki &b = BITKI[tr.bitki - 1];
      alt = o >= 1 ? String(b.ad) + " hazir!" : String(b.ad) + " " + sureYazi((uint32_t)((1 - o) * b.sure * (tr.sulu ? .7f : 1) * hiz(tr.bitki))) + (tr.sulu ? "" : " sula"); }
  } else if (imlec < 8) alt = "Tarla al (pazar)";
  else if (imlec == 8) alt = "Ambar " + String(ambarToplam()) + " urun " + String(ambarDeger()) + "TL";
  else if (imlec == 9) alt = d.tavuk ? String(d.tavuk) + " tavuk, " + String(yumurta()) + " yumurta" : "Kumes bos";
  else if (imlec == 10) alt = d.inek || d.koyun ? String(d.inek) + " inek " + String(d.koyun) + " koyun" + (d.saat > d.besleme + 86400 ? " ac!" : "") : "Ahir bos";
  else alt = "Ayse: dostluk " + String(d.kalp) + "/10";
  if (alt.length() > 21) alt = alt.substring(0, 21);
  yaz(0, 56, 1, alt);
}
void kutu(const char *bas, String *og, int n) {
  ekran.fillRect(6, 8, 116, 48, B); ekran.drawRoundRect(6, 8, 116, 48, 3, W); yaz(11, 10, 1, bas); ekran.drawFastHLine(9, 18, 110, W);
  int ilk = constrain(menuImlec - 2, 0, max(0, n - 4));
  for (int i = ilk; i < min(n, ilk + 4); i++) { int y = 21 + (i - ilk) * 8; if (i == menuImlec) { ekran.fillRect(8, y - 1, 112, 8, W); ekran.setTextColor(B); } yaz(11, y, 1, og[i].substring(0, 18)); ekran.setTextColor(W); }
}
void pazar(bool gez, bool sec) {
  const int N = 7;
  String og[N] = {"Sat: " + String(ambarDeger()) + "TL", "Tohum: " + String(BITKI[d.tohum].ad) + " " + String(BITKI[d.tohum].tohum),
                  d.tarlaN < 8 ? "Tarla " + String(200 * (d.tarlaN - 3)) + "TL" : String("Tarla: tamam"),
                  d.tavuk < 3 ? String("Tavuk 300TL") : String("Tavuk: dolu"), d.inek < 2 ? String("Inek 600TL") : String("Inek: dolu"),
                  d.koyun < 2 ? String("Koyun 450TL") : String("Koyun: dolu"), "Geri"};
  if (gez) menuImlec = (menuImlec + 1) % N;
  if (sec) {
    auto al = [&](int fiyat) { if (d.para < fiyat) { soyle("Para yetmiyor"); return false; } d.para -= fiyat; return true; };
    switch (menuImlec) {
      case 0: { int v = ambarDeger(); if (v) { d.para += v; memset(d.ambar, 0, sizeof(d.ambar)); soyle("Satildi +" + String(v) + "TL"); } else soyle("Ambar bos"); } break;
      case 1: do d.tohum = (d.tohum + 1) % 6; while ((d.tohum == 5 && d.kalp < 5) || (mevsim() == 3 && !kisinOlur(d.tohum + 1))); break;
      case 2: if (d.tarlaN < 8 && al(200 * (d.tarlaN - 3))) { d.tarlaN++; soyle("Yeni tarla!"); } break;
      case 3: if (d.tavuk < 3 && al(300)) { if (!d.tavuk) d.yumurtaSaat = d.saat; d.tavuk++; soyle("Gidak gidak!"); } break;
      case 4: if (d.inek < 2 && al(600)) { if (!d.inek) { d.sutSaat = d.saat; d.besleme = d.saat; } d.inek++; soyle("Moo!"); } break;
      case 5: if (d.koyun < 2 && al(450)) { if (!d.koyun) { d.yunSaat = d.saat; d.besleme = d.saat; } d.koyun++; soyle("Mee!"); } break;
      case 6: menu = 0; break;
    }
    kaydet();
  }
  ciz(); kutu(("PAZAR  " + String(d.para) + "TL").c_str(), og, N);
}
void ahir(bool gez, bool sec) {
  String og[3] = {"Besle (1 urun ya da 10TL)", "Topla: " + String(sut()) + " sut " + String(yun()) + " yun", "Geri"};
  if (gez) menuImlec = (menuImlec + 1) % 3;
  if (sec) {
    if (menuImlec == 0) {
      if (!d.inek && !d.koyun) soyle("Ahir bos (pazar)");
      else { int i = 0; while (i < 6 && !d.ambar[i]) i++; if (i < 6) d.ambar[i]--; else if (d.para >= 10) d.para -= 10; else { soyle("Yem yok"); return; }
             if (d.saat > d.besleme + 86400) { d.sutSaat = max(d.sutSaat, d.saat); d.yunSaat = max(d.yunSaat, d.saat); }   // aç kaldıkları süre üretim sayılmaz
             d.besleme = d.saat; soyle("Hayvanlar doydu"); }
    }
    if (menuImlec == 1) { int s = sut(), y = yun(); d.ambar[7] += s; d.ambar[8] += y; if (s) d.sutSaat = d.saat; if (y) d.yunSaat = d.saat; soyle(s || y ? "Toplandi" : "Henuz yok"); }
    if (menuImlec == 2) menu = 0;
    kaydet();
  }
  ciz(); kutu("AHIR", og, 3);
}
void komsu(bool gez, bool sec) {                      // hediye: ambardaki bir ürün; günde bir kez; sevdiği iki kat
  uint8_t bugun = d.saat / 86400 % 256;
  int sahip[9], n = 0; for (int i = 0; i < 9; i++) if (d.ambar[i]) sahip[n++] = i;
  String og[2] = {n ? "Hediye: " + String(URUN_AD[sahip[hediyeSec % n]]) : String("Ambarda urun yok"), "Geri"};
  if (gez) { if (menuImlec == 0 && n > 1) hediyeSec++; else menuImlec = (menuImlec + 1) % 2; }   // hediye satırında sağ: başka ürün
  if (sec) {
    if (menuImlec == 1) { menu = 0; return; }
    if (!n) soyle("Once hasat et");
    else if (d.hediyeGun == bugun && d.kalp) soyle("Ayse: yarin gel :)");
    else {
      int u = sahip[hediyeSec % n]; d.ambar[u]--; d.hediyeGun = bugun;
      int artis = u == SEVDIGI[mevsim()] ? 2 : 1; d.kalp = min(10, d.kalp + artis);
      soyle(artis > 1 ? "Ayse: en sevdigim!" : "Ayse: tesekkurler!");
      if (d.kalp >= 3 && !(d.odul & 1)) { d.odul |= 1; d.para += 100; soyle("Ayse: 100TL hediye!"); }
      if (d.kalp >= 5 && !(d.odul & 2)) { d.odul |= 2; soyle("Cilek tohumu acildi!"); }
      if (d.kalp >= 10 && !(d.odul & 4)) { d.odul |= 4; d.para += 1000; soyle("En iyi dost! +1000TL"); }
      kaydet();
    }
  }
  ciz(); kutu(("AYSE  " + String(d.kalp) + "/10").c_str(), og, 2);
  ekran.fillRect(8, 40, 112, 14, B); yaz(11, 41, 1, "Sevdigi: " + String(URUN_AD[SEVDIGI[mevsim()]])); yaz(11, 48, 1, "sag: urun degistir");
}

bool oyna(Girdi &g) {
  if (g.ilk) { yukle(); imlec = 0; menu = 0; kx = 10; }
  kayitSay += g.dt;
  static float kesir = 0; kesir += g.dt; if (kesir >= 1) { d.saat += (uint32_t)kesir; kesir -= (int)kesir; }
  if (kayitSay > 30) { kayitSay = 0; kaydet(); }
  sulamaAnim -= g.dt;
  bool gez = g.o1 == KISA, sec = g.o2 == KISA;
  if (menu == 1) { pazar(gez, sec); return true; }
  if (menu == 2) { ahir(gez, sec); return true; }
  if (menu == 3) { komsu(gez, sec); return true; }
  if (gez) imlec = (imlec + 1) % YER_N;
  if (sec) {
    if (imlec < d.tarlaN) {
      Tarla &tr = d.t[imlec];
      if (!tr.bitki) {
        const Bitki &b = BITKI[d.tohum];
        if (mevsim() == 3 && !kisinOlur(d.tohum + 1)) soyle("Kisin yetismez");
        else if (d.para >= b.tohum) { d.para -= b.tohum; tr = {(uint8_t)(d.tohum + 1), 0, d.saat}; soyle(String(b.ad) + " ekildi"); } else soyle("Para yetmiyor");
      }
      else if (olgunluk(tr) >= 1) { d.ambar[tr.bitki - 1]++; sayac("b_hasat", 1); soyle(String(BITKI[tr.bitki - 1].ad) + " ambara"); tr = {0, 0, 0}; }
      else if (!tr.sulu) { tr.sulu = 1; sulamaAnim = .6f; soyle("Sulandi: %30 hizli"); }
      else soyle("Buyuyor, sabret");
    } else if (imlec < 8) { menu = 1; menuImlec = 2; }
    else if (imlec == 8) { menu = 1; menuImlec = 0; }
    else if (imlec == 9) { int y = yumurta(); if (y) { d.ambar[6] += y; d.yumurtaSaat = d.saat; soyle(String(y) + " yumurta ambara"); } else soyle(d.tavuk ? "Yumurta yok, bekle" : "Kumes bos"); }
    else if (imlec == 10) { menu = 2; menuImlec = 0; }
    else { menu = 3; menuImlec = 0; hediyeSec = 0; }
    kaydet();
  }
  ciz();
  return true;
}
}  // namespace kasaba
bool kubitKasabasi(Girdi &g) { return kasaba::oyna(g); }
