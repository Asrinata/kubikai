// Kübit Doom: raycasting FPS. Sol/sağ basılı: dön. İki tuş basılı: yürü. İki tuşa kısa tık: ateş. Sağ tuşa çift tık: silah değiştir.
// 3 bölüm: kapılar (D) kendiliğinden açılır, kilitli kapı (L) için anahtar (K) bul, çıkışa (E) ulaş. Pompalı (S), roketatar (R),
// 3. bölümde patron (X): ölmeden çıkış açılmaz. Başlangıç menüsünde kendi bölümünü yapabileceğin düzenleyici var (NVS "doom_oz").
#pragma once

namespace doom {
const int N = 16;
const char *HARITA[3][N] = {
  {"################", "#P....#........#", "#.....#..Z.....#", "#.....D........#", "#.....#....H...#", "###D###.......Z#", "#.....#####D####", "#..A..#...S....#",
   "#.....#..Z.....#", "#..Z..#........#", "#.....#...##L###", "###.###...#....#", "#.........#..E.#", "#..K......#....#", "#.....Z...#....#", "################"},
  {"################", "#P.#.....#.....#", "#..#..Z..#..A..#", "#..D.....D.....#", "#..#.....#..Z..#", "#..###D###.....#", "#....#...####D##", "##.Z.#.H.#.....#",
   "#.R..#...#..Z..#", "#.K..D...#.....#", "######.###.....#", "#....#.#..###L##", "#..Z.#.#..#....#", "#....D.D..#..E.#", "#..A.#.#.Z#....#", "################"},
  {"################", "#P...#....Z....#", "#....#.........#", "#....D...##D####", "#.Z..#...#.....#", "##D###...#..Z..#", "#....#...#.....#", "#.A..##D##..H..#",
   "#....#.Z.#.....#", "#..Z.#...###D###", "#....#.K.#.....#", "###D##...#.Z...#", "#....#####..X..#", "#.Z..L..E#..A..#", "#....#...#..Z..#", "################"}};
enum Karo : uint8_t { ZEMIN, DUVAR, KAPI, KILIT, CIKIS };
const uint16_t ZOMBI[2][16] = {
  {0x03C0, 0x07E0, 0x05A0, 0x07E0, 0x0240, 0x0180, 0x1FF8, 0x2FF4, 0x2FF4, 0x27E4, 0x07E0, 0x0660, 0x0660, 0x0C30, 0x0810, 0x1818},
  {0x03C0, 0x07E0, 0x05A0, 0x07E0, 0x0240, 0x0180, 0x1FF8, 0x2FF4, 0x2FF4, 0x27E4, 0x07E0, 0x0660, 0x0240, 0x0240, 0x0660, 0x0000}};
const uint16_t OLU[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0DE0, 0x3FFC, 0x7FFE, 0x2DB8};
const uint16_t SAGLIK[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0FF0, 0x0990, 0x0BD0, 0x0BD0, 0x0990, 0x0FF0, 0};
const uint16_t MERMI[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0540, 0x0540, 0x0FF0, 0x0810, 0x0BD0, 0x0810, 0x0FF0};
const uint16_t ANAHTAR[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0E00, 0x11FF, 0x110A, 0x0E00, 0, 0};
const uint16_t POMPALI_S[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FF0, 0x7FF0, 0x0F80, 0x0C00, 0x0C00, 0};
const uint16_t ROKET_S[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0x3FFC, 0x7FFE, 0x7FFE, 0x3FFC, 0x0300, 0x0300, 0};
const uint16_t PATRON_S[16] = {0x4002, 0x6006, 0x37EC, 0x1FF8, 0x1BD8, 0x1FF8, 0x0E70, 0x07E0, 0x3FFC, 0x7FFE, 0xDFFB, 0x9FF9, 0x1E78, 0x1C38, 0x1818, 0x3C3C};
const uint16_t ATES_S[16] = {0, 0, 0, 0, 0, 0, 0x0180, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x0180, 0, 0, 0, 0};
const char *SILAH_AD[3] = {"Tabanca", "Pompali", "Roketatar"};
const int SILAH_MERMI[3] = {1, 2, 4}; const float SILAH_BEKLE[3] = {.25f, .7f, 1.0f};
struct Mermi { float x, y, vx, vy; bool dusman, var; };
struct Dusman { float x, y, vurus, adim; int can; bool olu, patron; };
struct Esya { float x, y; char tur; bool alindi; };
uint8_t m[N][N];
Dusman dus[12]; int dn;
Esya esy[10]; int en;
Mermi mr[8];
char ozel[N][N + 1];                                  // düzenleyicideki bölüm
int silah, hal, imlec, edX, edY; bool sahip[3]; float atesBekle; uint32_t sonSagTik;   // hal 0 menü, 1 oyun, 2 düzenleyici, 3 düzenleyici menüsü
float px, py, aci, sure, flas, hasarFlas, mesajBitis, yurumeAdim;
int can, mermi, bolum, olduren; bool anahtar, bitti, kazandi;
String mesaj;
float zb[64]; int duvarAlt[64];

void soyle(const String &s) { mesaj = s; mesajBitis = sure + 1.5f; }
char hucre(int b, int x, int y) { return b == 3 ? ozel[y][x] : HARITA[b][y][x]; }
void yukle(int b) {
  bolum = b; dn = en = 0; anahtar = false; for (auto &k : mr) k.var = false; px = py = 1.5f; aci = 0;
  for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
    char c = hucre(b, x, y); if (x == 0 || y == 0 || x == N - 1 || y == N - 1) c = '#'; float cx = x + .5f, cy = y + .5f;
    m[y][x] = c == '#' ? DUVAR : c == 'D' ? KAPI : c == 'L' ? KILIT : c == 'E' ? CIKIS : ZEMIN;
    if (c == 'P') { px = cx; py = cy; aci = 0; }
    if (c == 'Z' && dn < 12) dus[dn++] = {cx, cy, 0, 0, 2 + min(b, 2), false, false};
    if (c == 'X' && dn < 12) dus[dn++] = {cx, cy, 0, 0, 30, false, true};
    if ((c == 'H' || c == 'A' || c == 'K' || c == 'S' || c == 'R') && en < 10) esy[en++] = {cx, cy, c, false};
  }
  soyle(b == 3 ? "Ozel bolum" : "Bolum " + String(b + 1));
}
bool gecilir(int x, int y) { return x >= 0 && y >= 0 && x < N && y < N && (m[y][x] == ZEMIN || m[y][x] == CIKIS); }
void kapiDene(int x, int y) {
  if (m[y][x] == KAPI) m[y][x] = ZEMIN;
  else if (m[y][x] == KILIT) { if (anahtar) { m[y][x] = ZEMIN; soyle("Kilit acildi"); } else if (mesajBitis < sure) soyle("Anahtar lazim!"); }
}
void yuru(float dx, float dy) {                       // eksenler ayrı: duvar boyunca kayar, kapıya değince açar
  const float R = .25f;
  int tx = (int)(px + dx + (dx > 0 ? R : -R));
  if (gecilir(tx, (int)py)) px += dx; else kapiDene(tx, (int)py);
  int ty = (int)(py + dy + (dy > 0 ? R : -R));
  if (gecilir((int)px, ty)) py += dy; else kapiDene((int)px, ty);
}
bool gorur(float x, float y) {                       // düşmandan oyuncuya duvarsız çizgi var mı
  float dx = px - x, dy = py - y, d = sqrtf(dx * dx + dy * dy);
  for (float t = 0; t < d; t += .2f) if (!gecilir((int)(x + dx * t / d), (int)(y + dy * t / d))) return false;
  return true;
}
// sprite: dünya konumundan ekrana; duvarların arkasında kalan sütunlar çizilmez, siyah kenarla duvardan ayrılır
void sprite(const uint16_t *bmp, float sx, float sy, float dirX, float dirY, float plX, float plY, float olcek = 1) {
  float rx = sx - px, ry = sy - py, ters = 1 / (plX * dirY - dirX * plY);
  float tx = ters * (dirY * rx - dirX * ry), ty = ters * (-plY * rx + plX * ry);
  if (ty < .25f) return;
  int ekX = lroundf(64 * (1 + tx / ty)), s = min(120, (int)(50 * olcek / ty)), taban = 32 + (int)(28 / ty), ust = taban - s;
  for (int x = max(0, ekX - s / 2); x < min(128, ekX + s / 2); x++) {
    if (ty >= zb[x / 2]) continue;
    int u = (x - (ekX - s / 2)) * 16 / s;
    for (int y = max(0, ust); y < min(56, taban); y++) {
      int v = (y - ust) * 16 / s;
      auto pikselVar = [&](int uu, int vv) { return uu >= 0 && uu < 16 && vv >= 0 && vv < 16 && (bmp[vv] >> (15 - uu) & 1); };
      if (pikselVar(u, v)) ekran.drawPixel(x, y, W);
      else if (pikselVar(u - 1, v) || pikselVar(u + 1, v) || pikselVar(u, v - 1) || pikselVar(u, v + 1)) ekran.drawPixel(x, y, B);
    }
  }
}
void ciz(bool yuruyor) {
  float dirX = cosf(aci), dirY = sinf(aci), plX = -dirY * .66f, plY = dirX * .66f;
  for (int c = 0; c < 64; c++) {                     // duvarlar: 2 piksellik sütunlar, tuğla dokusu
    float kx = 2 * c / 64.0f - 1, rx = dirX + plX * kx, ry = dirY + plY * kx;
    int mx = px, my = py, adx = rx < 0 ? -1 : 1, ady = ry < 0 ? -1 : 1, yan = 0;
    float ddx = fabsf(1 / rx), ddy = fabsf(1 / ry), sx = (rx < 0 ? px - mx : mx + 1 - px) * ddx, sy = (ry < 0 ? py - my : my + 1 - py) * ddy;
    uint8_t tur = ZEMIN;
    for (int k = 0; k < 40 && tur == ZEMIN; k++) { if (sx < sy) { sx += ddx; mx += adx; yan = 0; } else { sy += ddy; my += ady; yan = 1; } tur = gecilir(mx, my) ? ZEMIN : m[my & 15][mx & 15]; }
    float d = yan ? sy - ddy : sx - ddx; zb[c] = d;
    int h = min(200, (int)(56 / max(d, .05f))), y0 = 32 - h / 2, x = c * 2;
    duvarAlt[c] = min(64, y0 + h);
    float vurX = yan ? px + d * rx : py + d * ry; vurX -= floorf(vurX);
    float uf = vurX * 16, kolon = max(.5f, .33f * d), satir = 16.0f / h;   // bir ekran sütunu / satırı kaç doku pikseli: çizgiler hep 1 piksel kalsın
    for (int y = max(0, y0); y < min(64, y0 + h); y++) {
      float vf = (y - y0) * satir; bool p;
      if (tur == KAPI || tur == KILIT) p = uf < kolon || uf > 16 - kolon || vf < satir || fabsf(uf - 8) < kolon || (tur == KILIT && fabsf(vf - 8) < satir * 1.5f && uf > 10 && uf < 13);
      else {
        int sira = vf / 4;
        bool harc = fmodf(vf, 4) < satir || fmodf(uf + (sira % 2) * 4, 8) < kolon;   // tuğla: yatay harç + kaydırmalı dikey derz
        p = harc || (yan && (y % 3 == 0) && (c % 2 == 0));                            // bir eksen gölgeli: köşeler belli olur
      }
      if (p) { ekran.drawPixel(x, y, W); ekran.drawPixel(x + 1, y, W); }
    }
  }
  for (int y = 33; y < 56; y++) {                    // zemin: oyuncuyla birlikte kayan karo ızgarası
    float uz = 28.0f / (y - 32); if (uz > 6) continue;
    float fx = px + uz * (dirX - plX), fy = py + uz * (dirY - plY), adX = uz * 2 * plX / 128, adY = uz * 2 * plY / 128;
    for (int x = 0; x < 128; x++, fx += adX, fy += adY) {
      if (y < duvarAlt[x / 2]) continue;
      float a = fx - floorf(fx), b2 = fy - floorf(fy);
      if (a < .07f || b2 < .07f) ekran.drawPixel(x, y, W);
      if (m[(int)fy & 15][(int)fx & 15] == CIKIS && (x + y) % 2) ekran.drawPixel(x, y, W);   // çıkış karosu parlak
    }
  }
  int sira[30], sn = 0; float uzak[30];              // sprite'lar uzaktan yakına
  for (int i = 0; i < dn; i++) { sira[sn] = i; uzak[sn++] = (dus[i].x - px) * (dus[i].x - px) + (dus[i].y - py) * (dus[i].y - py); }
  for (int i = 0; i < en; i++) if (!esy[i].alindi) { sira[sn] = 100 + i; uzak[sn++] = (esy[i].x - px) * (esy[i].x - px) + (esy[i].y - py) * (esy[i].y - py); }
  for (int i = 0; i < 8; i++) if (mr[i].var && sn < 30) { sira[sn] = 200 + i; uzak[sn++] = (mr[i].x - px) * (mr[i].x - px) + (mr[i].y - py) * (mr[i].y - py); }
  for (int i = 0; i < sn; i++) for (int j = i + 1; j < sn; j++) if (uzak[j] > uzak[i]) { std::swap(uzak[i], uzak[j]); std::swap(sira[i], sira[j]); }
  for (int i = 0; i < sn; i++) {
    if (sira[i] >= 200) { Mermi &k = mr[sira[i] - 200]; sprite(ATES_S, k.x, k.y, dirX, dirY, plX, plY, .8f); }
    else if (sira[i] >= 100) { Esya &e = esy[sira[i] - 100]; sprite(e.tur == 'H' ? SAGLIK : e.tur == 'A' ? MERMI : e.tur == 'S' ? POMPALI_S : e.tur == 'R' ? ROKET_S : ANAHTAR, e.x, e.y, dirX, dirY, plX, plY); }
    else { Dusman &z = dus[sira[i]]; sprite(z.olu ? OLU : z.patron ? PATRON_S : ZOMBI[(int)(z.adim * 3) % 2], z.x, z.y, dirX, dirY, plX, plY, z.patron && !z.olu ? 1.6f : 1); }
  }
  int sal = yuruyor ? lroundf(sinf(yurumeAdim * 9) * 2) : 0, yk = yuruyor ? lroundf(fabsf(cosf(yurumeAdim * 9)) * 2) : 0;   // silah: yürürken sallanır
  if (flas > 0) { ekran.fillTriangle(64 + sal, 30, 58 + sal, 42, 70 + sal, 42, W); ekran.fillTriangle(56 + sal, 36, 72 + sal, 36, 64 + sal, 46, W); }
  if (silah == 0) { ekran.fillRect(59 + sal, 40 + yk, 10, 24, B); ekran.fillRect(60 + sal, 41 + yk, 8, 23, W); ekran.fillRect(63 + sal, 41 + yk, 2, 6, B); }
  else if (silah == 1) { ekran.fillRect(55 + sal, 38 + yk, 18, 26, B); ekran.fillRect(56 + sal, 39 + yk, 7, 25, W); ekran.fillRect(65 + sal, 39 + yk, 7, 25, W); ekran.fillRect(58 + sal, 39 + yk, 2, 5, B); ekran.fillRect(68 + sal, 39 + yk, 2, 5, B); }
  else { ekran.fillRect(52 + sal, 34 + yk, 24, 30, B); ekran.drawRoundRect(53 + sal, 35 + yk, 22, 29, 4, W); ekran.fillCircle(64 + sal, 40 + yk, 6, W); ekran.fillCircle(64 + sal, 40 + yk, 3, B); }
  ekran.fillRoundRect(53 + sal, 52 + yk, 22, 14, 3, B); ekran.drawRoundRect(53 + sal, 52 + yk, 22, 14, 3, W); ekran.drawFastHLine(56 + sal, 57 + yk, 16, W);
  ekran.fillRect(0, 56, 40, 8, B); ekran.fillRect(96, 56, 32, 8, B);
  ekran.fillRect(1, 58, 5, 1, W); ekran.fillRect(3, 56, 1, 5, W); yaz(8, 57, 1, String(can));   // can: artı işareti
  if (anahtar) { ekran.drawCircle(31, 59, 2, W); ekran.drawFastHLine(33, 59, 5, W); }
  ekran.fillRect(98, 57, 2, 5, W); ekran.fillRect(101, 57, 2, 5, W); yaz(106, 57, 1, String(mermi));
  for (int i = 0; i < 3; i++) if (sahip[i]) { if (i == silah) ekran.fillRect(82 + i * 4, 58, 3, 4, W); else ekran.drawRect(82 + i * 4, 58, 3, 4, W); }
  if (hasarFlas > 0) { ekran.drawRect(0, 0, 128, 64, W); ekran.drawRect(1, 1, 126, 62, W); }
  if (sure < mesajBitis) { int w = mesaj.length() * 6 + 6; ekran.fillRect(64 - w / 2, 2, w, 11, B); ekran.drawRect(64 - w / 2, 2, w, 11, W); ortala(4, 1, mesaj); }
}

