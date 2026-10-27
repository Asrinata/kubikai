// Keşif: uçak radarı (Flightradar gibi) ve uzay (güneş sistemi + ISS). Uçak ve ISS verisini telefon getirir (adsb.lol, adsbdb,
// wheretheiss.at); gezegenlerin konumu Kübit'te hesaplanır (JPL'nin 1800-2050 yaklaşık yörünge öğeleri), internet gerekmez.
// Telefona: RADAR:<km>, ROTA:<çağrı>|<hex>, UZAY:ISS, UZAY:ISSYOL.  Telefondan:
//   U<adet>|<sıra>|<çağrı>|<tip>|<irtifa m>|<hız km/sa>|<yön>|<doğu 0.1km>|<kuzey 0.1km>|<tescil>|<hex>|<dikey m/dk>
//   V<çağrı>|<kalkış IATA>|<kalkış şehri>|<varış IATA>|<varış şehri>|<şirket>|<model>
//   I<enlem>|<boylam>|<irtifa km>|<hız km/sa>|<güneşte 1/0>|<senin enlem>|<senin boylam>     J<enlem>,<boylam>;...  (ISS'in önümüzdeki 90 dk'sı)
#pragma once
#include "dunya.h"

// ---------------------------------------------------------------- uçak radarı
struct Ucak { String cs, tip, tescil, hex; int irtifa, hiz, yon, dikey; float x, y; };
std::vector<Ucak> ucaklar, gelenUcak;
struct { String cs, kalkis, kalkisSehir, varis, varisSehir, sirket, model; } rota;
int radarSec = -1, radarMenzil = 1, radarBilgi = 0; bool radarDetay = false;
String radarSecCs, rotaSorgu; uint32_t radarZaman = 0, radarIstek = 0, rotaSonIstek = 0;
const int MENZILLER[] = {25, 50, 100};
String ucakKimlik(const Ucak &u) { return u.hex.length() ? u.hex : u.cs; }
void ucakSatiri(const String &g) {
  auto p = parcala(g, '|'); int adet = p[0].toInt();
  if (adet == 0) { ucaklar.clear(); gelenUcak.clear(); radarSec = -1; radarSecCs = ""; radarDetay = false; radarZaman = millis(); return; }
  if (p.size() < 12 || adet < 0 || adet > 64) return;
  int sira = p[1].toInt(); if (sira == 0) gelenUcak.clear();
  if (sira < 0 || sira >= adet || sira != (int)gelenUcak.size()) return;
  gelenUcak.push_back({ascii(p[2]), ascii(p[3]), ascii(p[9]), p[10], (int)p[4].toInt(), (int)p[5].toInt(), (int)p[6].toInt(), (int)p[11].toInt(), p[7].toInt() / 10.0f, p[8].toInt() / 10.0f});
  if (sira == adet - 1) {
    ucaklar = gelenUcak; radarZaman = millis(); radarSec = -1;
    for (int i = 0; i < (int)ucaklar.size(); i++) if (ucakKimlik(ucaklar[i]) == radarSecCs) radarSec = i;
    if (radarDetay && radarSec < 0) { radarDetay = false; radarSecCs = ""; }
  }
}
void rotaSatiri(const String &g) {
  auto p = parcala(g, '|'); if (p.size() < 7) return;
  rota.cs = p[0]; rota.kalkis = ascii(p[1]); rota.kalkisSehir = ascii(p[2]); rota.varis = ascii(p[3]); rota.varisSehir = ascii(p[4]); rota.sirket = ascii(p[5]); rota.model = ascii(p[6]);
}
void rotaIste() {
  if (radarSec < 0 || radarSec >= (int)ucaklar.size() || !bagli) return;
  Ucak &u = ucaklar[radarSec];
  if (rota.cs != u.cs && (rotaSorgu != ucakKimlik(u) || millis() - rotaSonIstek > 15000)) {
    rotaSorgu = ucakKimlik(u); rotaSonIstek = millis(); telefonaGonder("ROTA:" + u.cs + "|" + u.hex, nullptr);
  }
}
void radarSecim(int yeni) {
  int n = ucaklar.size(); radarSec = yeni;
  if (radarSec >= n) radarSec = -1; if (radarSec < -1) radarSec = n - 1;
  radarSecCs = radarSec >= 0 ? ucakKimlik(ucaklar[radarSec]) : String(""); rotaIste();
}
void imlecOk(int x, int y) {
  static const char *OK[] = {"2       ", "22      ", "212     ", "2112    ", "21112   ", "211112  ", "2111112 ", "2112222 ", "212     ", "22      "};
  for (int j = 0; j < 10; j++) for (int i = 0; i < 8; i++) { char c = OK[j][i]; if (c != ' ') ekran.drawPixel(x + i, y + j, c == '1' ? W : B); }
}
String uzakYaz(float km) { return km < 10 ? String(km, 1) + "km" : String(lround(km)) + "km"; }
void kesifUst(const String &ad, const String &durum) {
  ekran.fillRect(0, 0, 128, 9, W); ekran.setTextColor(B);
  yaz(1, 1, 1, ad.substring(0, 12)); yaz(128 - min(8, (int)durum.length()) * 6, 1, 1, durum.substring(0, 8)); ekran.setTextColor(W);
}
void kesifAlt(const String &s) { ekran.fillRect(0, 55, 128, 9, B); ekran.drawFastHLine(0, 54, 128, W); yaz(1, 56, 1, s.substring(0, 21)); }
void ucakSimge(int x, int y, int yon, bool secili) {
  float a = yon * DEG_TO_RAD, dx = sin(a), dy = -cos(a);
  ekran.drawLine(x - dx * 3, y - dy * 3, x + dx * 4, y + dy * 4, W);
  ekran.drawLine(x - dy * 3, y + dx * 3, x + dy * 3, y - dx * 3, W);
  if (secili) ekran.drawRect(x - 5, y - 5, 11, 11, W);
}
void radarSayfasi(Olay o1, Olay o2) {
  static uint32_t son = 0; uint32_t t = millis();
  float dt = son && t - son < 500 ? (t - son) / 1000.0f : 0; son = t;
  int km = MENZILLER[radarMenzil], n = ucaklar.size(); bool eski = radarZaman && t - radarZaman > 25000;
  if (bagli && (t - radarIstek > 8000 || !radarIstek)) { radarIstek = t; telefonaGonder("RADAR:" + String(km), nullptr); }
  if (radarZaman && t - radarZaman < 16000) for (auto &u : ucaklar) { float yol = u.hiz / 3600.0f * dt; u.x += yol * sin(u.yon * DEG_TO_RAD); u.y += yol * cos(u.yon * DEG_TO_RAD); }
  if (radarSec >= n) radarSecim(-1);
  if (radarDetay && radarSec >= 0) {
    if (o1 == KISA || geriAdim) radarBilgi = 1 - radarBilgi;
    if (o2 == KISA) { radarDetay = false; return; }
    rotaIste(); Ucak &u = ucaklar[radarSec]; bool rv = rota.cs == u.cs;
    kesifUst(u.cs.length() ? u.cs : u.tescil, String(radarBilgi + 1) + "/2");
    if (!radarBilgi) {
      String kalk = rv && rota.kalkis.length() ? rota.kalkis.substring(0, 3) : String("---");
      String var = rv && rota.varis.length() ? rota.varis.substring(0, 3) : String("---");
      yaz(1, 13, 2, kalk); yaz(46, 17, 1, "-->"); yaz(84, 13, 2, var);
      yaz(1, 31, 1, rv ? rota.sirket.substring(0, 21) : String(bagli ? dl("Rota araniyor...", "Finding route...") : dl("Telefon bagli degil", "Phone disconnected")));
      yaz(1, 43, 1, rv ? (rota.kalkisSehir.substring(0, 9) + ">" + rota.varisSehir.substring(0, 10)) : u.tescil.substring(0, 21));
    } else {
      yaz(1, 12, 1, dl("IRTIFA", "ALTITUDE")); yaz(80, 12, 1, "km/h");
      ekran.drawFastVLine(76, 12, 29, W);
      yaz(1, 23, 2, u.irtifa <= 0 ? String("0m") : String(u.irtifa) + "m"); yaz(80, 23, 2, String(u.hiz));
      yaz(1, 43, 1, u.tip.substring(0, 5) + " " + uzakYaz(sqrt(u.x * u.x + u.y * u.y)) + " " + (u.dikey > 150 ? "+" : u.dikey < -150 ? "-" : "=") + String(abs(u.dikey)));
    }
    kesifAlt(dl("R/L:bilgi 2t:harita", "R/L:info both:map")); return;
  }
  if (o1 == KISA) radarSecim(radarSec + 1); if (geriAdim) radarSecim(radarSec - 1);
  if (o2 == KISA) {
    if (radarSec < 0) { radarMenzil = (radarMenzil + 1) % 3; radarIstek = 0; km = MENZILLER[radarMenzil]; }
    else { radarDetay = true; radarBilgi = 0; rotaIste(); return; }
  }
  kesifUst(dl("UCAK RADARI", "FLIGHT RADAR"), !bagli ? dl("KOPUK", "OFFLINE") : eski ? dl("ESKI", "STALE") : String(n) + dl(" ucak", " planes"));
  const int CX = 33, CY = 32, R = 20;
  ekran.drawCircle(CX, CY, R, W);
  for (int a = 0; a < 360; a += 30) ekran.drawPixel(CX + R / 2 * cos(a * DEG_TO_RAD), CY + R / 2 * sin(a * DEG_TO_RAD), W);
  ekran.drawFastHLine(CX - 2, CY, 5, W); ekran.drawFastVLine(CX, CY - 2, 5, W);
  yaz(1, 12, 1, dil ? "N" : "K"); yaz(1, 44, 1, String(km));
  for (int i = 0; i < n; i++) {
    Ucak &u = ucaklar[i]; float mes = sqrt(u.x * u.x + u.y * u.y); if (mes > km) continue;
    ucakSimge(lround(CX + u.x / km * (R - 5)), lround(CY - u.y / km * (R - 5)), u.yon, i == radarSec);
  }
  ekran.drawFastVLine(65, 11, 42, W);
  if (radarSec >= 0) {
    Ucak &u = ucaklar[radarSec]; rotaIste();
    yaz(69, 12, 1, (u.cs.length() ? u.cs : u.tescil).substring(0, 9));
    yaz(69, 23, 1, String(u.irtifa) + "m"); yaz(69, 34, 1, String(u.hiz) + "km/h");
    yaz(69, 45, 1, uzakYaz(sqrt(u.x * u.x + u.y * u.y)));
  } else {
    yaz(69, 13, 2, String(km)); yaz(105, 18, 1, "km");
    yaz(69, 34, 1, !bagli ? dl("Telefonu", "Connect") : !radarZaman ? dl("Veri", "Awaiting") : n ? dl("Ucak sec", "Select") : dl("Ucak yok", "No planes"));
    yaz(69, 45, 1, !bagli ? dl("bagla", "phone") : !radarZaman ? dl("bekleniyor", "data") : dl("2t:menzil", "both:zoom"));
  }
  kesifAlt(radarSec < 0 ? dl("R/L:ucak 2t:menzil", "R/L:plane both:zoom") : dl("R/L:ucak 2t:detay", "R/L:plane both:info"));
}

