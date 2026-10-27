// Kübit Tarayıcı'nın çekirdeği (Arduino'suz, PC'de de derlenir: araclar/web_test.cpp). Sayfa hiç bütün olarak tutulmaz:
// indirilen baytlar akarken HTML durum makinesinden geçer, yalnız görünen yazı (ASCII'ye çevrilmiş), linkler ve başlık saklanır.
// YouTube arama sayfası (~1 MB) da akarken taranır: "videoRenderer" kalıplarından en çok 15 videonun kimliği/başlığı/kanalı çekilir.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace web {

const int METIN_MAKS = 22000, LINK_MAKS = 140, HAVUZ_MAKS = 10000, SATIR_MAKS = 1500, GENISLIK = 21;

// ---------------------------------------------------------------- Unicode → ASCII (ekran yazı tipinde Türkçe harf yok)
inline const char *cpAscii(uint32_t cp) {
  static char tek[2];
  if (cp == 9 || cp == 10 || cp == 13) return " ";
  if (cp < 32) return "";
  if (cp < 127) { tek[0] = (char)cp; tek[1] = 0; return tek; }
  switch (cp) {
    case 0xA0: return " "; case 0xAB: case 0xBB: case 0x201C: case 0x201D: case 0x201E: return "\"";
    case 0x2018: case 0x2019: case 0x201A: case 0xB4: return "'"; case 0x2013: case 0x2014: case 0x2212: return "-";
    case 0x2026: return "..."; case 0x2022: case 0xB7: return "*"; case 0xB0: return "o"; case 0x20AC: return "EUR"; case 0x20BA: return "TL";
    case 0xA9: return "(c)"; case 0xD7: return "x"; case 0xDF: return "ss"; case 0x11E: return "G"; case 0x11F: return "g";
    case 0x130: return "I"; case 0x131: return "i"; case 0x15E: return "S"; case 0x15F: return "s"; case 0x2192: return "->"; case 0x2190: return "<-";
  }
  if (cp >= 0xC0 && cp <= 0xFF) {                     // Latin-1 harfleri: aksanı at
    static const char L[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";
    tek[0] = L[cp - 0xC0]; tek[1] = 0; return tek;
  }
  if (cp >= 0x100 && cp <= 0x17F) {                   // Latin Genişletilmiş-A: çiftler büyük/küçük
    static const char E[] = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";
    int i = cp - 0x100; if (i < (int)sizeof(E) - 1) { tek[0] = E[i]; tek[1] = 0; return tek; }
  }
  return "";
}

struct Link { uint16_t bas, son, href; };           // metindeki aralık [bas, son), href: havuzdaki başlangıç

// ---------------------------------------------------------------- HTML → düz yazı + linkler
struct Sayfa {
  char *metin = nullptr, *havuz = nullptr; int n = 0, havuzN = 0;
  Link link[LINK_MAKS]; int linkN = 0;
  uint16_t satir[SATIR_MAKS]; int satirN = 0;          // satır başlangıçları (sar() sonrası)
  char baslik[64] = "";
  bool dolu() const { return n >= METIN_MAKS - 8; }
  bool hazirla() { if (!metin) metin = (char *)malloc(METIN_MAKS); if (!havuz) havuz = (char *)malloc(HAVUZ_MAKS); return metin && havuz; }
  void temizle() { n = havuzN = linkN = satirN = 0; baslik[0] = 0; }
  const char *href(int i) const { return havuz + link[i].href; }
  int hrefEkle(const char *h) {
    int u = strlen(h); if (havuzN + u + 1 > HAVUZ_MAKS) return -1;
    memcpy(havuz + havuzN, h, u + 1); int r = havuzN; havuzN += u + 1; return r;
  }
  int satirSonu(int s) const {                       // s. satırın bittiği yer (sonundaki boşluk/yeni satır hariç)
    int e = s + 1 < satirN ? satir[s + 1] : n;
    while (e > satir[s] && (metin[e - 1] == ' ' || metin[e - 1] == '\n')) e--;
    return e;
  }
  int satirBul(int ofs) const {                      // ofs hangi satırda (ikili arama)
    int a = 0, b = satirN - 1;
    while (a < b) { int m = (a + b + 1) / 2; if (satir[m] <= ofs) a = m; else b = m - 1; }
    return a;
  }
  void sar() {                                       // kelime kelime GENISLIK sütuna böl
    satirN = 0; int i = 0;
    while (i < n && satirN < SATIR_MAKS) {
      while (i < n && metin[i] == ' ') i++;
      if (i >= n) break;
      satir[satirN++] = i;
      int son = -1, j = i;
      for (; j < n && j - i < GENISLIK; j++) { if (metin[j] == '\n') break; if (metin[j] == ' ') son = j; }
      if (j >= n) break;
      if (metin[j] == '\n') i = j + 1;
      else if (metin[j] == ' ') i = j + 1;
      else i = son > i ? son + 1 : j;                  // sözcük sığmıyorsa böl
    }
  }
};

class Html {
  Sayfa &s;
  enum { METIN, ETIKET, YORUM, VARLIK } hal = METIN;
  char etiket[400]; int en = 0; char tirnak = 0;
  char varlik[12]; int vn = 0;
  uint32_t cp = 0; int kalan = 0;                     // UTF-8 çözümü
  bool bosluk = false, buyuk = false, baslikta = false;
  char atla[12] = ""; int atlaDerin = 0;              // script/style/nav... içi atlanır
  int acikLink = -1, acikHref = -1;
  int baslikN = 0;
 public:
  explicit Html(Sayfa &s) : s(s) { s.temizle(); }
  void ekle(const char *a) {
    if (atla[0]) return;
    for (; *a; a++) {
      char c = *a;
      if (baslikta) { if (c == ' ' && (baslikN == 0 || s.baslik[baslikN - 1] == ' ')) continue; if (baslikN < 63) { s.baslik[baslikN++] = c; s.baslik[baslikN] = 0; } continue; }
      if (c == ' ') { bosluk = true; continue; }
      if (s.dolu()) return;
      if (bosluk && s.n && s.metin[s.n - 1] != '\n') s.metin[s.n++] = ' ';
      bosluk = false;
      s.metin[s.n++] = buyuk && c >= 'a' && c <= 'z' ? c - 32 : c;
    }
  }
  void satir() { bosluk = false; if (s.n && s.metin[s.n - 1] != '\n' && !s.dolu()) s.metin[s.n++] = '\n'; }
  void bosSatir() { satir(); if (s.n > 1 && s.metin[s.n - 2] != '\n' && !s.dolu()) s.metin[s.n++] = '\n'; }
  static bool ayni(const char *a, const char *b) { return strcmp(a, b) == 0; }
  static bool adAyni(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) { char c = a[i]; if (!c) return false; if (c >= 'A' && c <= 'Z') c += 32; if (c != b[i]) return false; }
    return true;
  }
  // etiketteki bir özelliği (href, src, alt...) çıkarır
  bool ozellik(const char *ad, char *cikti, int maks) {
    int al = strlen(ad);
    for (int i = 0; i < en; i++) {
      if ((i == 0 || etiket[i - 1] == ' ' || etiket[i - 1] == '\n' || etiket[i - 1] == '\t') && adAyni(etiket + i, ad, al)) {
        int j = i + al; while (j < en && etiket[j] == ' ') j++;
        if (j >= en || etiket[j] != '=') continue;
        j++; while (j < en && etiket[j] == ' ') j++;
        char q = (etiket[j] == '"' || etiket[j] == '\'') ? etiket[j++] : 0; int k = 0;
        while (j < en && (q ? etiket[j] != q : etiket[j] != ' ' && etiket[j] != '>') && k < maks - 1) cikti[k++] = etiket[j++];
        cikti[k] = 0;
        varlikCoz(cikti);
        return true;
      }
    }
    return false;
  }
  static void varlikCoz(char *h) {                    // href'lerdeki &amp; → &
    char *o = h;
    for (char *i = h; *i; ) { if (strncmp(i, "&amp;", 5) == 0) { *o++ = '&'; i += 5; } else *o++ = *i++; }
    *o = 0;
  }
  static bool jpgMi(const char *u) {
    const char *q = strchr(u, '?'); int l = q ? q - u : strlen(u);
    return (l > 4 && strncmp(u + l - 4, ".jpg", 4) == 0) || (l > 4 && strncmp(u + l - 4, ".JPG", 4) == 0) || (l > 5 && strncmp(u + l - 5, ".jpeg", 5) == 0);
  }
  void linkBaslat(const char *h, bool resim) {
    linkBitir();
    if (!h[0] || h[0] == '#' || strncmp(h, "javascript:", 11) == 0 || strncmp(h, "mailto:", 7) == 0 || strncmp(h, "tel:", 4) == 0) return;
    char tmp[260]; tmp[0] = resim ? '!' : 0; tmp[1] = 0;      // '!': resim linki
    strncat(tmp, h, sizeof(tmp) - 2);
    acikHref = s.hrefEkle(tmp);
    if (acikHref >= 0) acikLink = s.n + (bosluk && s.n && s.metin[s.n - 1] != '\n' ? 1 : 0);
  }
  void linkBitir() {
    if (acikLink >= 0 && s.n > acikLink && s.linkN < LINK_MAKS) s.link[s.linkN++] = {(uint16_t)acikLink, (uint16_t)s.n, (uint16_t)acikHref};
    acikLink = -1;
  }
  void etiketBitti() {
    etiket[en] = 0;
    if (en >= 3 && strncmp(etiket, "!--", 3) == 0) return;
    bool kapanis = etiket[0] == '/';
    char ad[12]; int i = kapanis ? 1 : 0, k = 0;
    while (i < en && k < 11 && etiket[i] != ' ' && etiket[i] != '/' && etiket[i] != '\n' && etiket[i] != '\t') { char c = etiket[i++]; ad[k++] = c >= 'A' && c <= 'Z' ? c + 32 : c; }
    ad[k] = 0;
    if (atla[0]) {                                     // atlanan bölümün içi: yalnız aynı etiketin açılış/kapanışını say
      if (ayni(ad, atla)) { if (kapanis) { if (--atlaDerin <= 0) atla[0] = 0; } else atlaDerin++; }
      return;
    }
    static const char *ATLA[] = {"script", "style", "noscript", "svg", "template", "iframe", "select", "textarea", "nav", "footer", "button"};
    if (!kapanis) for (auto a : ATLA) if (ayni(ad, a)) { if (etiket[en - 1] != '/') { strcpy(atla, ad); atlaDerin = 1; } return; }
    if (ayni(ad, "title")) { baslikta = !kapanis; return; }
    if (ayni(ad, "a")) {
      if (kapanis) { linkBitir(); return; }
      char h[256]; if (ozellik("href", h, sizeof(h))) linkBaslat(h, false);
      return;
    }
    if (ayni(ad, "img") || (!kapanis && en > 8 && strstr(etiket, "data-src"))) {
      char src[256] = "", alt[48] = "";
      if (!ozellik("data-src", src, sizeof(src))) ozellik("src", src, sizeof(src));
      if (!ozellik("alt", alt, sizeof(alt))) ozellik("data-alt", alt, sizeof(alt));
      if (src[0] && jpgMi(src)) {                    // resmi saran <a> varsa onun yerine resim linki olur
        satir(); linkBaslat(src, true); ekle("[resim"); if (alt[0]) { ekle(": "); char t[48]; int o = 0; for (char *p = alt; *p && o < 40; p++) if ((uint8_t)*p < 128) t[o++] = *p; t[o] = 0; ekle(t); } ekle("]"); linkBitir(); satir();
      }
      return;
    }
    if (ad[0] == 'h' && ad[1] >= '1' && ad[1] <= '4' && !ad[2]) { if (kapanis) { buyuk = false; satir(); } else { bosSatir(); buyuk = ad[1] <= '2'; } return; }
    if (ayni(ad, "li")) { if (!kapanis) { satir(); ekle("- "); } return; }
    if (ayni(ad, "td") || ayni(ad, "th")) { if (kapanis) bosluk = true; return; }
    static const char *BLOK[] = {"p", "div", "br", "tr", "ul", "ol", "table", "section", "article", "header", "main", "blockquote", "pre", "dt", "dd", "figcaption", "hr", "form", "dl", "summary", "details", "center"};
    for (auto b : BLOK) if (ayni(ad, b)) { if (ayni(ad, "p")) { if (!kapanis) bosSatir(); else satir(); } else satir(); return; }
  }
  void varlikBitti() {
    varlik[vn] = 0; uint32_t c = 0;
    if (varlik[0] == '#') c = (varlik[1] == 'x' || varlik[1] == 'X') ? strtoul(varlik + 2, nullptr, 16) : strtoul(varlik + 1, nullptr, 10);
    else {
      static const struct { const char *ad; uint16_t cp; } V[] = {{"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", 0xA0}, {"mdash", 0x2014}, {"ndash", 0x2013},
        {"hellip", 0x2026}, {"laquo", 0xAB}, {"raquo", 0xBB}, {"rsquo", 0x2019}, {"lsquo", 0x2018}, {"ldquo", 0x201C}, {"rdquo", 0x201D}, {"copy", 0xA9}, {"deg", 0xB0},
        {"uuml", 0xFC}, {"Uuml", 0xDC}, {"ouml", 0xF6}, {"Ouml", 0xD6}, {"ccedil", 0xE7}, {"Ccedil", 0xC7}, {"eacute", 0xE9}, {"middot", 0xB7}, {"bull", 0x2022}, {"times", 0xD7}, {"euro", 0x20AC}};
      for (auto &v : V) if (strcmp(v.ad, varlik) == 0) { c = v.cp; break; }
      if (!c) { ekle("&"); ekle(varlik); return; }
    }
    ekle(cpAscii(c));
  }
  void bayt(uint8_t b) {
    switch (hal) {
      case METIN:
        if (b == '<') { hal = ETIKET; en = 0; tirnak = 0; return; }
        if (b == '&') { hal = VARLIK; vn = 0; return; }
        if (kalan) { if ((b & 0xC0) == 0x80) { cp = (cp << 6) | (b & 0x3F); if (--kalan == 0) ekle(cpAscii(cp)); return; } kalan = 0; }
        if (b >= 0xF0) { cp = b & 7; kalan = 3; } else if (b >= 0xE0) { cp = b & 15; kalan = 2; } else if (b >= 0xC0) { cp = b & 31; kalan = 1; }
        else if (b < 0x80) { char t[2] = {(char)b, 0}; if (b == '\n' || b == '\r' || b == '\t') t[0] = ' '; if (b >= 32 || t[0] == ' ') ekle(t); }
        return;
      case VARLIK:
        if (b == ';' || vn >= 10 || !((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '#')) {
          hal = METIN; varlikBitti(); if (b != ';') bayt(b); return;
        }
        varlik[vn++] = b; return;
      case ETIKET:
        if (en == 3 && strncmp(etiket, "!--", 3) == 0) { hal = YORUM; en = 0; }
        if (tirnak) { if (b == tirnak) tirnak = 0; }
        else if (b == '"' || b == '\'') tirnak = b;
        else if (b == '>') { hal = METIN; etiketBitti(); return; }
        if (en < (int)sizeof(etiket) - 1) etiket[en++] = b == '\n' || b == '\t' || b == '\r' ? ' ' : b;
        if (en == 3 && strncmp(etiket, "!--", 3) == 0) { hal = YORUM; en = 0; }
        return;
      case YORUM:                                     // "-->" bekle
        en = b == '-' ? en + 1 : (b == '>' && en >= 2 ? -1 : 0);
        if (en < 0) { hal = METIN; en = 0; }
        return;
    }
  }
  void bitir() { linkBitir(); while (s.n && (s.metin[s.n - 1] == '\n' || s.metin[s.n - 1] == ' ')) s.n--; s.metin[s.n] = 0; s.sar(); }
};

// ---------------------------------------------------------------- URL yardımcıları
// göreli linki mevcut sayfaya göre tam adrese çevirir; DuckDuckGo yönlendirmesini (uddg=) açar
inline void cozHex(const char *a, char *o, int maks) {
  int k = 0;
  for (; *a && *a != '&' && k < maks - 1; a++) {
    if (*a == '%' && a[1] && a[2]) { char h[3] = {a[1], a[2], 0}; o[k++] = (char)strtol(h, nullptr, 16); a += 2; }
    else o[k++] = *a == '+' ? ' ' : *a;
  }
  o[k] = 0;
}
inline void cozumle(const char *taban, const char *h, char *o, int maks) {
  const char *u = strstr(h, "uddg=");
  if (u && strstr(h, "duckduckgo.com/l/")) { cozHex(u + 5, o, maks); return; }
  if (strncmp(h, "http://", 7) == 0 || strncmp(h, "https://", 8) == 0) { strncpy(o, h, maks - 1); o[maks - 1] = 0; return; }
  const char *sema = strstr(taban, "://"); if (!sema) { strncpy(o, h, maks - 1); o[maks - 1] = 0; return; }
  if (h[0] == '/' && h[1] == '/') { int l = sema - taban + 1; snprintf(o, maks, "%.*s%s", l, taban, h); return; }
  const char *kok = strchr(sema + 3, '/'); int kl = kok ? kok - taban : strlen(taban);
  if (h[0] == '/') { snprintf(o, maks, "%.*s%s", kl, taban, h); return; }
  if (h[0] == '?') { const char *q = strchr(taban, '?'); int l = q ? q - taban : strlen(taban); snprintf(o, maks, "%.*s%s", l, taban, h); return; }
  const char *q = strchr(taban, '?'); int l = q ? q - taban : strlen(taban);   // aynı klasöre göre
  while (l > kl && taban[l - 1] != '/') l--;
  if (l <= kl) snprintf(o, maks, "%.*s/%s", kl, taban, h); else snprintf(o, maks, "%.*s%s", l, taban, h);
}
inline void kodla(const char *a, char *o, int maks) {   // arama sözcüğünü URL'ye koy
  int k = 0; const char *H = "0123456789ABCDEF";
  for (; *a && k < maks - 4; a++) {
    uint8_t c = *a;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_') o[k++] = c;
    else if (c == ' ') o[k++] = '+';
    else { o[k++] = '%'; o[k++] = H[c >> 4]; o[k++] = H[c & 15]; }
  }
  o[k] = 0;
}

// ---------------------------------------------------------------- YouTube arama sayfası tarayıcı
struct Video { char id[12], baslik[96], kanal[40], izlenme[32], sure[10], tarih[28]; };
const int VIDEO_MAKS = 15;
class YouTube {
  struct Kalip { const char *k; int i; };
  Kalip K[6] = {{"\"videoRenderer\":{\"videoId\":\"", 0}, {"\"title\":{\"runs\":[{\"text\":\"", 0}, {"\"longBylineText\":{\"runs\":[{\"text\":\"", 0},
                {"\"viewCountText\":{\"simpleText\":\"", 0}, {"\"lengthText\":{", 0}, {"\"publishedTimeText\":{\"simpleText\":\"", 0}};
  Kalip basit = {"\"simpleText\":\"", 0};
  char *hedef = nullptr; int hn = 0, hm = 0; bool kacis = false; int uni = -1; uint32_t ucp = 0;
  int idKalan = 0; bool sureBekle = false, bitti = false;
 public:
  Video *v; int n = 0;
  explicit YouTube(Video *v) : v(v) {}
  static bool ilerle(Kalip &k, uint8_t c) {
    if (c == (uint8_t)k.k[k.i]) { if (!k.k[++k.i]) { k.i = 0; return true; } }
    else k.i = c == (uint8_t)k.k[0] ? 1 : 0;
    return false;
  }
  void oku(char *h, int maks) { hedef = h; hn = 0; hm = maks; h[0] = 0; kacis = false; uni = -1; }
  void yaz(const char *a) { for (; *a && hn < hm - 1; a++) if (hn || *a != ' ') hedef[hn++] = *a; hedef[hn] = 0; }
  void bayt(uint8_t c) {
    if (idKalan) { Video &x = v[n - 1]; x.id[11 - idKalan] = c; if (--idKalan == 0) x.id[11] = 0; return; }
    if (hedef) {                                       // JSON metni okunuyor
      if (uni >= 0) { ucp = ucp * 16 + (c <= '9' ? c - '0' : (c | 32) - 'a' + 10); if (++uni == 4) { uni = -1; yaz(cpAscii(ucp)); } return; }
      if (kacis) { kacis = false; if (c == 'u') { uni = 0; ucp = 0; } else if (c == 'n') yaz(" "); else { char t[2] = {(char)c, 0}; yaz(t); } return; }
      if (c == '\\') { kacis = true; return; }
      if (c == '"') { hedef = nullptr; return; }
      if (c < 0x80) { char t[2] = {(char)c, 0}; yaz(t); return; }
      // ham UTF-8: Türkçe harfleri çevir (2 baytlıklar yeter)
      static uint32_t cp; static int kalan;
      if (c >= 0xC0) { kalan = c >= 0xE0 ? 2 : 1; cp = c & (c >= 0xE0 ? 15 : 31); return; }
      if (kalan) { cp = (cp << 6) | (c & 63); if (--kalan == 0) yaz(cpAscii(cp)); }
      return;
    }
    if (ilerle(K[0], c)) { if (n >= VIDEO_MAKS) { bitti = true; return; } memset(&v[n], 0, sizeof(Video)); n++; idKalan = 11; sureBekle = false; return; }
    bool b = ilerle(basit, c);
    if (!n) return;
    Video &x = v[n - 1];
    if (ilerle(K[1], c) && !x.baslik[0]) oku(x.baslik, sizeof(x.baslik));
    else if (ilerle(K[2], c) && !x.kanal[0]) oku(x.kanal, sizeof(x.kanal));
    else if (ilerle(K[3], c) && !x.izlenme[0]) oku(x.izlenme, sizeof(x.izlenme));
    else if (ilerle(K[5], c) && !x.tarih[0]) oku(x.tarih, sizeof(x.tarih));
    else if (ilerle(K[4], c)) sureBekle = true;
    else if (b && sureBekle && !x.sure[0]) { sureBekle = false; oku(x.sure, sizeof(x.sure)); }
  }
  bool dolu() const { return bitti; }
};

}  // namespace web
