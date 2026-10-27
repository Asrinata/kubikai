// Oyunlar: beceri ve refleks (Reaksiyon, Simon, Hafıza, Yazı-Tura Serisi, Okçuluk, Bowling, Mini Golf, Paraşüt, Tırmanış).
#pragma once

void elips(int cx, int cy, int rx, int ry, uint16_t renk) {   // dolu elips (Adafruit_GFX'te yok)
  for (int y = -ry; y <= ry; y++) { int w = rx * sqrt(max(0.0f, 1 - (float)y * y / (ry * ry))); ekran.drawFastHLine(cx - w, cy + y, 2 * w + 1, renk); }
}

// ---------------------------------------------------------------- Reaksiyon testi: ekran "BAS!" deyince hemen bir tuşa bas, 5 tur ortalaması
bool reaksiyon(Girdi &g) {
  static int tur, toplam, hal; static uint32_t hedef, son; static bool bitti;   // hal 0 bekle, 1 bas, 2 sonuç, 3 erken
  auto bekle = [] { hal = 0; hedef = millis() + random(1500, 4000); };
  auto basla = [&] { tur = toplam = 0; bitti = false; bekle(); };
  if (g.ilk) basla();
  if (bitti) { int ort = toplam / 5; int b = bitis(g, "Ortalama " + String(ort) + " ms", "Rekor " + String(rekorKaydet("reaksiyon", ort, true)) + " ms"); if (b == 1) basla(); return b != 2; }
  bool bas = g.o1 == BASTI || g.o2 == BASTI;
  uint32_t t = millis();
  if (hal == 0) { if (bas) { hal = 3; hedef = t + 1200; } else if (t >= hedef) { hal = 1; hedef = t; } }
  else if (hal == 1) { if (bas) { son = t - hedef; toplam += son; tur++; hal = 2; hedef = t + 1000; } }
  else if (t >= hedef) { if (hal == 2 && tur >= 5) bitti = true; else bekle(); }
  yaz(0, 0, 1, "REAKSIYON  " + String(min(tur + 1, 5)) + "/5");
  if (hal == 0) ortala(28, 1, "bekle...");
  else if (hal == 1) { ekran.fillRect(0, 12, 128, 52, W); ekran.setTextColor(B); ortala(30, 2, "BAS!"); ekran.setTextColor(W); }
  else if (hal == 2) ortala(26, 2, String(son) + " ms");
  else ortala(28, 1, "Erken! tekrar");
  return true;
}

