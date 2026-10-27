// Süper Kübit: Mario tarzı platform. Sol/sağ basılı: koş. İki tuşa birlikte bas: zıpla (basılı tutarsan daha yüksek).
// Koşarken öbür tuşa da basınca koştuğun yöne zıplar. Bölümler numarasından üretilir, dünya haritasından seçilir (4 bölüm bir dünya).
// Büyükken mantar bloğu ateş çiçeği verir: bir yön tuşuna çift tık ateş topu atar. İlerleme NVS "mario_il".
#pragma once

namespace mario {
const int SAT = 8, LW = 168, T = 8;                   // bölüm: 8 satır x 168 sütun, karo 8 piksel
enum Karo : uint8_t { BOS, ZEMIN, TUGLA, SORU, MANTARK, KULLANILMIS, BORU, PARA, BAYRAK };
const uint8_t DOKU[9][8] = {
  {0}, {0xFF, 0x91, 0x91, 0xFF, 0x11, 0x11, 0xFF, 0x89}, {0xFF, 0x08, 0x08, 0xFF, 0x80, 0x80, 0xFF, 0x08},
  {0x7E, 0x99, 0xA5, 0x85, 0x89, 0x81, 0x89, 0x7E}, {0x7E, 0x99, 0xA5, 0x85, 0x89, 0x81, 0x89, 0x7E},
  {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF}, {0}, {0x00, 0x18, 0x24, 0x24, 0x24, 0x24, 0x18, 0x00}, {0}};
const uint8_t KUCUK[2][8] = {{0x3C, 0x42, 0xA5, 0x81, 0x99, 0x42, 0x3C, 0x42}, {0x3C, 0x42, 0xA5, 0x81, 0x99, 0x42, 0x3C, 0x24}};
const uint8_t BUYUK_GOVDE[2][7] = {{0x3C, 0x7E, 0xBD, 0xBD, 0x7E, 0x24, 0x66}, {0x3C, 0x7E, 0xBD, 0xBD, 0x7E, 0x66, 0x42}};
const uint8_t MANTAR_S[8] = {0x3C, 0x5A, 0xBD, 0xFF, 0x24, 0x3C, 0x00, 0x00};
const uint8_t DUSMAN_S[2][8] = {{0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0x3C, 0x66, 0xC3}, {0x3C, 0x7E, 0xDB, 0xFF, 0x7E, 0x3C, 0x66, 0x66}};
struct Varlik { float x, y, vx, vy; uint8_t tur; bool var; float ezik; };   // tur 0 düşman, 1 mantar, 2 uçan para
uint8_t h[SAT][LW];
Varlik v[24];
float x, y, vx, vy, kam, sure, korunma, bitisT;
bool yerde, buyuk, onceIkisi, bitti, gecti, atesli; int yon, bolum, para, puan, can, oncekiYon, hal, ilerleme, secim;
float haritaX; uint32_t sonTik[2]; int secim2;
const uint8_t CICEK_S[8] = {0x66, 0xFF, 0xDB, 0xFF, 0x66, 0x18, 0x5A, 0x3C};
String bolumAd(int n) { return n < 0 ? String("OZEL") : String(n / 4 + 1) + "-" + String(n % 4 + 1); }

bool kati(int tx, int ty) { if (tx < 0 || tx >= LW) return true; if (ty < 0 || ty >= SAT) return false; uint8_t k = h[ty][tx]; return k != BOS && k != PARA && k != BAYRAK; }
void ekle(float ex, float ey, uint8_t tur, float evx) { for (auto &e : v) if (!e.var) { e = {ex, ey, evx, 0, tur, true, 0}; return; } }
int boy() { return buyuk ? 14 : 8; }

void ozelKur() {                                      // editördeki bölüm: . # b ? M c g P F
  memset(h, 0, sizeof(h)); for (auto &e : v) e.var = false;
  bool bayrak = false;
  for (int r = 0; r < SAT; r++) for (int c = 0; c < LW; c++) {
    char k = c < OZ_SUT ? ozelMario[r * OZ_SUT + c] : (r >= 6 ? '#' : '.');
    h[r][c] = k == '#' ? ZEMIN : k == 'b' ? TUGLA : k == '?' ? SORU : k == 'M' ? MANTARK : k == 'P' ? BORU : k == 'c' ? PARA : k == 'F' ? BAYRAK : BOS;
    if (k == 'g') ekle(c * T, r * T, 0, -18);
    if (k == 'F') bayrak = true;
  }
  if (!bayrak) h[5][OZ_SUT + 4] = BAYRAK;
  x = 2 * T; y = 0; vx = vy = 0; kam = 0; sure = 200; yon = 1; gecti = false; korunma = 0;
  while (y < 6 * T && !kati((int)(x / T), (int)((y + boy()) / T))) y += 1;   // başlangıç: ilk sütundaki zeminin üstü
}
void uret(int n) {                                    // bölüm n: aynı numara hep aynı bölüm (-1: özel)
  if (n < 0) { ozelKur(); return; }
  randomSeed(1000 + n * 7919);
  memset(h, 0, sizeof(h)); for (auto &e : v) e.var = false;
  for (int c = 0; c < LW; c++) h[6][c] = h[7][c] = ZEMIN;
  int c = 14;
  while (c < LW - 30) {
    int r = random(10);
    if (r < 2 && c > 20) { int w = 2 + random(1 + min(2, n / 2)); for (int k = 0; k < w; k++) h[6][c + k] = h[7][c + k] = BOS; c += w + 3; }   // uçurum
    else if (r < 4) { int hh = 1 + random(min(3, 1 + n / 2) + 1); hh = min(hh, 3); for (int k = 0; k < hh; k++) h[5 - k][c] = h[5 - k][c + 1] = BORU; if (random(2)) ekle((c + 4) * T, 5 * T, 0, -18); c += 6; }   // boru
    else if (r < 6) { int w = 3 + random(3); for (int k = 0; k < w; k++) h[3][c + k] = k % 2 ? (random(4) ? SORU : MANTARK) : TUGLA; if (random(2)) ekle((c + 1) * T, 5 * T, 0, -18); c += w + 3; }   // tuğla sırası
    else if (r < 7) { for (int k = 0; k < 3; k++) for (int j = 0; j <= k; j++) h[5 - j][c + k] = ZEMIN; c += 6; }   // basamak
    else if (r < 9) { int k = 1 + random(1 + min(3, n)); for (int i = 0; i < k; i++) ekle((c + i * 2) * T, 5 * T, 0, -18 - n * 2); c += k * 2 + 3; }   // düşmanlar
    else { for (int k = 0; k < 4; k++) h[3 - (k == 1 || k == 2)][c + k] = PARA; c += 6; }   // para kavisi
  }
  int s = LW - 26;                                    // son: merdiven, bayrak
  for (int k = 0; k < 5; k++) for (int j = 0; j <= k; j++) h[5 - j][s + k] = ZEMIN;
  h[5][LW - 12] = BAYRAK;
  x = 3 * T; y = 6 * T - boy(); vx = vy = 0; kam = 0; sure = 150; yon = 1; gecti = false; korunma = 0;
  randomSeed(esp_random());
}
void olum() {
  if (--can <= 0) { bitti = true; return; }
  buyuk = false; uret(bolum);
}
void kafaVurdu(int tx, int ty) {                     // aşağıdan bloğa vurmak
  uint8_t &k = h[ty][tx];
  if (k == SORU) { k = KULLANILMIS; para++; puan += 100; ekle(tx * T, (ty - 1) * T, 2, 0); }
  else if (k == MANTARK) { k = KULLANILMIS; if (buyuk) ekle(tx * T, (ty - 1) * T, 3, 0); else ekle(tx * T, (ty - 1) * T, 1, 22); }   // büyükse ateş çiçeği
  else if (k == TUGLA && buyuk) { k = BOS; puan += 50; }
}
void fizik(float dt, int yuru, bool ikisi) {
  float hedef = yuru * 72;
  vx += constrain(hedef - vx, -260 * dt, 260 * dt);   // ivmelenme: kayar gibi durur
  float nx = x + vx * dt;
  if (nx < kam) { nx = kam; vx = 0; }                  // geri dönemezsin (ekran sol kenarı)
  int ust = (int)floorf(y / T), alt = (int)floorf((y + boy() - 1) / T);
  bool engel = false;
  for (int ty = ust; ty <= alt; ty++) if (kati((int)floorf((vx > 0 ? nx + 5 : nx) / T), ty)) engel = true;
  if (engel) { vx = 0; } else x = nx;
  vy += (ikisi && vy < 0 ? 520 : 1150) * dt; vy = min(vy, 320.0f);   // iki tuş basılıyken daha az yerçekimi: yüksek zıplama
  float ny = y + vy * dt; yerde = false;
  int sol = (int)floorf(x / T), sag = (int)floorf((x + 5) / T);
  if (vy > 0) {
    int ty = (int)floorf((ny + boy()) / T);
    if (kati(sol, ty) || kati(sag, ty)) { ny = ty * T - boy(); vy = 0; yerde = true; }
  } else if (vy < 0) {
    int ty = (int)floorf(ny / T);
    for (int tx : {sol, sag}) if (kati(tx, ty)) { ny = (ty + 1) * T; vy = 0; kafaVurdu(constrain((int)floorf((x + 3) / T), 0, LW - 1), ty); break; }
  }
  y = ny;
  for (int tx = sol; tx <= sag; tx++) for (int ty = max(0, (int)floorf(y / T)); ty <= min(SAT - 1, (int)floorf((y + boy() - 1) / T)); ty++) {
    if (tx < 0 || tx >= LW) continue;
    if (h[ty][tx] == PARA) { h[ty][tx] = BOS; para++; puan += 100; }
    if (h[ty][tx] == BAYRAK || tx >= LW - 12) gecti = true;
  }
}
void ciz(uint32_t t, bool kosuyor) {
  for (int i = 0; i < 4; i++) {                      // arka plan: bulutlar ve tepeler (yavaş kayar)
    int bx = ((i * 97 - (int)(kam * .3f)) % 200 + 200) % 200 - 30;
    ekran.drawCircle(bx, 10 + i % 2 * 6, 4, W); ekran.drawCircle(bx + 6, 8 + i % 2 * 6, 5, W); ekran.drawCircle(bx + 12, 10 + i % 2 * 6, 4, W); ekran.fillRect(bx - 2, 11 + i % 2 * 6, 17, 4, B);
    int tx = ((i * 131 - (int)(kam * .5f)) % 220 + 220) % 220 - 40;
    ekran.drawCircleHelper(tx + 14, 48, 14, 1 | 2, W);
  }
  int c0 = (int)floorf(kam / T);
  for (int c = c0; c <= c0 + 16 && c < LW; c++) for (int r = 0; r < SAT; r++) {
    uint8_t k = h[r][c]; if (k == BOS) continue;
    int sx = lroundf(c * T - kam), sy = r * T;
    if (k == BORU) {                                  // boru: üstte geniş ağız, gövdede kenar çizgileri
      bool sagParca = c > 0 && h[r][c - 1] == BORU, ust = r == 0 || h[r - 1][c] != BORU;
      ekran.fillRect(sx, sy, 8, 8, B);
      if (ust) { ekran.drawRect(sx - (sagParca ? 0 : 1), sy, sagParca ? 9 : 9, 4, W); }
      ekran.drawFastVLine(sagParca ? sx + 6 : sx + 1, sy + (ust ? 4 : 0), ust ? 4 : 8, W);
      if (!sagParca) ekran.drawFastVLine(sx + 3, sy + (ust ? 4 : 0), ust ? 4 : 8, W);
      continue;
    }
    if (k == BAYRAK) {
      ekran.drawFastVLine(sx + 3, 4, sy - 4 + 8, W); ekran.fillCircle(sx + 3, 3, 1, W);
      int fy = 6 + (gecti ? (int)(min(1.0f, bitisT) * 34) : 0); ekran.fillTriangle(sx + 3, fy, sx - 6, fy + 3, sx + 3, fy + 6, W);
      ekran.drawRect(sx + 14, 32, 22, 16, W); ekran.drawRect(sx + 21, 40, 8, 8, W); for (int i = 0; i < 4; i++) ekran.fillRect(sx + 14 + i * 6, 29, 4, 3, W);   // kale
      continue;
    }
    if (k == PARA && (t / 200 + c) % 2) { ekran.drawFastVLine(sx + 3, sy + 2, 4, W); ekran.drawFastVLine(sx + 4, sy + 2, 4, W); continue; }   // dönen para
    ekran.drawBitmap(sx, sy, DOKU[k], 8, 8, W);
  }
  for (auto &e : v) if (e.var) {
    int sx = lroundf(e.x - kam), sy = lroundf(e.y);
    if (e.tur == 0) { if (e.ezik > 0) ekran.fillRect(sx, sy + 6, 8, 2, W); else ekran.drawBitmap(sx, sy, DUSMAN_S[(t / 200) % 2], 8, 8, W); }
    else if (e.tur == 3) ekran.drawBitmap(sx, sy, CICEK_S, 8, 8, W);
    else if (e.tur == 4) { ekran.fillCircle(sx + 2, sy + 2, 2, W); ekran.drawPixel(sx + 2 - (e.vx > 0 ? 3 : -3), sy + 1 + (t / 60) % 2, W); }
    else if (e.tur == 1) ekran.drawBitmap(sx, sy, MANTAR_S, 8, 8, W);
    else { ekran.drawCircle(sx + 3, sy + 3, 2, W); }
  }
  if (korunma <= 0 || (t / 80) % 2) {
    int sx = lroundf(x - kam) - 1, sy = lroundf(y), f = kosuyor && (t / 110) % 2;
    if (buyuk) { ekran.drawBitmap(sx, sy, KUCUK[0], 8, 7, W); ekran.drawBitmap(sx, sy + 7, BUYUK_GOVDE[f], 8, 7, W); if (atesli) { ekran.drawPixel(sx + 3, sy - 1 - (t / 120) % 2, W); ekran.drawPixel(sx + 4, sy - 2, W); } }
    else ekran.drawBitmap(sx, sy, KUCUK[f], 8, 8, W);
  }
  ekran.fillRect(0, 0, 128, 9, B);
  yaz(0, 1, 1, "o" + String(para)); yaz(30, 1, 1, bolumAd(bolum));
  for (int i = 0; i < can; i++) ekran.fillCircle(66 + i * 6, 4, 2, W);
  String s = String((int)max(0.0f, sure)); yaz(128 - s.length() * 6, 1, 1, s);
}

void haritaCiz(uint32_t t) {                          // dünya haritası: dalgalı yol üstünde bölümler; geçilenler dolu, kilitliler noktalı
  haritaX += (secim * 26 - 51 - haritaX) * .25f;
  ortala(0, 1, "DUNYA " + String(secim / 4 + 1));
  auto nY = [](int i) { return 34 + (int)(sinf(i * 1.3f) * 10); };
  for (int i = 0; i <= ilerleme + 1; i++) {
    int x = i * 26 - (int)haritaX, y = nY(i);
    if (i <= ilerleme) { int x2 = (i + 1) * 26 - (int)haritaX, y2 = nY(i + 1); for (int k = 0; k < 26; k += 3) ekran.drawPixel(x + (x2 - x) * k / 26, y + (y2 - y) * k / 26, W); }
    if (x < -10 || x > 138) continue;
    if (i % 4 == 3) { ekran.drawRect(x - 5, y - 8, 11, 9, W); ekran.fillRect(x - 5, y - 10, 3, 2, W); ekran.fillRect(x + 3, y - 10, 3, 2, W); }   // her dünyanın sonu kale
    else if (i < ilerleme) ekran.fillCircle(x, y, 4, W);
    else if (i == ilerleme) ekran.drawCircle(x, y, 4, W);
    else for (int a = 0; a < 8; a++) ekran.drawPixel(x + cosf(a * .785f) * 4, y + sinf(a * .785f) * 4, W);
  }
  int kx = secim * 26 - (int)haritaX, ky = nY(secim) - 14 - (t / 300) % 2;
  ekran.drawBitmap(kx - 4, ky, KUCUK[0], 8, 8, W);
  ekran.drawFastHLine(0, 54, 128, W);
  yaz(0, 56, 1, bolumAd(secim)); yaz(40, 56, 1, secim < ilerleme ? "gecildi" : "yeni!"); yaz(92, 56, 1, "sol:git");
}
void bolumBasla(int n) { bolum = n; can = 3; buyuk = false; atesli = false; bitti = false; onceIkisi = false; bitisT = 0; uret(n); hal = 1; }

bool oyna(Girdi &g) {
  uint32_t t = millis();
  if (g.ilk) { para = puan = 0; ilerleme = kayit.getInt("mario_il", 0); secim = ilerleme; haritaX = secim * 26 - 51; hal = 0; }
  if (g.ilk && ozelIstek == 'm') { ozelIstek = 0; bolumBasla(-1); }
  if (hal == 0) {
    if (g.o1 == KISA) secim = secim >= ilerleme ? 0 : secim + 1;
    if (g.o2 == KISA) { bolumBasla(secim); return true; }
    if (g.o2 == UZUN) { hal = 4; secim2 = 0; return true; }   // uzun sol: özel bölüm menüsü
    haritaCiz(t); ekran.fillRect(52, 0, 76, 8, B); yaz(56, 0, 1, dl("uzun sol:ozel", "hold L:custom")); return true;
  }
  if (hal == 4) {                                     // özel bölüm: oyna, düzenle, telefona gönder
    const char *og[4] = {dl("Ozel bolumu oyna", "Play custom level"), dl("Duzenle", "Edit"), dl("Telefona gonder", "Send to phone"), dl("Haritaya don", "Back to map")};
    if (g.o1 == KISA) secim2 = (secim2 + 1) % 4;
    if (g.o2 == KISA) switch (secim2) { case 0: bolumBasla(-1); return true; case 1: editorAc('m'); hal = 5; return true; case 2: seviyeGonder('m'); break; case 3: hal = 0; return true; }
    baslik(dl("OZEL BOLUM", "CUSTOM LEVEL"));
    for (int i = 0; i < 4; i++) { int y = 13 + i * 11; if (i == secim2) { ekran.fillRect(0, y - 1, 128, 10, W); ekran.setTextColor(B); } yaz(3, y, 1, og[i]); ekran.setTextColor(W); }
    altMesaj(); return true;
  }
  if (hal == 5) { bool oyna; if (!seviyeEditoru(g, oyna)) hal = 4; else if (oyna) bolumBasla(-1); return true; }
  if (bitti) { int b = bitis(g, "Puan " + String(puan) + "  " + bolumAd(bolum), "Rekor " + String(rekorKaydet("mario", puan))); if (b == 1) { bolumBasla(bolum); return true; } if (b == 2) { hal = bolum < 0 ? 4 : 0; bitti = false; } return true; }
  if (gecti) {                                        // bayrak: kısa kutlama, sonra sonraki bölüm
    if (!bitisT) { bitisT = .001f; puan += (int)sure * 10; }
    bitisT += g.dt; ciz(t, false);
    ekran.fillRect(24, 22, 80, 20, B); ekran.drawRect(24, 22, 80, 20, W); ortala(25, 1, "BOLUM GECILDI"); ortala(33, 1, "+" + String((int)sure * 10));
    if (bitisT > 2 && bolum < 0) { bitisT = 0; gecti = false; hal = 4; goster(dl("Ozel bolum bitti!", "Custom level done!")); return true; }
    if (bitisT > 2) { bitisT = 0; if (bolum + 1 > ilerleme) { ilerleme = bolum + 1; kayit.putInt("mario_il", ilerleme); rozetKontrol("mario_il"); } rekorKaydet("mario", puan); secim = min(bolum + 1, ilerleme); hal = 0; gecti = false; }
    return true;
  }
  bool ikisi = g.b1 && g.b2;
  int yuru = ikisi ? oncekiYon : g.b1 ? 1 : g.b2 ? -1 : 0;   // iki tuştayken önceki yönde koşmaya devam
  if (!ikisi) oncekiYon = yuru;
  if (ikisi && !onceIkisi && yerde) vy = -285;        // zıpla: ikinci tuşa basıldığı an
  for (int i = 0; i < 2 && atesli; i++) {             // ateşliyken yön tuşuna çift tık: ateş topu
    Olay ev = i ? g.o1 : g.o2;
    if (ev != KISA) continue;
    if (t - sonTik[i] < 320) { int say = 0; for (auto &e : v) say += e.var && e.tur == 4; if (say < 2) ekle(x + (i ? 6 : -4), y + 3, 4, i ? 110 : -110); sonTik[i] = 0; }
    else sonTik[i] = t;
  }
  onceIkisi = ikisi;
  sure -= g.dt; korunma -= g.dt;
  if (sure <= 0) { olum(); return true; }
  fizik(g.dt, yuru, ikisi);
  if (y > 70) { olum(); return true; }
  kam = max(kam, min((float)(LW * T - 128), x - 48));
  for (auto &e : v) if (e.var) {                      // düşman, mantar, uçan para
    if (e.tur == 2) { e.vy += 400 * g.dt; e.y += (e.vy - 140) * g.dt; if (e.vy > 260) e.var = false; continue; }
    if (e.tur == 4) {                                   // ateş topu: zıplayarak gider, düşmanı yakar
      e.ezik += g.dt; e.x += e.vx * g.dt; e.vy += 900 * g.dt; e.y += e.vy * g.dt;
      if (kati((int)floorf((e.x + 2) / T), (int)floorf((e.y + 4) / T))) { e.y = floorf((e.y + 4) / T) * T - 4; e.vy = -150; }   // yere değince sekiyor
      if (kati((int)floorf((e.x + (e.vx > 0 ? 4 : 0)) / T), (int)floorf((e.y + 1) / T)) || e.ezik > 1.6f || e.x > kam + 140) { e.var = false; continue; }
      for (auto &d2 : v) if (d2.var && d2.tur == 0 && d2.ezik <= 0 && carpisma(e.x, e.y, 4, 4, d2.x, d2.y, 8, 8)) { d2.var = false; e.var = false; puan += 200; break; }
      continue;
    }
    if (e.tur == 3) { if (carpisma(x, y, 6, boy(), e.x, e.y, 8, 8)) { e.var = false; atesli = true; if (!buyuk) { buyuk = true; y -= 6; } puan += 1000; } continue; }   // ateş çiçeği
    if (e.x < kam - 16) { e.var = false; continue; }
    if (e.x > kam + 140) continue;                    // ekran dışındaki düşman bekler
    if (e.ezik > 0) { if ((e.ezik -= g.dt) <= 0) e.var = false; continue; }
    float nx = e.x + e.vx * g.dt; int ty = (int)floorf((e.y + 4) / T);
    if (kati((int)floorf((e.vx > 0 ? nx + 7 : nx) / T), ty)) e.vx = -e.vx; else e.x = nx;
    e.vy = min(e.vy + 900 * g.dt, 300.0f); float ny = e.y + e.vy * g.dt; int alt = (int)floorf((ny + 8) / T);
    if (kati((int)floorf((e.x + 1) / T), alt) || kati((int)floorf((e.x + 6) / T), alt)) { ny = alt * T - 8; e.vy = 0; }
    e.y = ny; if (e.y > 70) { e.var = false; continue; }
    if (!carpisma(x, y, 6, boy(), e.x + 1, e.y + 1, 6, 7)) continue;
    if (e.tur == 1) { e.var = false; if (!buyuk) { buyuk = true; y -= 6; } puan += 1000; continue; }
    if (vy > 0 && y + boy() - e.y < 6) { e.ezik = .4f; vy = -170; puan += 200; }   // üstüne bastı: ezildi
    else if (korunma <= 0) { if (atesli) { atesli = false; korunma = 1.5f; } else if (buyuk) { buyuk = false; y += 6; korunma = 1.5f; } else { olum(); return true; } }
  }
  ciz(t, yuru != 0 && yerde);
  return true;
}
}  // namespace mario
bool superKubit(Girdi &g) { return mario::oyna(g); }
