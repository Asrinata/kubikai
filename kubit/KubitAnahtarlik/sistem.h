// Sistem: Araçlar (telefon bilgisi, zamanlayıcı, kronometre, pomodoro, alarm, büyük saat), gelen arama, rozetler (100),
// telefona yedek, ekran koruyucu, duvar kağıdı, menü stilleri, PIN/ebeveyn sınırı, hakkında ve gizli geliştirici sayfası.
// Ayar değişkenleri .ino'nun başında; bu dosya oyunlardan sonra eklenir.
#pragma once

// ---------------------------------------------------------------- telefon verisi (P pil, R arama, T takvim, D adım)
String bleLog[5]; int bleLogN = 0;                   // geliştirici sayfası: son BLE satırları
void bleKaydet(const String &s) { bleLog[bleLogN++ % 5] = s.substring(0, 21); }

// ---------------------------------------------------------------- araçlar: zamanlayıcı, kronometre, pomodoro (arka planda da işler)
String uyariYazi; uint32_t uyariBitis = 0;          // süre doldu ekranı
void bip() { if (bagli) { tusKar->setValue("BIP"); tusKar->notify(); } }
void uyar(const String &s) { uyariYazi = s; uyariBitis = millis() + 8000; bip(); }
void araclarGuncelle() {
  uint32_t t = millis();
  if (zam.calisiyor && t >= zam.bitis) { zam.calisiyor = false; zam.kalan = 0; uyar(dl("SURE DOLDU!", "TIME'S UP!")); }
  if (pom.calisiyor && t >= pom.bitis) {
    if (!pom.mola) { pom.tamam++; sayac("c_pom", 1); }
    pom.mola = !pom.mola; pom.kalan = pom.mola ? (pom.tamam % 4 == 0 ? 15 : 5) * 60000UL : 25 * 60000UL; pom.bitis = t + pom.kalan;
    uyar(pom.mola ? dl("Mola zamani!", "Break time!") : dl("Calisma zamani!", "Focus time!"));
  }
}
bool araclarMesgul() { return zam.calisiyor || kro.calisiyor || pom.calisiyor || millis() < uyariBitis || tel.araniyor; }
String dkSn(uint32_t ms) { uint32_t s = ms / 1000; return ikiHane(s / 60) + ":" + ikiHane(s % 60); }
bool uyariCiz(Olay &o1, Olay &o2) {                  // süre doldu: tam ekran yanıp söner, bir tuş kapatır
  if (millis() >= uyariBitis) return false;
  if (o1 != YOK || o2 != YOK) { uyariBitis = 0; o1 = o2 = YOK; tusYut = true; return false; }
  if ((millis() / 300) % 2) ekran.fillRect(0, 0, 128, 64, W), ekran.setTextColor(B);
  ortala(20, 2, uyariYazi.substring(0, 10)); if (uyariYazi.length() > 10) ortala(38, 1, uyariYazi);
  ekran.setTextColor(W);
  return true;
}
bool aramaCiz(Olay &o1, Olay &o2) {                   // gelen arama: sol reddet, sağ sessize al
  if (!tel.araniyor) return false;
  if (o2 == KISA) { telefonaGonder("ARAMA:RED", nullptr); tel.araniyor = false; o2 = YOK; }
  if (o1 == KISA) { telefonaGonder("ARAMA:SES", nullptr); o1 = YOK; }
  ekran.fillRect(0, 0, 128, 64, B); ekran.drawRoundRect(0, 0, 128, 64, 4, W);
  int r = 6 + (millis() / 120) % 6; ekran.drawCircle(64, 18, r, W); ekran.fillCircle(64, 18, 5, W);
  ortala(30, 1, dl("Ariyor", "Calling")); ortala(40, 1, tel.arayan.substring(0, 20));
  yaz(4, 54, 1, dl("<reddet", "<decline")); yaz(80, 54, 1, dl("sessiz>", "silent>"));
  return true;
}