void patlat(float x, float y) {                      // roket: 1.5 karelik alanda 4 hasar
  for (int i = 0; i < dn; i++) if (!dus[i].olu) { float dx = dus[i].x - x, dy = dus[i].y - y; if (dx * dx + dy * dy < 2.25f && (dus[i].can -= 4) <= 0) { dus[i].olu = true; olduren++; } }
  float dx = px - x, dy = py - y; if (dx * dx + dy * dy < 1.2f) { can -= 15; hasarFlas = .3f; }
  flas = .2f;
}
void ates() {
  if (atesBekle > 0) return;
  if (mermi < SILAH_MERMI[silah]) { soyle("Mermi yetmiyor!"); return; }
  mermi -= SILAH_MERMI[silah]; atesBekle = SILAH_BEKLE[silah]; flas = .1f;
  float dirX = cosf(aci), dirY = sinf(aci);
  if (silah == 2) { for (auto &k : mr) if (!k.var) { k = {px + dirX * .4f, py + dirY * .4f, dirX * 7, dirY * 7, false, true}; break; } return; }
  float plX = -dirY * .66f, plY = dirX * .66f, ters = 1 / (plX * dirY - dirX * plY);
  int vurulan = 0;
  for (int tur = 0; tur < (silah == 1 ? 2 : 1); tur++) {   // pompalı: geniş açı, iki düşmana kadar, 3 hasar
    float enYakin = 99; int hedef = -1;
    for (int i = 0; i < dn; i++) if (!dus[i].olu) {
      float rx = dus[i].x - px, ry = dus[i].y - py, tx = ters * (dirY * rx - dirX * ry), ty = ters * (-plY * rx + plX * ry);
      if (ty < .25f || ty > zb[32] + .5f || ty > enYakin || (tur && i == vurulan - 1)) continue;
      if (fabsf(64 * tx / ty) < max(5.0f, (silah == 1 ? 34 : 18) / ty)) { enYakin = ty; hedef = i; }
    }
    if (hedef < 0) break;
    vurulan = hedef + 1;
    if ((dus[hedef].can -= silah == 1 ? 3 : 1) <= 0) { dus[hedef].olu = true; olduren++; if (dus[hedef].patron) soyle("PATRON YENILDI!"); }
  }
}
bool patronCanli() { for (int i = 0; i < dn; i++) if (dus[i].patron && !dus[i].olu) return true; return false; }
void ozelYukle() {
  if (kayit.getBytes("doom_oz", ozel, sizeof(ozel)) != sizeof(ozel)) {
    for (int y = 0; y < N; y++) { for (int x = 0; x < N; x++) ozel[y][x] = (x == 0 || y == 0 || x == N - 1 || y == N - 1) ? '#' : '.'; ozel[y][N] = 0; }
    ozel[1][1] = 'P'; ozel[14][14] = 'E';
  }
}
const char KAROLAR[] = ".#DZHAKLESRXP";
const char *KARO_AD[] = {"bos", "duvar", "kapi", "zombi", "can", "mermi", "anahtar", "kilitli kapi", "cikis", "pompali", "roketatar", "patron", "baslangic"};
void duzenleyici(Girdi &g) {                         // sağ tık: imleç ilerler (basılı tut: hızlı), sol tık: karo değiştir, iki tuş: menü
  static float tekrar = 0;
  if (g.o1 == BASTI) { tekrar = .35f; if (++edX >= N - 1) { edX = 1; if (++edY >= N - 1) edY = 1; } }
  else if (g.b1 && !g.b2 && (tekrar -= g.dt) <= 0) { tekrar = .07f; if (++edX >= N - 1) { edX = 1; if (++edY >= N - 1) edY = 1; } }
  if (g.o2 == KISA) { char &c = ozel[edY][edX]; const char *k = strchr(KAROLAR, c); int i = k ? (k - KAROLAR + 1) % (int)strlen(KAROLAR) : 0;
    if (KAROLAR[i] == 'P') for (auto &r : ozel) for (int x = 0; x < N; x++) if (r[x] == 'P') r[x] = '.';   // tek başlangıç
    c = KAROLAR[i]; }
  if (g.combo) { hal = 3; imlec = 0; return; }
  for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {   // 4x4 piksellik kuşbakışı harita
    char c = ozel[y][x]; int sx = x * 4, sy = y * 4;
    if (c == '#') ekran.fillRect(sx, sy, 4, 4, W);
    else if (c == 'D' || c == 'L') { ekran.drawRect(sx, sy, 4, 4, W); if (c == 'L') ekran.drawPixel(sx + 1, sy + 1, W); }
    else if (c == 'Z' || c == 'X') { ekran.fillRect(sx + 1, sy + 1, 2, 2, W); if (c == 'X') ekran.drawRect(sx, sy, 4, 4, W); }
    else if (c == 'P') { ekran.drawLine(sx, sy + 3, sx + 3, sy, W); ekran.drawLine(sx, sy, sx + 3, sy + 3, W); }
    else if (c == 'E') { ekran.drawPixel(sx, sy, W); ekran.drawPixel(sx + 3, sy, W); ekran.drawPixel(sx, sy + 3, W); ekran.drawPixel(sx + 3, sy + 3, W); }
    else if (c != '.') ekran.drawPixel(sx + 1, sy + 1, W);
  }
  if ((millis() / 250) % 2) ekran.drawRect(edX * 4 - 1, edY * 4 - 1, 6, 6, W);
  yaz(68, 0, 1, "DUZENLE"); yaz(68, 12, 1, String(edX) + "," + String(edY));
  const char *k = strchr(KAROLAR, ozel[edY][edX]); yaz(68, 24, 1, KARO_AD[k ? k - KAROLAR : 0]);
  yaz(68, 40, 1, "sag: ilerle"); yaz(68, 48, 1, "sol: degis"); yaz(68, 56, 1, "2 tus: menu");
}

