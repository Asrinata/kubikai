// Oyunlar: ortak araçlar + klasikler (Yılan, Dino, Pong, Breakout, Uzay İstilası, Asteroid, Tetris, 2048).
// Her oyun bool f(Girdi&) döner: false -> Oyunlar listesine çık. Kartta sol tuş Tuş 2 (b2/o2), sağ tuş Tuş 1 (b1/o1).
// Oyun bitince: sağ tuş tekrar, sol tuş çık. İki tuşu birlikte bırakmak g.combo (g.comboSure ms basılı tutuldu).
#pragma once

int rekorKaydet(const char *k, int v, bool kucukIyi = false) {   // rekoru günceller, güncel rekoru döner
  int r = kayit.getInt(k, 0);
  if (kucukIyi ? (v > 0 && (r == 0 || v < r)) : v > r) { kayit.putInt(k, v); r = v; rozetKontrol(k); }
  return r;
}
int bitis(Girdi &g, const String &s1, const String &s2) {   // 1 tekrar, 2 çık; ilk 0.6 sn tuşları yok sayar (ölürken basılan tuş hemen yeniden başlatmasın)
  static uint32_t son, bas;
  if (millis() - son > 100) bas = millis();
  son = millis();
  oyunBitti(s1, s2);
  if (millis() - bas < 600) return 0;
  return g.o1 == KISA ? 1 : g.o2 == KISA ? 2 : 0;
}
void ortalaX(int cx, int y, const String &s) { yaz(cx - (int)s.length() * 3, y, 1, s); }
bool carpisma(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) { return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by; }
void kucukKubit(int x, int y, int kare) {            // 9x11 koşan Kübit (Dino, Tırmanış)
  ekran.drawRoundRect(x, y, 9, 6, 2, W); ekran.drawPixel(x + 3, y + 2, W); ekran.drawPixel(x + 6, y + 2, W);
  ekran.drawRect(x + 2, y + 6, 5, 3, W);
  bool a = (kare / 4) % 2; ekran.drawLine(x + 3, y + 9, x + (a ? 2 : 3), y + 10, W); ekran.drawLine(x + 5, y + 9, x + (a ? 6 : 5), y + 10, W);
}

// ---------------------------------------------------------------- Yılan: sol tuş sola, sağ tuş sağa döner
bool yilan(Girdi &g) {
  const int C = 4, GW = 32, GH = 13, OY = 10;
  static int8_t gx[GW * GH], gy[GW * GH];
  static int n, yon, yeniYon, yemX, yemY, skor; static float zaman; static bool bitti;
  auto yemKoy = [] { for (;;) { yemX = random(GW); yemY = random(GH); bool dolu = false; for (int i = 0; i < n; i++) if (gx[i] == yemX && gy[i] == yemY) dolu = true; if (!dolu) return; } };
  auto basla = [&] { n = 4; for (int i = 0; i < n; i++) { gx[i] = 10 - i; gy[i] = 6; } yon = yeniYon = 0; skor = 0; zaman = 0; bitti = false; yemKoy(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("yilan", skor))); if (b == 1) basla(); return b != 2; }
  if (g.o2 == BASTI) yeniYon = (yon + 3) % 4;        // 0 sağ, 1 aşağı, 2 sol, 3 yukarı
  if (g.o1 == BASTI) yeniYon = (yon + 1) % 4;
  zaman += g.dt * 1000;
  if (zaman > max(55, 150 - skor * 4)) {
    zaman = 0; yon = yeniYon;
    int nx = gx[0] + (yon == 0) - (yon == 2), ny = gy[0] + (yon == 1) - (yon == 3);
    bool ol = nx < 0 || ny < 0 || nx >= GW || ny >= GH;
    for (int i = 0; i < n - 1 && !ol; i++) if (gx[i] == nx && gy[i] == ny) ol = true;
    if (ol) bitti = true;
    else {
      bool ye = nx == yemX && ny == yemY;
      if (ye && n < GW * GH) n++;
      for (int i = n - 1; i > 0; i--) { gx[i] = gx[i - 1]; gy[i] = gy[i - 1]; }
      gx[0] = nx; gy[0] = ny;
      if (ye) { skor++; yemKoy(); }
    }
  }
  yaz(0, 0, 1, "YILAN"); yaz(90, 0, 1, String(skor));
  ekran.drawRect(0, OY - 1, 128, GH * C + 2, W);
  for (int i = 0; i < n; i++) ekran.fillRect(gx[i] * C, OY + gy[i] * C, C - 1, C - 1, W);
  ekran.drawCircle(yemX * C + 1, OY + yemY * C + 1, 1, W);
  return true;
}

