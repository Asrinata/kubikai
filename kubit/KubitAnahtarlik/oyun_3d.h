// Oyunlar: 3B (raycasting labirent, uzay tüneli). İkisinde de iki tuşa birlikte basılıyken ilerlenir; çıkış: sol tuşu 3 sn tut.
#pragma once

float aciFark(float a, float b) { float d = fmod(a - b + 3 * PI, 2 * PI) - PI; return d; }   // -PI..PI

// ---------------------------------------------------------------- 3B Labirent: sol/sağ tuş döner, ikisi birden yürür. Çıkışı (E) bul.
bool labirent(Girdi &g) {
  const int N = 15;                                    // tek sayı: hücreler tek koordinatlarda
  static uint8_t m[N][N]; static float px, py, yon, sure; static int seviye; static bool bitti;
  auto kur = [] {
    memset(m, 1, sizeof(m));
    static uint8_t yigin[N * N]; int n = 0;            // yinelemeli DFS ile mükemmel labirent
    m[1][1] = 0; yigin[n++] = 1 * N + 1;
    while (n) {
      int c = yigin[n - 1], cx = c % N, cy = c / N, k[4], kn = 0;
      const int dx[4] = {2, -2, 0, 0}, dy[4] = {0, 0, 2, -2};
      for (int i = 0; i < 4; i++) { int nx = cx + dx[i], ny = cy + dy[i]; if (nx > 0 && ny > 0 && nx < N - 1 && ny < N - 1 && m[ny][nx]) k[kn++] = i; }
      if (!kn) { n--; continue; }
      int i = k[random(kn)]; m[cy + dy[i] / 2][cx + dx[i] / 2] = 0; m[cy + dy[i]][cx + dx[i]] = 0; yigin[n++] = (cy + dy[i]) * N + cx + dx[i];
    }
    m[N - 2][N - 2] = 2;                               // çıkış
    px = py = 1.5; yon = 0; sure = 0; bitti = false;
  };
  if (g.ilk) { seviye = 0; kur(); }
  if (bitti) {
    int ds = lround(sure * 10), r = rekorKaydet("labirent", ds, true);
    int b = bitis(g, "Cikis! " + String(ds / 10) + "." + String(ds % 10) + " sn", "Rekor " + String(r / 10) + "." + String(r % 10) + " sn");
    if (b == 1) kur(); return b != 2;
  }
  sure += g.dt;
  if (g.b1 && g.b2) {
    float nx = px + cos(yon) * 1.6 * g.dt, ny = py + sin(yon) * 1.6 * g.dt;
    if (m[(int)py][(int)(nx + (cos(yon) > 0 ? .2 : -.2))] != 1) px = nx;   // duvara .2 yaklaşabilir, eksenler ayrı: duvar boyunca kayar
    if (m[(int)(ny + (sin(yon) > 0 ? .2 : -.2))][(int)px] != 1) py = ny;
    if (m[(int)py][(int)px] == 2) bitti = true;
  } else if (g.b2) yon -= 2.2 * g.dt; else if (g.b1) yon += 2.2 * g.dt;
  float dx = cos(yon), dy = sin(yon), plx = -dy * .66, ply = dx * .66;
  for (int c = 0; c < 64; c++) {                       // 2 piksellik sütunlar
    float kx = 2 * c / 64.0 - 1, rx = dx + plx * kx, ry = dy + ply * kx;
    int mx = px, my = py, adx = rx < 0 ? -1 : 1, ady = ry < 0 ? -1 : 1, yan = 0;
    float ddx = fabs(1 / rx), ddy = fabs(1 / ry), sx = (rx < 0 ? px - mx : mx + 1 - px) * ddx, sy = (ry < 0 ? py - my : my + 1 - py) * ddy;
    uint8_t tur = 0;
    for (int k = 0; k < 40 && !tur; k++) { if (sx < sy) { sx += ddx; mx += adx; yan = 0; } else { sy += ddy; my += ady; yan = 1; } tur = m[my][mx]; }
    float d = yan ? sy - ddy : sx - ddx; int h = min(64, (int)(56 / max(d, .05f)));
    int y0 = 32 - h / 2, x = c * 2;
    if (tur == 2) { for (int y = y0; y < y0 + h; y += 2) ekran.drawFastHLine(x, y, 2, W); continue; }   // çıkış kapısı: çizgili
    ekran.drawFastHLine(x, y0, 2, W); ekran.drawFastHLine(x, y0 + h - 1, 2, W);
    if (!yan) ekran.fillRect(x, y0, 2, h, W);          // bir eksen dolu, diğeri dama: köşeler belli olur
    else if (d < 6) for (int y = y0; y < y0 + h; y++) ekran.drawPixel(x + (y & 1), y, W);
  }
  for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) if (m[y][x] == 1) ekran.drawPixel(127 - N + x, y, W);   // köşede minik harita
  ekran.drawPixel(127 - N + (int)px, (int)py, (millis() / 200) % 2 ? W : B);
  ekran.fillRect(0, 0, 30, 9, B); yaz(0, 1, 1, String((int)sure) + " sn");
  return true;
}