// ---------------------------------------------------------------- sayfalar: araçlar listesi ve alt sayfaları
const char *ARAC_TR[] = {"Telefon", "Zamanlayici", "Kronometre", "Pomodoro", "Alarm kur", "Buyuk saat", "Bilgisayar", "Sunum kumandasi", "Kamera", "3B yazici", "Tarayici (Wi-Fi)", "YouTube (Wi-Fi)", "Ucak radari", "Uzay", "Wi-Fi baglan", "WhatsApp / kisiler"};
const char *ARAC_EN[] = {"Phone", "Timer", "Stopwatch", "Pomodoro", "Set alarm", "Big clock", "Computer", "Presenter", "Camera", "3D printer", "Browser (Wi-Fi)", "YouTube (Wi-Fi)", "Flight radar", "Space", "Connect Wi-Fi", "WhatsApp / contacts"};
const Sayfa ARAC_HEDEF[] = {TELEFON_S, ZAMANLAYICI, KRONOMETRE, POMODORO, ALARM_KUR, BUYUK_SAAT, PC_S, SUNUM_S, KAMERA_S, YAZICI_S, WEB_S, YT_S, RADAR_S, UZAY_S, WIFI_S, WHATSAPP_S};
const int ARAC_N = 16;
void araclarSayfasi(Olay o1, Olay o2) {
  String og[ARAC_N]; for (int i = 0; i < ARAC_N; i++) og[i] = dil ? ARAC_EN[i] : ARAC_TR[i];
  if (zam.calisiyor) og[1] += "  " + dkSn(zam.bitis - millis());
  if (kro.calisiyor) og[2] += "  *";
  if (pom.calisiyor) og[3] += String("  ") + (pom.mola ? dl("mola", "break") : dkSn(pom.bitis - millis()));
  int s = liste(dl("ARACLAR", "TOOLS"), og, ARAC_N, o1, o2);
  if (s >= 0) { if (s == 14) wifiEkranAc(); if (s == 15) waAc(); git(ARAC_HEDEF[s]); if (s == 0) telefonaGonder("TEL:YENILE", nullptr); }
}
void pilCiz(int x, int y, int yuzde, bool sarj) {
  ekran.drawRect(x, y, 22, 10, W); ekran.fillRect(x + 22, y + 3, 2, 4, W);
  if (yuzde >= 0) ekran.fillRect(x + 2, y + 2, 18 * yuzde / 100, 6, W);
  if (sarj) { ekran.fillTriangle(x + 10, y + 1, x + 7, y + 5, x + 11, y + 5, B); ekran.fillTriangle(x + 11, y + 4, x + 14, y + 4, x + 10, y + 9, B); }
}
void telefonSayfasi(Olay o2) {
  if (o2 == KISA) telefonaGonder("TEL:YENILE", dl("yenileniyor", "refreshing"));
  baslik(dl("TELEFON", "PHONE"));
  if (!bagli) { ortala(28, 1, dl("telefon bagli degil", "phone not connected")); return; }
  pilCiz(0, 13, tel.pil, tel.sarj); yaz(28, 14, 1, tel.pil >= 0 ? String(tel.pil) + "%" + (tel.sarj ? dl(" sarjda", " charging") : "") : String("--"));
  yaz(0, 28, 1, dl("Adim: ", "Steps: ") + (tel.adim >= 0 ? String(tel.adim) : String("--")));
  if (tel.adim >= 0) { ekran.drawRect(0, 37, 128, 5, W); ekran.fillRect(1, 38, min(126L, 126 * tel.adim / max(1L, tel.hedef)), 3, W); }
  if (tel.etkinlik.length()) {
    long fark = (long)tel.etkinlikEpoch - (long)time(nullptr);
    yaz(0, 45, 1, tel.etkinlik.substring(0, 21));
    yaz(0, 55, 1, fark > 0 ? (fark < 3600 ? String(fark / 60) + dl(" dk sonra", " min") : String(fark / 3600) + dl(" sa sonra", " h")) : String(dl("simdi", "now")));
  } else yaz(0, 50, 1, dl("takvimde etkinlik yok", "no upcoming events"));
}
void zamanlayiciSayfasi(Olay o1, Olay o2) {          // sağ: +1 dk (basılı: hızlı), sol: başlat/durdur
  static float tekrar = 0;
  if (o1 == BASTI) { tekrar = .45f; if (zam.calisiyor) zam.bitis += 60000; else zam.dk = zam.dk % 99 + 1; }
  else if (tus1.basili && !tus2.basili && (tekrar -= .04f) <= 0) { tekrar = .08f; if (!zam.calisiyor) zam.dk = zam.dk % 99 + 1; }
  if (o2 == KISA) { if (zam.calisiyor) { zam.kalan = zam.bitis - millis(); zam.calisiyor = false; } else { zam.bitis = millis() + (zam.kalan ? zam.kalan : zam.dk * 60000UL); zam.kalan = 0; zam.calisiyor = true; } }
  baslik(dl("ZAMANLAYICI", "TIMER"));
  uint32_t ms = zam.calisiyor ? zam.bitis - millis() : zam.kalan ? zam.kalan : zam.dk * 60000UL;
  ortala(20, 3, dkSn(ms));
  yaz(0, 56, 1, zam.calisiyor ? dl("sol:durdur sag:+1dk", "L:pause R:+1min") : dl("sol:baslat sag:+1dk", "L:start R:+1min"));
}
void kronometreSayfasi(Olay o1, Olay o2) {           // sol: başlat/durdur, sağ: tur (çalışırken) / sıfırla
  uint32_t gecen = kro.birikim + (kro.calisiyor ? millis() - kro.bas : 0);
  if (o2 == KISA) { if (kro.calisiyor) { kro.birikim = gecen; kro.calisiyor = false; } else { kro.bas = millis(); kro.calisiyor = true; } }
  if (o1 == KISA) { if (kro.calisiyor) { kro.tur[1] = kro.tur[0]; kro.tur[0] = gecen; } else { kro.birikim = 0; kro.tur[0] = kro.tur[1] = 0; gecen = 0; } }
  baslik(dl("KRONOMETRE", "STOPWATCH"));
  ortala(16, 2, dkSn(gecen) + "." + String(gecen / 100 % 10));
  for (int i = 0; i < 2; i++) if (kro.tur[i]) yaz(20, 36 + i * 9, 1, dl("tur ", "lap ") + String(i + 1) + "  " + dkSn(kro.tur[i]) + "." + String(kro.tur[i] / 100 % 10));
  yaz(0, 56, 1, kro.calisiyor ? dl("sol:dur sag:tur", "L:stop R:lap") : dl("sol:basla sag:sifirla", "L:start R:reset"));
}
void pomodoroSayfasi(Olay o1, Olay o2) {             // sol: başlat/duraklat, sağ: aşamayı atla
  if (o2 == KISA) { if (pom.calisiyor) { pom.kalan = pom.bitis - millis(); pom.calisiyor = false; } else { pom.bitis = millis() + pom.kalan; pom.calisiyor = true; } }
  if (o1 == KISA) { pom.bitis = millis(); if (!pom.calisiyor) { pom.calisiyor = true; } }
  uint32_t kalan = pom.calisiyor ? pom.bitis - millis() : pom.kalan, toplam = pom.mola ? (pom.tamam % 4 == 0 ? 15 : 5) * 60000UL : 25 * 60000UL;
  baslik(String("POMODORO  ") + pom.tamam);
  float oran = 1 - (float)kalan / toplam;
  for (int a = 0; a < 60; a++) { float r = a * 6 * DEG_TO_RAD - HALF_PI; if (a < oran * 60) ekran.fillCircle(30 + cosf(r) * 18, 36 + sinf(r) * 18, 1, W); else ekran.drawPixel(30 + cosf(r) * 18, 36 + sinf(r) * 18, W); }
  yaz(18, 33, 1, dkSn(kalan));
  yaz(58, 18, 1, pom.mola ? dl("MOLA", "BREAK") : dl("ODAK", "FOCUS"));
  for (int i = 0; i < 4; i++) { if (i < pom.tamam % 4 || (pom.tamam && pom.tamam % 4 == 0)) ekran.fillCircle(62 + i * 8, 32, 2, W); else ekran.drawCircle(62 + i * 8, 32, 2, W); }
  yaz(58, 44, 1, pom.calisiyor ? dl("sol:duraklat", "L:pause") : dl("sol:baslat", "L:start")); yaz(58, 53, 1, dl("sag:atla", "R:skip"));
}
void alarmSayfasi(Olay o1, Olay o2) {                // sağ: +1 (basılı: hızlı), sol: sonraki alan / kur
  static int sa = 7, dk = 0, alan = 0; static float tekrar = 0;
  auto artir = [] { if (alan == 0) sa = (sa + 1) % 24; else dk = (dk + 1) % 60; };
  if (o1 == BASTI) { artir(); tekrar = .45f; } else if (tus1.basili && !tus2.basili && (tekrar -= .04f) <= 0) { tekrar = .08f; artir(); }
  if (o2 == KISA) { if (alan == 0) alan = 1; else { alan = 0; if (bagli) { telefonaGonder("ALARM:" + ikiHane(sa) + ":" + ikiHane(dk), dl("alarm telefona kuruldu", "alarm set on phone")); } else goster(dl("telefon bagli degil", "phone not connected")); } }
  baslik(dl("ALARM KUR", "SET ALARM"));
  yaz(28, 22, 3, ikiHane(sa)); yaz(64, 22, 3, ":"); yaz(76, 22, 3, ikiHane(dk));
  ekran.fillRect(alan ? 76 : 28, 46, 34, 2, W);
  yaz(0, 56, 1, alan ? dl("sol:telefona kur", "L:set on phone") : dl("sol:dakikaya gec", "L:next")); altMesaj();
}
void yediSeg(int x, int y, int n, int w, int h) {     // büyük saat rakamı: 7 bölüm
  const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; uint8_t s = SEG[n]; int t = 3, yh = h / 2;
  if (s & 1) ekran.fillRect(x + t, y, w - 2 * t, t, W);
  if (s & 2) ekran.fillRect(x + w - t, y + t, t, yh - t, W);
  if (s & 4) ekran.fillRect(x + w - t, y + yh, t, yh - t, W);
  if (s & 8) ekran.fillRect(x + t, y + h - t, w - 2 * t, t, W);
  if (s & 16) ekran.fillRect(x, y + yh, t, yh - t, W);
  if (s & 32) ekran.fillRect(x, y + t, t, yh - t, W);
  if (s & 64) ekran.fillRect(x + t, y + yh - 1, w - 2 * t, t, W);
}
void buyukSaatSayfasi() {
  if (!saatVar()) { ortala(24, 2, "--:--"); return; }
  tm z = simdiTm();
  yediSeg(2, 4, z.tm_hour / 10, 24, 44); yediSeg(30, 4, z.tm_hour % 10, 24, 44);
  if (z.tm_sec % 2) { ekran.fillRect(60, 16, 4, 4, W); ekran.fillRect(60, 32, 4, 4, W); }
  yediSeg(70, 4, z.tm_min / 10, 24, 44); yediSeg(98, 4, z.tm_min % 10, 24, 44);
  ekran.fillRect(0, 55, 128 * z.tm_sec / 60, 2, W);
  const char *GUN[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"}, *GUN_EN[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  yaz(0, 57, 1, String(dil ? GUN_EN[z.tm_wday] : GUN[z.tm_wday]) + " " + String(z.tm_mday));
  if (tel.pil >= 0) yaz(98, 57, 1, String(tel.pil) + "%");
}

// ---------------------------------------------------------------- rozetler: 100 tane; anahtar değeri eşiği geçince (tip 1: küçük iyi)
struct Rozet { const char *ad, *anahtar; int esik; uint8_t tip; };
const Rozet ROZET[100] = {
  {"Flappy'ye basla", "oy0", 1, 0}, {"Geometry'ye basla", "oy1", 1, 0}, {"Racer'a basla", "oy2", 1, 0}, {"Kubitcraft kasifi", "oy3", 1, 0}, {"Doom'a gir", "oy4", 1, 0},
  {"Super Kubit", "oy5", 1, 0}, {"Kubimon egitmeni", "oy6", 1, 0}, {"Zindana in", "oy7", 1, 0}, {"Metroya bin", "oy8", 1, 0}, {"Ciftci", "oy9", 1, 0},
  {"Yilan oyna", "oy10", 1, 0}, {"Dino oyna", "oy11", 1, 0}, {"Pong oyna", "oy12", 1, 0}, {"Breakout oyna", "oy13", 1, 0}, {"Istilaci avi", "oy14", 1, 0},
  {"Asteroid oyna", "oy15", 1, 0}, {"Labirente gir", "oy16", 1, 0}, {"Tunele gir", "oy17", 1, 0}, {"Tetris oyna", "oy18", 1, 0}, {"2048 oyna", "oy19", 1, 0},
  {"Reaksiyon testi", "oy20", 1, 0}, {"Simon oyna", "oy21", 1, 0}, {"Hafiza oyna", "oy22", 1, 0}, {"Yazi-tura at", "oy23", 1, 0}, {"Okcu", "oy24", 1, 0},
  {"Bowling oyna", "oy25", 1, 0}, {"Golfcu", "oy26", 1, 0}, {"Satranc bulmaca", "oy27", 1, 0}, {"XOX oyna", "oy28", 1, 0}, {"Adam asmaca", "oy29", 1, 0},
  {"Wordle oyna", "oy30", 1, 0}, {"Matematik yarisi", "oy31", 1, 0}, {"Kafadan hesap", "oy32", 1, 0}, {"Parasutcu", "oy33", 1, 0}, {"Tirmanisci", "oy34", 1, 0},
  {"Flappy 10", "flappy", 10, 0}, {"Flappy 30", "flappy", 30, 0}, {"Zor Flappy 5", "flappyz", 5, 0}, {"Geometry 1 bitti", "gd0", 100, 0}, {"Geometry 2 bitti", "gd1", 100, 0},
  {"Geometry 3 bitti", "gd2", 100, 0}, {"Geometry 4 bitti", "gd3", 100, 0}, {"Geometry 5 bitti", "gd4", 100, 0}, {"Kabus bitti", "gd5", 100, 0}, {"Sahil 3 tur", "rc0", 99999, 1},
  {"Dag 3 tur", "rc1", 99999, 1}, {"Gece sehri", "rc2", 99999, 1}, {"Yilan 30", "yilan", 30, 0}, {"Dino 500", "dino", 500, 0}, {"Pong 10 galibiyet", "pong", 10, 0},
  {"Breakout 100", "breakout", 100, 0}, {"Istila 500", "istila", 500, 0}, {"Asteroid 500", "asteroid", 500, 0}, {"Labirent 60 sn", "labirent", 600, 1}, {"Tunel 20 halka", "tunel", 20, 0},
  {"Tetris 1000", "tetris", 1000, 0}, {"2048: 1000 puan", "2048", 1000, 0}, {"Simsek refleks", "reaksiyon", 250, 1}, {"Simon 10", "simon", 10, 0}, {"Hafiza 12 hamle", "hafiza", 12, 1},
  {"Yazi-tura 5 seri", "para", 5, 0}, {"Okculuk 40", "okculuk", 40, 0}, {"Bowling 30", "bowling", 30, 0}, {"Golf par 8", "golf", 8, 1}, {"Satranc ustasi", "satranc", 13, 0},
  {"XOX 5 galibiyet", "xox", 5, 0}, {"Adam asmaca 5", "adam", 5, 0}, {"Wordle 5", "wordle", 5, 0}, {"Matematik 25", "matyaris", 25, 0}, {"Kafadan 20", "kafadan", 20, 0},
  {"Parasut 100", "parasut", 100, 0}, {"Tirmanis 300", "tirmanis", 300, 0}, {"Doom bitti", "doom", 99999, 1}, {"Doom 5 dakika", "doom", 3000, 1}, {"Super Kubit 5000", "mario", 5000, 0},
  {"Dunya 1 bitti", "mario_il", 4, 0}, {"Dunya 3 bitti", "mario_il", 12, 0}, {"Kubidex 8", "kubimon", 8, 0}, {"Kubidex tam", "kubimon", 16, 0}, {"3 salon rozeti", "kmrozet", 3, 0},
  {"Sampiyon", "kmrozet", 6, 0}, {"Zindan kat 5", "zindan", 5, 0}, {"Zindan kat 10", "zindan", 10, 0}, {"Metro 500", "metro", 500, 0}, {"Elmas buldun", "b_elmas", 1, 0},
  {"Hasat 50", "b_hasat", 50, 0}, {"10 oyun", "c_oyun", 10, 0}, {"100 oyun", "c_oyun", 100, 0}, {"500 oyun", "c_oyun", 500, 0}, {"Ilk baglanti", "c_bag", 1, 0},
  {"25 bildirim", "c_bild", 25, 0}, {"Kubik'e soru", "c_soru", 1, 0}, {"Meraklı: 10 soru", "c_soru", 10, 0}, {"3 gun", "c_gun", 3, 0}, {"7 gun", "c_gun", 7, 0},
  {"30 gun", "c_gun", 30, 0}, {"Pomodoro", "c_pom", 1, 0}, {"Ilk yedek", "c_yedek", 1, 0}, {"Gelistirici", "c_gel", 1, 0}, {"Hepsi! (99 rozet)", "c_rozet", 99, 0}};
uint8_t rozetBit[13]; bool rozetYuklu = false;
bool rozetVar(int i) { return rozetBit[i / 8] >> (i % 8) & 1; }
void rozetKontrol(const char *anahtar) {              // anahtar nullptr: hepsi
  if (!rozetYuklu) { memset(rozetBit, 0, 13); kayit.getBytes("rozetler", rozetBit, 13); rozetYuklu = true; }
  bool degisti = false; int say = 0;
  for (int i = 0; i < 100; i++) {
    if (rozetVar(i)) { say++; continue; }
    const Rozet &r = ROZET[i];
    if (anahtar && strcmp(anahtar, r.anahtar)) continue;
    int v = kayit.getInt(r.anahtar, 0);
    if (r.tip ? (v > 0 && v <= r.esik) : v >= r.esik) { rozetBit[i / 8] |= 1 << (i % 8); degisti = true; say++; goster(String(dl("Rozet: ", "Badge: ")) + r.ad, 3500); }
  }
  if (degisti) { kayit.putBytes("rozetler", rozetBit, 13); kayit.putInt("c_rozet", say); if (anahtar) rozetKontrol("c_rozet"); }
}
void sayac(const char *anahtar, int artis) { kayit.putInt(anahtar, kayit.getInt(anahtar, 0) + artis); rozetKontrol(anahtar); }
void rozetSayfasi(Olay o1, Olay o2) {                 // sağ: sonraki, sol: 10 ileri
  static float tekrar = 0;
  if (!rozetYuklu) rozetKontrol(nullptr);
  if (o1 == BASTI) { imlec = (imlec + 1) % 100; tekrar = .45f; } else if (tus1.basili && !tus2.basili && (tekrar -= .04f) <= 0) { tekrar = .07f; imlec = (imlec + 1) % 100; }
  if (o2 == KISA) imlec = (imlec + 10) % 100;
  int say = 0; for (int i = 0; i < 100; i++) say += rozetVar(i);
  baslik(String(dl("ROZETLER ", "BADGES ")) + say + "/100");
  for (int i = 0; i < 100; i++) { int x = (i % 10) * 5, y = 12 + (i / 10) * 5; if (rozetVar(i)) ekran.fillRect(x, y, 4, 4, W); else ekran.drawPixel(x + 1, y + 1, W); if (i == imlec && (millis() / 250) % 2) ekran.drawRect(x - 1, y - 1, 6, 6, W); }
  const Rozet &r = ROZET[imlec];
  int y = 12; for (auto &s : sar(r.ad, 12)) { yaz(54, y, 1, s); y += 9; }
  int v = kayit.getInt(r.anahtar, 0);
  yaz(54, 40, 1, rozetVar(imlec) ? dl("KAZANILDI", "EARNED") : r.tip ? String("<=") + r.esik : String(v) + "/" + r.esik);
  yaz(54, 55, 1, dl("sol:+10", "L:+10"));
}

// ---------------------------------------------------------------- telefona yedek: Y satırları gider, geri yüklemede Z satırları gelir
const char *YEDEK_INT[] = {"acilis", "uyku", "uykutip", "unutma", "kiyafet", "k_s0", "k_s1", "k_s2", "k_s3", "cark_gun", "parlak", "dil", "fbskin", "menustil", "ters", "buyuk", "koruyucu", "duvar", "piltas", "gece", "gizli", "eb_sinir", "pin",
                           "mario_il", "mc_var", "wa_yol", "fav0", "fav1", "fav2", "son", "c_oyun", "c_bag", "c_bild", "c_soru", "c_gun", "c_songun", "c_pom", "c_yedek", "c_gel", "c_rozet",
                           "kmrozet", "b_elmas", "b_hasat", "flappyz", "rc0", "rc1", "rc2", "rt0", "rt1", "rt2", "rl0", "rl1", "rl2", "gd0", "gd1", "gd2", "gd3", "gd4", "gd5"};
const char *YEDEK_BLOB[] = {"sv_m", "sv_g", "rozetler", "kasaba", "kubimon", "doom_oz", "wa_rehber", "mc_s2", "mc_ek", "mc_b0", "mc_b1", "mc_b2", "mc_b3", "rg0", "rg1", "rg2"};
const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
String b64Kodla(const uint8_t *v, int n) {
  String s; s.reserve(n * 4 / 3 + 4);
  for (int i = 0; i < n; i += 3) { uint32_t b = v[i] << 16 | (i + 1 < n ? v[i + 1] << 8 : 0) | (i + 2 < n ? v[i + 2] : 0);
    s += B64[b >> 18 & 63]; s += B64[b >> 12 & 63]; s += i + 1 < n ? B64[b >> 6 & 63] : '='; s += i + 2 < n ? B64[b & 63] : '='; }
  return s;
}
int b64Coz(const String &s, std::vector<uint8_t> &cikti) {
  uint32_t b = 0; int bit = 0;
  for (unsigned i = 0; i < s.length(); i++) { const char *p = strchr(B64, s[i]); if (!p || s[i] == '=' || !s[i]) continue; b = b << 6 | (p - B64); bit += 6; if (bit >= 8) { bit -= 8; cikti.push_back(b >> bit & 255); } }
  return cikti.size();
}
void yedekSatir(const String &s) { tusKar->setValue(s.c_str()); tusKar->notify(); delay(18); }   // telefon yetişsin
void yedekle() {
  if (!bagli) { goster(dl("telefon bagli degil", "phone not connected")); return; }
  ekran.clearDisplay(); ortala(28, 1, dl("Yedekleniyor...", "Backing up...")); ekran.display();
  uzunIs = true; yedekSatir("Y+");
  auto intGonder = [](const char *k) { if (kayit.isKey(k)) yedekSatir(String("Yi") + k + "|" + String(kayit.getInt(k, 0))); };
  for (auto k : YEDEK_INT) intGonder(k);
  for (int i = 0; i < OYUN_N; i++) if (*OYUN_LISTE[i].rekor) intGonder(OYUN_LISTE[i].rekor);
  for (int i = 0; i < OYUN_N + OYUN_ILK; i++) intGonder(("oy" + String(i)).c_str());
  intGonder("flappy");
  static uint8_t tampon[4200];
  for (auto k : YEDEK_BLOB) {
    if (!kayit.isKey(k)) continue;
    int n = kayit.getBytes(k, tampon, sizeof(tampon)); String b = b64Kodla(tampon, n); int parca = (b.length() + 119) / 120;
    for (int p = 0; p < parca; p++) yedekSatir(String("Yb") + k + "|" + p + "|" + parca + "|" + b.substring(p * 120, p * 120 + 120));
  }
  yedekSatir("Y."); uzunIs = false;
  sayac("c_yedek", 1); goster(dl("Yedek telefonda", "Backup saved on phone"));
}
String geriBlob, geriAnahtar;
void geriSatir(const String &g) {                     // telefondan: Zi<anahtar>|<sayı>  Zb<anahtar>|<parça>|<toplam>|<veri>  Z.
  if (g == ".") { goster(dl("Yedek yuklendi, yeniden basliyor", "Restored, restarting"), 3000); uyariBitis = 0; ekran.clearDisplay(); ortala(28, 1, dl("Yeniden basliyor", "Restarting")); ekran.display(); delay(1500); ESP.restart(); }
  auto p = parcala(g.substring(1), '|');
  if (g[0] == 'i' && p.size() >= 2) kayit.putInt(p[0].c_str(), p[1].toInt());
  if (g[0] == 'b' && p.size() >= 4) {
    if (p[1].toInt() == 0) { geriBlob = ""; geriAnahtar = p[0]; }
    if (p[0] == geriAnahtar) geriBlob += p[3];
    if (p[1].toInt() == p[2].toInt() - 1 && p[0] == geriAnahtar) { std::vector<uint8_t> v; b64Coz(geriBlob, v); kayit.putBytes(geriAnahtar.c_str(), v.data(), v.size()); geriBlob = ""; }
  }
}

// ---------------------------------------------------------------- PIN ve ebeveyn sınırı
int pinHedef = 0; uint8_t pinRakam[4]; int pinAdim = 0; bool pinYeni = false;   // pinHedef: 1 ebeveyn sınırı değiştir, 2 PIN değiştir
const int SINIR_DK[] = {0, 30, 60, 120};
uint32_t oyunSn = 0; float oyunKesir = 0; int ebGun = -1;
int bugun() { return saatVar() ? time(nullptr) / 86400 : 0; }
void oyunSuresiEkle(float dt) {
  if (ebGun != bugun()) { ebGun = bugun(); oyunSn = kayit.getInt("eb_gun", -1) == ebGun ? kayit.getInt("eb_sn", 0) : 0; }
  oyunKesir += dt; if (oyunKesir >= 1) { oyunSn += (int)oyunKesir; oyunKesir -= (int)oyunKesir; if (oyunSn % 30 == 0) { kayit.putInt("eb_gun", ebGun); kayit.putInt("eb_sn", oyunSn); } }
}
bool oyunSuresiBitti() { return ayarSinir && oyunSn >= (uint32_t)SINIR_DK[ayarSinir] * 60; }
void pinIste(int hedef) {
  pinHedef = hedef; memset(pinRakam, 0, 4); pinAdim = 0; pinYeni = ayarPin < 0 && hedef == 2;
  if (ayarPin < 0 && hedef == 1) { ayarSinir = (ayarSinir + 1) % 4; kayit.putInt("eb_sinir", ayarSinir); return; }   // PIN yoksa doğrudan
  git(PIN_GIR);
}
void pinSayfasi(Olay o1, Olay o2) {                   // sağ: rakam +1, sol: sonraki; 4 rakam sonra kontrol (yeni PIN'de kaydet)
  if (o1 == KISA) pinRakam[pinAdim] = (pinRakam[pinAdim] + 1) % 10;
  if (o2 == KISA && ++pinAdim >= 4) {
    int p = pinRakam[0] * 1000 + pinRakam[1] * 100 + pinRakam[2] * 10 + pinRakam[3];
    if (pinYeni) { ayarPin = p; kayit.putInt("pin", p); goster(dl("PIN kaydedildi", "PIN saved")); git(AYAR); return; }
    if (p != ayarPin) { goster(dl("Yanlis PIN", "Wrong PIN")); git(AYAR); return; }
    if (pinHedef == 1) { ayarSinir = (ayarSinir + 1) % 4; kayit.putInt("eb_sinir", ayarSinir); git(AYAR); imlec = 13; return; }
    pinYeni = true; memset(pinRakam, 0, 4); pinAdim = 0; return;   // eski PIN doğru: yenisini gir (0000 dışında her şey)
  }
  baslik(pinYeni ? dl("YENI PIN", "NEW PIN") : dl("PIN GIR", "ENTER PIN"));
  for (int i = 0; i < 4; i++) { int x = 16 + i * 26; ekran.drawRoundRect(x, 20, 20, 22, 3, W); if (i == pinAdim) ekran.fillRoundRect(x, 20, 20, 22, 3, W);
    ekran.setTextColor(i == pinAdim ? B : W); yaz(x + 5, 24, 2, i < pinAdim && !pinYeni ? String("*") : String(pinRakam[i])); ekran.setTextColor(W); }
  yaz(0, 52, 1, dl("sag:rakam sol:sonraki", "R:digit L:next"));
}

// ---------------------------------------------------------------- görünüm: ekran koruyucu, duvar kağıdı
void koruyucuCiz(uint32_t t) {
  if (ayarKoruyucu == 1) {                            // akvaryum: balıklar, kabarcıklar, yosun
    for (int i = 0; i < 4; i++) {
      int yon = i % 2 ? 1 : -1; float x = fmodf(t / (40.0f + i * 9) * yon + i * 50 + 400, 160) - 16; if (yon < 0) x = 128 - x;
      int y = 12 + i * 12 + sinf(t / 700.0f + i) * 3;
      ekran.fillCircle(x, y, 3, W); ekran.fillTriangle(x - 3 * yon, y, x - 7 * yon, y - 3, x - 7 * yon, y + 3, W); ekran.drawPixel(x + 1 * yon, y - 1, B);
    }
    for (int i = 0; i < 6; i++) ekran.drawCircle(20 + i * 19, 63 - (int)(t / (25 + i * 4) + i * 17) % 60, 1, W);
    for (int i = 0; i < 5; i++) for (int k = 0; k < 8; k++) ekran.drawPixel(10 + i * 26 + sinf(t / 500.0f + k * .8f + i) * 2, 63 - k * 2, W);
  } else if (ayarKoruyucu == 2) {                      // yıldız alanı: merkezden dışa akar
    for (int i = 0; i < 40; i++) { float a = i * 2.39996f, z = fmodf(t / 2000.0f + i * .025f, 1.0f); float r = z * z * 90; int x = 64 + cosf(a) * r, y = 32 + sinf(a) * r * .55f; ekran.drawPixel(x, y, W); if (z > .6f) ekran.drawPixel(x + 1, y, W); }
  } else {                                            // matrix yağmuru
    for (int c = 0; c < 21; c++) { int bas = (t / (60 + c * 7 % 40) + c * 13) % 90; for (int k = 0; k < 6; k++) { int y = (bas - k * 8) % 90; if (y < 64 && y >= 0) { char ch = 33 + (c * 7 + y + t / 400) % 90; ekran.drawChar(c * 6, y, ch, W, B, 1); } } }
  }
}
void duvarCiz(uint32_t t) {                           // ana ekran arka planı: seyrek ve sakin
  if (ayarDuvar == 1) for (int i = 0; i < 18; i++) { int x = (i * 53) % 128, y = 10 + (i * 29) % 44; if ((t / 600 + i) % 5) ekran.drawPixel(x, y, W); }
  else if (ayarDuvar == 2) {                          // havaya göre: yağmur, kar, güneşte ışık noktaları
    bool yagis = hava.kod >= 51; bool kar = karli(hava.kod);
    for (int i = 0; i < 14; i++) { int x = (i * 41 + (kar ? (int)(sinf(t / 800.0f + i) * 3) : 0)) % 128, y = 10 + (int)((t / (kar ? 90 : yagis ? 25 : 300) + i * 11) % 44);
      if (yagis && !kar) ekran.drawFastVLine(x, y, 2, W); else ekran.drawPixel(x, y, W); }
  } else if (ayarDuvar == 3) for (int i = 0; i < 8; i++) ekran.drawCircle((i * 37) % 128, 63 - (int)(t / (50 + i * 9) + i * 19) % 54, 1 + i % 2, W);
}

// ---------------------------------------------------------------- menü: favoriler, son oynanan, gizlenen uygulamalar
int menuListe[24], menuListeN = 0;                    // < 100: ana uygulama, >= 100: oyun (ızgara sırası + 100)
void menuListeKur() {
  menuListeN = 0;
  for (int i = 0; i < MENU_N; i++) {
    if (!(ayarGizli >> i & 1) || i == 0 || i == 11) menuListe[menuListeN++] = i;
    if (MENU_HEDEF[i] == OYUNLAR) {                   // oyunların hemen arkasına: son oynanan + favoriler
      int son = kayit.getInt("son", -1); bool sonFav = false;
      for (int f = 0; f < 3; f++) { int g = kayit.getInt(("fav" + String(f)).c_str(), -1); if (g == son) sonFav = true; }
      if (son >= 0 && !sonFav) menuListe[menuListeN++] = 100 + son;
      for (int f = 0; f < 3; f++) { int g = kayit.getInt(("fav" + String(f)).c_str(), -1); if (g >= 0) menuListe[menuListeN++] = 100 + g; }
    }
  }
}
bool favMi(int g) { for (int f = 0; f < 3; f++) if (kayit.getInt(("fav" + String(f)).c_str(), -1) == g) return true; return false; }
void favDegistir(int g) {                             // varsa çıkar, yoksa ilk boş yuvaya (dolu ise en eskinin yerine)
  for (int f = 0; f < 3; f++) if (kayit.getInt(("fav" + String(f)).c_str(), -1) == g) { kayit.putInt(("fav" + String(f)).c_str(), -1); goster(dl("Favoriden cikti", "Unfavorited")); menuListeKur(); return; }
  int bos = 0; for (int f = 0; f < 3; f++) if (kayit.getInt(("fav" + String(f)).c_str(), -1) < 0) { bos = f; break; } else bos = (f + 1) % 3;
  kayit.putInt(("fav" + String(bos)).c_str(), g); goster(dl("Favori: ana menude", "Favorite: in main menu")); menuListeKur();
}

// ---------------------------------------------------------------- menüyü düzenle, hakkında, geliştirici
void menuDuzenleSayfasi(Olay o1, Olay o2) {
  String og[MENU_N + 1];
  for (int i = 0; i < MENU_N; i++) og[i] = String(i == 0 || i == 11 ? "[*] " : (ayarGizli >> i & 1) ? "[ ] " : "[x] ") + menuAd(i);
  og[MENU_N] = dl("Bitti", "Done");
  int s = liste(dl("MENUYU DUZENLE", "EDIT MENU"), og, MENU_N + 1, o1, o2);
  if (s == MENU_N) { git(AYAR); imlec = 11; menuListeKur(); return; }
  if (s > 0 && s != 11) { ayarGizli ^= 1 << s; kayit.putInt("gizli", ayarGizli); }
}
void hakkindaSayfasi(Olay o1) {                       // sağ tuşa 7 kez bas: geliştirici sayfası
  static int say = 0; static uint32_t son = 0;
  if (o1 == KISA) { say = millis() - son < 1500 ? say + 1 : 1; son = millis(); if (say >= 7) { say = 0; sayac("c_gel", 1); git(GELISTIRICI); return; } if (say >= 4) goster(String(7 - say) + dl(" adim kaldi", " steps left"), 800); }
  baslik(dl("HAKKINDA", "ABOUT"));
  kubit(MUTLU, 0, 1, 0, millis() / 40);
  yaz(54, 14, 1, "Kubit s" + String(SURUM)); yaz(54, 24, 1, "AI Pocket"); yaz(54, 32, 1, "Buddy"); yaz(54, 44, 1, String(OYUN_N + OYUN_ILK) + dl(" oyun", " games"));
  altMesaj();
}
void gelistiriciSayfasi() {
  static uint32_t son = 0, kare = 0, fps = 0; kare++; if (millis() - son >= 1000) { fps = kare; kare = 0; son = millis(); }
  baslik(dl("GELISTIRICI", "DEVELOPER"));
  yaz(0, 11, 1, "FPS " + String(fps) + "  up " + String(millis() / 60000) + "dk");
  yaz(0, 20, 1, "heap " + String(ESP.getFreeHeap() / 1024) + "K min " + String(ESP.getMinFreeHeap() / 1024) + "K");
  yaz(0, 29, 1, String("BLE ") + (bagli ? "bagli" : "yok") + " profil " + String(ayarProfil + 1));
  for (int i = 0; i < 3; i++) { int j = (bleLogN - 1 - i + 50) % 5; if (bleLogN - i > 0) yaz(0, 38 + i * 9, 1, bleLog[j]); }
}
