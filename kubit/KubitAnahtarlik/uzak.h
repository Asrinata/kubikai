// Uzaktan kumanda sayfaları: bilgisayar (PC ajanı), sunum, telefon kamerası, Bambu 3B yazıcı; telefonu unutma uyarısı.
// Komutlar telefona gider (PC:x, KAMERA:x, YAZICI:x), telefon PC ajanına / kameraya / yazıcının MQTT'sine iletir.
// Telefondan: C<pc adı>|<cpu>|<ram> (PC durumu, boş: bulunamadı), B<durum>|<yüzde>|<kalan dk>|<dosya>|<nozül>|<tabla>|<katman>|<toplam katman>.
#pragma once

struct { String ad, fpsUyg; int cpu = -1, ram = -1, cpuC = -1, gpuC = -1, gpu = -1, fps = -1; uint32_t zaman = 0; } pc;
int ayarYakinlik = 0;                               // yakınlık kilidi: Kübit'le uzaklaşınca PC kilitlenir (PC ajanı Kübit'in yayınını dinler)
struct { String durum, dosya; int yuzde = -1, kalan = 0, nozul = 0, tabla = 0, katman = 0, toplam = 0; uint32_t zaman = 0; } yazici;
void pcSatiri(const String &g) {                    // Q<ad>|<cpu %>|<ram %>|<cpu °C>|<gpu °C>|<gpu %>|<fps>|<fps uygulaması>
  auto p = parcala(g, '|'); auto al = [&](int i) { return (int)p.size() > i && p[i].length() ? (int)p[i].toInt() : -1; };
  pc.ad = p[0]; pc.cpu = al(1); pc.ram = al(2); pc.cpuC = al(3); pc.gpuC = al(4); pc.gpu = al(5); pc.fps = al(6); pc.fpsUyg = p.size() > 7 ? ascii(p[7]) : ""; pc.zaman = millis();
}
void yakinlikReklam() {
  static uint32_t son = 0;
  if (!ayarYakinlik || !bagli || millis() - son < 3000) return;
  son = millis();
  if (!NimBLEDevice::getAdvertising()->isAdvertising()) NimBLEDevice::startAdvertising();
}
void pcMonSayfasi() {                                // canlı monitör: büyük sayılar
  static uint32_t istek = 0;
  if (bagli && millis() - istek > 1500) { istek = millis(); telefonaGonder("PC:durum", nullptr); }
  bool taze = millis() - pc.zaman < 6000;
  baslik(taze ? (pc.fpsUyg.length() && pc.fps > 0 ? pc.fpsUyg : pc.ad).substring(0, 21) : String(bagli ? dl("PC araniyor...", "looking for PC...") : dl("telefon yok", "no phone")));
  const char *ad[6] = {"CPU %", "CPU C", "RAM %", "GPU %", "GPU C", "FPS"};
  int v[6] = {pc.cpu, pc.cpuC, pc.ram, pc.gpu, pc.gpuC, pc.fps};
  for (int i = 0; i < 6; i++) {
    int x = (i % 3) * 43, y = 11 + (i / 3) * 27;
    ekran.drawRoundRect(x, y, 41, 25, 3, W); yaz(x + 3, y + 2, 1, ad[i]);
    String s = taze && v[i] >= 0 ? String(v[i]) : String("--");
    yaz(x + 20 - s.length() * 6, y + 11, 2, s);
    if (taze && v[i] >= 0 && ((i == 1 && v[i] >= 90) || (i == 4 && v[i] >= 85)) && millis() / 300 % 2) ekran.fillRoundRect(x, y, 41, 25, 3, SH110X_INVERSE);   // çok sıcak: yanıp söner
  }
}
void yaziciSatiri(const String &g) {
  auto p = parcala(g, '|'); if (p.size() < 8) return;
  String eski = yazici.durum;
  yazici.durum = p[0]; yazici.yuzde = p[1].toInt(); yazici.kalan = p[2].toInt(); yazici.dosya = ascii(p[3]); yazici.nozul = p[4].toInt(); yazici.tabla = p[5].toInt();
  yazici.katman = p[6].toInt(); yazici.toplam = p[7].toInt(); yazici.zaman = millis();
  if (eski.length() && eski != yazici.durum) {         // durum değişince her sayfada bildir
    if (yazici.durum == "FINISH") uyar(dl("BASKI BITTI!", "PRINT DONE!"));
    else if (yazici.durum == "FAILED") uyar(dl("BASKI HATASI!", "PRINT FAILED!"));
    else if (yazici.durum == "PAUSE") goster(dl("Baski duraklatildi", "Print paused"), 4000);
  }
}

