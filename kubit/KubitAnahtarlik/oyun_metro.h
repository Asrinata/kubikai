// Metro Koşusu: Subway Surfers tarzı. Sol/sağ tık: şerit değiştir. İki tuşa birlikte bas: zıpla (bariyerin üstünden).
// Trenden kaçmak için şerit değiştir, altınları topla; hız gittikçe artar.
#pragma once

namespace metro {
const int UFUK = 14; const float YUK = 46, SERIT = 40;  // perspektif: y = UFUK + YUK/z, x = 64 + şerit*SERIT/z
struct Engel { float z; int8_t serit; uint8_t tur; bool var; };   // tur 0 tren, 1 bariyer, 2 altın
Engel e[24];
float hiz, mesafe, siradaki, serX, zipY, zipV, traversKay; int serit, altin; bool bitti, onceIkisi;

float ekY(float z) { return UFUK + YUK / z; }
float ekX(float s, float z) { return 64 + s * SERIT / z; }
void ekle(float z, int s, uint8_t t) { for (auto &k : e) if (!k.var) { k = {z, (int8_t)s, t, true}; return; } }
void desen() {                                        // her seferde en az bir şerit açık kalır
  int bos = random(3);
  for (int s = 0; s < 3; s++) {
    if (s == bos) { if (random(2)) for (int i = 0; i < 4; i++) ekle(30 + i * 1.5f, s, 2); continue; }
    int r = random(10);
    if (r < 4) ekle(30, s, 0); else if (r < 7) ekle(30, s, 1); else if (r < 9) for (int i = 0; i < 3; i++) ekle(30 + i * 1.5f, s, 2);
  }
}
void ciz(uint32_t t) {
  for (int b = -3; b <= 3; b += 2) ekran.drawLine(ekX(b / 2.0f, 30), ekY(30), ekX(b / 2.0f, .85f), ekY(.85f), W);   // şerit kenarları (raylar)
  for (float z = 1 + fmodf(traversKay, 1.0f); z < 18; z += 1) {   // traversler: oyuncuya doğru akar
    int y = ekY(z); for (int s = -1; s <= 1; s++) { int x1 = ekX(s - .38f, z), x2 = ekX(s + .38f, z); for (int x = x1; x <= x2; x += (z < 4 ? 1 : 2)) ekran.drawPixel(x, y, W); }
  }
  for (int i = 0; i < 5; i++) { int x = ((i * 41 - (int)(mesafe * 2)) % 160 + 160) % 160 - 16; ekran.drawFastVLine(x, 2, UFUK - 2, W); ekran.drawFastHLine(x - 3, 4, 7, W); }   // uzakta direkler
  ekran.drawFastHLine(0, UFUK, 128, W);
  int sira[24], n = 0;                                // uzaktan yakına çiz
  for (int i = 0; i < 24; i++) if (e[i].var && e[i].z > .6f && e[i].z < 30) sira[n++] = i;
  for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (e[sira[j]].z > e[sira[i]].z) std::swap(sira[i], sira[j]);
  for (int i = 0; i < n; i++) {
    const Engel &k = e[sira[i]]; float z = k.z; int x = ekX(k.serit - 1, z), y = ekY(z), w = SERIT / z * .8f;
    if (k.tur == 0) { int h = 44 / z; ekran.fillRect(x - w / 2, y - h, w, h, B); ekran.drawRect(x - w / 2, y - h, w, h, W);   // tren önü: camlar, farlar
      if (w > 8) { ekran.drawRect(x - w / 3, y - h + h / 6, w * 2 / 3, h / 3, W); ekran.fillCircle(x - w / 3, y - h / 5, max(1, w / 12), W); ekran.fillCircle(x + w / 3, y - h / 5, max(1, w / 12), W); } }
    else if (k.tur == 1) { int h = 12 / z; ekran.fillRect(x - w / 2, y - h, w, h, B); ekran.drawRect(x - w / 2, y - h, w, h, W);
      for (int s = 0; s < w; s += 4) ekran.drawLine(x - w / 2 + s, y - 1, x - w / 2 + s + 2, y - h + 1, W); }
    else { int r = max(1, (int)(4 / z)); if ((t / 150) % 2) ekran.drawCircle(x, y - r * 2, r, W); else ekran.drawFastVLine(x, y - r * 3, r * 2, W); }
  }
  int px = lroundf(64 + serX * SERIT / 1.25f), py = lroundf(ekY(1.25f) - zipY * 26);   // oyuncu: arkadan Kübit
  if (zipY > 0) ekran.drawFastHLine(px - 3, ekY(1.25f), 7, W);   // gölge
  ekran.fillRect(px - 5, py - 16, 11, 16, B);
  ekran.drawRoundRect(px - 5, py - 16, 11, 8, 2, W); ekran.drawRect(px - 3, py - 8, 7, 5, W);
  bool f = (t / 90) % 2 && zipY <= 0; ekran.drawLine(px - 2, py - 3, px - 2 - f, py, W); ekran.drawLine(px + 2, py - 3, px + 2 + !f, py, W);
  ekran.fillRect(0, 0, 40, 9, B); yaz(0, 1, 1, String((int)mesafe)); ekran.fillRect(96, 0, 32, 9, B); ekran.fillCircle(100, 4, 2, W); yaz(105, 1, 1, String(altin));
}

bool oyna(Girdi &g) {
  uint32_t t = millis();
  if (g.ilk) { for (auto &k : e) k.var = false; hiz = 9; mesafe = 0; siradaki = 0; serit = 1; serX = 0; zipY = zipV = 0; altin = 0; bitti = false; onceIkisi = false; }
  if (bitti) { int skor = (int)mesafe + altin * 10; int b = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("metro", skor))); if (b == 1) { g.ilk = true; return oyna(g); } return b != 2; }
  if (g.o2 == BASTI) serit = max(0, serit - 1);
  if (g.o1 == BASTI) serit = min(2, serit + 1);
  bool ikisi = g.b1 && g.b2;
  if (ikisi && !onceIkisi && zipY <= 0) zipV = 3.2f;
  onceIkisi = ikisi;
  zipV -= 9 * g.dt; zipY = max(0.0f, zipY + zipV * g.dt); if (zipY <= 0) zipV = 0;
  serX += ((serit - 1) - serX) * min(1.0f, g.dt * 14);
  hiz = min(22.0f, hiz + .25f * g.dt); float yol = hiz * g.dt;
  mesafe += yol; traversKay -= yol / 1.5f; siradaki -= yol;
  if (siradaki <= 0) { desen(); siradaki = random(6, 11) + hiz * .25f; }
  for (auto &k : e) {
    if (!k.var) continue;
    float once = k.z; k.z -= yol / 1.5f;               // dünya birimi -> derinlik
    if (k.z < .6f) { k.var = false; continue; }
    if (once >= 1.25f && k.z < 1.25f && k.serit == serit) {   // oyuncunun hizasını bu karede geçti
      if (k.tur == 2) { altin++; k.var = false; }
      else if (k.tur == 0 || zipY < .35f) { bitti = true; }
    }
  }
  ciz(t);
  return true;
}
}  // namespace metro
bool metroKosusu(Girdi &g) { return metro::oyna(g); }