bool oyna(Girdi &g) {
  if (g.ilk) { hal = 0; imlec = 0; ozelYukle(); }
  if (hal == 0 || hal == 3) {                        // başlangıç menüsü ya da düzenleyici menüsü
    const char *ana[3] = {"Kampanya", "Ozel bolum", "Duzenleyici"}, *ed[4] = {"Oyna", "Kaydet", "Temizle", "Geri"};
    int n = hal ? 4 : 3;
    if (g.o1 == KISA) imlec = (imlec + 1) % n;
    if (g.o2 == KISA) {
      if (hal == 0 && imlec < 2) { can = 100; mermi = 25; olduren = 0; sure = 0; bitti = kazandi = false; flas = hasarFlas = 0; silah = 0; sahip[0] = true; sahip[1] = sahip[2] = false; yukle(imlec ? 3 : 0); hal = 1; return true; }
      if (hal == 0) { hal = 2; edX = edY = 1; return true; }
      if (imlec == 0) { can = 100; mermi = 40; olduren = 0; sure = 0; bitti = kazandi = false; silah = 0; sahip[0] = true; sahip[1] = sahip[2] = false; yukle(3); hal = 1; return true; }
      if (imlec == 1) { kayit.putBytes("doom_oz", ozel, sizeof(ozel)); soyle("Kaydedildi"); }
      if (imlec == 2) { kayit.remove("doom_oz"); ozelYukle(); hal = 2; return true; }
      if (imlec == 3) { hal = 0; imlec = 2; return true; }
    }
    sure += g.dt;
    ortala(2, 2, "KUBIT DOOM");
    for (int i = 0; i < n; i++) { int y = 22 + i * 10; if (i == imlec) { ekran.fillRect(24, y - 1, 80, 9, W); ekran.setTextColor(B); } ortala(y, 1, hal ? ed[i] : ana[i]); ekran.setTextColor(W); }
    if (sure < mesajBitis) ortala(56, 1, mesaj);
    return true;
  }
  if (hal == 2) { duzenleyici(g); return true; }
  if (bitti) {
    int ds = lroundf(sure * 10);
    int b = kazandi ? bitis(g, "KAZANDIN! " + String(ds / 10) + " sn", "Rekor " + String(rekorKaydet("doom", ds, true) / 10) + " sn")
                    : bitis(g, "Oldun. Bolum " + String(bolum + 1), String(olduren) + " zombi");
    if (b == 1) { bitti = false; hal = 0; return true; } return b != 2;
  }
  sure += g.dt; flas -= g.dt; hasarFlas -= g.dt; atesBekle -= g.dt;
  if (g.o1 == KISA) {                                 // sağ tuşa çift tık: sıradaki silah
    if (millis() - sonSagTik < 320) { do silah = (silah + 1) % 3; while (!sahip[silah]); soyle(SILAH_AD[silah]); sonSagTik = 0; }
    else sonSagTik = millis();
  }
  bool ikisi = g.b1 && g.b2;
  if (ikisi && g.ikiSure > 120) { yuru(cosf(aci) * 2.3f * g.dt, sinf(aci) * 2.3f * g.dt); yurumeAdim += g.dt; }
  else if (!ikisi) { if (g.b2) aci -= 2.4f * g.dt; if (g.b1) aci += 2.4f * g.dt; }
  if (g.combo && g.comboSure < 300) ates();
  for (auto &k : mr) if (k.var) {                     // roketler ve patronun ateş topları
    k.x += k.vx * g.dt; k.y += k.vy * g.dt;
    if (!gecilir((int)k.x, (int)k.y)) { k.var = false; if (!k.dusman) patlat(k.x - k.vx * .03f, k.y - k.vy * .03f); continue; }
    if (k.dusman) { if ((k.x - px) * (k.x - px) + (k.y - py) * (k.y - py) < .16f) { k.var = false; can -= 15; hasarFlas = .3f; } }
    else for (int i = 0; i < dn; i++) if (!dus[i].olu && (k.x - dus[i].x) * (k.x - dus[i].x) + (k.y - dus[i].y) * (k.y - dus[i].y) < (dus[i].patron ? .5f : .2f)) { k.var = false; patlat(k.x, k.y); break; }
  }
  if (can <= 0) { can = 0; bitti = true; }
  for (int i = 0; i < dn; i++) {                     // zombiler: görünce kovalar, yakındaysa vurur
    Dusman &z = dus[i]; if (z.olu) continue;
    float dx = px - z.x, dy = py - z.y, d = sqrtf(dx * dx + dy * dy);
    z.vurus -= g.dt;
    if (d < 9 && gorur(z.x, z.y)) {
      if (z.patron && z.vurus <= 0 && d > 1.5f) { z.vurus = 2; for (auto &k : mr) if (!k.var) { k = {z.x, z.y, dx / d * 4, dy / d * 4, true, true}; break; } }   // patron: ateş topu
      if (d > .8f) { float h = (z.patron ? .8f : 1 + min(bolum, 2) * .25f) * g.dt, nx = z.x + dx / d * h, ny = z.y + dy / d * h; if (gecilir((int)nx, (int)z.y)) z.x = nx; if (gecilir((int)z.x, (int)ny)) z.y = ny; z.adim += g.dt; }
      else if (z.vurus <= 0) { z.vurus = 1.1f; can -= random(6, 14); hasarFlas = .25f; if (can <= 0) { can = 0; bitti = true; } }
    }
  }
  for (int i = 0; i < en; i++) {
    Esya &e = esy[i]; if (e.alindi || (e.x - px) * (e.x - px) + (e.y - py) * (e.y - py) > .3f) continue;
    if (e.tur == 'H') { if (can >= 100) continue; can = min(100, can + 30); soyle("+30 can"); }
    if (e.tur == 'A') { mermi += 12; soyle("+12 mermi"); }
    if (e.tur == 'K') { anahtar = true; soyle("Anahtar!"); }
    if (e.tur == 'S' || e.tur == 'R') { int s2 = e.tur == 'S' ? 1 : 2; sahip[s2] = true; silah = s2; mermi += 10; soyle(String(SILAH_AD[s2]) + "! (sag cift tik)"); }
    e.alindi = true;
  }
  if (m[(int)py][(int)px] == CIKIS) {
    if (patronCanli()) { if (mesajBitis < sure) soyle("Once patronu yen!"); }
    else if (bolum < 2) yukle(bolum + 1); else { kazandi = bitti = true; }
  }
  ciz(ikisi && g.ikiSure > 120);
  return true;
}
}  // namespace doom
bool kubitDoom(Girdi &g) { return doom::oyna(g); }
