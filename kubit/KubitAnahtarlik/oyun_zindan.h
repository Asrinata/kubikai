// Zindan: roguelike. Sol tık: 90° dön. Sağ tık: baktığın yöne adım (canavar varsa saldır); sağ basılı: yürümeye devam.
// İki tuşa kısa tık: sınıf yeteneği, iki tuşu tut: iksir. Sınıflar: Savaşçı (döner kılıç), Büyücü (ateş topu, mana), Okçu (ok).
// Her 3 katta bir tüccar var (üstüne bas: dükkan). Her kat yeni üretilir, merdivenle (>) in. Ölünce biter; en derin kat rekor.
#pragma once

namespace zindan {
const int ZW = 32, ZH = 20;
enum Karo : uint8_t { DUVAR, ZEMIN, MERDIVEN, DUKKAN };
const char *SINIF[3] = {"Savasci", "Buyucu", "Okcu"};
const uint8_t TUCCAR[8] = {0x3C, 0x7E, 0x18, 0x3C, 0x7E, 0xBD, 0x3C, 0x24};
struct Canavar { int8_t x, y; uint8_t tur; int8_t can; bool var; };
struct Esya { int8_t x, y; uint8_t tur; bool var; };   // 0 altın, 1 iksir, 2 kılıç, 3 kalkan
struct CTur { const char *ad; int8_t can, guc; };
const CTur CT[5] = {{"Yarasa", 3, 2}, {"Balcik", 6, 2}, {"Iskelet", 9, 4}, {"Ork", 15, 6}, {"Ejderha", 30, 9}};
const uint8_t C_SPR[5][8] = {{0x00, 0x81, 0xC3, 0xFF, 0x5A, 0x24, 0x00, 0x00}, {0x00, 0x00, 0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0x00},
  {0x3C, 0x5A, 0x3C, 0x18, 0x7E, 0x18, 0x24, 0x42}, {0x3C, 0x7E, 0x5A, 0x7E, 0xFF, 0xBD, 0x24, 0x66}, {0x60, 0xF6, 0x7F, 0x3E, 0x7C, 0xFE, 0x24, 0x66}};
const uint8_t E_SPR[4][8] = {{0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00}, {0x18, 0x18, 0x24, 0x42, 0x7E, 0x7E, 0x3C, 0x00},
  {0x01, 0x02, 0x04, 0x48, 0x30, 0x30, 0x48, 0x80}, {0x7E, 0x5A, 0x7E, 0x7E, 0x7E, 0x3C, 0x18, 0x00}};
const uint8_t KAHRAMAN[8] = {0x18, 0x3C, 0x18, 0x7E, 0x18, 0x18, 0x24, 0x24};
const uint8_t TUGLA[8] = {0xFF, 0x88, 0xFF, 0x22, 0xFF, 0x88, 0xFF, 0x22}, HATIRA[8] = {0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00};
const uint8_t MERD[8] = {0x00, 0x03, 0x03, 0x0F, 0x0F, 0x3F, 0x3F, 0xFF};
const int8_t DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
uint8_t m[ZH][ZW]; bool gordu[ZH][ZW], gorunur[ZH][ZW];
Canavar cv[16]; Esya es[12];
int px, py, yon, can, maxCan, guc, zirh, altin, iksir, kat, sinif, mana, ok, yetenekBekle, hal, imlec; bool bitti;   // hal 0 sınıf, 1 oyun, 2 dükkan
float efektT; int efX1, efY1, efX2, efY2;
float kx, ky, adimT, vurusFlas; String mesaj; uint32_t mesajBitis;

void soyle(const String &s) { mesaj = s; mesajBitis = millis() + 1600; }
bool bosMu(int x, int y) { return x >= 0 && y >= 0 && x < ZW && y < ZH && m[y][x] != DUVAR; }
Canavar *canavarAt(int x, int y) { for (auto &c : cv) if (c.var && c.x == x && c.y == y) return &c; return nullptr; }
void gorus() {                                        // 4 karelik görüş, duvar arkası görünmez
  memset(gorunur, 0, sizeof(gorunur));
  for (int y = py - 4; y <= py + 4; y++) for (int x = px - 4; x <= px + 4; x++) {
    if (x < 0 || y < 0 || x >= ZW || y >= ZH || (x - px) * (x - px) + (y - py) * (y - py) > 18) continue;
    int n = max(abs(x - px), abs(y - py)); bool acik = true;
    for (int i = 1; i < n && acik; i++) { int ax = px + (x - px) * i / n, ay = py + (y - py) * i / n; if (m[ay][ax] == DUVAR) acik = false; }
    if (acik) gorunur[y][x] = gordu[y][x] = true;
  }
}
void uret() {                                         // odalar + L koridorlar; ilk odada sen, son odada merdiven
  memset(m, 0, sizeof(m)); memset(gordu, 0, sizeof(gordu));
  for (auto &c : cv) c.var = false; for (auto &e : es) e.var = false;
  int oda[10][4], n = 0;
  for (int deneme = 0; deneme < 60 && n < 9; deneme++) {
    int w = random(4, 8), h = random(3, 6), x = random(1, ZW - w - 1), y = random(1, ZH - h - 1); bool cak = false;
    for (int i = 0; i < n; i++) if (x < oda[i][0] + oda[i][2] + 1 && x + w + 1 > oda[i][0] && y < oda[i][1] + oda[i][3] + 1 && y + h + 1 > oda[i][1]) cak = true;
    if (cak) continue;
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) m[yy][xx] = ZEMIN;
    oda[n][0] = x; oda[n][1] = y; oda[n][2] = w; oda[n][3] = h; n++;
  }
  for (int i = 1; i < n; i++) {
    int ax = oda[i - 1][0] + oda[i - 1][2] / 2, ay = oda[i - 1][1] + oda[i - 1][3] / 2, bx = oda[i][0] + oda[i][2] / 2, by = oda[i][1] + oda[i][3] / 2;
    for (int x = min(ax, bx); x <= max(ax, bx); x++) m[ay][x] = ZEMIN;
    for (int y = min(ay, by); y <= max(ay, by); y++) m[y][bx] = ZEMIN;
  }
  px = oda[0][0] + 1; py = oda[0][1] + 1; yon = 0;
  m[oda[n - 1][1] + oda[n - 1][3] / 2][oda[n - 1][0] + oda[n - 1][2] / 2] = MERDIVEN;
  if (kat % 3 == 0 && n > 2) m[oda[1][1]][oda[1][0]] = DUKKAN;   // her 3 katta tüccar
  int ci = 0, ei = 0;
  for (int i = 1; i < n; i++) {
    for (int k = 0; k < 1 + random(2) && ci < 16; k++) {
      int t = min(4, (int)random(0, 2 + kat / 2)); if (kat >= 5 && i == n - 1 && k == 0) t = 4;   // 5. kattan sonra son odada ejderha
      cv[ci++] = {(int8_t)(oda[i][0] + random(oda[i][2])), (int8_t)(oda[i][1] + random(oda[i][3])), (uint8_t)t, (int8_t)(CT[t].can + kat), true};
    }
    if (random(3) && ei < 12) { int r = random(10); es[ei++] = {(int8_t)(oda[i][0] + random(oda[i][2])), (int8_t)(oda[i][1] + random(oda[i][3])), (uint8_t)(r < 5 ? 0 : r < 8 ? 1 : r < 9 ? 2 : 3), true}; }
  }
  kx = px * 8 - 60; ky = py * 8 - 28;
  gorus();
}
void canavarlar() {
  for (auto &c : cv) {
    if (!c.var) continue;
    int dx = px - c.x, dy = py - c.y;
    if (abs(dx) + abs(dy) == 1) {                     // bitişik: saldır
      int h = max(1, CT[c.tur].guc + kat / 2 - zirh + (int)random(-1, 2)); can -= h; vurusFlas = .2f;
      soyle(String(CT[c.tur].ad) + " vurdu -" + String(h));
      if (can <= 0) { bitti = true; return; }
      continue;
    }
    if (!gorunur[c.y][c.x] && random(3)) continue;     // görmüyorsa ara sıra gezinir
    int sx = gorunur[c.y][c.x] ? (dx > 0) - (dx < 0) : (int)random(-1, 2), sy = gorunur[c.y][c.x] ? (dy > 0) - (dy < 0) : (int)random(-1, 2);
    if (c.tur == 0 && random(2)) { sx = random(-1, 2); sy = random(-1, 2); }   // yarasa sağa sola çırpınır
    int nx = c.x, ny = c.y;
    if (sx && (abs(dx) >= abs(dy) || !sy) && bosMu(c.x + sx, c.y) && !canavarAt(c.x + sx, c.y)) nx += sx;
    else if (sy && bosMu(c.x, c.y + sy) && !canavarAt(c.x, c.y + sy)) ny += sy;
    if (!(nx == px && ny == py)) { c.x = nx; c.y = ny; }
  }
}
void tur();
void adim() {                                         // baktığın yöne: saldır, yürü, eşya al, merdivenden in
  int nx = px + DX[yon], ny = py + DY[yon];
  if (Canavar *c = canavarAt(nx, ny)) {
    int h = max(1, guc + (int)random(-1, 2)); c->can -= h;
    if (c->can <= 0) { c->var = false; int a = 2 + random(3 + kat); altin += a; soyle(String(CT[c->tur].ad) + " oldu +" + String(a) + "a"); }
    else soyle(String(CT[c->tur].ad) + " -" + String(h));
  } else if (bosMu(nx, ny)) {
    px = nx; py = ny;
    for (auto &e : es) if (e.var && e.x == px && e.y == py) {
      e.var = false;
      if (e.tur == 0) { int a = 3 + random(5 + kat * 2); altin += a; soyle("+" + String(a) + " altin"); }
      if (e.tur == 1) { iksir++; soyle("Iksir buldun"); }
      if (e.tur == 2) { guc++; soyle("Kilic! guc " + String(guc)); }
      if (e.tur == 3) { zirh++; soyle("Kalkan! zirh " + String(zirh)); }
    }
    if (m[py][px] == MERDIVEN) { kat++; uret(); soyle("Kat " + String(kat)); return; }
    if (m[py][px] == DUKKAN) { hal = 2; imlec = 0; return; }
  } else return;                                     // duvar: tur geçmez
  tur();
}
void tur() {                                          // her eylemden sonra: canavarlar oynar, mana/ok dolar
  gorus(); canavarlar(); gorus();
  if (sinif == 1 && mana < 9) mana++;
  if (sinif == 2 && ok < 10 && random(4) == 0) ok++;
  if (yetenekBekle > 0) yetenekBekle--;
}
void hatHasar(int menzil, int hasar) {                // baktığın yönde ilk canavara (duvara kadar)
  int x = px, y = py;
  for (int i = 0; i < menzil; i++) {
    x += DX[yon]; y += DY[yon];
    if (!bosMu(x, y)) break;
    if (Canavar *c = canavarAt(x, y)) {
      c->can -= hasar; efX2 = x; efY2 = y;
      if (c->can <= 0) { c->var = false; int a = 2 + random(3 + kat); altin += a; soyle(String(CT[c->tur].ad) + " oldu +" + String(a) + "a"); } else soyle(String(CT[c->tur].ad) + " -" + String(hasar));
      return;
    }
    efX2 = x; efY2 = y;
  }
  soyle("Iskaladin");
}
void yetenek() {
  efX1 = px; efY1 = py; efX2 = px; efY2 = py;
  if (sinif == 0) {                                   // döner kılıç: dört yandaki herkese
    if (yetenekBekle > 0) { soyle("Bekle: " + String(yetenekBekle) + " tur"); return; }
    int vurulan = 0;
    for (int k = 0; k < 4; k++) if (Canavar *c = canavarAt(px + DX[k], py + DY[k])) { c->can -= guc + 2; vurulan++; if (c->can <= 0) { c->var = false; altin += 2 + random(3 + kat); } }
    soyle(vurulan ? "Doner kilic! " + String(vurulan) : "Kimse yok"); yetenekBekle = 5; efektT = .3f;
  } else if (sinif == 1) {
    if (mana < 3) { soyle("Mana yetmiyor"); return; }
    mana -= 3; hatHasar(6, 8); efektT = .3f;
  } else {
    if (!ok) { soyle("Ok kalmadi"); return; }
    ok--; hatHasar(7, 5); efektT = .2f;
  }
  tur();
}
void ciz() {
  float hx = px * 8 - 60, hy = py * 8 - 28;
  kx += (constrain(hx, 0.0f, ZW * 8 - 128.0f) - kx) * .35f; ky += (constrain(hy, -8.0f, ZH * 8 - 64.0f) - ky) * .35f;
  int bx = (int)kx / 8, by = (int)ky / 8;
  for (int y = by; y <= by + 8; y++) for (int x = bx; x <= bx + 16; x++) {
    if (x < 0 || y < 0 || x >= ZW || y >= ZH || !gordu[y][x]) continue;
    int sx = lroundf(x * 8 - kx), sy = lroundf(y * 8 - ky) + 8;
    if (m[y][x] == DUVAR) { bool kom = false; for (int k = 0; k < 4; k++) if (bosMu(x + DX[k], y + DY[k])) kom = true; if (kom) ekran.drawBitmap(sx, sy, gorunur[y][x] ? TUGLA : HATIRA, 8, 8, W); }
    else if (m[y][x] == MERDIVEN) ekran.drawBitmap(sx, sy, MERD, 8, 8, W);
    else if (gorunur[y][x]) ekran.drawPixel(sx + 3, sy + 4, W);
  }
  for (auto &e : es) if (e.var && gorunur[e.y][e.x]) ekran.drawBitmap(lroundf(e.x * 8 - kx), lroundf(e.y * 8 - ky) + 8, E_SPR[e.tur], 8, 8, W);
  for (auto &c : cv) if (c.var && gorunur[c.y][c.x]) ekran.drawBitmap(lroundf(c.x * 8 - kx), lroundf(c.y * 8 - ky) + 8, C_SPR[c.tur], 8, 8, W);
  int sx = lroundf(px * 8 - kx), sy = lroundf(py * 8 - ky) + 8;
  ekran.drawBitmap(sx, sy, KAHRAMAN, 8, 8, W);
  ekran.drawLine(sx + 3 + DX[yon] * 4, sy + 3 + DY[yon] * 4, sx + 3 + DX[yon] * 7, sy + 3 + DY[yon] * 7, W);   // kılıç: baktığın yön
  ekran.fillRect(0, 0, 128, 9, B); ekran.drawFastHLine(0, 8, 128, W);
  ekran.fillRect(1, 2, 5, 1, W); ekran.fillRect(3, 0, 1, 5, W); yaz(8, 0, 1, String(can) + "/" + String(maxCan));
  yaz(52, 0, 1, "K" + String(kat)); yaz(70, 0, 1, String(altin) + "a"); yaz(98, 0, 1, sinif == 1 ? "m" + String(mana) : sinif == 2 ? ">" + String(ok) : "i" + String(iksir));
  if (vurusFlas > 0) ekran.drawRect(0, 9, 128, 55, W);
  for (int y = by; y <= by + 8; y++) for (int x = bx; x <= bx + 16; x++) if (x >= 0 && y >= 0 && x < ZW && y < ZH && m[y][x] == DUKKAN && gordu[y][x]) ekran.drawBitmap(lroundf(x * 8 - kx), lroundf(y * 8 - ky) + 8, TUCCAR, 8, 8, W);
  if (efektT > 0) {                                   // yetenek izi
    int x1 = lroundf(efX1 * 8 - kx) + 4, y1 = lroundf(efY1 * 8 - ky) + 12, x2 = lroundf(efX2 * 8 - kx) + 4, y2 = lroundf(efY2 * 8 - ky) + 12;
    if (sinif == 0) ekran.drawCircle(x1, y1, 9 - (int)(efektT * 10), W);
    else { ekran.drawLine(x1, y1, x2, y2, W); if (sinif == 1) ekran.fillCircle(x2, y2, 3, W); }
  }
  if (millis() < mesajBitis) { int w = min(128, (int)mesaj.length() * 6 + 6); ekran.fillRect(64 - w / 2, 54, w, 10, B); ekran.drawRect(64 - w / 2, 54, w, 10, W); ortala(55, 1, mesaj); }
}

