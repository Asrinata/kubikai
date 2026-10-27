// Seviye editörleri: Süper Kübit (8 satır x 96 sütun) ve Geometry (160 karo). Kübit'te düzenle ya da telefondan gönder;
// iki yerde de oyna. Telefonla aktarım: L<tür>|<parça>|<toplam>|<veri>  (tür m: Süper Kübit, g: Geometry), istek: SEVIYE:ISTE:<tür>.
// NVS: "sv_m" (768 bayt, satır satır), "sv_g" (metin). Süper Kübit karoları: . # b ? M P c g F  Geometry: . ^ # H p o O x E
#pragma once

const int OZ_SUT = 96, OZ_SAT = 8, OZ_GD = 160;
char ozelMario[OZ_SAT * OZ_SUT + 1], ozelGd[OZ_GD + 1];
char ozelIstek = 0;                                 // 'm': Süper Kübit açılınca doğrudan özel bölüm (telefondaki oynatıcı)
void ozelVarsayilan(char tur) {
  if (tur == 'm') { for (int r = 0; r < OZ_SAT; r++) for (int c = 0; c < OZ_SUT; c++) ozelMario[r * OZ_SUT + c] = r >= 6 ? '#' : '.'; ozelMario[5 * OZ_SUT + OZ_SUT - 4] = 'F'; ozelMario[OZ_SAT * OZ_SUT] = 0; }
  else { for (int i = 0; i < OZ_GD; i++) ozelGd[i] = '.'; ozelGd[OZ_GD - 1] = 'E'; ozelGd[OZ_GD] = 0; }
}
void ozelYukle() {
  if (kayit.getBytes("sv_m", ozelMario, OZ_SAT * OZ_SUT) != OZ_SAT * OZ_SUT) ozelVarsayilan('m'); ozelMario[OZ_SAT * OZ_SUT] = 0;
  if (kayit.getBytes("sv_g", ozelGd, OZ_GD) != OZ_GD) ozelVarsayilan('g'); ozelGd[OZ_GD] = 0;
}
void ozelKaydet(char tur) { if (tur == 'm') kayit.putBytes("sv_m", ozelMario, OZ_SAT * OZ_SUT); else kayit.putBytes("sv_g", ozelGd, OZ_GD); }

// ---- telefonla aktarım
void seviyeGonder(char tur) {
  if (!bagli) { goster(dl("telefon bagli degil", "phone not connected")); return; }
  const char *v = tur == 'm' ? ozelMario : ozelGd; int n = strlen(v), parca = (n + 149) / 150;
  for (int p = 0; p < parca; p++) { String s = String("L") + tur + "|" + p + "|" + parca + "|" + String(v + p * 150).substring(0, 150); tusKar->setValue(s.c_str()); tusKar->notify(); delay(25); }
  goster(dl("Telefona gonderildi", "Sent to phone"));
}
String gelenSeviye;
void seviyeSatiri(const String &g) {                  // telefondan gelen bölüm parçası
  auto p = parcala(g, '|'); if (p.size() < 4) return;
  char tur = p[0][0]; int parca = p[1].toInt(), toplam = p[2].toInt();
  if (parca == 0) gelenSeviye = "";
  gelenSeviye += p[3];
  if (parca != toplam - 1) return;
  if (tur == 'm' && (int)gelenSeviye.length() == OZ_SAT * OZ_SUT) { memcpy(ozelMario, gelenSeviye.c_str(), OZ_SAT * OZ_SUT); ozelKaydet('m'); goster(dl("Super Kubit bolumu geldi!", "Level received!"), 3000); }
  else if (tur == 'g' && gelenSeviye.length()) { memset(ozelGd, '.', OZ_GD); memcpy(ozelGd, gelenSeviye.c_str(), min((int)gelenSeviye.length(), OZ_GD)); ozelGd[OZ_GD] = 0; ozelKaydet('g'); goster(dl("Geometry bolumu geldi!", "Level received!"), 3000); }
  gelenSeviye = "";
}