// ---------------------------------------------------------------- Dino: herhangi bir tuş zıplar (basılı tutunca daha yüksek)
bool dino(Girdi &g) {
  const int ZEMIN_Y = 54;
  struct Engel { float x; int w, h, y; bool kus; };
  static Engel e[3]; static float y, vy, hiz, mesafe, sonraki; static bool bitti; static int kare;
  auto basla = [] { y = ZEMIN_Y - 11; vy = 0; hiz = 70; mesafe = 0; sonraki = 140; bitti = false; for (auto &k : e) k.x = -100; };
  if (g.ilk) basla();
  int skor = mesafe / 10;
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("dino", skor))); if (b == 1) basla(); return b != 2; }
  kare++;
  bool yerde = y >= ZEMIN_Y - 11;
  if (yerde && (g.o1 == BASTI || g.o2 == BASTI)) vy = -150;
  vy += ((g.b1 || g.b2) && vy < 0 ? 300 : 600) * g.dt;   // basılı tutarken daha az yerçekimi -> daha yüksek zıplama
  y = min((float)ZEMIN_Y - 11, y + vy * g.dt); if (y >= ZEMIN_Y - 11) vy = 0;
  hiz += 2 * g.dt; mesafe += hiz * g.dt; sonraki -= hiz * g.dt;
  if (sonraki <= 0) {
    for (auto &k : e) if (k.x < -20) {
      bool kus = skor > 30 && random(4) == 0;
      k = {130, kus ? 9 : (int)random(4, 9), kus ? 5 : (int)random(7, 13), 0, kus};
      k.y = kus ? (random(2) ? ZEMIN_Y - 20 : ZEMIN_Y - 9) : ZEMIN_Y - k.h;
      break;
    }
    sonraki = random(70, 150) + hiz * 0.3;
  }
  for (auto &k : e) {
    k.x -= hiz * g.dt;
    if (k.x > -20 && carpisma(14, y + 1, 7, 9, k.x + 1, k.y + 1, k.w - 2, k.h - 2)) bitti = true;
    if (k.kus) { bool kanat = (kare / 6) % 2; ekran.drawLine(k.x, k.y + 2, k.x + 8, k.y + 2, W); ekran.drawLine(k.x + 2, k.y + 2, k.x + 4, k.y + (kanat ? -1 : 5), W); ekran.drawLine(k.x + 4, k.y + (kanat ? -1 : 5), k.x + 6, k.y + 2, W); }
    else { ekran.fillRect(k.x + k.w / 2 - 1, k.y, 3, k.h, W); ekran.drawFastHLine(k.x, k.y + k.h / 3, k.w, W); ekran.drawFastVLine(k.x, k.y + k.h / 3 - 2, 3, W); ekran.drawFastVLine(k.x + k.w - 1, k.y + k.h / 3 - 2, 3, W); }
  }
  ekran.drawFastHLine(0, ZEMIN_Y, 128, W);
  for (int i = 0; i < 8; i++) ekran.drawPixel(((int)(i * 37 - mesafe) % 128 + 128) % 128, ZEMIN_Y + 3 + i % 3 * 2, W);
  kucukKubit(13, (int)y, yerde ? kare : 0);
  yaz(80, 0, 1, String(skor));
  return true;
}

// ---------------------------------------------------------------- Pong: sağ raket senin (sol tuş yukarı, sağ tuş aşağı), 5'e kadar
bool pong(Girdi &g) {
  static float bx, by, vx, vy, ben, o; static int sb, so; static bool bitti;
  auto top = [](int yon) { bx = 64; by = 32; vx = 60 * yon; vy = random(-40, 41); };
  auto basla = [&] { ben = o = 25; sb = so = 0; bitti = false; top(1); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, sb > so ? "Kazandin! " + String(sb) + "-" + String(so) : "Kubit kazandi " + String(so) + "-" + String(sb), "Galibiyet " + String(rekorKaydet("pong", kayit.getInt("pong", 0) + (sb > so)))); if (b == 1) basla(); return b != 2; }
  if (g.b2) ben -= 70 * g.dt; if (g.b1) ben += 70 * g.dt;
  ben = constrain(ben, 0.0f, 50.0f);
  float hedef = by - 7; o += constrain(hedef - o, -45 * g.dt, 45 * g.dt); o = constrain(o, 0.0f, 50.0f);   // Kübit'in raketi biraz yavaş
  bx += vx * g.dt; by += vy * g.dt;
  if (by < 1 || by > 62) { vy = -vy; by = constrain(by, 1.0f, 62.0f); }
  if (vx > 0 && bx >= 122 && bx < 126 && by >= ben - 1 && by <= ben + 15) { vx = -vx * 1.06; vy += (by - ben - 7) * 6; }
  if (vx < 0 && bx <= 5 && bx > 1 && by >= o - 1 && by <= o + 15) { vx = -vx * 1.06; vy += (by - o - 7) * 6; }
  if (bx < -2) { sb++; top(-1); } if (bx > 130) { so++; top(1); }
  if (sb >= 5 || so >= 5) bitti = true;
  for (int y = 0; y < 64; y += 4) ekran.drawFastVLine(64, y, 2, W);
  yaz(50, 0, 1, String(so)); yaz(72, 0, 1, String(sb));
  ekran.fillRect(2, o, 2, 14, W); ekran.fillRect(124, ben, 2, 14, W); ekran.fillRect(bx - 1, by - 1, 3, 3, W);
  return true;
}

