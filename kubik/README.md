# Kübik Pages

Bu klasörü yeni GitHub deposunun içeriği olarak yükle. Depoyu public yap ve Settings → Pages → Build and deployment → Source bölümünden GitHub Actions seç. ZIP'i indirip GitHub'da Add file → Upload files ile içeriğini yükleyebilirsin. `https://KULLANICI.github.io/depo/` sohbeti, `https://KULLANICI.github.io/depo/panel/` yönetim sayfasıdır.

1. Pi hazır olunca PC'de `outputs/Baslat-Indirme-Sunucusu.ps1` başlat.
2. `outputs/Kurulum.txt` içindeki tek komutu Pi terminalinde çalıştır.
3. Kurulumda ekrana gelen HTTPS adresini sohbet ve panel sayfasına gir. Adres bu tarayıcıda saklanır.
4. Panel kullanıcısı admin; rastgele parola `outputs/Yonetici-Bilgileri.txt` dosyasında.

GitHub Pages statik dosya sunar. Model, sohbetler, hesaplar, arama, kota ve VM Pi'deki API üzerinden çalışır. Beta erişimi panelden ayarlanır. Pi ve internet açık kalmalıdır. Pi hazır olmadığı için bu statik dosyalar henüz GitHub'a yüklenmedi.

Bu depoya kullanıcı parolası, yayın anahtarı, model ya da yönetici bilgisi koyma. GitHub'a giriş yaptıktan sonra bu klasörü herkese açık bir depoya koyup GitHub Pages'i etkinleştir.
