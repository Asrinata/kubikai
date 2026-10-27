#include <cassert>
#include <cstdio>
#include <string>
#include "../web_ayristir.h"

int main() {
  web::Sayfa s; assert(s.hazirla());
  auto oku = [&](const char *html) { web::Html h(s); for (const unsigned char *p = (const unsigned char *)html; *p; p++) h.bayt(*p); h.bitir(); };
  oku("<TITLE>Buyuk Harf</TITLE><SCRIPT>gorunmemeli</SCRIPT><H1>Baslik</H1><p><A HREF = '/Wiki/Buyuk?A=B&amp;x=2' TITLE = 'a > b'>Baglanti</A> Son.</p>");
  assert(!strcmp(s.baslik, "Buyuk Harf")); assert(s.linkN == 1);
  assert(!strcmp(s.href(0), "/Wiki/Buyuk?A=B&x=2")); assert(!strstr(s.metin, "gorunmemeli"));
  assert(strstr(s.metin, "Baglanti Son."));
  oku("<p>Bir &amp; iki &Ccedil;ay &#351; &#x131;</p><!-- gizli --><p>Son</p>");
  assert(strstr(s.metin, "Bir & iki Cay s i")); assert(!strstr(s.metin, "gizli"));
  oku("<a href='/x'><b>Uzun</b> baglanti</a><p>Sonraki</p>");
  assert(s.linkN == 1); assert(std::string(s.metin + s.link[0].bas, s.link[0].son - s.link[0].bas) == "Uzun baglanti");
  std::string uzun = "<p>"; for (int i = 0; i < 10000; i++) uzun += "Kelime "; uzun += "</p>";
  oku(uzun.c_str()); assert(s.n < web::METIN_MAKS); assert(s.satirN <= web::SATIR_MAKS);
  for (int i = 0; i < s.satirN; i++) assert(s.satirSonu(i) - s.satir[i] <= web::GENISLIK);
  char o[300]; web::cozumle("https://example.test/A/B", "/Case?Query=UPPER", o, sizeof o);
  assert(!strcmp(o, "https://example.test/Case?Query=UPPER"));
  puts("HTML ozellikleri, tirnaklar, varliklar, link araliklari, uzun sayfa ve URL testleri gecti.");
  free(s.metin); free(s.havuz);
}