// ---------------------------------------------------------------- uzay: güneş sistemi (hesaplanır) ve ISS (telefondan)
const char *GEZEGEN_AD[] = {"Gunes", "Merkur", "Venus", "Dunya", "Mars", "Jupiter", "Saturn", "Uranus", "Neptun"};
// a (AU), e, I, L, uzun.günberi, düğüm (derece) ve yüzyıllık değişimleri: JPL "Approximate Positions of the Planets", 1800-2050
const float YORUNGE[8][12] = {
  {0.38709927, 0.20563593, 7.00497902, 252.25032350, 77.45779628, 48.33076593, 0.00000037, 0.00001906, -0.00594749, 149472.67411175, 0.16047689, -0.12534081},
  {0.72333566, 0.00677672, 3.39467605, 181.97909950, 131.60246718, 76.67984255, 0.00000390, -0.00004107, -0.00078890, 58517.81538729, 0.00268329, -0.27769418},
  {1.00000261, 0.01671123, -0.00001531, 100.46457166, 102.93768193, 0.0, 0.00000562, -0.00004392, -0.01294668, 35999.37244981, 0.32327364, 0.0},
  {1.52371034, 0.09339410, 1.84969142, -4.55343205, -23.94362959, 49.55953891, 0.00001847, 0.00007882, -0.00813131, 19140.30268499, 0.44441088, -0.29257343},
  {5.20288700, 0.04838624, 1.30439695, 34.39644051, 14.72847983, 100.47390909, -0.00011607, -0.00013253, -0.00183714, 3034.74612775, 0.21252668, 0.20469106},
  {9.53667594, 0.05386179, 2.48599187, 49.95424423, 92.59887831, 113.66242448, -0.00125060, -0.00050991, 0.00193609, 1222.49362201, -0.41897216, -0.28867794},
  {19.18916464, 0.04725744, 0.77263783, 313.23810451, 170.95427630, 74.01692503, -0.00196176, -0.00004397, -0.00242939, 428.48202785, 0.40805281, 0.04240589},
  {30.06992276, 0.00859048, 1.77004347, -55.12002969, 44.96476227, 131.78422574, 0.00026291, 0.00005105, 0.00035372, 218.45945325, -0.32241464, -0.00508664}};
