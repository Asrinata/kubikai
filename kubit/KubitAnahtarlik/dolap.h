// Kıyafet dolabı + günlük çark: 16 kıyafet (kafa, göz, boyun, sırt; her yuvadan 4). Çark günde bir kez döner ve her seferinde
// sahip olmadığın bir kıyafet verir. Giyilenler ana ekrandaki Kübit'te görünür. NVS: "kiyafet" (sahip olunanlar, bit), "k_s0..3" (giyilen), "cark_gun".
#pragma once

const char *KIYAFET_AD[16] = {"Silindir sapka", "Kep", "Tac", "Parti sapkasi", "Gunes gozlugu", "Monokl", "Maske", "Kalp gozluk",
                              "Papyon", "Kravat", "Sari-kirmizi atki", "Madalya", "Pelerin", "Kanatlar", "Jetpack", "Hale"};
const char *YUVA_AD[4] = {"kafa", "goz", "boyun", "sirt"};
int kiyafetSahip() { return kayit.getInt("kiyafet", 0); }
int giyili[4] = {-1, -1, -1, -1};                    // her yuvada giyilen (açılışta NVS'ten)
int kiyafetGiyili(int yuva) { return giyili[yuva]; }
void kiyafetYukle() { for (int i = 0; i < 4; i++) giyili[i] = kayit.getInt(("k_s" + String(i)).c_str(), -1); }
void kiyafetGiy(int yuva, int k) { giyili[yuva] = k; kayit.putInt(("k_s" + String(yuva)).c_str(), k); }

