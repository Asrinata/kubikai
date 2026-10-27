// Otomatik hata raporu: çökme (panic / watchdog), donma (bekçi görevi: ana döngü 20 sn durursa) ve düşük pil (brownout) sonrası
// açılışta ekranda kısa "CRASH REPORTER" çıkar, rapor NVS kuyruğuna (en çok 5) yazılır. Telefon bağlanınca X satırlarıyla gider
// (X+<id>, X<satır>..., X.<id>); telefon "x<id>" ile alındı deyince silinir. Telefon raporu PC ajanına iletir (masaüstü klasörü).
#pragma once
#ifndef SIMULATOR
#include "esp_core_dump.h"
#include "esp_system.h"
#endif

struct RtcIz { uint32_t sihir, sure, bellek, nabiz; uint16_t sayfa, oyun; char not_[16]; };
RTC_NOINIT_ATTR RtcIz rtcIz;                          // çökmede silinmez (yalnız güç kesilince)
const uint32_t RTC_SIHIR = 0x4B554249;               // "KUBI"
volatile bool uzunIs = false;                        // güncelleme gibi bilerek uzun süren işlerde bekçi beklesin
uint32_t hataGonderZaman = 0; int hataGonderilen = -1;

void izGuncelle() {                                  // her döngüde: çökerse nerede olduğumuzu bilelim
  rtcIz.sihir = RTC_SIHIR; rtcIz.sayfa = sayfa; rtcIz.oyun = sayfa == OYUN ? oyunNo : 0xFFFF;
  rtcIz.sure = millis(); rtcIz.bellek = ESP.getFreeHeap(); rtcIz.nabiz = millis();
}
bool donmaVar() { return !uzunIs && rtcIz.sihir == RTC_SIHIR && millis() > 30000 && millis() - rtcIz.nabiz > 20000; }   // bekçi görevi kontrol eder
void donmaKaydet() { strncpy(rtcIz.not_, "DONMA", sizeof(rtcIz.not_)); }

