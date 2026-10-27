// Cihaz ustunden Wi-Fi: asenkron tarama, gizli SSID, sifre, kayitli aga yeniden baglanma.
// Basarili baglanti acik kalir; yanlis sifre kayitli agin ustune yazilmaz.
#pragma once
struct Ag { String ad; int8_t guc; bool acik; };
std::vector<Ag> aglar;
int wifiHal = 0;  // 0: taramayi baslat, 1: liste, 2: baglaniyor, 3: bagli, 4: hata, 5: taraniyor
String wifiSecilen, wifiDenenenSifre, wifiHata;
uint32_t wifiBaslangic = 0;
String wifiAdOnbellek; bool wifiAdOkundu = false;

String wifiAdi() {
  if (!wifiAdOkundu) { Preferences a; a.begin("kubit", true); wifiAdOnbellek = a.getString("wifi_ad", ""); a.end(); wifiAdOkundu = true; }
  return wifiAdOnbellek;
}
String wifiSifresi() { Preferences a; a.begin("kubit", true); String s = a.getString("wifi_sifre", ""); a.end(); return s; }
void wifiKaydet(const String &ad, const String &sifre) {
  Preferences a; a.begin("kubit", false); a.putString("wifi_ad", ad); a.putString("wifi_sifre", sifre); a.end();
  wifiAdOnbellek = ad; wifiAdOkundu = true;
}
void wifiEkranAc() { wifiHal = WiFi.status() == WL_CONNECTED ? 3 : 0; }
void wifiVazgec() {
  if (wifiHal == 2) WiFi.disconnect();
  if (wifiHal == 5) WiFi.scanDelete();
  wifiDenenenSifre = ""; wifiDevam = 0;
}
void wifiDene(const String &ad, const String &sifre) {
  if (!ad.length() || ad.length() > 32 || sifre.length() > 63 || (sifre.length() && sifre.length() < 8)) {
    wifiHata = dl("Ag adi veya sifre hatali. Sifre 8-63 karakter.", "Invalid SSID/password. Password: 8-63 chars."); wifiHal = 4; imlec = 0; return;
  }
  wifiSecilen = ad; wifiDenenenSifre = sifre;
  WiFi.mode(WIFI_STA); WiFi.disconnect(); WiFi.begin(ad.c_str(), sifre.c_str());
  wifiBaslangic = millis(); wifiHal = 2; imlec = 0; sonDokunus = millis();
}
void wifiDevamEt() {
  int hedef = wifiDevam; wifiDevam = 0;
  if (hedef == 1 || hedef == 2) tara::calistir(hedef == 2);
  else if (hedef == 3) guncelle();
}
void wifiSayfasi(Olay o1, Olay o2) {
  if (wifiHal == 0 || wifiHal == 2 || wifiHal == 5) sonDokunus = millis();
  if (wifiHal == 2) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiKaydet(wifiSecilen, wifiDenenenSifre); wifiDenenenSifre = ""; wifiHal = 3; imlec = 0;
      if (wifiDevam) { wifiDevamEt(); return; }
    } else if (millis() - wifiBaslangic >= 15000) {
      WiFi.disconnect(); wifiDenenenSifre = ""; wifiHata = dl("Baglanamadi. Sifreyi ve 2.4 GHz agi kontrol et.", "Failed. Check password and 2.4 GHz network."); wifiHal = 4; imlec = 0;
    } else {
      baslik(dl("WI-FI BAGLANIYOR", "WI-FI CONNECTING")); yaz(0, 15, 1, ascii(wifiSecilen).substring(0, 21));
      yaz(0, 29, 1, String((millis() - wifiBaslangic) / 1000) + "/15 sn");
      ekran.drawRect(0, 41, 128, 6, W); ekran.fillRect(2, 43, 124 * (millis() - wifiBaslangic) / 15000, 2, W);
      yaz(0, 55, 1, dl("2 tus: iptal", "both: cancel"));
      if (o2 == KISA) { wifiVazgec(); wifiHal = 1; imlec = 0; }
      return;
    }
  }
  if (wifiHal == 3) {
    if (WiFi.status() != WL_CONNECTED) { wifiHal = 4; wifiHata = dl("Wi-Fi baglantisi koptu", "Wi-Fi disconnected"); imlec = 0; }
    else {
      String og[] = {dl("Tarayiciyi ac", "Open browser"), dl("YouTube'u ac", "Open YouTube"), dl("Baska ag sec", "Choose network"), dl("Baglantiyi kes", "Disconnect")};
      int s = liste("WI-FI " + ascii(wifiAdi()).substring(0, 14), og, 4, o1, o2);
      String durum = dl("BAGLI ", "ONLINE ");
#ifndef SIMULATOR
      durum += WiFi.localIP().toString();
#else
      durum += "192.0.2.10";
#endif
      ekran.fillRect(0, 55, 128, 9, B); yaz(0, 56, 1, durum.substring(0, 21));
      if (s == 0 || s == 1) tara::calistir(s == 1);
      else if (s == 2) { wifiHal = 0; imlec = 0; }
      else if (s == 3) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); wifiHal = 1; imlec = 0; }
      return;
    }
  }
  if (wifiHal == 4) {
    baslik(dl("WI-FI BAGLANAMADI", "WI-FI FAILED")); auto sat = sar(wifiHata, 21);
    for (int i = 0; i < (int)sat.size() && i < 3; i++) yaz(0, 13 + i * 9, 1, sat[i]);
    yaz(0, 45, 1, dl("sag: aglari gor", "R: networks")); yaz(0, 56, 1, dl("2 tus: sifre duzelt", "both: edit password"));
    if (o1 == KISA || geriAdim) { wifiHal = 1; imlec = 0; }
    else if (o2 == KISA) klavyeAc(WIFI_SIFRE);
    return;
  }
  if (wifiHal == 0) {
    aglar.clear(); WiFi.mode(WIFI_STA); WiFi.scanNetworks(true); wifiBaslangic = millis(); wifiHal = 5;
  }
  if (wifiHal == 5) {
    int n = WiFi.scanComplete();
    if (n == -1 && millis() - wifiBaslangic < 12000) {
      baslik("WI-FI  2.4 GHz"); yaz(0, 19, 1, dl("Aglar taraniyor...", "Scanning networks...")); yaz(0, 55, 1, dl("2 tus: iptal", "both: cancel"));
      if (o2 == KISA) { WiFi.scanDelete(); wifiHal = 1; imlec = 0; } return;
    }
    for (int i = 0; i < n && aglar.size() < 12; i++) {
      String s = WiFi.SSID(i); bool var = !s.length(); for (auto &a : aglar) if (a.ad == s) var = true;
      if (!var) aglar.push_back({s, (int8_t)WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN});
    }
    WiFi.scanDelete(); wifiHal = 1; imlec = 0;
  }
  String og[16]; int n = 0; String kayitli = wifiAdi();
  for (auto &a : aglar) {
    String s = (a.ad == kayitli ? "*" : " ") + ascii(a.ad).substring(0, 14);
    while (s.length() < 15) s += ' ';
    og[n++] = s + (a.acik ? " " : "#") + (a.guc > -60 ? "|||" : a.guc > -75 ? "||" : "|");
  }
  int tekrar = n; og[n++] = dl("Yeniden tara", "Rescan"); int gizli = n; og[n++] = dl("Gizli ag / SSID yaz", "Hidden network / SSID");
  int kayitliNo = n; if (kayitli.length()) og[n++] = dl("Kayitli aga baglan", "Connect saved network");
  int s = liste(aglar.size() ? "WI-FI  2.4 GHz" : dl("WI-FI: ag bulunamadi", "WI-FI: no networks"), og, n, o1, o2);
  altMesaj(); if (s < 0) return;
  if (s == tekrar) { wifiHal = 0; return; }
  if (s == gizli) { klavyeAc(WIFI_AD); return; }
  if (s == kayitliNo) { wifiDene(kayitli, wifiSifresi()); return; }
  wifiSecilen = aglar[s].ad;
  if (aglar[s].acik) wifiDene(wifiSecilen, "");
  else if (wifiSecilen == kayitli) wifiDene(wifiSecilen, wifiSifresi());
  else klavyeAc(WIFI_SIFRE);
}
