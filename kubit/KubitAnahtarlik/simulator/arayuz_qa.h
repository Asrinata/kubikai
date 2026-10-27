// Yalniz --qa derlemesinde: gercek firmware fonksiyonlariyla senaryolar ve durum denetimi.
SIM_EXPORT(sim_qa) int sim_qa(int senaryo, int adim) {
  geriAdim = false; ekran.clearDisplay(); mesajBitis = 0; sonDokunus = millis();
  switch (senaryo) {
    case 0: { // radar: secim, rota, iki detay sayfasi, bos liste
      bagli = true; git(RADAR_S);
      if (adim == 0) {
        ucakSatiri("3|0|THY1843|A321|11280|837|65|95|120|TC-JSU|4BAA73|200");
        ucakSatiri("3|1|PGT215|B738|8500|710|210|-180|55|TC-CPN|4B842E|-250");
        ucakSatiri("3|2|SXS34|B738|10100|794|320|230|-130|TC-SOE|4BCE01|0"); radarSecim(0);
        rotaSatiri("THY1843|IST|Istanbul|ATH|Atina|Turkish Airlines|Airbus A321");
      }
      if (adim == 1 || adim == 2) { radarDetay = true; radarBilgi = adim - 1; }
      if (adim == 3) ucakSatiri("0");
      radarSayfasi(YOK, YOK); ekran.display(); return adim == 3 ? ucaklar.empty() && radarSec == -1 && !radarDetay : ucaklar.size() == 3;
    }
    case 1: { // ISS: harita ve koordinatlari, eski veriyi isaretle
      git(UZAY_S); uzayMod = 1; bagli = adim != 3;
      if (!adim) { issSatiri("28.6|35.1|418|27612|1|41.0|29.0"); issYolSatiri("-10,-55;0,-35;15,-10;28,15;37,40;45,70;50,110;45,155;30,179;15,-165;0,-145"); }
      if (adim == 3) iss.zaman = millis() - 25000;
      uzaySayfasi(adim == 1 || adim == 2 ? KISA : YOK, YOK); ekran.display(); return iss.var;
    }
    case 2: { // Wi-Fi: kayit, yanlis sifre, asenkron baglanti, iptal
      git(WIFI_S); bagli = false;
      if (adim == 0) { wifiHal = 0; wifiSayfasi(YOK, YOK); }
      if (adim == 1) { WiFi.sonuc = true; wifiDene("Kubit_Test", "Example_123!"); wifiSayfasi(YOK, YOK); }
      if (adim == 2) { WiFi.sonuc = false; wifiDene("Yanlis_Ag", "Wrong_123!"); simVt += 15001; wifiSayfasi(YOK, YOK); }
      if (adim == 3) { WiFi.sonuc = false; wifiDene("Baska_Ag", "Cancel_123!"); wifiSayfasi(YOK, KISA); }
      if (adim == 4) { klavyeAc(WIFI_SIFRE); yazi = "A_b C!23"; klavyeSayfasi(YOK, YOK); }
      ekran.display();
      if (adim == 1) return wifiHal == 3 && WiFi.status() == WL_CONNECTED && wifiAdi() == "Kubit_Test";
      if (adim == 2) return wifiHal == 4 && wifiAdi() == "Kubit_Test" && wifiSifresi() == "Example_123!";
      if (adim == 3) return wifiHal == 1 && !wifiDenenenSifre.length() && WiFi.status() != WL_CONNECTED;
      return true;
    }
    case 3: { // web: okuma, linkler, uzun sayfa, URL harf duyarliligi, gecmis
      tara::aktif = true; tara::sayfa.hazirla(); tara::sonTus = millis();
      if (!adim) { tara::gezinme = 0; tara::sayfaYukle("https://ornek.test/Baslik?q=Buyuk"); }
      if (adim == 1) { tara::gezinme = 1; tara::sec = 1; tara::ust = constrain(tara::sayfa.satirBul(tara::sayfa.link[1].bas) - 1, 0, max(0, tara::sayfa.satirN - tara::IZ_SAT)); }
      if (adim == 2) { tara::gezinme = 0; tara::sayfaYukle("https://ornek.test/3"); tara::ust = 12; }
      if (adim == 3) { tara::sayfaYukle("https://ornek.test/2"); tara::geri(); }
      tara::sayfaCiz(); ekran.display();
      return adim != 3 || (tara::ust == 12 && !strcmp(tara::adres, "https://ornek.test/3"));
    }
    case 4: { // gecersiz BLE paketleri canli goruntuyu bozmamali
      float lat = iss.lat, lon = iss.lon; issSatiri("999|999|400|27000|1");
      bool issOk = iss.lat == lat && iss.lon == lon;
      int n = ucaklar.size(); ucakSatiri("999|0|X|A|1|2|3|4|5|R|H|0");
      return issOk && (int)ucaklar.size() == n;
    }
    case 5: { // web girisi kayit yoksa telefonsuz kurulum ekranina yonelmeli
      tara::aktif = false; WiFi.disconnect(); wifiAdOnbellek = ""; wifiAdOkundu = true;
      tara::calistir(false); return sayfa == WIFI_S && wifiDevam == 1 && !tara::aktif;
    }
    case 10: { // hayatta kalma, uretim ve envanter
      tara::aktif = false;
      mc::uret(12345); mc::hal = 1; mc::menuYut = false;
      mc::kamX = constrain((mc::o.x + mc::OW / 2) * 8 - 64, 0.0f, mc::DW * 8 - 128.0f); mc::kamY = (mc::o.y + mc::OH / 2) * 8 - 34;
      if (adim == 0) { mc::ciz(0); ekran.display(); return !mc::yaratici && !mc::bitirdi && mc::can == mc::CAN_MAX && mc::env[mc::TAHTA] == 0; }
      mc::hal = 3; mc::imlec = 0; mc::env[mc::KUTUK] = 2;
      Girdi g = {YOK, KISA, false, false, false, false, 0, 0, .05f};
      mc::oyna(g); mc::oyna(g);
      if (adim == 1) { ekran.display(); return mc::env[mc::KUTUK] == 0 && mc::env[mc::TAHTA] == 8 && (mc::gorevler & 3) == 3; }
      mc::hal = 5; mc::imlec = 0; g.o2 = YOK; mc::oyna(g); ekran.display(); return true;
    }
    case 11: { // kazma gereksinimi, ana kaya
      mc::uret(2468); mc::kir.aktif = false;
      if (adim == 0) { mc::koyBlok(12, 22, mc::ANAKAYA); mc::kirBasla(12, 22); return !mc::kir.aktif && mc::d[22][12] == mc::ANAKAYA; }
      mc::koyBlok(12, 22, mc::TAS); mc::kirBasla(12, 22); mc::kirBitir();
      if (mc::env[mc::TAS] != 0) return false;
      mc::env[mc::KAZMA_A] = 1; mc::koyBlok(12, 22, mc::TAS); mc::kirBasla(12, 22); mc::kirBitir();
      return mc::env[mc::TAS] == 1 && mc::d[22][12] == mc::HAVA;
    }
    case 12: { // kayit turu ve bozuk bolgeyi canli dunyaya uygulamama
      mc::uret(777); mc::env[mc::ELMAS] = 7; mc::gorevler = 85; mc::gunSayisi = 4; mc::koyBlok(20, 10, mc::TAHTA);
      if (!mc::kaydet()) return false;
      if (adim == 0) { mc::env[mc::ELMAS] = 0; mc::gorevler = 0; mc::d[10][20] = mc::HAVA; return mc::yukle() && mc::env[mc::ELMAS] == 7 && mc::gorevler == 85 && mc::gunSayisi == 4 && mc::d[10][20] == mc::TAHTA; }
      uint8_t paket[4096]; size_t n = kayit.getBytes("mc_b2", paket, sizeof(paket)); kayit.remove("mc_b2");
      mc::d[1][1] = mc::MESALE; bool reddetti = !mc::yukle() && mc::d[1][1] == mc::MESALE;
      kayit.putBytes("mc_b2", paket, n); return reddetti;
    }
    case 13: { // yaratici mod: can dusmez ve dunya kaydi modu korur
      mc::baslat(false, true); mc::menuYut = false;
      int once = mc::can; mc::hasar(999, "test"); mc::cv[0].var = true; mc::canavarlar(.05f);
      bool iyi = mc::yaratici && mc::can == once && !mc::cv[0].var && mc::env[mc::TAHTA] == 999;
      mc::ciz(0); ekran.display(); if (!mc::kaydet()) return false; mc::yaratici = false; return iyi && mc::yukle() && mc::yaratici;
    }
    case 14: { // menuye giris basisi secim veya blok koymaya donusmemeli
      mc::uret(123); mc::hal = 1; mc::menuYut = mc::menuAcildi = false;
      Girdi g = {YOK, YOK, true, true, false, false, 0, 1001, .05f}; mc::oyna(g);
      if (mc::hal != 2) return false;
      g = {YOK, YOK, false, false, false, true, 1001, 0, .05f}; mc::oyna(g);
      if (mc::hal != 2 || mc::imlec != 0) return false;
      g = {YOK, KISA, false, false, false, false, 0, 0, .05f}; mc::oyna(g);
      return mc::hal == 1 && mc::menuYut;
    }
    case 15: { // sinyal hedefinin kaydi ve kazanan ekran
      mc::uret(456); memset(mc::d, mc::HAVA, sizeof(mc::d));
      for (int x = 0; x < mc::DW; x++) mc::koyBlok(x, 20, mc::TAS);
      mc::o = {128, 20 - mc::OH, 0, 1, true}; mc::dogX = 128; mc::dogY = mc::o.y;
      mc::env[mc::ELMAS] = 3; mc::env[mc::DEMIR] = 8; mc::env[mc::MESALE] = 4;
      mc::sinyalYak();
      if (!mc::bitirdi || mc::hal != 9 || mc::env[mc::ELMAS] || mc::env[mc::DEMIR] || mc::env[mc::MESALE]) return false;
      Girdi g = {YOK, YOK, false, false, false, false, 0, 0, .05f}; mc::oyna(g); ekran.display();
      mc::bitirdi = false; return mc::yukle() && mc::bitirdi && (mc::gorevler & 128);
    }
    case 16: { // bitis maliyeti eksikse kaynak tuketme
      mc::uret(456); mc::env[mc::ELMAS] = 2; mc::env[mc::DEMIR] = 8; mc::env[mc::MESALE] = 4; mc::sinyalYak();
      return !mc::bitirdi && mc::env[mc::ELMAS] == 2 && mc::env[mc::DEMIR] == 8;
    }
    case 17: { // yardim sayfalari, gorev ve baslik gorunumleri
      Girdi g = {YOK, YOK, false, false, false, false, 0, 0, .05f}; mc::menuYut = false;
      if (adim < 5) { mc::hal = 7; mc::imlec = adim; }
      else if (adim == 5) { mc::hal = 6; mc::imlec = 3; }
      else { mc::hal = 0; mc::imlec = 1; }
      mc::oyna(g); ekran.display(); return true;
    }
    case 18: { // dunyayi sifirlama onayindan vazgec
      mc::uret(888); mc::kaydet(); uint32_t tohum = mc::dunyaTohum;
      mc::hal = 8; mc::imlec = 0; mc::menuYut = false;
      Girdi g = {YOK, KISA, false, false, false, false, 0, 0, .05f}; mc::oyna(g);
      return mc::hal == 0 && mc::dunyaTohum == tohum;
    }
    case 20: { // telefonsuz PC ekran aboneligi ve firmware'den gercek PackBits paketleri
      bagli = false; ekranIstemci = false; NimBLEConnInfo pcInfo; pcInfo.handle = 1;
      ekranKar->cb->onSubscribe(ekranKar, pcInfo, 1); ekranPaylas = true;
      for (int y = 0; y < 64; y++) for (int x = 0; x < 128; x++) if ((x + y) % 3 == 0) ekran.drawPixel(x, y, W);
      simVt += 201; ekranGonder(); ekran.display(); return !bagli && ekranIstemci;
    }
    case 21: { // PC ayrilirken telefon baglantisi korunmali
      NimBLEConnInfo telInfo, pcInfo; pcInfo.handle = 1;
      tusKar->cb->onSubscribe(tusKar, telInfo, 1); ekranKar->cb->onSubscribe(ekranKar, pcInfo, 1);
      simSunucu->cb->onDisconnect(simSunucu, pcInfo, 0);
      return bagli && !ekranIstemci && baglantiNo == 0;
    }
    case 22: { // telefon ayrilirken dogrudan ekran korunmali
      NimBLEConnInfo telInfo, pcInfo; pcInfo.handle = 1;
      tusKar->cb->onSubscribe(tusKar, telInfo, 1); ekranKar->cb->onSubscribe(ekranKar, pcInfo, 1);
      simSunucu->cb->onDisconnect(simSunucu, telInfo, 0);
      return !bagli && ekranIstemci && ekranBaglantiNo == 1;
    }
    case 23: { ekranPaylas = false; simVt += 201; ekranGonder(); return true; }
    case 30: {
      tara::aktif = false; waAc(); memset(&waRehber, 0, sizeof waRehber); waRehber.sihir = 0x57413235;
      waDuzen = -1; waKlavyeBitti(WA_AD, "Test Kisi");
      if (sayfa != KLAVYE || klavyeHedef != WA_NO) return false;
      waKlavyeBitti(WA_NO, "+15550000000"); waYukle();
      return waRehber.adet == 1 && String(waRehber.kisi[0].no) == "15550000000" && String(waRehber.kisi[0].ad) == "Test Kisi";
    }
    case 31: {
      waDuzen = -1; waKlavyeBitti(WA_AD, "Ayni Kisi"); waKlavyeBitti(WA_NO, "15550000000");
      if (waRehber.adet != 1) return false;
      waDuzen = -1; waKlavyeBitti(WA_AD, "Gecersiz"); waKlavyeBitti(WA_NO, "05550000000");
      return waRehber.adet == 1 && waNumara("12a3456789").length() == 0;
    }
    case 32: {
      waIstemci = waPCHazir = true; bagli = false; waSec = 0; waYol = waGonderYol = 1; waTaslak = "Merhaba | Kubit testi"; waBekliyor = false;
      waGonder();
      if (!waBekliyor || waHal != 5 || waIs == "0") return false;
      waSonuc("99999999999|SENT|yanlis is"); if (!waBekliyor) return false;
      waSonuc(waIs + "|SENT|WhatsApp a iletildi");
      waSayfasi(YOK, YOK); ekran.display(); return !waBekliyor && waKod == "SENT";
    }
    case 33: {
      waYol = waGonderYol = 2; bagli = true; waTaslak = "Telefonda taslak"; waBekliyor = false; waGonder();
      waSonuc(waIs + "|READY|Telefonda Gonder e bas");
      waSayfasi(YOK, YOK); ekran.display(); return waKod == "READY" && !waBekliyor;
    }
    case 34: {
      waBekliyor = false; waHal = 3; waGonderYol = waYol = 1; imlec = 0; waTaslak = "";
      for (int i = 0; i < 120; i++) waTaslak += (char)('a' + i % 26);
      int n = max(1, ((int)sar(waTaslak, 21).size() + 2) / 3);
      for (int i = 0; i < n; i++) { waSayfasi(YOK, KISA); if (waBekliyor) return false; }
      waSayfasi(YOK, KISA); return waHal == 1 && !waBekliyor;
    }
    case 35: {
      waGonderYol = waYol = 1; waPCHazir = waIstemci = true; waBekliyor = false; waTaslak = "Sonuc testi"; waGonder(); simVt += 30001;
      waSayfasi(YOK, YOK); if (waBekliyor || waKod != "UNKNOWN") return false;
      waSonuc(waIs + "|SENT|Gec gelen kesin sonuc"); return waKod == "SENT";
    }
    case 36: {
      // Gercek kontrol donusturucusu: iki tus tutulunca yazar/siler; birakista fazladan karakter yok.
      tus1.basili = tus2.basili = false; Olay a = YOK, b = YOK; yeniKontrol(a, b, true, false, true);
      klavyeAc(WA_METIN); yazi = adim == 1 ? "sil" : ""; imlec = adim == 1 ? 1 : 3;
      auto adimla = [&]() { Olay x = YOK, y = YOK; Kontrol k = yeniKontrol(x, y, true, false, true); if (k.sec) y = KISA; geriAdim = k.onceki; klavyeSayfasi(x, y); return k; };
      tus1.basili = tus2.basili = true; adimla();
      for (int i = 0; i < 7; i++) { simVt += i == 0 ? 450 : 160; Kontrol k = adimla(); if (k.geri || sayfa != KLAVYE) return false; }
      int n = yazi.length(); tus1.basili = tus2.basili = false; adimla();
      ekran.display(); return adim == 1 ? n == 0 && yazi.length() == 0 : n == 7 && yazi.length() == 7;
    }
    case 37: {
      tus1.basili = tus2.basili = false; Olay a = YOK, b = YOK; yeniKontrol(a, b, true, false, false);
      tus1.basili = tus2.basili = true; yeniKontrol(a, b, true, false, false); simVt += 1001;
      Kontrol k = yeniKontrol(a, b, true, false, false); tus1.basili = tus2.basili = false; yeniKontrol(a, b, true, false, false);
      return k.geri;
    }
    case 38: {
      waHal = 0; imlec = 0; git(WHATSAPP_S); waSayfasi(YOK, YOK); ekran.display(); return true;
    }
    case 39: {
      waHal = 3; waSec = 0; waTaslak = "Kayitli kisiden mesaj. Numara ve mesaji gor, sonra onayla."; waGonderYol = 1; imlec = adim;
      waSayfasi(YOK, YOK); ekran.display(); return true;
    }
  }
  return false;
}