// ---------------------------------------------------------------- ekran paylaşımı: kare telefon üzerinden PC ajanının penceresine gider
// Kare: 128x64, satır satır 1 bit (1024 bayt) -> PackBits -> base64 -> "EKR:<no>/<parça>/<toplam>:<veri>" (en çok 8 kare/sn, yalnız değişince)
bool ekranPaylas = false;
int packbits(const uint8_t *a, int n, uint8_t *o) {
  int k = 0, i = 0;
  while (i < n) {
    int j = i; while (j + 1 < n && a[j + 1] == a[i] && j - i < 127) j++;
    if (j - i >= 2) { o[k++] = 257 - (j - i + 1); o[k++] = a[i]; i = j + 1; continue; }   // tekrar: 129..255 = (257-adet)
    int bas = k++, say = 0;
    while (i < n && say < 128 && !(i + 2 < n && a[i] == a[i + 1] && a[i] == a[i + 2])) { o[k++] = a[i++]; say++; }
    o[bas] = say - 1;                                 // düz: 0..127 = adet-1
  }
  return k;
}
int b64(const uint8_t *a, int n, char *o) {
  const char *T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; int k = 0;
  for (int i = 0; i < n; i += 3) {
    uint32_t v = a[i] << 16 | (i + 1 < n ? a[i + 1] << 8 : 0) | (i + 2 < n ? a[i + 2] : 0);
    o[k++] = T[v >> 18 & 63]; o[k++] = T[v >> 12 & 63]; o[k++] = i + 1 < n ? T[v >> 6 & 63] : '='; o[k++] = i + 2 < n ? T[v & 63] : '=';
  }
  o[k] = 0; return k;
}
void ekranGonder() {                                 // loop() başında: son çizilen kare hâlâ tamponda
  static uint32_t son = 0, sonOzet = 0; static uint8_t no = 0;
  if (!ekranPaylas || (!bagli && !ekranIstemci) || millis() - son < 125) return;
  static uint8_t k[1024]; memset(k, 0, sizeof k);
  for (int y = 0; y < 64; y++) for (int x = 0; x < 128; x++) if (ekran.getPixel(x, y)) k[y * 16 + x / 8] |= 0x80 >> (x & 7);
  uint32_t oz = 2166136261u; for (int i = 0; i < 1024; i++) oz = (oz ^ k[i]) * 16777619u;
  if (oz == sonOzet && millis() - son < 2000) return;  // değişmediyse 2 sn'de bir tazele
  son = millis(); sonOzet = oz; no++;
  static uint8_t p[1040]; static char s[1400]; int sn = b64(p, packbits(k, 1024, p), s);
  const int PARCA = 160; int t = (sn + PARCA - 1) / PARCA;
  for (int i = 0; i < t; i++) {
    char satir[190]; snprintf(satir, sizeof satir, "EKR:%d/%d/%d:%.*s", no, i, t, PARCA, s + i * PARCA);
    if (bagli) { tusKar->setValue(satir); tusKar->notify(); }
    if (ekranIstemci) { ekranKar->setValue(satir); ekranKar->notify(); }
  }
}