// ---------------------------------------------------------------- Breakout: sol/sağ tuş raketi kaydırır
bool breakout(Girdi &g) {
  static bool tug[4][10]; static float px, bx, by, vx, vy; static int can, skor, seviye; static bool bitti, yapisik;
  auto dizi = [] { for (auto &s : tug) for (auto &t : s) t = true; };
  auto topHazir = [] { yapisik = true; vx = 45 + seviye * 8; vy = -55 - seviye * 8; };
  auto basla = [&] { px = 55; can = 3; skor = 0; seviye = 0; bitti = false; dizi(); topHazir(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("breakout", skor))); if (b == 1) basla(); return b != 2; }
  if (g.b2) px -= 90 * g.dt; if (g.b1) px += 90 * g.dt;
  px = constrain(px, 0.0f, 110.0f);
  if (yapisik) { bx = px + 9; by = 57; if (g.o1 == BASTI || g.o2 == BASTI) yapisik = false; }
  else {
    bx += vx * g.dt; by += vy * g.dt;
    if (bx < 1 || bx > 126) { vx = -vx; bx = constrain(bx, 1.0f, 126.0f); }
    if (by < 9) { vy = fabs(vy); }
    if (vy > 0 && by >= 58 && by <= 61 && bx >= px - 1 && bx <= px + 19) { vy = -fabs(vy); vx = (bx - px - 9) * 7; }
    if (by > 66) { if (--can <= 0) bitti = true; else topHazir(); }
    int c = (bx - 4) / 12, r = (by - 10) / 5;
    if (r >= 0 && r < 4 && c >= 0 && c < 10 && tug[r][c]) { tug[r][c] = false; vy = -vy; skor += 4 - r; }
    bool kaldi = false; for (auto &s : tug) for (auto t : s) kaldi |= t;
    if (!kaldi) { seviye++; dizi(); topHazir(); }
  }
  yaz(0, 0, 1, String(skor)); for (int i = 0; i < can; i++) ekran.fillCircle(124 - i * 6, 3, 2, W);
  for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) if (tug[r][c]) ekran.fillRect(4 + c * 12, 10 + r * 5, 11, 4, W);
  ekran.fillRect(px, 59, 19, 2, W); ekran.fillCircle(bx, by, 1, W);
  return true;
}