// ---------------------------------------------------------------- Simon: diziyi izle, aynısını tekrarla (sol, sağ ya da ikisi birden)
bool simon(Girdi &g) {
  static uint8_t dizi[64]; static int n, sira, hal; static uint32_t zaman; static bool bitti;   // hal 0 gösteriyor, 1 sen
  auto ekle = [] { dizi[n++] = random(3); sira = 0; hal = 0; zaman = millis() + 600; };
  auto basla = [&] { n = 0; bitti = false; ekle(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Seviye " + String(n - 1), "Rekor " + String(rekorKaydet("simon", n - 1))); if (b == 1) basla(); return b != 2; }
  uint32_t t = millis(); int goster = -1;
  if (hal == 0) {
    if (t >= zaman) { int i = (t - zaman) / 650; if (i >= n) { hal = 1; sira = 0; } else if ((t - zaman) % 650 < 450) goster = dizi[i]; }
  } else {
    int giris = g.combo ? 2 : g.o2 == KISA ? 0 : g.o1 == KISA ? 1 : -1;
    if (g.b1 || g.b2) goster = g.b1 && g.b2 ? 2 : g.b2 ? 0 : 1;
    if (giris >= 0) { if (giris != dizi[sira]) bitti = true; else if (++sira >= n) { if (n < 64) ekle(); else bitti = true; } }
  }
  yaz(0, 0, 1, "SIMON  seviye " + String(n));
  ekran.drawRoundRect(10, 16, 46, 30, 4, W); ekran.drawRoundRect(72, 16, 46, 30, 4, W);
  if (goster == 0 || goster == 2) ekran.fillRoundRect(12, 18, 42, 26, 3, W);
  if (goster == 1 || goster == 2) ekran.fillRoundRect(74, 18, 42, 26, 3, W);
  ortala(54, 1, hal == 0 ? "izle..." : "sira sende " + String(sira) + "/" + String(n));
  return true;
}

// ---------------------------------------------------------------- Hafıza kartları: sağ tuş imleci gezdirir, sol tuş kartı çevirir
void kartSimge(int x, int y, int s) {                 // 6 çift: kare, daire, üçgen, artı, kalp, yıldız
  switch (s) {
    case 0: ekran.fillRect(x - 4, y - 4, 9, 9, W); break;
    case 1: ekran.fillCircle(x, y, 4, W); break;
    case 2: ekran.fillTriangle(x - 5, y + 4, x, y - 5, x + 5, y + 4, W); break;
    case 3: ekran.fillRect(x - 5, y - 1, 11, 3, W); ekran.fillRect(x - 1, y - 5, 3, 11, W); break;
    case 4: ekran.fillCircle(x - 2, y - 1, 2, W); ekran.fillCircle(x + 2, y - 1, 2, W); ekran.fillTriangle(x - 4, y, x + 4, y, x, y + 4, W); break;
    default: ekran.fillTriangle(x - 5, y - 1, x + 5, y - 1, x, y + 4, W); ekran.fillTriangle(x, y - 5, x - 3, y + 4, x + 3, y + 4, W);
  }
}
bool hafiza(Girdi &g) {
  static uint8_t kart[12]; static bool acik[12]; static int imlec, a, b, hamle, bulunan; static uint32_t kapat; static bool bitti;
  auto basla = [] {
    for (int i = 0; i < 12; i++) { kart[i] = i / 2; acik[i] = false; }
    for (int i = 11; i > 0; i--) { int j = random(i + 1); uint8_t t = kart[i]; kart[i] = kart[j]; kart[j] = t; }
    imlec = 0; a = b = -1; hamle = bulunan = 0; kapat = 0; bitti = false;
  };
  if (g.ilk) basla();
  if (bitti) { int b2 = bitis(g, "Bitti! " + String(hamle) + " hamle", "Rekor " + String(rekorKaydet("hafiza", hamle, true)) + " hamle"); if (b2 == 1) basla(); return b2 != 2; }
  if (kapat && millis() > kapat) { acik[a] = acik[b] = false; a = b = -1; kapat = 0; }
  if (g.o1 == KISA) imlec = (imlec + 1) % 12;
  if (g.o2 == KISA && !kapat && !acik[imlec]) {
    acik[imlec] = true;
    if (a < 0) a = imlec;
    else { b = imlec; hamle++;
      if (kart[a] == kart[b]) { a = b = -1; if (++bulunan == 6) bitti = true; } else kapat = millis() + 800; }
  }
  for (int i = 0; i < 12; i++) {
    int x = 4 + (i % 6) * 20, y = 2 + (i / 6) * 26;
    if (acik[i]) { ekran.drawRoundRect(x, y, 18, 23, 3, W); kartSimge(x + 9, y + 11, kart[i]); }
    else { ekran.fillRoundRect(x, y, 18, 23, 3, W); ekran.drawRoundRect(x + 3, y + 3, 12, 17, 2, B); }
    if (i == imlec) ekran.drawRoundRect(x - 2, y - 2, 22, 27, 4, W);
  }
  yaz(0, 56, 1, "hamle " + String(hamle) + "   sag:gez sol:cevir");
  return true;
}

// ---------------------------------------------------------------- Yazı-Tura Serisi: sol YAZI, sağ TURA; üst üste kaç kez bilirsin?
bool paraYaris(Girdi &g) {
  static int seri, tahmin, sonuc; static uint32_t anim; static bool bitti;
  auto basla = [] { seri = 0; tahmin = -1; anim = 0; bitti = false; };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Seri " + String(seri) + (sonuc ? " (TURA geldi)" : " (YAZI geldi)"), "Rekor " + String(rekorKaydet("para", seri))); if (b == 1) basla(); return b != 2; }
  uint32_t t = millis();
  if (!anim && (g.o2 == KISA || g.o1 == KISA)) { tahmin = g.o1 == KISA; sonuc = random(2); anim = t; }
  yaz(0, 0, 1, "YAZI-TURA SERISI  " + String(seri));
  float p = anim ? min(1.0f, (t - anim) / 1100.0f) : 0;
  float w = anim && p < 1 ? fabs(cos(p * 9 * PI)) : 1;                   // dönen para: genişliği salınır
  int y = 32 - sin(p * PI) * 14;
  elips(64, y, max(1, (int)(16 * w)), 16, W);
  if (w > .3) { elips(64, y, (int)(12 * w), 12, B); elips(64, y, max(0, (int)(11 * w)), 11, W); }
  if (anim && p >= 1) { ekran.setTextColor(B); ortala(y - 3, 1, sonuc ? "T" : "Y"); ekran.setTextColor(W);
    if (t - anim > 1700) { if (sonuc == tahmin) { seri++; anim = 0; } else bitti = true; } }
  if (!anim) { yaz(0, 56, 1, "< YAZI"); yaz(92, 56, 1, "TURA >"); }
  return true;
}

// ---------------------------------------------------------------- Okçuluk: sol tuş basılı nişan yukarı/aşağı salınır, sağ tuş basılı güç toplar, bırak = at. 5 ok.
bool okculuk(Girdi &g) {
  static float aci, guc, ox, oy, vx, vy, ruzgar, hedefY; static int ok, puan, sonPuan; static bool ucuyor, bitti; static uint32_t goster;
  auto yeniAtis = [] { ruzgar = random(-30, 31) / 10.0; hedefY = random(16, 48); guc = 0; ucuyor = false; };
  auto basla = [&] { aci = -.3; ok = 0; puan = 0; bitti = false; goster = 0; yeniAtis(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Puan " + String(puan) + "/50", "Rekor " + String(rekorKaydet("okculuk", puan))); if (b == 1) basla(); return b != 2; }
  if (!ucuyor && !goster) {
    if (g.b2) aci = -.3 + sin(millis() / 400.0) * .6;
    if (g.b1) guc = min(1.0f, guc + g.dt * .9f);
    if (g.o1 == KISA || g.o1 == UZUN) { ucuyor = true; ox = 8; oy = 44; vx = cos(aci) * (60 + guc * 90); vy = sin(aci) * (60 + guc * 90); }
  }
  if (ucuyor) {
    vx += ruzgar * 6 * g.dt; vy += 60 * g.dt; ox += vx * g.dt; oy += vy * g.dt;
    if (ox >= 116 && ox < 122) { float d = fabs(oy - hedefY); if (d <= 10) { sonPuan = max(1, 10 - (int)d); ucuyor = false; } }
    if (ox > 130 || oy > 64) { sonPuan = 0; ucuyor = false; }
    if (!ucuyor) { puan += sonPuan; ok++; goster = millis() + 900; }
  }
  if (goster && millis() > goster) { goster = 0; if (ok >= 5) bitti = true; else yeniAtis(); }
  ekran.fillRect(118, hedefY - 10, 3, 21, W); ekran.fillRect(117, hedefY - 5, 5, 11, B); ekran.drawRect(117, hedefY - 5, 5, 11, W); ekran.fillRect(118, hedefY - 1, 3, 3, W);
  ekran.drawFastVLine(120, hedefY + 11, 64, W);
  ekran.drawCircleHelper(6, 44, 9, 2 | 4, W); ekran.drawFastVLine(6, 35, 19, W);   // yay
  if (!ucuyor) { float c = cos(aci), s = sin(aci); ekran.drawLine(8 - c * guc * 5, 44 - s * guc * 5, 8 + c * 14, 44 + s * 14, W); }
  else ekran.drawLine(ox, oy, ox - vx * .05, oy - vy * .05, W);
  yaz(0, 0, 1, "OK " + String(min(ok + 1, 5)) + "/5  " + String(puan)); yaz(80, 0, 1, String(ruzgar > 0 ? ">" : "<") + String(fabs(ruzgar), 1));
  ekran.drawRect(0, 58, 40, 5, W); ekran.fillRect(1, 59, 38 * guc, 3, W);
  if (goster) ortala(24, 2, sonPuan ? "+" + String(sonPuan) : "KACTI");
  return true;
}

// ---------------------------------------------------------------- Bowling: sağ tuş: konumu kilitle, sonra falsoyu kilitle ve at. 5 çerçeve, çerçevede 2 atış.
bool bowling(Girdi &g) {
  static bool lobut[10]; static float bx, by, falso, x0; static int adim, cerceve, atis, puan, devrilen; static uint32_t goster; static bool bitti;
  const int8_t LX[10] = {0, -1, 1, -2, 0, 2, -3, -1, 1, 3}, LY[10] = {0, 1, 1, 2, 2, 2, 3, 3, 3, 3};
  auto lobutXY = [&](int i, float &x, float &y) { x = 64 + LX[i] * 5; y = 18 - LY[i] * 5; };
  auto dizi = [] { for (auto &l : lobut) l = true; };
  auto hazir = [] { adim = 0; by = 60; goster = 0; };
  auto basla = [&] { cerceve = 0; atis = 0; puan = 0; bitti = false; dizi(); hazir(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Puan " + String(puan) + "/50", "Rekor " + String(rekorKaydet("bowling", puan))); if (b == 1) basla(); return b != 2; }
  float t = millis() / 1000.0;
  if (adim == 0) { bx = 64 + sin(t * 2.4) * 18; if (g.o1 == KISA) { adim = 1; x0 = bx; } }
  else if (adim == 1) { falso = sin(t * 3.1) * 25; if (g.o1 == KISA) { adim = 2; devrilen = 0; } }
  else if (adim == 2) {
    by -= 55 * g.dt; bx = x0 + falso * pow((60 - by) / 60, 2);
    for (int i = 0; i < 10; i++) if (lobut[i]) { float lx, ly; lobutXY(i, lx, ly);
      if (fabs(lx - bx) < 4 && fabs(ly - by) < 3) { lobut[i] = false; devrilen++;
        for (int j = 0; j < 10; j++) if (lobut[j] && LY[j] == LY[i] + 1 && abs(LX[j] - LX[i]) == 1 && random(100) < 65) { lobut[j] = false; devrilen++; } } }
    if (by < 0 || bx < 38 || bx > 90) { adim = 3; goster = millis() + 1100; puan += devrilen; }
  } else if (millis() > goster) {
    int kalan = 0; for (auto l : lobut) kalan += l;
    if (atis == 0 && kalan) { atis = 1; hazir(); }
    else { atis = 0; dizi(); hazir(); if (++cerceve >= 5) bitti = true; }
  }
  ekran.drawFastVLine(40, 0, 64, W); ekran.drawFastVLine(88, 0, 64, W);
  for (int i = 0; i < 10; i++) if (lobut[i]) { float lx, ly; lobutXY(i, lx, ly); ekran.fillCircle(lx, ly, 1, W); ekran.drawPixel(lx, ly - 2, W); }
  if (adim < 3 || by >= 0) ekran.drawCircle(bx, by, 2, W);
  if (adim == 1) { for (int i = 0; i < 20; i++) { float yy = 60 - i * 3, xx = x0 + falso * pow(i * 3 / 60.0, 2); ekran.drawPixel(xx, yy, W); } }
  yaz(0, 0, 1, "CERCEVE"); yaz(0, 10, 1, String(min(cerceve + 1, 5)) + "/5"); yaz(0, 24, 1, "PUAN"); yaz(0, 34, 1, String(puan));
  yaz(92, 0, 1, adim == 0 ? "1:konum" : adim == 1 ? "1:falso" : "");
  if (adim == 3) { int kalan = 0; for (auto l : lobut) kalan += l; yaz(92, 24, 1, !kalan && atis == 0 ? "STRIKE" : !kalan ? "SPARE" : String(devrilen) + " lobut"); }
  return true;
}

// ---------------------------------------------------------------- Mini Golf: sol/sağ tuş nişan döndürür, iki tuşu birlikte basılı tut (güç) ve bırak. 3 delik.
bool miniGolf(Girdi &g) {
  struct Duvar { int8_t x, y, w, h; };
  static const Duvar DUV[3][3] = {{{60, 0, 6, 40}, {0, 0, 0, 0}, {0, 0, 0, 0}},
                                  {{30, 20, 6, 44}, {80, 0, 6, 44}, {0, 0, 0, 0}},
                                  {{40, 14, 48, 6}, {40, 44, 48, 6}, {84, 20, 4, 24}}};
  static const int8_t TOP[3][2] = {{14, 50}, {12, 52}, {14, 32}}, DELIK[3][2] = {{110, 12}, {110, 52}, {64, 32}};
  static float x, y, vx, vy, aci; static int delik, vurus, toplam; static bool bitti, hareket; static uint32_t girdi;
  auto kur = [] { x = TOP[delik][0]; y = TOP[delik][1]; vx = vy = 0; aci = -.6; vurus = 0; hareket = false; girdi = 0; };
  auto basla = [&] { delik = 0; toplam = 0; bitti = false; kur(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Toplam " + String(toplam) + " vurus (par 6)", "Rekor " + String(rekorKaydet("golf", toplam, true))); if (b == 1) basla(); return b != 2; }
  if (girdi) { if (millis() > girdi) { toplam += vurus; if (++delik >= 3) bitti = true; else kur(); } }
  else if (!hareket) {
    if (!(g.b1 && g.b2)) { if (g.b2) aci -= 2 * g.dt; if (g.b1) aci += 2 * g.dt; }
    if (g.combo) { float guc = min(1.5f, g.comboSure / 1000.0f) / 1.5f; vx = cos(aci) * guc * 160; vy = sin(aci) * guc * 160; vurus++; hareket = true; }
  } else {
    float nx = x + vx * g.dt, ny = y + vy * g.dt;
    if (nx < 2 || nx > 125) { vx = -vx; nx = x; } if (ny < 2 || ny > 61) { vy = -vy; ny = y; }
    for (auto &d : DUV[delik]) if (d.w && carpisma(nx - 1, ny - 1, 3, 3, d.x, d.y, d.w, d.h)) {
      if (carpisma(x - 1, ny - 1, 3, 3, d.x, d.y, d.w, d.h)) vy = -vy; else vx = -vx; nx = x; ny = y; }
    x = nx; y = ny;
    float h = sqrt(vx * vx + vy * vy), yeni = max(0.0f, h - 70 * g.dt);
    if (h > 0) { vx *= yeni / h; vy *= yeni / h; }
    float dx = x - DELIK[delik][0], dy = y - DELIK[delik][1];
    if (dx * dx + dy * dy < 9 && h < 110) { girdi = millis() + 1200; hareket = false; }
    else if (yeni < 2) { vx = vy = 0; hareket = false; }
  }
  ekran.drawRect(0, 0, 128, 64, W);
  for (auto &d : DUV[delik]) if (d.w) ekran.fillRect(d.x, d.y, d.w, d.h, W);
  ekran.drawCircle(DELIK[delik][0], DELIK[delik][1], 3, W); ekran.drawFastVLine(DELIK[delik][0], DELIK[delik][1] - 11, 9, W);
  ekran.fillTriangle(DELIK[delik][0], DELIK[delik][1] - 11, DELIK[delik][0] + 5, DELIK[delik][1] - 9, DELIK[delik][0], DELIK[delik][1] - 7, W);
  if (!girdi) ekran.fillCircle(x, y, 1, W);
  if (!hareket && !girdi) {
    float guc = g.b1 && g.b2 ? min(1.5f, g.ikiSure / 1000.0f) / 1.5f : 0, u = 8 + guc * 16;
    for (int i = 3; i < u; i += 2) ekran.drawPixel(x + cos(aci) * i, y + sin(aci) * i, W);
  }
  yaz(3, 3, 1, String(delik + 1) + "/3 vurus " + String(vurus));
  if (girdi) ortala(28, 2, vurus == 1 ? "HOLE!" : "GIRDI");
  return true;
}

// ---------------------------------------------------------------- Paraşüt: sol/sağ tuş süzül, kuşlardan kaç, yıldız topla, piste in.
bool parasut(Girdi &g) {
  struct Nesne { float x, y; bool kus, var; };
  static Nesne n[6]; static float x, vx, irtifa, ruzgar, pistX, uret; static int skor, atlayis; static bool bitti, indi; static uint32_t inis;
  auto atla = [] { x = 64; vx = 0; irtifa = 300; ruzgar = random(-20, 21) / 10.0 * (1 + atlayis * .3); pistX = random(20, 108); for (auto &k : n) k.var = false; indi = false; uret = 0; };
  auto basla = [&] { skor = 0; atlayis = 0; bitti = false; atla(); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("parasut", skor))); if (b == 1) basla(); return b != 2; }
  float yer = 20 + irtifa;                             // yerin ekrandaki y'si
  if (indi) { if (millis() > inis) { atlayis++; atla(); } }
  else {
    if (g.b2) vx -= 60 * g.dt; if (g.b1) vx += 60 * g.dt;
    vx = constrain(vx + ruzgar * g.dt * 6, -30.0f, 30.0f) * (1 - g.dt); x = constrain(x + vx * g.dt, 4.0f, 124.0f);
    float dus = 22 + atlayis * 3; irtifa -= dus * g.dt;
    if ((uret -= g.dt) <= 0 && irtifa > 60) { uret = max(.5f, 1.3f - atlayis * .1f); for (auto &k : n) if (!k.var) { k = {(float)random(8, 120), 70, random(3) > 0, true}; break; } }
    for (auto &k : n) if (k.var) {
      k.y -= dus * g.dt; if (k.kus) k.x += sin(millis() / 300.0 + k.y) * .4;
      if (k.y < -6) k.var = false;
      else if (fabs(k.x - x) < 5 && fabs(k.y - 22) < 5) { k.var = false; if (k.kus) bitti = true; else skor += 5; }
    }
    if (yer <= 32) {
      indi = true; inis = millis() + 1300; float d = fabs(x - pistX);
      if (d > 14) bitti = true; else skor += 20 - (int)d;
    }
  }
  for (auto &k : n) if (k.var) {
    if (k.kus) { bool f = (millis() / 150) % 2; ekran.drawLine(k.x - 4, k.y + (f ? -2 : 1), k.x, k.y, W); ekran.drawLine(k.x, k.y, k.x + 4, k.y + (f ? -2 : 1), W); }
    else { ekran.drawLine(k.x - 2, k.y, k.x + 2, k.y, W); ekran.drawLine(k.x, k.y - 2, k.x, k.y + 2, W); ekran.drawPixel(k.x - 1, k.y - 1, W); ekran.drawPixel(k.x + 1, k.y + 1, W); }
  }
  if (yer < 64) { int yy = max(32, (int)yer); ekran.drawFastHLine(0, yy, 128, W); ekran.fillRect(pistX - 14, yy - 1, 29, 2, W); ekran.drawFastVLine(pistX, yy - 6, 5, W); }
  int py = indi ? 24 : 18;                             // paraşütçü
  ekran.drawCircleHelper(x, py, 7, 1 | 2, W); ekran.drawFastHLine(x - 7, py, 15, W);
  ekran.drawLine(x - 6, py, x, py + 7, W); ekran.drawLine(x + 6, py, x, py + 7, W); ekran.fillRect(x - 1, py + 7, 3, 4, W);
  yaz(0, 0, 1, String(skor)); yaz(56, 0, 1, String((int)irtifa) + "m"); yaz(98, 0, 1, String(ruzgar > 0 ? ">" : "<") + String(fabs(ruzgar), 1));
  if (indi && !bitti) ortala(44, 1, "Guzel inis!");
  return true;
}

// ---------------------------------------------------------------- Sonsuz Tırmanış: Kübit kendiliğinden zıplar, sol/sağ tuşla yönlendir; ekranın kenarından öbür tarafa geçer.
bool tirmanis(Girdi &g) {
  struct Plat { float x, y; bool kirik, yok; };
  static Plat p[8]; static float x, y, vy, kamera, vx; static int en; static bool bitti; static int kare;
  auto yer = [](Plat &q, float yy) { q = {(float)random(0, 108), yy, en > 300 && random(5) == 0, false}; };
  auto basla = [&] { x = 60; y = 50; vy = -120; kamera = 0; en = 0; bitti = false; for (int i = 0; i < 8; i++) yer(p[i], 60 - i * 9); p[0].x = 50; p[0].kirik = false; };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Yukseklik " + String(en), "Rekor " + String(rekorKaydet("tirmanis", en))); if (b == 1) basla(); return b != 2; }
  kare++;
  vx = (g.b1 ? 70 : 0) - (g.b2 ? 70 : 0);
  x += vx * g.dt; if (x < -9) x = 128; if (x > 128) x = -9;
  float once = y; vy += 260 * g.dt; y += vy * g.dt;
  if (vy > 0) for (auto &q : p) if (!q.yok && once + 11 <= q.y && y + 11 >= q.y && x + 7 > q.x && x + 2 < q.x + 20) {
    if (q.kirik) q.yok = true; else { y = q.y - 11; vy = -150; }
  }
  if (y < 24) { float d = 24 - y; y = 24; kamera += d; for (auto &q : p) q.y += d; }   // yukarı çıktıkça dünya aşağı kayar
  en = max(en, (int)(kamera / 3));
  for (auto &q : p) if (q.y > 66) { float ust = 99; for (auto &o : p) ust = min(ust, o.y); yer(q, ust - random(8, 11 + min(10, en / 60))); }
  if (y > 64) bitti = true;
  for (auto &q : p) if (!q.yok) { if (q.kirik) for (int i = 0; i < 20; i += 2) ekran.drawPixel(q.x + i, q.y, W); else ekran.drawFastHLine(q.x, q.y, 20, W); ekran.drawFastHLine(q.x + 1, q.y + 1, 18, W); }
  kucukKubit((int)x, (int)y, vy < 0 ? kare : 0);
  ekran.fillRect(0, 0, 30, 9, B); yaz(0, 1, 1, String(en));
  return true;
}