// ---- Kübit'teki editör: sağ tık ilerle (basılı: hızlı), sol tık karoyu değiştir, iki tuş kısa: (Süper Kübit) alt satıra geç, iki tuş 1 sn: menü
const char MARIO_KARO[] = ".#b?McgPF", GD_KARO[] = ".^#HpoOxE";
const char *MARIO_KARO_AD[] = {"bos", "zemin", "tugla", "soru", "mantar", "para", "dusman", "boru", "bayrak"};
const char *GD_KARO_AD[] = {"bos", "diken", "blok", "cift blok", "zipla pedi", "alcak kure", "yuksek kure", "diken+kure", "bitis"};
struct { char tur; int x, y; bool menu; int menuImlec; float tekrar; } ed;
void editorAc(char tur) { ed = {tur, 0, tur == 'm' ? 5 : 0, false, 0, 0}; }
bool seviyeEditoru(Girdi &g, bool &oyna) {            // false: çık
  oyna = false;
  int en = ed.tur == 'm' ? OZ_SUT : OZ_GD;
  char *hucre = ed.tur == 'm' ? &ozelMario[ed.y * OZ_SUT + ed.x] : &ozelGd[ed.x];
  const char *karo = ed.tur == 'm' ? MARIO_KARO : GD_KARO;
  if (ed.menu) {
    const char *og[5] = {"Oyna", "Kaydet", dl("Telefona gonder", "Send to phone"), "Temizle", dl("Cik", "Exit")};
    if (g.o1 == KISA) ed.menuImlec = (ed.menuImlec + 1) % 5;
    if (g.o2 == KISA) switch (ed.menuImlec) {
      case 0: ozelKaydet(ed.tur); oyna = true; ed.menu = false; return true;
      case 1: ozelKaydet(ed.tur); goster(dl("Kaydedildi", "Saved")); ed.menu = false; break;
      case 2: ozelKaydet(ed.tur); seviyeGonder(ed.tur); ed.menu = false; break;
      case 3: ozelVarsayilan(ed.tur); ed.menu = false; break;
      case 4: return false;
    }
    ekran.fillRect(20, 6, 88, 52, B); ekran.drawRoundRect(20, 6, 88, 52, 3, W);
    for (int i = 0; i < 5; i++) { int y = 9 + i * 10; if (i == ed.menuImlec) { ekran.fillRect(22, y - 1, 84, 10, W); ekran.setTextColor(B); } yaz(26, y, 1, og[i]); ekran.setTextColor(W); }
    return true;
  }
  if (g.ikiSure >= 1000) { ed.menu = true; ed.menuImlec = 0; tusYut = true; return true; }
  if (g.combo && g.comboSure < 1000 && ed.tur == 'm') ed.y = (ed.y + 1) % OZ_SAT;
  if (g.o1 == BASTI) { ed.tekrar = .35f; ed.x = (ed.x + 1) % en; }
  else if (g.b1 && !g.b2 && (ed.tekrar -= g.dt) <= 0) { ed.tekrar = .06f; ed.x = (ed.x + 1) % en; }
  if (g.o2 == KISA) { const char *k = strchr(karo, *hucre); *hucre = karo[k ? (k - karo + 1) % strlen(karo) : 0]; }
  // çizim: Süper Kübit 16x8 görünüm (8 piksel karo), Geometry 16 karo şerit + zemin
  int bas = constrain(ed.x - 8, 0, en - 16);
  if (ed.tur == 'm') {
    for (int r = 0; r < OZ_SAT; r++) for (int c = bas; c < bas + 16; c++) {
      char k = ozelMario[r * OZ_SUT + c]; int sx = (c - bas) * 8, sy = 8 + r * 7;
      if (k == '#') ekran.fillRect(sx, sy, 8, 8, W); else if (k == 'b') { ekran.drawRect(sx, sy, 8, 8, W); ekran.drawFastHLine(sx, sy + 4, 8, W); }
      else if (k == '?' || k == 'M') { ekran.drawRoundRect(sx, sy, 8, 8, 2, W); yaz(sx + 1, sy, 1, k == '?' ? "?" : "M"); }
      else if (k == 'c') ekran.drawCircle(sx + 3, sy + 3, 2, W); else if (k == 'g') ekran.fillCircle(sx + 3, sy + 4, 3, W);
      else if (k == 'P') { ekran.drawFastVLine(sx + 1, sy, 8, W); ekran.drawFastVLine(sx + 6, sy, 8, W); } else if (k == 'F') { ekran.drawFastVLine(sx + 3, sy, 8, W); ekran.fillTriangle(sx + 3, sy, sx + 7, sy + 2, sx + 3, sy + 4, W); }
    }
    if ((millis() / 250) % 2) ekran.drawRect((ed.x - bas) * 8 - 1, 7 + ed.y * 7, 10, 9, W);
    ekran.fillRect(0, 0, 128, 8, B);
  } else {
    for (int c = bas; c < bas + 16; c++) {
      char k = ozelGd[c]; int sx = (c - bas) * 8, y0 = 48;
      if (k == '^' || k == 'x') ekran.fillTriangle(sx, y0, sx + 4, y0 - 8, sx + 7, y0, W);
      if (k == '#') ekran.drawRect(sx, y0 - 8, 8, 8, W); if (k == 'H') ekran.drawRect(sx, y0 - 16, 8, 16, W);
      if (k == 'p') ekran.fillRect(sx, y0 - 2, 8, 2, W); if (k == 'o' || k == 'x') ekran.drawCircle(sx + 3, y0 - 22, 3, W); if (k == 'O') ekran.drawCircle(sx + 3, y0 - 34, 3, W);
      if (k == 'E') for (int y = 12; y < y0; y += 4) ekran.drawFastVLine(sx, y, 2, W);
    }
    ekran.drawFastHLine(0, 48, 128, W);
    if ((millis() / 250) % 2) ekran.drawRect((ed.x - bas) * 8 - 1, 10, 10, 40, W);
  }
  const char *k = strchr(karo, *hucre); int ki = k ? k - karo : 0;
  yaz(0, 0, 1, String(ed.x + 1) + (ed.tur == 'm' ? "," + String(ed.y + 1) : "") + " " + (ed.tur == 'm' ? MARIO_KARO_AD[ki] : GD_KARO_AD[ki]));
  if (ed.tur == 'g') { ekran.fillRect(0, 55, 128, 9, B); yaz(0, 56, 1, dl("sag>  sol:karo  2tus1sn:menu", "R>  L:tile  2:menu")); }
  return true;
}