void kiyafetCiz(int x, int y, uint32_t t) {          // ana ekrandaki Kübit üstüne (x, y: kubit() ile aynı sol üst, kafa 41x21)
  for (int yuva = 3; yuva >= 0; yuva--) {             // önce sırt, sonra boyun, göz, kafa
    int k = giyili[yuva]; if (k < 0) continue;
    switch (k) {
      case 0: ekran.fillRect(x + 13, y - 9, 15, 9, W); ekran.drawFastHLine(x + 9, y - 1, 23, W); ekran.drawFastHLine(x + 13, y - 3, 15, B); break;   // silindir
      case 1: ekran.fillCircle(x + 20, y + 1, 9, W); ekran.fillRect(x + 10, y + 1, 22, 9, B); ekran.fillRect(x + 20, y - 1, 16, 3, W); break;   // kep
      case 2: for (int i = 0; i < 4; i++) ekran.fillTriangle(x + 9 + i * 7, y - 1, x + 13 + i * 7, y - 9, x + 17 + i * 7, y - 1, W); ekran.fillRect(x + 9, y - 3, 30, 3, W); break;   // taç
      case 3: ekran.fillTriangle(x + 14, y, x + 26, y, x + 20, y - 12, W); ekran.fillCircle(x + 20, y - 13, 2, W); ekran.drawLine(x + 17, y - 4, x + 23, y - 8, B); break;   // parti
      case 4: ekran.fillRoundRect(x + 8, y + 7, 9, 5, 1, W); ekran.fillRoundRect(x + 24, y + 7, 9, 5, 1, W); ekran.drawLine(x + 17, y + 9, x + 23, y + 9, W); break;
      case 5: ekran.drawCircle(x + 28, y + 9, 5, W); ekran.drawLine(x + 33, y + 9, x + 36, y + 20, W); break;   // monokl
      case 6: ekran.fillRect(x + 3, y + 6, 35, 7, W); ekran.fillCircle(x + 12, y + 9, 2, B); ekran.fillCircle(x + 28, y + 9, 2, B); break;   // maske
      case 7: for (int gx : {12, 28}) { ekran.fillCircle(x + gx - 2, y + 8, 2, W); ekran.fillCircle(x + gx + 2, y + 8, 2, W); ekran.fillTriangle(x + gx - 4, y + 9, x + gx + 4, y + 9, x + gx, y + 13, W); } break;
      case 8: ekran.fillTriangle(x + 20, y + 23, x + 14, y + 20, x + 14, y + 26, W); ekran.fillTriangle(x + 20, y + 23, x + 26, y + 20, x + 26, y + 26, W); break;   // papyon
      case 9: ekran.fillTriangle(x + 18, y + 22, x + 22, y + 22, x + 20, y + 25, W); ekran.fillTriangle(x + 18, y + 25, x + 22, y + 25, x + 20, y + 37, W); break;   // kravat
      case 10: for (int i = 0; i < 7; i++) if (i % 2 == (t / 400) % 2) ekran.fillRect(x + 8 + i * 4, y + 21, 4, 4, W); else ekran.drawRect(x + 8 + i * 4, y + 21, 4, 4, W);   // atkı
               ekran.fillRect(x + 30, y + 25, 4, 8, W); break;
      case 11: ekran.drawLine(x + 16, y + 21, x + 20, y + 30, W); ekran.drawLine(x + 24, y + 21, x + 20, y + 30, W); ekran.fillCircle(x + 20, y + 33, 3, W); break;   // madalya
      case 12: ekran.drawLine(x + 7, y + 27, x - 1, y + 48, W); ekran.drawLine(x + 33, y + 27, x + 41, y + 48, W); ekran.drawLine(x - 1, y + 48, x + 41, y + 48, W); break;   // pelerin
      case 13: for (int s : {-1, 1}) { int ux = s < 0 ? x + 6 : x + 34; ekran.drawLine(ux, y + 30, ux + s * 10, y + 22 - (t / 200) % 2 * 2, W); ekran.drawLine(ux + s * 10, y + 22 - (t / 200) % 2 * 2, ux + s * 6, y + 36, W); } break;   // kanat
      case 14: ekran.drawRect(x + 35, y + 27, 5, 12, W); if ((t / 100) % 2) ekran.fillTriangle(x + 35, y + 39, x + 39, y + 39, x + 37, y + 45, W); break;   // jetpack
      case 15: { int hy = y - 6 - (int)(sinf(t / 300.0f) * 1.5f); ekran.drawFastHLine(x + 12, hy, 17, W); ekran.drawFastHLine(x + 10, hy + 1, 21, W); ekran.drawFastHLine(x + 12, hy + 2, 17, W); } break;   // hale
    }
  }
}
void kiyafetSimge(int k, int x, int y) {              // listede minik simge (8x8)
  const uint8_t S[16][8] = {{0x3C, 0x3C, 0x3C, 0x3C, 0xFF, 0, 0, 0}, {0x3C, 0x7E, 0x7E, 0xFF, 0x0F, 0, 0, 0}, {0, 0xAA, 0xFF, 0xFF, 0x7E, 0, 0, 0}, {0x18, 0x18, 0x3C, 0x34, 0x7E, 0x6E, 0xFF, 0},
                              {0, 0, 0xE7, 0xE7, 0x42, 0, 0, 0}, {0, 0x3C, 0x42, 0x42, 0x3C, 0x04, 0x04, 0x02}, {0, 0xFF, 0x99, 0x99, 0xFF, 0, 0, 0}, {0, 0x66, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0},
                              {0, 0xC3, 0xE7, 0xFF, 0xE7, 0xC3, 0, 0}, {0x18, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0}, {0, 0xAA, 0x55, 0xAA, 0x55, 0x06, 0x06, 0}, {0x42, 0x24, 0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0},
                              {0x18, 0x3C, 0x7E, 0x7E, 0xFF, 0xFF, 0xFF, 0}, {0x81, 0xC3, 0xE7, 0xFF, 0x7E, 0x24, 0, 0}, {0x3C, 0x24, 0x24, 0x24, 0x3C, 0x18, 0x08, 0x10}, {0x7E, 0x81, 0x7E, 0, 0, 0, 0, 0}};
  ekran.drawBitmap(x, y, S[k], 8, 8, W);
}
bool carkHazir() { return saatVar() && kayit.getInt("cark_gun", -1) != (int)(time(nullptr) / 86400); }