const char *PC_KOMUT[] = {"ses+", "ses-", "sessiz", "oynat", "sonraki", "onceki", "kilitle", "ekran", "uyku"};
const char *PC_AD_TR[] = {"Ses +", "Ses -", "Sessiz", "Oynat/Durdur", "Sonraki", "Onceki", "Kilitle", "Ekran goruntusu", "Uyut (2 kez bas)"};
const char *PC_AD_EN[] = {"Volume +", "Volume -", "Mute", "Play/Pause", "Next", "Previous", "Lock", "Screenshot", "Sleep (press 2x)"};
void pcSayfasi(Olay o1, Olay o2) {                    // sağ: gez, sol: gönder; 3 sn'de bir durum ister
  static uint32_t sonIstek = 0, uykuOnay = 0;
  if (millis() - sonIstek > 3000 && bagli) { sonIstek = millis(); telefonaGonder("PC:durum", nullptr); }
  const int N = 12;
  String og[N]; for (int i = 0; i < 9; i++) og[i] = dil ? PC_AD_EN[i] : PC_AD_TR[i];
  og[9] = String(dl("Ekrani paylas: ", "Share screen: ")) + (ekranPaylas ? dl("acik", "on") : dl("kapali", "off"));
  og[10] = dl("Canli monitor", "Live monitor");
  og[11] = String(dl("Yakinlik kilidi: ", "Proximity lock: ")) + (ayarYakinlik ? dl("acik", "on") : dl("kapali", "off"));
  imlec = min(imlec, N - 1);
  if (o1 == KISA) imlec = (imlec + 1) % N;
  if (geriAdim) imlec = (imlec + N - 1) % N;
  if (o2 == KISA && imlec == 10) { git(PCMON_S); return; }
  if (o2 == KISA && imlec == 11) {
    ayarYakinlik = !ayarYakinlik; kayit.putInt("yakinlik", ayarYakinlik); telefonaGonder(ayarYakinlik ? "PC:yakinlik_ac" : "PC:yakinlik_kapat", nullptr);
    goster(ayarYakinlik ? dl("uzaklasinca PC kilitlenir", "PC locks when you leave") : dl("yakinlik kilidi kapali", "proximity lock off"), 2500); o2 = YOK;
  }
  if (o2 == KISA && imlec == 9) { ekranPaylas = !ekranPaylas; goster(ekranPaylas ? dl("PC: Kubit-Ekran ac", "PC: open Kubit-Ekran") : dl("paylasim kapandi", "sharing off"), 1500); }
  else if (o2 == KISA) {
    if (imlec == 8 && millis() - uykuOnay > 2000) { uykuOnay = millis(); goster(dl("Uyutmak icin tekrar bas", "Press again to sleep")); }
    else { telefonaGonder(String("PC:") + PC_KOMUT[imlec], nullptr); goster(og[imlec], 900); }
  }
  baslik(pc.ad.length() && millis() - pc.zaman < 10000 ? pc.ad.substring(0, 21) : String(dl("BILGISAYAR", "COMPUTER")));
  if (!bagli) yaz(0, 12, 1, ekranIstemci ? dl("PC Bluetooth bagli", "PC Bluetooth online") : dl("Telefon/PC bekleniyor", "Waiting for phone/PC"));
  else if (millis() - pc.zaman > 10000) { yaz(0, 12, 1, dl("PC araniyor...", "looking for PC...")); }
  else if (pc.cpuC >= 0 || pc.gpuC >= 0) { yaz(0, 12, 1, "CPU " + String(max(0, pc.cpuC)) + "C  GPU " + String(max(0, pc.gpuC)) + "C" + (pc.fps > 0 ? " " + String(pc.fps) + "f" : "")); }
  else { yaz(0, 12, 1, "CPU"); ekran.drawRect(22, 13, 36, 5, W); ekran.fillRect(23, 14, 34 * max(0, pc.cpu) / 100, 3, W);
         yaz(66, 12, 1, "RAM"); ekran.drawRect(88, 13, 36, 5, W); ekran.fillRect(89, 14, 34 * max(0, pc.ram) / 100, 3, W); }
  int ilk = constrain(imlec - 1, 0, N - 4);
  for (int i = ilk; i < ilk + 4; i++) { int y = 23 + (i - ilk) * 10; if (i == imlec) { ekran.fillRect(0, y - 1, 126, 10, W); ekran.setTextColor(B); } yaz(3, y, 1, og[i]); ekran.setTextColor(W); }
  ekran.fillRect(126, 22 + 40 * ilk / N, 2, 40 * 4 / N, W);
  altMesaj();
}

void sunumSayfasi(Olay o1, Olay o2) {                 // sağ: ileri, sol: geri, iki tuş: süre başlat/durdur (10 dk; 1 dk kala bip)
  static int slayt = 1; static uint32_t bas = 0, birikim = 0; static bool calisiyor = false, uyardi = false;
  const uint32_t HEDEF = 10 * 60000UL;
  static bool ikili = false;
  if (tus1.basili && tus2.basili) ikili = true;
  if (o1 == KISA && !ikili) { slayt++; telefonaGonder("PC:sunum_ileri", nullptr); }
  if (o2 == KISA && !ikili) { slayt = max(1, slayt - 1); telefonaGonder("PC:sunum_geri", nullptr); }
  if (ikili && !tus1.basili && !tus2.basili) { ikili = false; if (calisiyor) { birikim += millis() - bas; calisiyor = false; } else { bas = millis(); calisiyor = true; } }
  uint32_t gecen = birikim + (calisiyor ? millis() - bas : 0);
  if (calisiyor && gecen > HEDEF - 60000 && !uyardi) { uyardi = true; uyar(dl("1 dakika kaldi!", "1 minute left!")); }
  baslik(dl("SUNUM", "PRESENTATION"));
  yaz(0, 14, 1, dl("Slayt", "Slide")); yaz(8, 24, 3, String(slayt));
  yaz(64, 14, 1, dkSn(gecen)); ekran.drawRect(64, 26, 62, 6, W); ekran.fillRect(65, 27, min(60UL, 60 * gecen / HEDEF), 4, W);
  yaz(64, 36, 1, calisiyor ? dl("suruyor", "running") : dl("durdu", "paused"));
  ekran.drawFastHLine(0, 53, 128, W); yaz(0, 55, 1, dl("<geri ileri> 2tus:sure", "<back next> 2:timer"));
}