// ---------------------------------------------------------------- Uzay İstilası: sol/sağ tuş gemiyi kaydırır, gemi kendiliğinden ateş eder
bool istila(Girdi &g) {
  static bool canli[3][7]; static float ox, oy, yon, gx, mx, my, dmx[3], dmy[3], hareket, atesSayac; static bool mermi; static int can, skor, dalga; static bool bitti;
  auto dalgaKur = [] { for (auto &s : canli) for (auto &c : s) c = true; ox = 10; oy = 10; yon = 1; for (auto &y : dmy) y = 99; };
  auto basla = [&] { gx = 60; can = 3; skor = 0; dalga = 0; bitti = false; mermi = false; dalgaKur(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("istila", skor))); if (b == 1) basla(); return b != 2; }
  if (g.b2) gx -= 70 * g.dt; if (g.b1) gx += 70 * g.dt;
  gx = constrain(gx, 0.0f, 119.0f);
  int kalan = 0; for (auto &s : canli) for (auto c : s) kalan += c;
  if (!kalan) { dalga++; dalgaKur(); }
  hareket += g.dt;
  float adim = 0.05 + kalan * 0.025 / (1 + dalga * 0.3);
  if (hareket > adim) {
    hareket = 0; ox += yon * 2;
    float sag = -1, sol = 999; for (int r = 0; r < 3; r++) for (int c = 0; c < 7; c++) if (canli[r][c]) { sag = max(sag, ox + c * 14 + 8); sol = min(sol, ox + c * 14); }
    if (sag > 127 || sol < 1) { yon = -yon; oy += 3; }
  }
  if (!mermi && (atesSayac += g.dt) > 0.35) { mermi = true; mx = gx + 4; my = 56; atesSayac = 0; }
  if (mermi) { my -= 120 * g.dt; if (my < 0) mermi = false; }
  for (int r = 0; r < 3; r++) for (int c = 0; c < 7; c++) if (canli[r][c]) {
    float x = ox + c * 14, y = oy + r * 8;
    if (mermi && carpisma(mx, my, 1, 3, x, y, 8, 5)) { canli[r][c] = false; mermi = false; skor += (3 - r) * 10; continue; }
    if (y + 5 > 54) bitti = true;
    bool f = ((int)(ox / 2)) % 2;
    ekran.fillRect(x + 1, y, 6, 4, W); ekran.drawPixel(x + 2, y + 1, B); ekran.drawPixel(x + 5, y + 1, B);
    ekran.drawLine(x, y + (f ? 4 : 2), x + 1, y + 3, W); ekran.drawLine(x + 7, y + (f ? 4 : 2), x + 6, y + 3, W);
  }
  for (int i = 0; i < 3; i++) {                       // düşman mermileri
    if (dmy[i] > 64 && random(60) == 0) { int c = random(7), r = 2; while (r >= 0 && !canli[r][c]) r--; if (r >= 0) { dmx[i] = ox + c * 14 + 4; dmy[i] = oy + r * 8 + 5; } }
    if (dmy[i] <= 64) { dmy[i] += 50 * g.dt; ekran.drawFastVLine(dmx[i], dmy[i], 3, W);
      if (carpisma(dmx[i], dmy[i], 1, 3, gx, 57, 9, 5)) { dmy[i] = 99; if (--can <= 0) bitti = true; } }
  }
  if (mermi) ekran.drawFastVLine(mx, my, 3, W);
  ekran.fillTriangle(gx, 62, gx + 4, 57, gx + 8, 62, W);
  yaz(0, 0, 1, String(skor)); for (int i = 0; i < can; i++) ekran.fillTriangle(110 + i * 6, 6, 112 + i * 6, 1, 114 + i * 6, 6, W);
  return true;
}