// ---------------------------------------------------------------- Uzay Tüneli: sol/sağ tuş gemiyi tünelde döndürür, iki tuşa birlikte basınca ilerler.
// Halkalarda tek bir boşluk var; boşluğa hizalan, sonra iki tuşla geç. Oksijen azalır, her halka doldurur.
bool tunel(Girdi &g) {
  struct Halka { float z, bosluk, donus; };
  static Halka h[5]; static float aci, hiz, oksijen, yol; static int skor; static bool bitti;
  auto halkaKur = [](Halka &k, float z) { k = {z, random(628) / 100.0f, skor > 8 ? (random(2) ? 1 : -1) * min(1.2f, skor * .04f) : 0}; };
  auto basla = [&] { aci = HALF_PI; hiz = 0; oksijen = 1; yol = 0; skor = 0; bitti = false; for (int i = 0; i < 5; i++) halkaKur(h[i], 3 + i * 2.5f); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Halka " + String(skor), "Rekor " + String(rekorKaydet("tunel", skor))); if (b == 1) basla(); return b != 2; }
  bool ileri = g.b1 && g.b2;
  if (!ileri) { if (g.b2) aci -= 3 * g.dt; if (g.b1) aci += 3 * g.dt; }
  hiz = ileri ? min(6.0f, hiz + 14 * g.dt) : max(0.0f, hiz - 18 * g.dt);
  oksijen -= g.dt / max(6.0f, 14 - skor * .2f);
  if (oksijen <= 0) bitti = true;
  yol += hiz * g.dt;
  const float BOS = .55;                               // boşluğun yarı açısı (radyan)
  for (auto &k : h) {
    k.z -= hiz * g.dt; k.bosluk += k.donus * g.dt;
    if (k.z < 1) {
      if (fabs(aciFark(aci, k.bosluk)) > BOS - .12) { bitti = true; k.z = 1; }
      else { skor++; oksijen = min(1.0f, oksijen + .25f); float uzak = 0; for (auto &o : h) uzak = max(uzak, o.z); halkaKur(k, uzak + 2.5f); }
    }
  }
  for (int i = 0; i < 10; i++) {                       // akan yıldızlar
    float t = fmod(i * .37f + yol * .15f, 1.0f), a = i * 2.4f, r = 4 + t * t * 60;
    ekran.drawPixel(64 + cos(a) * r * 1.9, 32 + sin(a) * r, W);
  }
  for (auto &k : h) {                                  // uzaktan yakına: her halka boşluğu dışında dolu bir elips yayı
    if (k.z > 12) continue;
    float r = 28 / k.z; int adim = r > 14 ? 90 : 48;
    for (int i = 0; i < adim; i++) {
      float a = i * 2 * PI / adim; if (fabs(aciFark(a, k.bosluk)) < BOS) continue;
      for (float rr = r; rr > r - max(1.0f, r * .12f); rr -= 1) ekran.drawPixel(64 + cos(a) * rr * 1.9, 32 + sin(a) * rr, W);
    }
  }
  float gx = 64 + cos(aci) * 26 * 1.9, gy = 32 + sin(aci) * 26;   // gemi: tünel duvarında, merkeze bakan üçgen
  ekran.fillTriangle(gx - cos(aci) * 5, gy - sin(aci) * 3, gx + cos(aci + 1.6) * 4, gy + sin(aci + 1.6) * 3, gx + cos(aci - 1.6) * 4, gy + sin(aci - 1.6) * 3, W);
  if (ileri) for (int i = 0; i < 3; i++) ekran.drawPixel(gx + cos(aci) * (3 + i * 2) , gy + sin(aci) * (2 + i), W);
  ekran.fillRect(0, 0, 20, 9, B); yaz(0, 1, 1, String(skor));
  ekran.drawRect(100, 1, 27, 5, W); ekran.fillRect(101, 2, 25 * oksijen, 3, W);
  if (oksijen < .25 && (millis() / 250) % 2) yaz(76, 1, 1, "O2!");
  return true;
}