void gezegenKonum(int i, double T, float &x, float &y, float &z) {   // T: J2000'den yüzyıl; sonuç güneş merkezli ekliptik (AU)
  const float *k = YORUNGE[i];
  double a = k[0] + k[6] * T, e = k[1] + k[7] * T, I = (k[2] + k[8] * T) * DEG_TO_RAD, L = k[3] + k[9] * T, w = k[4] + k[10] * T, O = (k[5] + k[11] * T) * DEG_TO_RAD;
  double M = fmod(L - w, 360.0); if (M > 180) M -= 360; if (M < -180) M += 360; M *= DEG_TO_RAD;
  double om = w * DEG_TO_RAD - O, E = M + e * sin(M);
  for (int j = 0; j < 6; j++) E -= (E - e * sin(E) - M) / (1 - e * cos(E));   // Kepler denklemi (Newton)
  double xp = a * (cos(E) - e), yp = a * sqrt(1 - e * e) * sin(E);
  x = (cos(om) * cos(O) - sin(om) * sin(O) * cos(I)) * xp + (-sin(om) * cos(O) - cos(om) * sin(O) * cos(I)) * yp;
  y = (cos(om) * sin(O) + sin(om) * cos(O) * cos(I)) * xp + (-sin(om) * sin(O) + cos(om) * cos(O) * cos(I)) * yp;
  z = sin(om) * sin(I) * xp + cos(om) * sin(I) * yp;
}
struct { float lat = 0, lon = 0, irtifa = 0, hiz = 0, senLat = 0, senLon = 0; bool gunes = false, var = false, senVar = false; uint32_t zaman = 0; } iss;
std::vector<std::pair<float, float>> issYol, issIz;    // önümüzdeki 90 dk, geçmiş iz (en çok 40 nokta)
void issSatiri(const String &g) {
  auto p = parcala(g, '|'); if (p.size() < 5) return;
  float lat = p[0].toFloat(), lon = p[1].toFloat(), irtifa = p[2].toFloat(), hiz = p[3].toFloat();
  if (!isfinite(lat) || !isfinite(lon) || fabs(lat) > 90 || fabs(lon) > 180 || !isfinite(irtifa) || irtifa < 0 || !isfinite(hiz) || hiz < 0) return;
  iss.lat = lat; iss.lon = lon; iss.irtifa = irtifa; iss.hiz = hiz; iss.gunes = p[4] == "1";
  if (p.size() >= 7 && p[5].length() && p[6].length()) {
    float a = p[5].toFloat(), b = p[6].toFloat();
    if (isfinite(a) && isfinite(b) && fabs(a) <= 90 && fabs(b) <= 180) { iss.senLat = a; iss.senLon = b; iss.senVar = true; }
  }
  iss.var = true; iss.zaman = millis();
  issIz.push_back({iss.lat, iss.lon}); if (issIz.size() > 40) issIz.erase(issIz.begin());
}
void issYolSatiri(const String &g) {
  issYol.clear();
  for (auto &n : parcala(g, ';')) {
    if (issYol.size() >= 96) break;
    int c = n.indexOf(','); if (c <= 0) continue;
    float a = n.substring(0, c).toFloat(), b = n.substring(c + 1).toFloat();
    if (isfinite(a) && isfinite(b) && fabs(a) <= 90 && fabs(b) <= 180) issYol.push_back({a, b});
  }
}
int uzayMod = 0, uzaySec = 3;                          // mod 0: güneş sistemi, 1: ISS; seçili gök cismi (3: Dünya)
float kureMesafe(float la1, float lo1, float la2, float lo2) {   // km
  float a = sin((la2 - la1) * DEG_TO_RAD / 2), b = sin((lo2 - lo1) * DEG_TO_RAD / 2);
  return 2 * 6371 * asin(sqrt(constrain(a * a + cos(la1 * DEG_TO_RAD) * cos(la2 * DEG_TO_RAD) * b * b, 0.0f, 1.0f)));
}
void haritaNokta(float lat, float lon, int &x, int &y) { x = (int)((lon + 180) / 360 * 128) & 127; y = 11 + constrain((int)((90 - lat) / 180 * 32), 0, 31); }
void issIzCiz(const std::vector<std::pair<float, float>> &yol, bool nokta) {
  int ox = 0, oy = 0; bool once = false;
  for (auto &n : yol) {
    if (!isfinite(n.first) || !isfinite(n.second) || fabs(n.first) > 90 || fabs(n.second) > 180) { once = false; continue; }
    int x, y; haritaNokta(n.first, n.second, x, y);
    if (nokta) ekran.drawPixel(x, y, W);
    else if (once && abs(x - ox) < 64) ekran.drawLine(ox, oy, x, y, W); // tarih cizgisinde ekrani boydan boya gecme
    else ekran.drawPixel(x, y, W);
    ox = x; oy = y; once = true;
  }
}
void uzaySayfasi(Olay o1, Olay o2) {
  if (o2 == KISA) { uzayMod = !uzayMod; o1 = YOK; geriAdim = false; }
  if (uzayMod == 1) {
    static uint32_t istek = 0, yolIstek = 0; static int bilgi = 0;
    if (o1 == KISA) bilgi = (bilgi + 1) % 3; if (geriAdim) bilgi = (bilgi + 2) % 3;
    if (bagli && (!istek || millis() - istek > 5000)) { istek = millis(); telefonaGonder("UZAY:ISS", nullptr); }
    if (bagli && (!yolIstek || millis() - yolIstek > 120000)) { yolIstek = millis(); telefonaGonder("UZAY:ISSYOL", nullptr); }
    bool eski = iss.var && millis() - iss.zaman > 20000;
    String durum = !bagli ? String(dl("KOPUK", "OFFLINE")) : !iss.var ? String("...") : eski ? String(dl("ESKI", "STALE")) : String((millis() - iss.zaman) / 1000) + "sn";
    kesifUst("ISS  " + String(bilgi + 1) + "/3", durum);
    auto kara = [](int x, int y) { x &= 127; if (y < 0 || y > 63) return false; return (pgm_read_byte(&DUNYA[y * 16 + x / 8]) & (0x80 >> (x & 7))) != 0; };
    for (int y = 0; y < 32; y++) for (int x = 0; x < 128; x++) {
      int sy = y * 2; if (!kara(x, sy) && !kara(x, sy + 1)) continue;
      bool kiyi = !kara(x - 1, sy) || !kara(x + 1, sy) || !kara(x, sy - 2) || !kara(x, sy + 2);
      if (kiyi || (x % 4 == 0 && y % 3 == 0)) ekran.drawPixel(x, 11 + y, W);
    }
    issIzCiz(issYol, true); issIzCiz(issIz, false);
    if (iss.senVar) { int x, y; haritaNokta(iss.senLat, iss.senLon, x, y); ekran.drawRect(x - 1, y - 1, 3, 3, W); }
    if (iss.var) {
      int x, y; haritaNokta(iss.lat, iss.lon, x, y);
      auto nokta = [&](int dx, int dy, int renk) { if (y + dy >= 11 && y + dy <= 42) ekran.drawPixel((x + dx + 128) % 128, y + dy, renk); };
      for (int dx = -6; dx <= 6; dx++) for (int dy = -5; dy <= 5; dy++) nokta(dx, dy, B);
      for (int a = 0; a < 360; a += 12) nokta(lround(5 * cos(a * DEG_TO_RAD)), lround(5 * sin(a * DEG_TO_RAD)), W);
      for (int dx = -3; dx <= 3; dx++) nokta(dx, 0, W);
      for (int dy = -2; dy <= 2; dy++) { nokta(0, dy, W); nokta(-3, dy, W); nokta(3, dy, W); }
    }
    ekran.drawFastHLine(0, 44, 128, W);
    if (!iss.var) {
      yaz(0, 46, 1, bagli ? dl("ISS verisi bekleniyor", "Waiting for ISS data") : dl("Veri icin telefon bagla", "Connect phone for data"));
      yaz(0, 56, 1, dl("2t:Gunes sistemi", "both:solar system")); return;
    }
    if (bilgi == 0) { yaz(0, 46, 1, String(lround(iss.irtifa)) + "km " + String(lround(iss.hiz)) + (dil ? "km/h" : "km/sa")); yaz(0, 56, 1, iss.gunes ? dl("GUNESTE  2t:Gunes", "SUNLIT  both:solar") : dl("GOLGEDE  2t:Gunes", "IN SHADOW both:solar")); }
    else if (bilgi == 1) { yaz(0, 46, 1, String(fabs(iss.lat), 1) + (iss.lat < 0 ? "S " : "N ") + String(fabs(iss.lon), 1) + (iss.lon < 0 ? "W" : "E")); yaz(0, 56, 1, dl("Iz:cizgi Rota:nokta", "Trail:line path:dots")); }
    else { yaz(0, 46, 1, iss.senVar ? dl("Sana: ", "To you: ") + uzakYaz(kureMesafe(iss.lat, iss.lon, iss.senLat, iss.senLon)) : String(dl("Konum bilgisi yok", "No observer location"))); yaz(0, 56, 1, dl("sag/sol:bilgi 2t:Gunes", "R/L:info both:solar")); }
    return;
  }
  // ---- güneş sistemi (yukarıdan, ekliptik; halkalar eşit aralıklı, açılar gerçek)
  if (o1 == KISA) uzaySec = (uzaySec + 1) % 9;
  if (geriAdim) uzaySec = (uzaySec + 8) % 9;
  if (!saatVar()) { baslik(dl("GUNES SISTEMI", "SOLAR SYSTEM")); yaz(0, 20, 1, dl("saat yok: telefonu", "no clock: connect")); yaz(0, 30, 1, dl("bagla", "phone")); return; }
  double T = ((double)time(nullptr) / 86400.0 + 2440587.5 - 2451545.0) / 36525.0;
  float px[8], py[8], pz[8];
  for (int i = 0; i < 8; i++) gezegenKonum(i, T, px[i], py[i], pz[i]);
  const int CX = 32, CY = 32;
  ekran.fillCircle(CX, CY, 2, W);
  for (int a = 0; a < 8; a++) ekran.drawPixel(CX + 4 * cos(a * PI / 4), CY + 4 * sin(a * PI / 4), W);   // güneş ışınları
  for (int i = 0; i < 8; i++) {
    int r = 7 + i * 3 + (i > 3 ? 2 : 0);              // dış gezegenler arası biraz açık (asteroit kuşağı)
    for (int a = 0; a < 360; a += (i < 3 ? 30 : 15)) ekran.drawPixel(CX + r * cos(a * DEG_TO_RAD), CY + r * sin(a * DEG_TO_RAD), W);
    float t = atan2(py[i], px[i]); int x = lround(CX + r * cos(t)), y = lround(CY - r * sin(t));
    if (i >= 4 && i <= 5) ekran.fillCircle(x, y, 2, W); else ekran.fillCircle(x, y, 1, W);
    if (i == 5) ekran.drawFastHLine(x - 4, y, 9, W);   // Satürn'ün halkası
    if (i == 2) ekran.drawCircle(x, y, 3, W);          // Dünya: biz buradayız
    if (uzaySec == i + 1) imlecOk(x + 2, y + 2);
  }
  if (uzaySec == 0) imlecOk(CX + 2, CY + 2);
  int X = 68;
  ekran.drawFastVLine(X - 3, 0, 64, W);
  ekran.fillRect(X - 1, 0, 62, 10, W); ekran.setTextColor(B); yaz(X, 1, 1, GEZEGEN_AD[uzaySec]); ekran.setTextColor(W);
  float ex = px[2], ey = py[2], ez = pz[2];
  if (uzaySec == 0) { yaz(X, 13, 1, dl("Dunyaya", "to Earth")); yaz(X, 22, 1, String(sqrt(ex * ex + ey * ey), 3) + " AU"); yaz(X, 31, 1, String(sqrt(ex * ex + ey * ey) * 8.3167f, 1) + dl(" isik dk", " lt-min")); }
  else {
    int i = uzaySec - 1;
    float r = sqrt(px[i] * px[i] + py[i] * py[i] + pz[i] * pz[i]);
    yaz(X, 13, 1, String(dl("Gunes ", "Sun ")) + String(r, r < 10 ? 2 : 1) + "AU");
    if (i != 2) {
      float dx = px[i] - ex, dy = py[i] - ey, dz = pz[i] - ez, d = sqrt(dx * dx + dy * dy + dz * dz);
      yaz(X, 22, 1, String(lround(d * 149.6f)) + dl(" mn km", "M km"));
      yaz(X, 31, 1, String(d * 8.3167f, d < 10 ? 1 : 0) + dl(" isik dk", " lt-min"));
      float boy = atan2(dy, dx) * (180.0f / PI) - atan2(-ey, -ex) * (180.0f / PI);   // gezegenin Güneş'e göre gökteki açısı
      while (boy > 180) boy -= 360; while (boy < -180) boy += 360;
      yaz(X, 40, 1, fabs(boy) < 15 ? dl("gorunmez", "hidden") : boy > 0 ? dl("aksam gokte", "evening sky") : dl("sabah gokte", "morning sky"));
    } else { yaz(X, 22, 1, dl("biz buradayiz", "we are here")); yaz(X, 31, 1, "29.8 km/sn"); }
  }
  yaz(X, 56, 1, dl("2tus: ISS", "both: ISS"));
}
