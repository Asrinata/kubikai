# Kübik

Kübik, Raspberry Pi üzerinde çalışan Türkçe bir yapay zekâ sohbet uygulamasıdır. Bu depo sitenin ön yüzünü
(GitHub Pages) ve Windows masaüstü uygulamasının kaynak kodunu içerir. Sunucu, modeller ve kullanıcı verisi
bu depoda **yoktur**; Pi'de durur.

- Sohbet: https://asrinata.github.io/kubikai/
- Yönetim paneli: https://asrinata.github.io/kubikai/panel.html
- Kübik Work (herkese açık mesaj sayfası): `https://asrinata.github.io/kubikai/work.html?w=<kod>`
- Belgeler (özellikler, kurallar, API, SSS): https://asrinata.github.io/kubikai/docs.html
- Model ömürleri (çıkış / kapanış tarihleri): https://asrinata.github.io/kubikai/modeller.html
- Gizlilik, çerezler ve kullanım koşulları: sitede **Gizlilik ve koşullar** bağlantısı
  (Veri sorumlusu: Asrın Ata Yılmazok · kubik.iletisim@gmail.com)

## Kübik Masaüstü (Windows)

İndir: [KubikMasaustu.exe](https://github.com/Asrinata/kubikai/releases/latest/download/KubikMasaustu.exe) ·
Kaynak kodu: [`masaustu/kubik_masaustu.py`](masaustu/kubik_masaustu.py) · SHA256 değeri her sürümün notunda ve sitede yazar.

Uygulama bu siteyi bir pencerede açar; kendi başına veri toplamaz, ayarlarını yalnız
`%APPDATA%\KubikMasaustu` klasöründe saklar. İmzasız olduğu için Windows ilk açılışta "bilinmeyen yayıncı"
uyarısı verebilir (Ek bilgi → Yine de çalıştır). İndirdiğin dosyayı doğrulamak için:

```powershell
Get-FileHash KubikMasaustu.exe -Algorithm SHA256
```

### PC erişimi

Kübik'in bu bilgisayarda iş yapabilmesi isteğe bağlıdır ve **varsayılan olarak kapalıdır**. Ayarlar → PC erişimi:

| Mod | Ne yapabilir |
|---|---|
| Kapalı | Hiçbir şey |
| Sadece klasör | Seçtiğin klasörde dosya listeleme, okuma, yazma |
| Klasör + onaylı komut | Yukarıdakiler + komut çalıştırma; her komut için Windows onay penceresi |
| Bypass | Klasör + komut, onaysız |
| Tam kontrol | Her yer + ekran görüntüsü, fare ve klavye. Ekranın etrafında kırmızı çerçeve olur, üstteki **DURDUR** ile anında kapanır |

Güvenlik:
- Yetkiyi her artırmada **Windows'un kendi onay penceresi** çıkar; web sayfası yetki yükseltemez ve onayı atlayamaz
  (denetim uygulamanın Python tarafındadır, sayfa yalnız `pc_durum`, `pc_islem`, `pc_klasor`, `pc_mod` çağırabilir).
- Klasör modlarında seçili klasörün dışına çıkılamaz. Tam kontrol her açılışta yeniden onay ister.
- Kübik'in okuduğu dosyalar, komut çıktıları ve ekran görüntüsü açıklamaları sohbet mesajı olarak Kübik
  sunucusuna gönderilir ve sohbet geçmişinde saklanır. Kişisel ya da gizli dosyaların olduğu klasörleri seçme.

Derleme: `py -3.13 -m PyInstaller --onefile --windowed --name KubikMasaustu masaustu/kubik_masaustu.py`
(gerekenler: `pywebview`, `pillow`).

## Kübik Android

İndir: [Kubik.apk](https://github.com/Asrinata/kubikai/releases/latest/download/Kubik.apk) ·
Kaynak: [`android/MainActivity.java`](android/MainActivity.java)

Siteyi açan ince bir WebView kabuğudur; normal giriş sitedeki gibidir. Kamera izni yalnız **Cube Connect** için
istenir: bilgisayardaki giriş ekranında "Cube Connect"e basınca birbirine bağlı yıldızlardan bir kod çıkar,
telefonda Profil → Cube Connect ile okutunca bilgisayar parolasız giriş yapar (kod 2 dakika geçerli, tek kullanımlık).
Görüntü telefonda işlenir, sunucuya yalnız okunan sayı gider.

## API

Max+ ve üstü planlarda Profil → API platformu'ndan anahtar alınır. Tek dosyalık Python kütüphanesi:
[`kubik.py`](kubik.py) · Ayrıntılar: [docs.html#api](https://asrinata.github.io/kubikai/docs.html#api)

```python
from kubik import Kubik
print(Kubik("kbk_ANAHTARIN").sor("Merhaba!"))
```

## Lisans notu

Kübik'in kullandığı açık modeller (Qwen, Kumru, Phi, EuroLLM vb.) kendi lisanslarıyla (Apache 2.0 / MIT) gelir;
hangilerinin çalıştığı sohbet ekranının altında yazar.