String sayfaAdi(int s, int o) {
  if (s == OYUN && o < OYUN_N) return String("OYUN: ") + OYUN_LISTE[o].ad;
  const char *AD[] = {"ANA", "MENU", "SAAT", "HAVA", "SOR", "KLAVYE", "CEVAP", "OZET", "BILDIRIM", "MUZIK", "BUL", "FB_SEC", "FLAPPY", "GD_SEC", "GD", "RC_MOD", "RC_PIST", "RC_ARABA",
                      "RC_YARIS", "SANS", "ZAR", "YAZITURA", "CARK", "KUMANDA", "ONAY", "AYAR", "OYUNLAR", "OYUN"};
  return s < (int)(sizeof(AD) / sizeof(*AD)) ? String(AD[s]) : "sayfa " + String(s);
}
// açılışta: anormal yeniden başlamaysa rapor yaz, kısa ekran göster
void hataKontrol() {
  int neden = 0; String nedenAd;
#ifndef SIMULATOR
  esp_reset_reason_t r = esp_reset_reason();
  bool donma = rtcIz.sihir == RTC_SIHIR && !strcmp(rtcIz.not_, "DONMA");
  if (r == ESP_RST_PANIC) { neden = 1; nedenAd = "PANIC (program hatasi)"; }
  else if (r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT || r == ESP_RST_WDT) { neden = 2; nedenAd = "WATCHDOG (gorev takildi)"; }
  else if (r == ESP_RST_BROWNOUT) { neden = 3; nedenAd = "BROWNOUT (pil/gerilim dustu)"; }
  else if (donma) { neden = 4; nedenAd = "DONMA (ana dongu 20 sn durdu)"; }
#endif
  if (!neden) { rtcIz.not_[0] = 0; return; }
  String r2 = "Kubit hata raporu\nSurum: " + String(SURUM) + "\nNeden: " + nedenAd + "\n";
  if (rtcIz.sihir == RTC_SIHIR) r2 += "Son sayfa: " + sayfaAdi(rtcIz.sayfa, rtcIz.oyun) + "\nAcik kalma: " + String(rtcIz.sure / 60000) + " dk " + String(rtcIz.sure / 1000 % 60) + " sn\nBos bellek: " + String(rtcIz.bellek / 1024) + " KB\n";
#ifndef SIMULATOR
  if (esp_core_dump_image_check() == ESP_OK) {      // çökme kaydı: görev, PC, geri izleme (PC ajanı bunları fonksiyon/satıra çevirir)
    esp_core_dump_summary_t *oz = (esp_core_dump_summary_t *)malloc(sizeof(esp_core_dump_summary_t));
    if (oz && esp_core_dump_get_summary(oz) == ESP_OK) {
      char t[24]; snprintf(t, sizeof(t), "0x%08lx", (unsigned long)oz->exc_pc);
      r2 += "Gorev: " + String(oz->exc_task) + "\nPC: " + t + "\nIstisna: EXCCAUSE " + String((int)oz->ex_info.exc_cause) + " adres ";
      snprintf(t, sizeof(t), "0x%08lx", (unsigned long)oz->ex_info.exc_vaddr); r2 += String(t) + "\nGeri izleme:";
      for (int i = 0; i < (int)oz->exc_bt_info.depth && i < 16; i++) { snprintf(t, sizeof(t), " 0x%08lx", (unsigned long)oz->exc_bt_info.bt[i]); r2 += t; }
      if (oz->exc_bt_info.corrupted) r2 += " (bozuk)";
      r2 += "\n";
    }
    free(oz);
    esp_core_dump_image_erase();
  }
#endif
  if (saatVar()) { tm z = simdiTm(); r2 += "Kubit saati: " + String(z.tm_mday) + "." + String(z.tm_mon + 1) + " " + ikiHane(z.tm_hour) + ":" + ikiHane(z.tm_min) + "\n"; }
  rtcIz.not_[0] = 0;
  int n = kayit.getInt("hata_n", 0), id = kayit.getInt("hata_id", 0) + 1;   // kuyruk: hata0..hata4 halkası
  kayit.putInt("hata_id", id);
  kayit.putString(("hata" + String(id % 5)).c_str(), String(id) + "\n" + r2.substring(0, 480));
  kayit.putInt("hata_n", min(5, n + 1));
  ekran.clearDisplay(); ekran.fillRect(0, 0, 128, 12, W); ekran.setTextColor(B); ortala(2, 1, "CRASH REPORTER"); ekran.setTextColor(W);
  auto s = sar(nedenAd, 21); int y = 18; for (auto &l : s) { yaz(0, y, 1, l); y += 10; }
  yaz(0, 46, 1, dl("Rapor telefona/PC'ye", "Report will be sent")); yaz(0, 55, 1, dl("gonderilecek.", "to phone/PC."));
  ekran.display(); delay(2800);
}
// döngüde: telefon bağlıysa kuyruktaki en eski raporu gönder (alındı gelmezse 15 sn sonra tekrar)
void hataGonder() {
  if (!bagli || !kayit.getInt("hata_n", 0) || millis() - hataGonderZaman < 15000) return;
  int id = kayit.getInt("hata_id", 0), n = kayit.getInt("hata_n", 0), en = id - n + 1;
  String r = kayit.getString(("hata" + String(en % 5)).c_str(), "");
  if (!r.length()) { kayit.putInt("hata_n", n - 1); return; }
  hataGonderZaman = millis(); hataGonderilen = en;
  auto satir = [](const String &s) { tusKar->setValue(s.c_str()); tusKar->notify(); delay(20); };
  satir("X+" + String(en));
  int bas = r.indexOf('\n') + 1;
  for (auto &l : parcala(r.substring(bas), '\n')) if (l.length()) for (unsigned i = 0; i < l.length(); i += 150) satir("X" + l.substring(i, i + 150));
  satir("X." + String(en));
}
void hataAlindi(int id) {                            // telefon "x<id>": kuyruktan çıkar
  if (id != hataGonderilen) return;
  int n = kayit.getInt("hata_n", 0); if (n > 0) kayit.putInt("hata_n", n - 1);
  kayit.remove(("hata" + String(id % 5)).c_str()); hataGonderilen = -1; hataGonderZaman = 0;
  goster(dl("Hata raporu gonderildi", "Crash report sent"));
}