// ---------------------------------------------------------------- Asteroid: sol/sağ tuş döndürür, gemi kendiliğinden ateş eder
bool asteroid(Girdi &g) {
  struct Kaya { float x, y, vx, vy; int r; bool var; };
  struct Mermi { float x, y, vx, vy, omur; };
  static Kaya k[12]; static Mermi m[4]; static float aci, ates, dogma, korunma; static int can, skor; static bool bitti;
  auto kayaEkle = [](float x, float y, int r) { for (auto &a : k) if (!a.var) { float t = random(628) / 100.0, h = random(10, 25); a = {x, y, cos(t) * h, sin(t) * h, r, true}; return; } };
  auto basla = [&] { aci = -HALF_PI; can = 3; skor = 0; bitti = false; dogma = 0; korunma = 2; for (auto &a : k) a.var = false; for (auto &b : m) b.omur = 0; };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("asteroid", skor))); if (b == 1) basla(); return b != 2; }
  if (g.b2) aci -= 4 * g.dt; if (g.b1) aci += 4 * g.dt;
  if ((ates -= g.dt) <= 0) { ates = 0.3; for (auto &b : m) if (b.omur <= 0) { b = {64 + cos(aci) * 6, 32 + sin(aci) * 6, cos(aci) * 90, sin(aci) * 90, 1.2}; break; } }
  if ((dogma -= g.dt) <= 0) { dogma = max(0.8f, 2.5f - skor / 400.0f); int kenar = random(4); kayaEkle(kenar % 2 ? random(128) : (kenar ? 132 : -4), kenar % 2 ? (kenar == 1 ? -4 : 68) : random(64), 7); }
  korunma -= g.dt;
  for (auto &a : k) if (a.var) {
    a.x += a.vx * g.dt; a.y += a.vy * g.dt;
    if (a.x < -10) a.x = 137; if (a.x > 137) a.x = -10; if (a.y < -10) a.y = 73; if (a.y > 73) a.y = -10;
    for (auto &b : m) if (b.omur > 0 && (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) < a.r * a.r) {
      b.omur = 0; a.var = false; skor += a.r > 4 ? 20 : 50;
      if (a.r > 4) { kayaEkle(a.x, a.y, 3); kayaEkle(a.x, a.y, 3); }
    }
    if (a.var && korunma <= 0 && (a.x - 64) * (a.x - 64) + (a.y - 32) * (a.y - 32) < (a.r + 3) * (a.r + 3)) { a.var = false; korunma = 2; if (--can <= 0) bitti = true; }
    if (a.var) { for (int i = 0; i < 7; i++) { float t1 = i * 2 * PI / 7, t2 = (i + 1) * 2 * PI / 7, r1 = a.r * (0.8 + (i % 3) * 0.12), r2 = a.r * (0.8 + ((i + 1) % 3) * 0.12);
      ekran.drawLine(a.x + cos(t1) * r1, a.y + sin(t1) * r1, a.x + cos(t2) * r2, a.y + sin(t2) * r2, W); } }
  }
  for (auto &b : m) if (b.omur > 0) { b.x += b.vx * g.dt; b.y += b.vy * g.dt; b.omur -= g.dt; ekran.drawPixel(b.x, b.y, W); }
  if (korunma <= 0 || (millis() / 150) % 2) {        // gemi (darbeden sonra yanıp söner)
    float c = cos(aci), s = sin(aci);
    ekran.drawTriangle(64 + c * 6, 32 + s * 6, 64 + cos(aci + 2.5) * 4, 32 + sin(aci + 2.5) * 4, 64 + cos(aci - 2.5) * 4, 32 + sin(aci - 2.5) * 4, W);
  }
  yaz(0, 0, 1, String(skor)); for (int i = 0; i < can; i++) ekran.drawTriangle(110 + i * 6, 6, 112 + i * 6, 1, 114 + i * 6, 6, W);
  return true;
}

// ---------------------------------------------------------------- Tetris: sol/sağ tuş kaydırır, ikisine kısa bas döndür, uzun bas düşür
bool tetris(Girdi &g) {
  const int TW = 10, TH = 20, C = 3, X0 = 49, Y0 = 2;
  static const uint16_t PARCA[7] = {0x0F00, 0x0660, 0x0E40, 0x06C0, 0x0C60, 0x0E80, 0x0E20};   // I O T S Z L J (4x4, satır satır)
  static uint8_t alan[TH][TW]; static uint16_t p; static int px, py, sonraki, satir, skor; static float dusme; static bool bitti;
  auto hucre = [](uint16_t q, int r, int c) { return (q >> (15 - (r * 4 + c))) & 1; };
  auto dondur = [&](uint16_t q) { uint16_t d = 0; for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (hucre(q, r, c)) d |= 1 << (15 - (c * 4 + (3 - r))); return d; };
  auto sigar = [&](uint16_t q, int x, int y) { for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (hucre(q, r, c)) {
      int ax = x + c, ay = y + r; if (ax < 0 || ax >= TW || ay >= TH || (ay >= 0 && alan[ay][ax])) return false; } return true; };
  auto yeni = [&] { p = PARCA[sonraki]; sonraki = random(7); px = 3; py = -1; if (!sigar(p, px, py)) bitti = true; };
  auto basla = [&] { memset(alan, 0, sizeof(alan)); satir = skor = 0; bitti = false; dusme = 0; sonraki = random(7); yeni(); };
  auto kilitle = [&] {
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (hucre(p, r, c) && py + r >= 0) alan[py + r][px + c] = 1;
    int silinen = 0;
    for (int r = TH - 1; r >= 0; r--) { bool dolu = true; for (int c = 0; c < TW; c++) dolu &= alan[r][c];
      if (dolu) { silinen++; for (int k = r; k > 0; k--) memcpy(alan[k], alan[k - 1], TW); memset(alan[0], 0, TW); r++; } }
    const int puan[5] = {0, 40, 100, 300, 1200}; satir += silinen; skor += puan[silinen] * (1 + satir / 10);
    yeni();
  };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("tetris", skor))); if (b == 1) basla(); return b != 2; }
  if (g.o2 == KISA && sigar(p, px - 1, py)) px--;
  if (g.o1 == KISA && sigar(p, px + 1, py)) px++;
  if (g.combo) {
    if (g.comboSure < 450) { uint16_t d = dondur(p); for (int k : {0, -1, 1, -2, 2}) if (sigar(d, px + k, py)) { p = d; px += k; break; } }
    else { while (sigar(p, px, py + 1)) { py++; skor += 2; } kilitle(); }
  }
  dusme += g.dt;
  if (!bitti && dusme > max(0.08f, 0.6f - (satir / 10) * 0.06f)) { dusme = 0; if (sigar(p, px, py + 1)) py++; else kilitle(); }
  ekran.drawRect(X0 - 2, Y0 - 1, TW * C + 3, TH * C + 2, W);
  for (int r = 0; r < TH; r++) for (int c = 0; c < TW; c++) if (alan[r][c]) ekran.fillRect(X0 + c * C, Y0 + r * C, C - 1, C - 1, W);
  for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (hucre(p, r, c) && py + r >= 0) ekran.fillRect(X0 + (px + c) * C, Y0 + (py + r) * C, C - 1, C - 1, W);
  yaz(0, 0, 1, "SKOR"); yaz(0, 10, 1, String(skor)); yaz(0, 26, 1, "SATIR"); yaz(0, 36, 1, String(satir));
  yaz(88, 0, 1, "SIRADAKI");
  for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (hucre(PARCA[sonraki], r, c)) ekran.fillRect(96 + c * 4, 12 + r * 4, 3, 3, W);
  yaz(84, 40, 1, "2 tus:"); yaz(84, 48, 1, "kisa dondu"); yaz(84, 56, 1, "uzun dus");
  return true;
}

