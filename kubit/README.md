# Kübit anahtarlık — s25

Beetle ESP32, 128×64 SH1106 OLED ve iki tuş için kaynaklar. Son uygulama: Android 1.6.5 (versionCode 12).

## Güncelleme

- Anahtarlık: **Ayarlar → Güncelleme ara**. [Yazılım](https://github.com/Asrinata/kubikai/releases/download/kubit/kubit.bin), [sürüm numarası](https://github.com/Asrinata/kubikai/releases/download/kubit/surum.txt).
- Android: uygulamadaki **Güncellemeleri denetle**. [Kubik.apk](https://github.com/Asrinata/kubikai/releases/download/v1.0.2/Kubik.apk).
- PC'de ekran: [Bluetooth izleyicisi](https://github.com/Asrinata/kubikai/releases/download/kubit/Kubit-Ekran-s24.zip).
- PC'de WhatsApp: [köprü paketi](https://github.com/Asrinata/kubikai/releases/download/kubit/Kubit-WhatsApp-s25.zip).

## Bu sürüm

- Uçak radarı ve ISS ekranlarında harita/bilgi ayrımı, belirgin işaretler ve eski veri durumu.
- Webde okuma ve bağlantı seçimi; geri dönüşte okuma yerini koruma.
- Cihazdan Wi-Fi ağı seçme, şifre girme ve bağlı kalma.
- Kübitcraft: yaratıcı mod, envanter, görevler, kontrol rehberi ve sinyal yakarak tamamlanan hedef.
- Ekranı PC'ye doğrudan Bluetooth ile aktarma; bu yol telefon, ortak Wi-Fi veya özel ağ/firewall kuralı istemez.
- 12 kayıtlı kişi, 120 karakter mesaj, tam numara/metin önizlemesi ve ayrı gönderim onayı.
- Klavyede sağ/solu tutarak hızlı gezinme, iki tuşu tutarak yazma/silme tekrarı. Klavyede çıkış için GERI seçilir.

## WhatsApp sınırları

**Kübit tek başına kişisel WhatsApp istemcisi değildir.** Telefonsuz mod PC açıkken çalışan, resmî olmayan WhatsApp Web köprüsünü kullanır. İlk eşleme ve WhatsApp'ın istediği oturum yenilemeleri gerekir. WhatsApp bu tür bağlantıları desteklemez; hesap kısıtlaması riski vardır. [WhatsApp koşulları](https://faq.whatsapp.com/378279804439436/).

Telefon modunda yeni sohbet hazır metinle açılır ve kullanıcı WhatsApp'taki Gönder düğmesine basar. Hazır taslak, gönderildi olarak gösterilmez. PC modunda istemciye iletilmesi, alıcıya teslim/okundu anlamına gelmez. Sonuç belirsizse otomatik tekrar gönderilmez.

## Kaynaklar ve derleme

- `KubitAnahtarlik/`: ESP32 kaynakları, simülatör ve testler. Yayın derlemesi: `python hizli.py --yayin`; yerel Wi-Fi dosyası bu derlemeye girmez. Arduino ESP32 çekirdeği ve kullanılan Adafruit/NimBLE kütüphaneleri gerekir.
- `KubitPC/`: PC ajanı, Bluetooth ekran izleyicisi ve WhatsApp köprüsü. Başlatıcılar Python 3.13 kullanır; WhatsApp için Node.js ve Chrome/Edge gerekir. Node bağımlılıkları `package-lock.json` ile sabitlenmiştir.
- `../android/`: Android Gradle projesi. Özel imza anahtarı depoda yoktur; genel kaynakların release çıktısı imzasızdır. Yayımlanan APK proje sahibinin mevcut anahtarıyla imzalanır.

Yayın paketlerinde gerçek hesaba bağlı oturum, özel Wi-Fi bilgisi, PC PIN'i ve imza anahtarı bulunmaz. Yerel testler geçmiştir; fiziksel anahtarlıkta canlı WhatsApp gönderimi henüz doğrulanmamıştır.
