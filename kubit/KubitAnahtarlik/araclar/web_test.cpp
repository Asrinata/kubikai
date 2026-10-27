// Kübit Tarayıcı ayrıştırıcısını PC'de dener:  web_test html <dosya> <adres>   |   web_test yt <dosya>
// Derleme: python -m ziglang c++ -O1 -std=c++17 -I.. web_test.cpp -o web_test.exe
#include <cstdio>
#include <vector>
#include "../web_ayristir.h"

int main(int argc, char **argv) {
  if (argc < 3) return 1;
  FILE *f = fopen(argv[2], "rb"); if (!f) return 2;
  std::vector<uint8_t> v; int c; while ((c = fgetc(f)) != EOF) v.push_back(c); fclose(f);
  if (argv[1][0] == 'y') {
    static web::Video vid[web::VIDEO_MAKS]; web::YouTube y(vid);
    for (auto b : v) { y.bayt(b); if (y.dolu()) break; }
    for (int i = 0; i < y.n; i++) printf("%2d %s | %s | %s | %s | %s | %s\n", i, vid[i].id, vid[i].baslik, vid[i].kanal, vid[i].izlenme, vid[i].sure, vid[i].tarih);
    return y.n ? 0 : 3;
  }
  static web::Sayfa s; s.hazirla(); web::Html h(s);
  size_t i = 0; for (; i < v.size() && !s.dolu(); i++) h.bayt(v[i]);
  h.bitir();
  printf("BASLIK: %s\nbayt %zu/%zu, metin %d, satir %d, link %d, havuz %d\n", s.baslik, i, v.size(), s.n, s.satirN, s.linkN, s.havuzN);
  int goster = argc > 4 ? atoi(argv[4]) : 60;
  for (int k = 0; k < s.satirN && k < goster; k++) {
    int a = s.satir[k], e = s.satirSonu(k);
    printf("|%-21.*s|", e - a, s.metin + a);
    for (int l = 0; l < s.linkN; l++) if (s.link[l].bas >= a && s.link[l].bas < e) printf(" L%d", l);
    printf("\n");
  }
  char o[300];
  for (int l = 0; l < s.linkN && l < 25; l++) { web::cozumle(argc > 3 ? argv[3] : "", s.href(l), o, sizeof o); printf("L%d [%.*s] -> %s\n", l, s.link[l].son - s.link[l].bas, s.metin + s.link[l].bas, o); }
  // URL çözümleme denetimi
  struct { const char *t, *h, *b; } U[] = {
    {"https://tr.m.wikipedia.org/wiki/Kedi", "/wiki/Kopek", "https://tr.m.wikipedia.org/wiki/Kopek"},
    {"https://a.com/x/y.html?q=1", "z.html", "https://a.com/x/z.html"},
    {"https://a.com", "z.html", "https://a.com/z.html"},
    {"https://a.com/x/", "//cdn.b.com/r.jpg", "https://cdn.b.com/r.jpg"},
    {"https://lite.duckduckgo.com/lite/?q=a", "//duckduckgo.com/l/?uddg=https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FCat&rut=x", "https://en.wikipedia.org/wiki/Cat"},
    {"https://a.com/s?q=1", "?q=2", "https://a.com/s?q=2"}};
  int hata = 0;
  for (auto &u : U) { web::cozumle(u.t, u.h, o, sizeof o); if (strcmp(o, u.b)) { printf("URL HATASI %s + %s = %s (beklenen %s)\n", u.t, u.h, o, u.b); hata++; } }
  printf(hata ? "URL testleri BASARISIZ\n" : "URL testleri tamam\n");
  return hata;
}