// ---------------------------------------------------------------- 2048: sol/sağ tuş kısa sol/sağ, uzun yukarı/aşağı
bool oyun2048(Girdi &g) {
  static uint16_t t[4][4]; static int skor; static bool bitti, kazandi;
  auto ekle = [] { int bos[16], n = 0; for (int i = 0; i < 16; i++) if (!t[i / 4][i % 4]) bos[n++] = i; if (n) { int i = bos[random(n)]; t[i / 4][i % 4] = random(10) ? 2 : 4; } };
  auto basla = [&] { memset(t, 0, sizeof(t)); skor = 0; bitti = kazandi = false; ekle(); ekle(); };
  auto kaydir = [&](int yon) {                       // 0 sol, 1 sağ, 2 yukarı, 3 aşağı
    bool degisti = false;
    for (int i = 0; i < 4; i++) {
      uint16_t *h[4];
      for (int j = 0; j < 4; j++) { int k = (yon == 1 || yon == 3) ? 3 - j : j; h[j] = yon < 2 ? &t[i][k] : &t[k][i]; }
      uint16_t v[4] = {0}; int n = 0;
      for (int j = 0; j < 4; j++) if (*h[j]) v[n++] = *h[j];
      uint16_t s[4] = {0}; int m = 0;
      for (int j = 0; j < n; j++) { if (j + 1 < n && v[j] == v[j + 1]) { s[m++] = v[j] * 2; skor += v[j] * 2; if (v[j] * 2 == 2048) kazandi = true; j++; } else s[m++] = v[j]; }
      for (int j = 0; j < 4; j++) { if (*h[j] != s[j]) degisti = true; *h[j] = s[j]; }
    }
    if (degisti) ekle();
    bool hamle = false;
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (!t[r][c] || (c < 3 && t[r][c] == t[r][c + 1]) || (r < 3 && t[r][c] == t[r + 1][c])) hamle = true;
    if (!hamle) bitti = true;
  };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("2048", skor))); if (b == 1) basla(); return b != 2; }
  if (g.o2 == KISA) kaydir(0); if (g.o1 == KISA) kaydir(1); if (g.o2 == UZUN) kaydir(2); if (g.o1 == UZUN) kaydir(3);
  for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) {
    int x = c * 24, y = r * 16; ekran.drawRoundRect(x, y, 23, 15, 2, W);
    if (t[r][c]) { String s = String(t[r][c]); if (t[r][c] >= 128) { ekran.fillRoundRect(x, y, 23, 15, 2, W); ekran.setTextColor(B); }
      yaz(x + (23 - s.length() * 6) / 2 + 1, y + 4, 1, s); ekran.setTextColor(W); }
  }
  yaz(98, 0, 1, "SKOR"); yaz(98, 10, 1, String(skor)); if (kazandi) yaz(98, 22, 1, "2048!");
  yaz(98, 36, 1, "kisa"); yaz(98, 44, 1, "<  >"); yaz(98, 52, 1, "uzun"); yaz(98, 58, 1, "^ v");
  return true;
}
