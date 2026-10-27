// Oyunlar: zeka (Satranç bulmacası, XOX, Adam Asmaca, Wordle, Matematik Yarışı, Kafadan Hesap).
// Harf seçen oyunlarda: sol/sağ tuş harfte geri/ileri (basılı tutunca hızlı), iki tuş birlikte = onayla.
#pragma once
#include "satranc_bulmaca.h"

const char ALFABE[] = "ABCDEFGHIJKLMNOPRSTUVYZ";  // Türkçe harfler, özel harfsiz (Ç Ğ Ö Ş Ü yok, I hem ı hem i)
const int ALFABE_N = 23;
int harfAdim(Girdi &g) {                               // -1 / +1 / 0: tek basış ya da basılı tutunca 0.4 sn sonra tekrar
  static uint32_t sonraki;
  if (g.b1 && g.b2) return 0;
  int y = g.o2 == BASTI ? -1 : g.o1 == BASTI ? 1 : 0;
  if (y) { sonraki = millis() + 400; return y; }
  if ((g.b1 || g.b2) && millis() > sonraki) { sonraki = millis() + 110; return g.b1 ? 1 : -1; }
  return 0;
}

// ---------------------------------------------------------------- Mini satranç: 1 hamlede mat. Sağ tuş seçeneği gezdirir, sol tuş seçer.
const char *TAS[6][7] = {
  {".......", "..XXX..", "..XXX..", "...X...", "..XXX..", ".XXXXX.", ".XXXXX."},   // piyon
  {"X.X.X.X", "XXXXXXX", ".XXXXX.", ".XXXXX.", ".XXXXX.", "XXXXXXX", "XXXXXXX"},   // kale
  {"..XX...", ".XXXX..", "XXXXXX.", "XX.XXX.", "...XXX.", "..XXXX.", ".XXXXXX"},   // at
  {"...X...", "..XXX..", ".XX.XX.", ".XXXXX.", "..XXX..", "..XXX..", ".XXXXX."},   // fil
  {"X..X..X", "XX.X.XX", "XXXXXXX", ".XXXXX.", ".XXXXX.", "..XXX..", "XXXXXXX"},   // vezir
  {"...X...", "..XXX..", "...X...", ".XXXXX.", "XXXXXXX", ".XXXXX.", "XXXXXXX"}};  // şah
void tasCiz(int x, int y, char t) {                   // beyaz taş: içi boş çerçeve, siyah taş: dolu
  int k = strchr("prnbqk", tolower(t)) - "prnbqk"; bool beyaz = isupper(t);
  auto dolu = [&](int r, int c) { return r >= 0 && r < 7 && c >= 0 && c < 7 && TAS[k][r][c] == 'X'; };
  for (int r = 0; r < 7; r++) for (int c = 0; c < 7; c++) if (dolu(r, c)) {
    bool ic = beyaz && dolu(r - 1, c) && dolu(r + 1, c) && dolu(r, c - 1) && dolu(r, c + 1);
    ekran.drawPixel(x + c, y + r, ic ? B : W);
  }
}
bool satranc(Girdi &g) {
  static int no, secili, dogru; static int sonuc; static uint32_t goster; static bool bitti;   // sonuc 0 yok, 1 doğru, 2 yanlış
  auto basla = [] { no = 0; secili = 0; dogru = 0; sonuc = 0; goster = 0; bitti = false; };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, "Cozulen " + String(dogru) + "/" + String(BULMACA_N), "Rekor " + String(rekorKaydet("satranc", dogru))); if (b == 1) basla(); return b != 2; }
  const SBulmaca &p = BULMACA[no];
  if (goster) { if (millis() > goster) { goster = 0; sonuc = 0; secili = 0; if (++no >= BULMACA_N) bitti = true; return true; } }
  else { if (g.o1 == KISA) secili = (secili + 1) % 4;
    if (g.o2 == KISA) { sonuc = secili == p.dogru ? 1 : 2; dogru += sonuc == 1; goster = millis() + 1500; } }
  const SHamle &h = p.sec[goster && sonuc == 2 ? p.dogru : secili];
  for (int i = 0; i < 64; i++) {
    int x = (i % 8) * 8, y = (i / 8) * 8; bool koyu = ((i % 8) + (i / 8)) % 2;
    if (i == h.den || i == h.ye) ekran.drawRect(x, y, 8, 8, W);
    else if (koyu) for (int k = 0; k < 8; k += 2) ekran.drawPixel(x + k + ((y / 8) % 2), y + 7, W);   // koyu kare: alt kenarda nokta deseni
    if (p.tahta[i] != '.') tasCiz(x, y, p.tahta[i]);
  }
  yaz(68, 0, 1, String(no + 1) + "/" + String(BULMACA_N)); yaz(68, 9, 1, p.beyaz ? "Beyaz mat" : "Siyah mat");
  for (int i = 0; i < 4; i++) {
    int y = 21 + i * 10;
    if (i == secili && !goster) { ekran.fillRect(66, y - 1, 62, 10, W); ekran.setTextColor(B); }
    yaz(70, y, 1, p.sec[i].ad); ekran.setTextColor(W);
  }
  if (goster) { ekran.fillRect(66, 20, 62, 44, B); ortalaX(97, 26, sonuc == 1 ? "MAT!" : "Hayir"); if (sonuc == 2) { yaz(70, 40, 1, "dogrusu:"); yaz(70, 50, 1, p.sec[p.dogru].ad); } }
  return true;
}

