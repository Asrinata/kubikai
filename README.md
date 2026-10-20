# Kübik — kolay GitHub Pages paketi

Bu pakette klasör gerekmez. ZIP'i çıkart; içindeki index.html, panel.html, JavaScript ve CSS dosyalarını deponun ana dizinine yükle. Dosya isimlerini değiştirme; index (1).html yerine index.html kullanılır. Aynı isimli mevcut dosyaları güncelle.

Settings > Pages > Source: Deploy from a branch > Branch: main > Folder: /(root) > Save.
Bu pakette GitHub Actions iş akışı gerekmez. Önceden ana dizine konulan pages.yml iş akışı olarak çalışmaz.

Sohbet: https://Asrinata.github.io/kubik/
Panel: https://Asrinata.github.io/kubik/panel.html

Yayın tamamlanınca Ctrl+F5 ile yenile. Pi adresi istenirse Pi'de sudo cat /var/lib/kubik/public-sites.json komutunun api_url değerini kullan. Tünel yeniden başlarsa adresi yenile.

Bu pakette model, kullanıcı veritabanı, yönetici parolası veya yayın anahtarı yoktur. Pi'ye tekrar model paketi indirmen gerekmez; bu düzeltme yalnız web dosyaları içindir.
