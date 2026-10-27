// Kayitli kisiler ve onayli mesaj. PC: ayri BLE kanali; telefon: WhatsApp taslagi.
#pragma once
struct WaKisi { char ad[13], no[17]; };
struct WaRehber { uint32_t sihir; uint8_t adet; WaKisi kisi[12]; } waRehber;
int waHal = 0, waSec = 0, waDuzen = -1, waYol = 0, waGonderYol = 0;
String waYeniAd, waTaslak, waIs, waDurumYazi, waKod;
uint32_t waBaslangic = 0;
bool waBekliyor = false;

String waNumara(const String &g) {
  String n;
  for (unsigned i = 0; i < g.length(); i++) { char c = g[i]; if (c >= '0' && c <= '9') n += c; else if (c == '+' && i == 0) {} else if (c != ' ' && c != '-') return ""; }
  if (n.length() < 8 || n.length() > 15 || n[0] == '0') return "";
  return n;
}
void waYukle() {
  WaRehber b;
  bool ok = kayit.getBytes("wa_rehber", &b, sizeof b) == sizeof b && b.sihir == 0x57413235 && b.adet <= 12;
  if (ok) for (int i = 0; i < b.adet; i++) {
    if (!memchr(b.kisi[i].ad, 0, 13) || !memchr(b.kisi[i].no, 0, 17)) { ok = false; break; }
    if (!b.kisi[i].ad[0] || waNumara(b.kisi[i].no) != String(b.kisi[i].no)) { ok = false; break; }
  }
  if (ok) waRehber = b; else { memset(&waRehber, 0, sizeof waRehber); waRehber.sihir = 0x57413235; }
  waYol = constrain(kayit.getInt("wa_yol", 0), 0, 2);
}
bool waKaydet(const WaRehber &r) {
  if (kayit.putBytes("wa_rehber", &r, sizeof r) != sizeof r) { goster("Kisi kaydedilemedi"); return false; }
  waRehber = r; return true;
}
void waAc() { waYukle(); waHal = 0; waSec = 0; waDuzen = -1; }
void waGeri() {
  if (sayfa == KLAVYE) { git(WHATSAPP_S); waHal = waDuzen < 0 && klavyeHedef != WA_METIN ? 0 : 1; return; }
  if (waHal == 0) { git(ARACLAR); imlec = 15; }
  else { waHal = waHal == 1 ? 0 : 1; imlec = 0; }
}
void waKlavyeBitti(Hedef h, const String &v) {
  git(WHATSAPP_S);
  if (h == WA_METIN) { waTaslak = v.substring(0, 120); waHal = 1; return; }
  if (h == WA_AD) {
    waYeniAd = ascii(v); waYeniAd.trim();
    if (!waYeniAd.length()) { waHal = 0; goster("Kisi adi bos olamaz"); return; }
    klavyeAc(WA_NO); if (waDuzen >= 0) yazi = waRehber.kisi[waDuzen].no; return;
  }
  String n = waNumara(v);
  if (!n.length()) { waHal = 0; goster("Ulke koduyla yaz: +90"); return; }
  WaRehber b = waRehber;
  for (int i = 0; i < b.adet; i++) if (i != waDuzen && n == String(b.kisi[i].no)) { waHal = 0; goster("Bu numara kayitli"); return; }
  int s = waDuzen < 0 ? b.adet : waDuzen;
  if (s >= 12) { waHal = 0; goster("Rehber dolu: 12 kisi"); return; }
  memset(&b.kisi[s], 0, sizeof(WaKisi));
  strncpy(b.kisi[s].ad, waYeniAd.c_str(), 12); strncpy(b.kisi[s].no, n.c_str(), 16);
  if (waDuzen < 0) b.adet++;
  waHal = 0; if (waKaydet(b)) { waSec = s; waHal = 1; waTaslak = ""; goster("Kisi kaydedildi"); }
}
void waSonuc(const String &g) {
  auto p = parcala(g, '|'); if (p.size() < 3) return;
  if (p[0] == "0") { waPCHazir = p[1] == "READY" && waIstemci; return; }
  if (p[0] != waIs || (!waBekliyor && !(waKod == "UNKNOWN" && p[1] == "SENT"))) return;
  waKod = p[1]; waDurumYazi = ascii(p[2]).substring(0, 100); waBekliyor = false;
}
int waAktifYol() {
  if (waYol == 1) return waIstemci && waPCHazir ? 1 : 0;
  if (waYol == 2) return bagli ? 2 : 0;
  if (waIstemci && waPCHazir) return 1;
  return bagli ? 2 : 0;
}
void waGonder() {
  if (waBekliyor) { goster("Onceki sonuc bekleniyor"); return; }
  if (waSec < 0 || waSec >= waRehber.adet || !waTaslak.length()) { goster("Once kisi ve mesaj sec"); return; }
  if (!waGonderYol || waGonderYol != waAktifYol()) { goster("Secilen baglanti yok"); return; }
  waIs = String((uint32_t)(esp_random() | 1)); waKod = "WAIT"; waDurumYazi = "Sonuc bekleniyor";
  String cmd = String(waGonderYol == 1 ? "WA1:" : "WA:HAZIRLA:") + waIs + "|" + waRehber.kisi[waSec].no + "|" + waTaslak;
  if (cmd.length() > 180) { goster("Mesaj cok uzun"); return; }
  waBekliyor = true; waBaslangic = millis(); waHal = 5;
  if (waGonderYol == 1) { waKar->setValue(cmd.c_str()); waKar->notify(); }
  else telefonaGonder(cmd, nullptr);
}
const char *WA_HAZIR[] = {"Merhaba", "Yoldayim", "5 dakikaya geliyorum", "Beni arar misin?", "Tamam", "Tesekkurler", "Su an mesgulum", "Geri"};
void waSayfasi(Olay o1, Olay o2) {
  if (waBekliyor && millis() - waBaslangic > 30000) { waBekliyor = false; waKod = "UNKNOWN"; waDurumYazi = "Sonuc belirsiz. Tekrar gondermeden once kontrol et."; }
  if (waHal == 0) {
    String og[14]; int n = waRehber.adet; for (int i = 0; i < n; i++) og[i] = waRehber.kisi[i].ad;
    og[n] = "+ Yeni kisi"; og[n + 1] = String("Yol: ") + (waYol == 0 ? "otomatik" : waYol == 1 ? "PC (telefonsuz)" : "telefon");
    int s = liste(waPCHazir ? "WHATSAPP PC HAZIR" : "WHATSAPP / KISILER", og, n + 2, o1, o2);
    if (s < 0) return;
    if (s < n) { waSec = s; waHal = 1; waTaslak = ""; imlec = 0; }
    else if (s == n) { if (n >= 12) goster("Rehber dolu: 12 kisi"); else { waDuzen = -1; klavyeAc(WA_AD); } }
    else { waYol = (waYol + 1) % 3; kayit.putInt("wa_yol", waYol); }
    return;
  }
  if (waHal == 1) {
    String og[] = {"Mesaj yaz", "Hazir mesaj sec", waTaslak.length() ? "Mesaji incele..." : "(Mesaj yazilmadi)", "Kisiyi duzenle", "Kisiyi sil", "Geri"};
    int s = liste(waRehber.kisi[waSec].ad, og, 6, o1, o2);
    if (s == 0) { klavyeAc(WA_METIN); yazi = waTaslak; }
    if (s == 1) { waHal = 2; imlec = 0; }
    if (s == 2) { if (!waTaslak.length()) { goster("Once mesaj yaz"); return; } waGonderYol = waAktifYol(); if (!waGonderYol) goster(waIstemci ? "PC'de WhatsApp'i esle" : "PC veya telefon bagla"); else { waHal = 3; imlec = 0; } }
    if (s == 3) { waDuzen = waSec; klavyeAc(WA_AD); yazi = waRehber.kisi[waSec].ad; }
    if (s == 4) { waHal = 4; imlec = 0; }
    if (s == 5) { waHal = 0; imlec = waSec; }
    return;
  }
  if (waHal == 2) {
    String og[8]; for (int i = 0; i < 8; i++) og[i] = WA_HAZIR[i]; int s = liste("HAZIR MESAJ", og, 8, o1, o2);
    if (s >= 0) { if (s < 7) waTaslak = WA_HAZIR[s]; waHal = 1; imlec = 2; } return;
  }
  if (waHal == 3 || waHal == 4) {
    if (waHal == 3) {
      auto sat = sar(waTaslak, 21); int n = max(1, ((int)sat.size() + 2) / 3);
      if (o1 == KISA) imlec = (imlec + 1) % (n + 2);
      if (geriAdim) imlec = (imlec + n + 1) % (n + 2);
      baslik(waGonderYol == 1 ? "PC / WHATSAPP" : "TELEFON / TASLAK");
      yaz(0, 12, 1, "+" + String(waRehber.kisi[waSec].no));
      int ilk = min(imlec, n - 1) * 3;
      for (int i = 0; i < 3 && ilk + i < (int)sat.size(); i++) yaz(0, 23 + i * 9, 1, sat[ilk + i]);
      ekran.fillRect(0, 54, 128, 10, B);
      yaz(0, 56, 1, imlec < n ? String(imlec + 1) + "/" + n + " 2t:sonraki" : imlec == n ? "2t:VAZGEC sag:onay" : waGonderYol == 1 ? "2t:GONDER sol:vazgec" : "2t:TELEFONDA HAZIRLA");
      if (o2 == KISA) { if (imlec < n) imlec++; else if (imlec == n) { waHal = 1; imlec = 0; } else waGonder(); }
      return;
    }
    if (o1 == KISA || geriAdim) imlec = !imlec;
    baslik(waHal == 4 ? "KISI SILINSIN MI?" : waGonderYol == 1 ? "PC'DEN GONDER?" : "TELEFONDA AC?");
    yaz(0, 12, 1, String(waRehber.kisi[waSec].ad) + " +" + String(waRehber.kisi[waSec].no).substring(0, 6));
    auto sat = sar(waHal == 4 ? String(waRehber.kisi[waSec].no) : waTaslak, 21);
    for (int i = 0; i < (int)sat.size() && i < 3; i++) yaz(0, 23 + i * 9, 1, sat[i]);
    ekran.fillRect(0, 54, 128, 10, B); yaz(0, 56, 1, imlec ? (waHal == 4 ? "  Geri   >SIL" : waGonderYol == 1 ? "  Geri   >GONDER" : "  Geri   >HAZIRLA") : ">Geri     Onayla");
    if (o2 == KISA) {
      if (!imlec) { waHal = 1; imlec = 0; }
      else if (waHal == 3) waGonder();
      else { WaRehber b = waRehber; for (int i = waSec; i + 1 < b.adet; i++) b.kisi[i] = b.kisi[i + 1]; b.adet--; memset(&b.kisi[b.adet], 0, sizeof(WaKisi)); if (waKaydet(b)) { waHal = 0; imlec = 0; waTaslak = ""; } }
    }
    return;
  }
  baslik(waBekliyor ? "WHATSAPP BEKLENIYOR" : waKod == "SENT" ? "WHATSAPP'A ILETILDI" : waKod == "READY" ? "TELEFONDA HAZIR" : waKod == "UNKNOWN" ? "SONUC BELIRSIZ" : "WHATSAPP SONUCU");
  auto sat = sar(waDurumYazi, 21); for (int i = 0; i < (int)sat.size() && i < 4; i++) yaz(0, 13 + i * 9, 1, sat[i]);
  yaz(0, 56, 1, waKod == "SENT" ? "Teslim bilgisi yok" : "2 tus: geri");
  if (o2 == KISA) { waHal = 1; imlec = 0; }
}