void dolapSayfasi(Olay o1, Olay o2) {                 // sağ: gez, sol: giy/çıkar; en üstte günlük çark
  int sahip = kiyafetSahip();
  String og[17]; og[0] = carkHazir() ? dl("* GUNLUK CARK hazir!", "* DAILY WHEEL ready!") : saatVar() ? dl("Cark: yarin tekrar", "Wheel: tomorrow") : dl("Cark: saat lazim", "Wheel: needs clock");
  for (int k = 0; k < 16; k++) {
    bool var = sahip >> k & 1, giyili = kiyafetGiyili(k / 4) == k;
    og[k + 1] = String(giyili ? "[x] " : var ? "[ ] " : " ?  ") + (var ? KIYAFET_AD[k] : "???");
  }
  imlec = min(imlec, 16);
  if (o1 == KISA) imlec = (imlec + 1) % 17;
  if (geriAdim) imlec = (imlec + 16) % 17;
  if (o2 == KISA) {
    if (imlec == 0) { if (carkHazir()) { git(CARK_GUNLUK); return; } goster(og[0]); }
    else { int k = imlec - 1; if (sahip >> k & 1) kiyafetGiy(k / 4, giyili[k / 4] == k ? -1 : k); else goster(dl("Carktan kazan!", "Win it on the wheel!")); }
  }
  kubit(MUTLU, 0, 1, 0, millis() / 40); kiyafetCiz(4, 14, millis());   // solda canlı önizleme
  ekran.fillRect(50, 0, 78, 64, B); yaz(52, 0, 1, dl("DOLAP", "WARDROBE")); yaz(104, 0, 1, String(__builtin_popcount(sahip)) + "/16"); ekran.drawFastHLine(50, 9, 78, W);
  int ilk = constrain(imlec - 2, 0, 17 - 5);
  for (int i = ilk; i < ilk + 5; i++) {
    int y = 12 + (i - ilk) * 10;
    if (i == imlec) { ekran.fillRect(50, y - 1, 78, 10, W); ekran.setTextColor(B); }
    if (i == 0) yaz(52, y, 1, carkHazir() ? dl("GUNLUK CARK!", "DAILY WHEEL!") : dl("cark: yarin", "wheel: tmrw"));
    else { int k = i - 1; bool var = sahip >> k & 1; if (var) { if (i == imlec) { ekran.fillRect(52, y, 8, 8, B); } kiyafetSimge(k, 52, y); }
      yaz(62, y, 1, var ? String(KIYAFET_AD[k]).substring(0, 9) : "???"); if (kiyafetGiyili(k / 4) == k) yaz(120, y, 1, "*"); }
    ekran.setTextColor(W);
  }
  altMesaj();
}

void gunlukCarkSayfasi(Olay o1, Olay o2) {            // sol: çevir; çark yavaşlar, oktaki kıyafet senin (hep sahip olmadığın biri)
  static float aci = 0, hiz = 0; static int dilim[8], hedef = -1; static bool bitti = false, hazir = false;
  const float ADIM = 2 * PI / 8, YAVAS = .014f;
  int sahip = kiyafetSahip();
  if (!hazir) { int n = 0; for (int k = 0; k < 16 && n < 8; k++) if (!(sahip >> k & 1)) dilim[n++] = k; for (int k = 0; n < 8; k++) dilim[n++] = k % 16; hazir = true; }
  if (o2 == KISA && hiz == 0 && !bitti) {
    int bos[16], n = 0; for (int k = 0; k < 16; k++) if (!(sahip >> k & 1)) bos[n++] = k;
    if (!n) { goster(dl("Tum kiyafetler sende!", "You have them all!")); kayit.putInt("cark_gun", time(nullptr) / 86400); hazir = false; git(DOLAP); return; }
    hedef = bos[random(n)]; int d = random(8); dilim[d] = hedef;
    hiz = .9f + random(20) / 100.0f;
    float yol = 0; for (float h = hiz; h > 0; h -= YAVAS) yol += h;   // toplam dönüş: tam hedef dilimde dursun
    aci = -HALF_PI - (d + .5f) * ADIM - yol;
  }
  if (hiz > 0) { aci += hiz; hiz -= YAVAS; if (hiz <= 0) { hiz = 0; bitti = true;
      kayit.putInt("kiyafet", sahip | 1 << hedef); kiyafetGiy(hedef / 4, hedef); kayit.putInt("cark_gun", time(nullptr) / 86400);
      goster(String(dl("Kazandin: ", "You won: ")) + KIYAFET_AD[hedef], 4000); } }
  if (bitti && (o1 == KISA || o2 == KISA)) { bitti = false; hedef = -1; hazir = false; git(DOLAP); return; }
  const int CX = 36, CY = 34, R = 27;
  ekran.drawCircle(CX, CY, R, W); ekran.drawCircle(CX, CY, R - 1, W);
  for (int i = 0; i < 8; i++) {
    float a = aci + i * ADIM; ekran.drawLine(CX, CY, CX + cosf(a) * R, CY + sinf(a) * R, W);
    float m = a + ADIM / 2; kiyafetSimge(dilim[i], CX + cosf(m) * 17 - 4, CY + sinf(m) * 17 - 4);
  }
  ekran.fillCircle(CX, CY, 4, W); ekran.fillTriangle(CX - 4, 2, CX + 4, 2, CX, 9, W);   // ok üstte
  yaz(70, 6, 1, dl("GUNLUK", "DAILY")); yaz(70, 15, 1, dl("CARK", "WHEEL"));
  if (bitti) { kiyafetSimge(hedef, 70, 28); auto s = sar(KIYAFET_AD[hedef], 9); int y = 40; for (auto &l : s) { yaz(70, y, 1, l); y += 9; } }
  else yaz(70, 50, 1, hiz > 0 ? dl("donuyor", "spinning") : dl("sol: cevir", "L: spin"));
}
