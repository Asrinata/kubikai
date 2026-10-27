// Kübit'in gerçek yazılımı + Adafruit_GFX tek parça halinde WebAssembly'ye derlenir (derle.py). JS bu fonksiyonları çağırır.
#include "Adafruit_GFX.cpp"
#include "build/KubitAnahtarlik.ino.cpp"
#define SIM_EXPORT(ad) extern "C" __attribute__((export_name(#ad)))

static char simTampon[2048];
SIM_EXPORT(sim_tampon) char *sim_tampon() { return simTampon; }
SIM_EXPORT(sim_setup) void sim_setup(int uyandi, uint32_t tohum) { simUyanma = uyandi ? ESP_SLEEP_WAKEUP_EXT0 : 0; randomSeed(tohum); setup(); }
SIM_EXPORT(sim_loop) void sim_loop() { loop(); }
SIM_EXPORT(sim_vt) uint32_t sim_vt() { return simVt; }
SIM_EXPORT(sim_vt_kur) void sim_vt_kur(uint32_t v) { simVt = v; }
SIM_EXPORT(sim_bagla) void sim_bagla(int b) {
  NimBLEConnInfo i;
  if (simSunucu && simSunucu->cb) { if (b) simSunucu->cb->onConnect(simSunucu, i); else simSunucu->cb->onDisconnect(simSunucu, i, 0); }
  if (tusKar && tusKar->cb) tusKar->cb->onSubscribe(tusKar, i, b ? 1 : 0);
}
SIM_EXPORT(sim_gonder) void sim_gonder(int n) {      // telefon -> Kübit satırı (simTampon'da)
  for (auto *k : simKarakter) if ((k->ozellik & NIMBLE_PROPERTY::WRITE) && k->cb) { k->deger.v.assign(simTampon, n); NimBLEConnInfo i; k->cb->onWrite(k, i); }
}
SIM_EXPORT(sim_oyun_sayisi) int sim_oyun_sayisi() { return OYUN_N; }
SIM_EXPORT(sim_oyun_adi) const char *sim_oyun_adi(int i) { return OYUN_LISTE[i].ad; }
SIM_EXPORT(sim_oyun) void sim_oyun(int i) { oyunNo = i; oyunImlec = i + OYUN_ILK; oyunGiris = true; oyunBas = millis(); sayfa = OYUN; sonDokunus = millis(); }
SIM_EXPORT(sim_git) void sim_git(int h) {            // hızlı git düğmeleri
  const Sayfa s[] = {MENU, OYUNLAR, FB_SEC, GD_SEC, RC_MOD, AYAR, ANA, DOLAP, ARACLAR, ROZETLER, MUZIK};
  if (h >= 0 && h < 11) { git(s[h]); if (s[h] == OYUNLAR) imlec = oyunImlec; }
  sonDokunus = millis();
}
SIM_EXPORT(sim_ozel) void sim_ozel(int tur) {        // telefondaki oynatıcı: özel bölümü doğrudan aç ('m' Süper Kübit, 'g' Geometry)
  ozelYukle();
  if (tur == 'g') { gdSeviye = 6; gdBasla(); sayfa = GD; return; }
  for (int i = 0; i < OYUN_N; i++) if (OYUN_LISTE[i].f == superKubit) { ozelIstek = 'm'; oyunNo = i; oyunGiris = false; oyunIlk = true; oyunSon = millis(); sayfa = OYUN; }
  sonDokunus = millis();
}

#ifdef KUBIT_QA
#include "arayuz_qa.h"
#endif