// ---------------------------------------------------------------- XOX: Kübit'e karşı. Sağ tuş boş kareler arasında gezer, sol tuş X koyar.
int xoxKazanan(const int8_t *t) {                     // 1 sen, 2 Kübit, 3 berabere, 0 sürüyor
  const uint8_t H[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
  for (auto &h : H) if (t[h[0]] && t[h[0]] == t[h[1]] && t[h[1]] == t[h[2]]) return t[h[0]];
  for (int i = 0; i < 9; i++) if (!t[i]) return 0;
  return 3;
}
int xoxMinimax(int8_t *t, bool kubitSirasi) {
  int k = xoxKazanan(t); if (k == 2) return 1; if (k == 1) return -1; if (k == 3) return 0;
  int en = kubitSirasi ? -2 : 2;
  for (int i = 0; i < 9; i++) if (!t[i]) { t[i] = kubitSirasi ? 2 : 1; int v = xoxMinimax(t, !kubitSirasi); t[i] = 0; en = kubitSirasi ? max(en, v) : min(en, v); }
  return en;
}
bool xox(Girdi &g) {
  static int8_t t[9]; static int imlec, sonuc; static uint32_t kubitHamle; static bool sayildi;
  auto bosaGit = [](int bas) { for (int k = 0; k < 9; k++) { int i = (bas + k) % 9; if (!t[i]) return i; } return 0; };
  auto basla = [&] { memset(t, 0, 9); sonuc = 0; kubitHamle = 0; imlec = 4; sayildi = false; };
  if (g.ilk) basla();
  if (sonuc) {
    if (sonuc == 1 && !sayildi) { sayildi = true; rekorKaydet("xox", kayit.getInt("xox", 0) + 1); }
    int b = bitis(g, sonuc == 1 ? "Kazandin!" : sonuc == 2 ? "Kubit kazandi" : "Berabere", "Toplam galibiyet " + String(kayit.getInt("xox", 0)));
    if (b == 1) basla(); return b != 2;
  }
  if (kubitHamle) {
    if (millis() > kubitHamle) {                        // Kübit: %25 rastgele, yoksa minimax (yenilebilir kalsın)
      int sec = -1, en = -2;
      if (random(4) == 0) sec = bosaGit(random(9));
      else for (int i = 0; i < 9; i++) if (!t[i]) { t[i] = 2; int v = xoxMinimax(t, false); t[i] = 0; if (v > en) { en = v; sec = i; } }
      t[sec] = 2; kubitHamle = 0; sonuc = xoxKazanan(t); if (!sonuc) imlec = bosaGit(imlec);
    }
  } else {
    if (g.o1 == KISA) imlec = bosaGit(imlec + 1);
    if (g.o2 == KISA && !t[imlec]) { t[imlec] = 1; sonuc = xoxKazanan(t); if (!sonuc) kubitHamle = millis() + 500; }
  }
  for (int i = 1; i < 3; i++) { ekran.drawFastVLine(34 + i * 20, 2, 60, W); ekran.drawFastHLine(34, 2 + i * 20, 60, W); }
  for (int i = 0; i < 9; i++) {
    int cx = 44 + (i % 3) * 20, cy = 12 + (i / 3) * 20;
    if (t[i] == 1) { ekran.drawLine(cx - 6, cy - 6, cx + 6, cy + 6, W); ekran.drawLine(cx + 6, cy - 6, cx - 6, cy + 6, W); }
    if (t[i] == 2) ekran.drawCircle(cx, cy, 7, W);
    if (i == imlec && !kubitHamle && !t[i] && (millis() / 300) % 2) ekran.drawRect(cx - 8, cy - 8, 17, 17, W);
  }
  yaz(0, 0, 1, "SEN X"); yaz(98, 0, 1, "KUBIT"); yaz(110, 9, 1, "O");
  if (kubitHamle) yaz(98, 30, 1, "...");
  return true;
}

// ---------------------------------------------------------------- Adam Asmaca
const char *KELIMELER[] = {"BILGISAYAR", "KLAVYE", "TELEFON", "ANAHTAR", "PILAV", "KARPUZ", "ZEYTIN", "MAKARNA", "YILDIZ", "GALATASARAY",
  "ISTANBUL", "ANKARA", "TELEVIZYON", "BISIKLET", "OTOMOBIL", "HELIKOPTER", "DINOZOR", "KELEBEK", "ROBOT", "KUMANDA", "SANDALYE", "PATATES",
  "DOMATES", "MANDALINA", "PORTAKAL", "ASTRONOT", "ANAHTARLIK", "KAMERA", "HOPARLOR", "BULUTLAR", "PENCERE", "KAPLAN", "ZURAFA", "TAVUK"};
const int KELIME_N = sizeof(KELIMELER) / sizeof(*KELIMELER);
bool adamAsmaca(Girdi &g) {
  static const char *k; static bool denendi[ALFABE_N]; static int harf, hata, seri; static bool bitti, kazandi;
  auto basla = [] { k = KELIMELER[random(KELIME_N)]; memset(denendi, 0, sizeof(denendi)); harf = 0; hata = 0; bitti = kazandi = false; };
  if (g.ilk) { seri = 0; basla(); }
  if (bitti) { int b = bitis(g, kazandi ? "Bildin! seri " + String(seri) : String("Cevap: ") + k, "Rekor seri " + String(rekorKaydet("adam", seri))); if (b == 1) { if (!kazandi) seri = 0; basla(); } return b != 2; }
  int a = harfAdim(g);
  if (a) for (int i = 0; i < ALFABE_N; i++) { harf = (harf + a + ALFABE_N) % ALFABE_N; if (!denendi[harf]) break; }
  if (g.combo && !denendi[harf]) {
    denendi[harf] = true;
    if (!strchr(k, ALFABE[harf])) { if (++hata >= 6) bitti = true; }
    bool tamam = true; for (const char *c = k; *c; c++) if (!denendi[strchr(ALFABE, *c) - ALFABE]) tamam = false;
    if (tamam) { kazandi = bitti = true; seri++; }
  }
  ekran.drawFastHLine(2, 50, 30, W); ekran.drawFastVLine(8, 4, 46, W); ekran.drawFastHLine(8, 4, 20, W); ekran.drawFastVLine(28, 4, 6, W);   // darağacı
  if (hata > 0) ekran.drawCircle(28, 14, 4, W);
  if (hata > 1) ekran.drawFastVLine(28, 18, 14, W);
  if (hata > 2) ekran.drawLine(28, 22, 22, 27, W);
  if (hata > 3) ekran.drawLine(28, 22, 34, 27, W);
  if (hata > 4) ekran.drawLine(28, 32, 23, 41, W);
  if (hata > 5) ekran.drawLine(28, 32, 33, 41, W);
  int n = strlen(k), w = min(9, 88 / n), x0 = 40;
  for (int i = 0; i < n; i++) { int idx = strchr(ALFABE, k[i]) - ALFABE; ekran.drawFastHLine(x0 + i * w, 30, w - 2, W); if (denendi[idx]) yaz(x0 + i * w, 21, 1, String(k[i])); }
  for (int d = -3; d <= 3; d++) {                       // harf şeridi: ortadaki seçili
    int i = (harf + d + ALFABE_N) % ALFABE_N, x = 77 + d * 12;
    if (!d) { ekran.fillRect(x - 3, 40, 12, 11, W); ekran.setTextColor(B); }
    if (!denendi[i] || !d) yaz(x, 42, 1, String(ALFABE[i]));
    ekran.setTextColor(W);
  }
  yaz(40, 56, 1, "2 tus: tahmin et");
  return true;
}

// ---------------------------------------------------------------- Türkçe Wordle: 5 harf, 6 hak. Dolu kutu = doğru yer, çerçeve = kelimede var ama başka yerde.
const char *WORDLE[] = {"KALEM", "KITAP", "MASAL", "DENIZ", "BULUT", "ELMAS", "BALIK", "KAPAK", "HAYAT", "YILAN", "SABAH", "KAVAK", "LIMON",
  "ARMUT", "TAVUK", "ROBOT", "RADYO", "KABLO", "MOTOR", "PILOT", "MAKAS", "TABAK", "KAZAK", "CEKET", "PALTO", "BEYAZ", "SIYAH", "BIBER",
  "KAHVE", "LIMAN", "TARLA", "ORMAN", "NEHIR", "DALGA", "YAZAR", "SINIF", "KALIP", "DOLAP", "PERDE", "YATAK", "MERAK", "SEVGI", "ZAMAN",
  "SAYFA", "KONUM", "HABER", "KANAL", "DUMAN", "BUHAR", "KAYAK", "KUZEY", "VAPUR", "METRO", "TAKSI", "ROKET", "BILGI", "ASLAN", "TILKI",
  "ZEBRA", "KOALA", "PANDA", "HOROZ", "MANDA", "KARGA", "MARTI", "DAVUL", "GITAR", "KEMAN", "SALON", "BAHAR"};
const int WORDLE_N = sizeof(WORDLE) / sizeof(*WORDLE);
bool wordle(Girdi &g) {
  static char tah[6][6]; static uint8_t renk[6][5]; static int satir, sutun, harf; static const char *k; static bool bitti, kazandi; static uint8_t bilgi[ALFABE_N];   // renk/bilgi: 0 yok 1 var 2 yerinde, bilgi 3 = denenmedi
  auto basla = [] { k = WORDLE[random(WORDLE_N)]; memset(tah, 0, sizeof(tah)); satir = sutun = harf = 0; bitti = kazandi = false; memset(bilgi, 3, sizeof(bilgi)); };
  if (g.ilk) basla();
  if (bitti) { int b = bitis(g, kazandi ? "Bildin! " + String(satir) + ". denemede" : String("Kelime: ") + k, "Kazanilan " + String(kayit.getInt("wordle", 0))); if (b == 1) basla(); return b != 2; }
  int a = harfAdim(g);
  const int SECIM_N = ALFABE_N + 1;                    // son seçenek "<" = sil
  if (a) harf = (harf + a + SECIM_N) % SECIM_N;
  if (g.combo) {
    if (harf == ALFABE_N) { if (sutun) tah[satir][--sutun] = 0; }
    else if (sutun < 5) tah[satir][sutun++] = ALFABE[harf];
    if (sutun == 5 && harf != ALFABE_N) {               // satır doldu: değerlendir
      bool kul[5] = {}; int dogru = 0;
      for (int i = 0; i < 5; i++) { renk[satir][i] = 0; if (tah[satir][i] == k[i]) { renk[satir][i] = 2; kul[i] = true; dogru++; } }
      for (int i = 0; i < 5; i++) if (!renk[satir][i]) for (int j = 0; j < 5; j++) if (!kul[j] && tah[satir][i] == k[j]) { renk[satir][i] = 1; kul[j] = true; break; }
      for (int i = 0; i < 5; i++) { uint8_t &b = bilgi[strchr(ALFABE, tah[satir][i]) - ALFABE]; b = b == 3 ? renk[satir][i] : max(b, renk[satir][i]); }
      satir++; sutun = 0;
      if (dogru == 5) { kazandi = bitti = true; rekorKaydet("wordle", kayit.getInt("wordle", 0) + 1); }
      else if (satir >= 6) bitti = true;
    }
  }
  for (int r = 0; r < 6; r++) for (int c = 0; c < 5; c++) {
    int x = 2 + c * 11, y = r * 10 + 1; char ch = tah[r][c];
    if (r < satir && renk[r][c] == 2) { ekran.fillRect(x, y, 10, 9, W); ekran.setTextColor(B); }
    else if (r < satir && renk[r][c] == 1) ekran.drawRect(x, y, 10, 9, W);
    else ekran.drawFastHLine(x, y + 8, 10, W);
    if (ch) yaz(x + 2, y + 1, 1, String(ch));
    ekran.setTextColor(W);
  }
  if (satir < 6 && sutun < 5 && (millis() / 300) % 2) ekran.drawFastHLine(2 + sutun * 11, satir * 10 + 9, 10, W);
  int x = 66;                                          // sağ: harf seçici + bilgi
  yaz(x, 0, 1, "harf sec:");
  for (int d = -2; d <= 2; d++) {
    int i = (harf + d + SECIM_N) % SECIM_N, xx = x + 24 + d * 12;
    char ch = i == ALFABE_N ? '<' : ALFABE[i];
    if (!d) { ekran.fillRect(xx - 3, 12, 12, 11, W); ekran.setTextColor(B); }
    yaz(xx, 14, 1, String(ch));
    if (i < ALFABE_N && bilgi[i] == 0 && d) ekran.drawLine(xx - 1, 18, xx + 6, 18, W);   // olmadığı bilinen harf: üstü çizili
    ekran.setTextColor(W);
  }
  if (harf < ALFABE_N) yaz(x, 28, 1, bilgi[harf] == 3 ? "denenmedi" : bilgi[harf] == 0 ? "yok" : bilgi[harf] == 1 ? "var" : "yerinde");
  else yaz(x, 28, 1, "sil");
  yaz(x, 44, 1, "2 tus:"); yaz(x, 53, 1, "onayla");
  return true;
}

// ---------------------------------------------------------------- Matematik Yarışı: 60 sn, işlem doğru mu? sol YANLIS, sağ DOGRU. Yanlış cevap -3 sn.
bool matYaris(Girdi &g) {
  static int a, b, op, gosterilen, skor; static float kalan; static bool dogruMu, bitti; static uint32_t flas; static bool sonDogru;
  auto soru = [] {
    op = random(3); int zor = 1 + skor / 6;
    a = random(2, 10 * zor + 2); b = random(2, op == 2 ? 10 : 10 * zor + 2);
    int c = op == 0 ? a + b : op == 1 ? a - b : a * b;
    dogruMu = random(2); gosterilen = dogruMu ? c : c + (random(2) ? 1 : -1) * (int)random(1, 4) * (op == 2 && random(2) ? b : 1);
    if (gosterilen == c) dogruMu = true;
  };
  auto basla = [&] { skor = 0; kalan = 60; bitti = false; flas = 0; soru(); };
  if (g.ilk) basla();
  if (bitti) { int r = bitis(g, "Skor " + String(skor), "Rekor " + String(rekorKaydet("matyaris", skor))); if (r == 1) basla(); return r != 2; }
  kalan -= g.dt; if (kalan <= 0) bitti = true;
  int cevap = g.o2 == BASTI ? 0 : g.o1 == BASTI ? 1 : -1;
  if (cevap >= 0) { sonDogru = cevap == dogruMu; if (sonDogru) skor++; else kalan -= 3; flas = millis() + 250; soru(); }
  const char *OP = "+-x";
  ortala(18, 2, String(a) + OP[op] + String(b) + "=" + String(gosterilen));
  yaz(0, 0, 1, "SKOR " + String(skor)); yaz(92, 0, 1, String(max(0, (int)ceil(kalan))) + " sn");
  ekran.drawRect(0, 10, 128, 3, W); ekran.fillRect(0, 10, 128 * max(0.0f, kalan) / 60, 3, W);
  yaz(0, 56, 1, "< YANLIS"); yaz(86, 56, 1, "DOGRU >");
  if (flas > millis()) ortala(40, 1, sonDogru ? "dogru!" : "yanlis -3 sn");
  return true;
}

// ---------------------------------------------------------------- Kafadan Hesap: iki şıktan doğruyu seç, 6 sn süre, 3 can. Sorular gittikçe zorlaşır.
bool kafadan(Girdi &g) {
  static int a, b, op, cevap, yanlis, dogruTaraf, skor, can; static float sure; static bool bitti;
  auto soru = [] {
    int seviye = skor / 4; op = random(min(4, 2 + seviye));
    if (op == 0) { a = random(10, 50 + seviye * 30); b = random(10, 50 + seviye * 30); cevap = a + b; }
    else if (op == 1) { a = random(30, 90 + seviye * 30); b = random(5, a); cevap = a - b; }
    else if (op == 2) { a = random(3, 10 + seviye * 2); b = random(3, 13); cevap = a * b; }
    else { b = random(2, 10); cevap = random(2, 13 + seviye); a = b * cevap; }
    int d[] = {1, 2, 10, -1, -2, -10}; yanlis = cevap + d[random(6)]; if (op == 2 && random(2)) yanlis = cevap + (random(2) ? a : -a);
    if (yanlis == cevap) yanlis++;
    dogruTaraf = random(2); sure = max(3.0f, 6 - skor * .1f);
  };
  auto basla = [&] { skor = 0; can = 3; bitti = false; soru(); };
  if (g.ilk) basla();
  if (bitti) { int r = bitis(g, "Dogru " + String(skor), "Rekor " + String(rekorKaydet("kafadan", skor))); if (r == 1) basla(); return r != 2; }
  sure -= g.dt;
  int sec = g.o2 == BASTI ? 0 : g.o1 == BASTI ? 1 : -1;
  if (sec >= 0 || sure <= 0) { if (sec == dogruTaraf) skor++; else if (--can <= 0) { bitti = true; return true; } soru(); }
  const char *OP = "+-x/";
  ortala(14, 2, String(a) + " " + OP[op] + " " + String(b));
  int sol = dogruTaraf == 0 ? cevap : yanlis, sag = dogruTaraf == 1 ? cevap : yanlis;
  ekran.drawRoundRect(4, 36, 54, 20, 3, W); ekran.drawRoundRect(70, 36, 54, 20, 3, W);
  ortalaX(31, 42, String(sol)); ortalaX(97, 42, String(sag));
  yaz(0, 0, 1, String(skor)); for (int i = 0; i < can; i++) ekran.fillCircle(124 - i * 7, 3, 2, W);
  ekran.fillRect(30, 2, 68 * max(0.0f, sure) / 6, 3, W);
  return true;
}