void dukkan(Girdi &g) {
  const int N = 6;
  int fiyat[N] = {15, 40 + kat * 5, 40 + kat * 5, 50, sinif == 1 ? 30 : sinif == 2 ? 20 : 35, 0};
  String og[N] = {"Iksir", "Kilic +1 guc", "Kalkan +1 zirh", "Can +5", sinif == 1 ? "Mana kristali" : sinif == 2 ? "5 ok" : "Bileme +1 guc", "Cik"};
  if (g.o1 == KISA) imlec = (imlec + 1) % N;
  if (g.o2 == KISA) {
    if (imlec == N - 1) { hal = 1; py += 1; if (!bosMu(px, py)) py -= 2; gorus(); return; }
    if (altin < fiyat[imlec]) soyle("Altin yetmiyor");
    else {
      altin -= fiyat[imlec];
      switch (imlec) { case 0: iksir++; break; case 1: guc++; break; case 2: zirh++; break; case 3: maxCan += 5; can += 5; break;
        case 4: if (sinif == 1) mana = 9; else if (sinif == 2) ok += 5; else guc++; break; }
      soyle("Alindi!");
    }
  }
  ciz();
  ekran.fillRect(10, 10, 108, 44, B); ekran.drawRoundRect(10, 10, 108, 44, 3, W); yaz(14, 12, 1, "TUCCAR  " + String(altin) + "a");
  int ilk = constrain(imlec - 1, 0, N - 4);
  for (int i = ilk; i < ilk + 4; i++) { int y = 22 + (i - ilk) * 8; if (i == imlec) { ekran.fillRect(12, y - 1, 104, 8, W); ekran.setTextColor(B); } yaz(14, y, 1, og[i]); if (fiyat[i]) yaz(96, y, 1, String(fiyat[i])); ekran.setTextColor(W); }
}

