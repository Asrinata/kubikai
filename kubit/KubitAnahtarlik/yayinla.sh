#!/bin/sh
# Kübit'in yeni sürümünü GitHub'a (Asrinata/kubikai, "kubit" sürümü) yükler; Kübit Ayarlar > Güncelleme ara ile indirir.
# Önce KubitAnahtarlik.ino'daki SURUM'u artır. wifi_ayar.h bu derlemeye GİRMEZ (yayın herkese açık). Derleme hizli.py ile.
set -e
cd "$(dirname "$0")"
SURUM=$(grep -o 'const int SURUM = [0-9]*' KubitAnahtarlik.ino | grep -o '[0-9]*$')
BIN=build-yayin/KubitAnahtarlik.ino.bin
rm -f "$BIN"                                        # derleme hata verirse eski .bin yeni sürüm diye yayınlanmasın
[ "$1" = "--tam" ] && rm -f build-ayrinti.log       # kütüphane/çekirdek güncellendiyse: sh yayinla.sh --tam
python hizli.py --yayin || { echo "DUR: derleme hatası"; exit 1; }   # ~15 sn; wifi_ayar.h dahil edilmez
[ -s "$BIN" ] || { echo "DUR: .bin yok"; exit 1; }
mkdir -p build-yayin/elf_arsiv && cp build-yayin/KubitAnahtarlik.ino.elf "build-yayin/elf_arsiv/kubit_s$SURUM.elf"   # PC ajanı çökme adreslerini bununla çözer

# Wi-Fi bilgisi sızmasın: .bin içinde ağ adı ya da şifresi geçerse yayınlama
if [ -f wifi_ayar.h ]; then
  for degisken in WIFI_ADI WIFI_SIFRE; do
    deger=$(grep "$degisken" wifi_ayar.h | sed 's/.*"\(.*\)".*/\1/')
    if [ -n "$deger" ] && grep -qF "$deger" "$BIN"; then echo "DUR: $degisken .bin içinde!"; exit 1; fi
  done
fi

echo "$SURUM" > build-yayin/surum.txt
gh release view kubit --repo Asrinata/kubikai >/dev/null 2>&1 || \
  gh release create kubit --repo Asrinata/kubikai --title "Kübit anahtarlık yazılımı" --latest=false \
     --notes "Kübit anahtarlığının kendi kendine güncellemesi (Ayarlar > Güncelleme ara). Elle indirmen gerekmez."
cp "$BIN" build-yayin/kubit.bin
gh release upload kubit build-yayin/kubit.bin --clobber --repo Asrinata/kubikai     # önce kod,
gh release upload kubit build-yayin/surum.txt --clobber --repo Asrinata/kubikai     # sonra sürüm numarası
echo "Yayınlandı: sürüm $SURUM"