void kameraSayfasi(Olay o1, Olay o2) {                // sol: çek, sağ: 3 sn zamanlı, iki tuş: kamera çevir
  static uint32_t geriSayim = 0, acildi = 0; static bool ikili = false;
  if (!acildi || millis() - acildi > 60000) { acildi = millis(); telefonaGonder("KAMERA:AC", nullptr); }   // telefon kamerayı açık tutar
  if (tus1.basili && tus2.basili) ikili = true;
  if (ikili && !tus1.basili && !tus2.basili) { ikili = false; telefonaGonder("KAMERA:CEVIR", nullptr); o1 = o2 = YOK; }
  if (o2 == KISA && !ikili) { telefonaGonder("KAMERA:FOTO:0", nullptr); geriSayim = 0; goster("Klik!", 800); }
  if (o1 == KISA && !ikili) { telefonaGonder("KAMERA:FOTO:3", nullptr); geriSayim = millis() + 3000; }
  baslik(dl("KAMERA", "CAMERA"));
  ekran.drawRoundRect(34, 16, 60, 34, 5, W); ekran.drawCircle(64, 33, 11, W); ekran.drawCircle(64, 33, 6, W); ekran.fillRect(42, 13, 12, 4, W);
  if (geriSayim > millis()) { ekran.fillRect(50, 23, 28, 21, B); ortala(26, 2, String((geriSayim - millis()) / 1000 + 1)); }
  yaz(0, 55, 1, dl("<cek  3sn>  2tus:cevir", "<shot 3s>  2:flip"));
  altMesaj();
}
void kameraKapat() { telefonaGonder("KAMERA:KAPAT", nullptr); }

void yaziciSayfasi(Olay o2) {                         // sol: yenile
  static uint32_t son = 0;
  if ((o2 == KISA || millis() - son > 30000) && bagli) { son = millis(); telefonaGonder("YAZICI:YENILE", nullptr); }
  baslik(dl("3B YAZICI", "3D PRINTER"));
  if (!yazici.zaman) { yaz(0, 16, 1, bagli ? dl("Yaziciya baglaniyor...", "Connecting to printer...") : dl("telefon bagli degil", "phone not connected"));
    yaz(0, 32, 1, dl("Kubit uygulamasinda", "Set printer in the")); yaz(0, 41, 1, dl("yaziciyi ayarla", "Kubit app")); return; }
  const char *d = yazici.durum == "RUNNING" ? dl("BASILIYOR", "PRINTING") : yazici.durum == "FINISH" ? dl("BITTI", "DONE") : yazici.durum == "PAUSE" ? dl("DURAKLADI", "PAUSED")
                : yazici.durum == "FAILED" ? dl("HATA", "FAILED") : yazici.durum == "PREPARE" ? dl("HAZIRLANIYOR", "PREPARING") : dl("BOSTA", "IDLE");
  yaz(0, 12, 1, d); if (yazici.yuzde >= 0) { String y = String(yazici.yuzde) + "%"; yaz(128 - y.length() * 6, 12, 1, y); }
  ekran.drawRect(0, 22, 128, 8, W); ekran.fillRect(2, 24, 124 * max(0, yazici.yuzde) / 100, 4, W);
  yaz(0, 33, 1, yazici.dosya.substring(0, 21));
  if (yazici.durum == "RUNNING") yaz(0, 43, 1, dl("kalan ", "left ") + (yazici.kalan >= 60 ? String(yazici.kalan / 60) + dl("sa ", "h ") : String("")) + String(yazici.kalan % 60) + dl("dk", "m") + "  " + String(yazici.katman) + "/" + String(yazici.toplam));
  yaz(0, 55, 1, dl("nozul ", "nozzle ") + String(yazici.nozul) + "C  " + dl("tabla ", "bed ") + String(yazici.tabla) + "C");
}

// telefonu unutma: Kübit uyanıkken bağlantı beklenmedik koparsa 30 sn sonra uyarır (bir tuş kapatır)
uint32_t kopmaZaman = 0; bool unutmaUyardi = false;
void unutmaKontrol() {
  static bool oncekiBagli = false;
  if (oncekiBagli && !bagli) { kopmaZaman = millis(); unutmaUyardi = false; }
  if (bagli) kopmaZaman = 0;
  oncekiBagli = bagli;
  if (ayarUnutma && kopmaZaman && !unutmaUyardi && millis() - kopmaZaman > 30000) { unutmaUyardi = true; uyar(dl("Telefonun nerede?", "Where's your phone?")); }
}
void uykuOncesiHaber() { if (bagli) { tusKar->setValue("UYUYOR"); tusKar->notify(); delay(60); } }   // telefon "unuttun" sanmasın