bool oyna(Girdi &g) {
  if (g.ilk) { hal = 0; imlec = 0; }
  if (hal == 0) {                                     // sınıf seçimi
    if (g.o1 == KISA) imlec = (imlec + 1) % 3;
    if (g.o2 == KISA) {
      sinif = imlec; const int CAN[3] = {26, 16, 20}, GUC[3] = {5, 3, 3};
      can = maxCan = CAN[sinif]; guc = GUC[sinif]; zirh = sinif == 0; mana = 9; ok = 10; yetenekBekle = 0;
      altin = 0; iksir = 1; kat = 1; bitti = false; vurusFlas = 0; uret(); soyle("Kat 1: merdiveni bul"); hal = 1; return true;
    }
    ortala(0, 1, "SINIFINI SEC");
    const char *aciklama[3] = {"guclu, doner kilic", "ates topu, mana", "uzaktan ok atar"};
    for (int i = 0; i < 3; i++) { int x = 4 + i * 42; ekran.drawRoundRect(x, 12, 36, 30, 3, W); if (i == imlec) ekran.fillRoundRect(x, 12, 36, 30, 3, W);
      ekran.setTextColor(i == imlec ? B : W); ekran.drawBitmap(x + 14, 16, KAHRAMAN, 8, 8, i == imlec ? B : W); yaz(x + 3, 30, 1, String(SINIF[i]).substring(0, 5)); ekran.setTextColor(W); }
    ortala(48, 1, aciklama[imlec]); ortala(56, 1, "sag:gez sol:sec");
    return true;
  }
  if (hal == 2) { dukkan(g); return true; }
  if (bitti) { int b = bitis(g, "Oldun! Kat " + String(kat) + " " + String(altin) + "a", "Rekor kat " + String(rekorKaydet("zindan", kat))); if (b == 1) { hal = 0; bitti = false; return true; } return b != 2; }
  vurusFlas -= g.dt; efektT -= g.dt;
  if (g.combo && g.comboSure < 600) yetenek();
  else if (g.combo) { if (iksir && can < maxCan) { iksir--; can = min(maxCan, can + 12); soyle("Iksir: +12 can"); } else soyle(iksir ? "Canin dolu" : "Iksir yok"); }
  if (g.o2 == KISA) yon = (yon + 1) % 4;
  if (g.o1 == BASTI) { adim(); adimT = .35f; }
  else if (g.b1 && !g.b2 && (adimT -= g.dt) <= 0) { adim(); adimT = .2f; }   // basılı tut: yürümeye devam
  if (kat > 0 && can > 0 && maxCan < 20 + kat * 3) { maxCan = 20 + kat * 3; }   // her kat azıcık güçlendirir
  ciz();
  return true;
}
}  // namespace zindan
bool zindanOyun(Girdi &g) { return zindan::oyna(g); }
