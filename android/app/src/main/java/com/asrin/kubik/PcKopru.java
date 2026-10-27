package com.asrin.kubik;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/**
 * Kübit PC ajanıyla köprü (KubitPC\kubit_pc.py). Ajan ev ağında UDP yayınıyla bulunur ("KUBITPC?" -> "KUBITPC|ad|port"),
 * komutlar PIN'le HTTP'den gider. Kübit'in hata raporları önce telefonda dosyaya yazılır (Kübit'e "alındı" denir),
 * sonra PC'ye ulaşınca gönderilip silinir: PC kapalıysa rapor kaybolmaz.
 */
class PcKopru {
    private final KubitServis s;
    private String ip, ad; private int port = 47821;
    private long sonArama;

    PcKopru(KubitServis s) { this.s = s; ip = s.ayar().getString("pc_ip", null); }

    String adres() { return ip == null ? "" : ip + ":" + port; }
    String ad() { return ad == null ? "" : ad; }

    /** Ağda ajanı bulur (en çok 1.5 sn), bulursa adresi saklar. Ana iş parçacığında çağrılmamalı. */
    boolean bul() {
        sonArama = System.currentTimeMillis();
        try (DatagramSocket d = new DatagramSocket()) {
            d.setBroadcast(true); d.setSoTimeout(1500);
            byte[] soru = "KUBITPC?".getBytes(StandardCharsets.UTF_8);
            d.send(new DatagramPacket(soru, soru.length, InetAddress.getByName("255.255.255.255"), 47822));
            byte[] tampon = new byte[256]; DatagramPacket p = new DatagramPacket(tampon, tampon.length);
            d.receive(p);
            String[] c = new String(p.getData(), 0, p.getLength(), StandardCharsets.UTF_8).split("\\|");
            if (c.length >= 3 && "KUBITPC".equals(c[0])) {
                ip = p.getAddress().getHostAddress(); ad = c[1]; port = Integer.parseInt(c[2].trim());
                s.ayar().edit().putString("pc_ip", ip).apply();
                return true;
            }
        } catch (Exception ignored) { }
        return false;
    }

    private String istek(String yol, String govde) throws Exception {
        if (ip == null && !bul()) throw new Exception("PC bulunamadi");
        HttpURLConnection c = (HttpURLConnection) new URL("http://" + ip + ":" + port + yol).openConnection();
        c.setConnectTimeout(2000); c.setReadTimeout(5000);
        if (govde != null) { c.setRequestMethod("POST"); c.setDoOutput(true); c.getOutputStream().write(govde.getBytes(StandardCharsets.UTF_8)); }
        int kod = c.getResponseCode();
        InputStream in = kod < 400 ? c.getInputStream() : c.getErrorStream();
        ByteArrayOutputStream b = new ByteArrayOutputStream(); byte[] parca = new byte[2048];
        for (int n; in != null && (n = in.read(parca)) > 0; ) b.write(parca, 0, n);
        if (kod == 403) throw new Exception("PIN yanlis");
        if (kod >= 400) throw new Exception("HTTP " + kod);
        return b.toString("UTF-8");
    }
    private String pin() { return s.ayar().getString("pc_pin", ""); }

    /** PIN yazdırmadan eşleşme: PC'de "izin verilsin mi?" penceresi çıkar, Evet denirse PIN gelir ve saklanır. Ana iş parçacığında çağrılmamalı. */
    String eslestir() {
        if (!bul()) return "Bilgisayar bulunamadı: PC'de Kübit ajanı açık mı, aynı Wi-Fi'da mısın?";
        try {
            HttpURLConnection c = (HttpURLConnection) new URL("http://" + ip + ":" + port + "/eslestir?ad=" + URLEncoder.encode(android.os.Build.MODEL, "UTF-8")).openConnection();
            c.setConnectTimeout(3000); c.setReadTimeout(90_000);       // kullanıcı PC'de pencereye bassın
            c.setRequestMethod("POST"); c.setDoOutput(true); c.getOutputStream().write(new byte[0]);
            if (c.getResponseCode() != 200) return "PC'de izin verilmedi.";
            ByteArrayOutputStream b = new ByteArrayOutputStream(); byte[] parca = new byte[512];
            try (InputStream in = c.getInputStream()) { for (int n; (n = in.read(parca)) > 0; ) b.write(parca, 0, n); }
            JSONObject j = new JSONObject(b.toString("UTF-8"));
            s.ayar().edit().putString("pc_pin", j.getString("pin")).putString("pc_ad", j.optString("ad", ad())).apply();
            return "Eşlendi ✓ " + j.optString("ad", ad());
        } catch (java.net.SocketTimeoutException e) { return "PC'deki pencereye cevap verilmedi.";
        } catch (Exception e) { return "Eşleşme hatası: " + e.getMessage(); }
    }

    /** Kübit'in PC sayfası: durum (Q<ad>|<cpu>|<ram>, boş Q: bulunamadı) */
    void durum() {
        new Thread(() -> {
            try {
                JSONObject j = new JSONObject(istek("/durum?pin=" + URLEncoder.encode(pin(), "UTF-8"), null));
                ad = j.optString("ad", "PC");
                s.disaridanYaz("Q" + KubitServis.temizle(ad, 20) + "|" + j.optInt("cpu") + "|" + j.optInt("ram") + "|" + j.optInt("cpu_c", -1) + "|" + j.optInt("gpu_c", -1) + "|"
                        + j.optInt("gpu", -1) + "|" + j.optInt("fps", -1) + "|" + KubitServis.temizle(j.optString("fps_uyg", ""), 20));
            } catch (Exception e) {
                if ("PIN yanlis".equals(e.getMessage())) s.disaridanYaz("!PC PIN'i yanlis: Kubit uygulamasi");
                if (System.currentTimeMillis() - sonArama > 10000) ip = null;   // belki IP değişti: sonra yeniden ara
                s.disaridanYaz("Q");
            }
        }).start();
    }
    void komut(String k) {
        new Thread(() -> {
            try { istek("/komut?pin=" + URLEncoder.encode(pin(), "UTF-8") + "&k=" + URLEncoder.encode(k, "UTF-8"), ""); }
            catch (Exception e) { s.disaridanYaz("!PC: " + KubitServis.temizle(e.getMessage(), 30)); }
        }).start();
    }

    // ---------------------------------------------------------------- ekran paylaşımı: kareler UDP 47823'e (PC ajanı pencere açar)
    private final java.util.concurrent.ExecutorService ekranIs = java.util.concurrent.Executors.newSingleThreadExecutor();
    private DatagramSocket ekranSoket;
    void ekranYolla(byte[] kare) {
        ekranIs.execute(() -> {
            try {
                if (ip == null) { if (System.currentTimeMillis() - sonArama > 10000) bul(); if (ip == null) return; }
                if (ekranSoket == null) ekranSoket = new DatagramSocket();
                byte[] p = new byte[4 + kare.length];
                p[0] = 'K'; p[1] = 'B'; p[2] = 'E'; p[3] = '1';
                System.arraycopy(kare, 0, p, 4, kare.length);
                ekranSoket.send(new DatagramPacket(p, p.length, InetAddress.getByName(ip), 47823));
            } catch (Exception ignored) { }
        });
    }

    // ---------------------------------------------------------------- hata raporları: telefonda sıra, PC'ye ulaşınca gider
    private File klasor() { File k = new File(s.getFilesDir(), "hatalar"); k.mkdirs(); return k; }
    void hataKaydet(String metin) {
        try (FileOutputStream f = new FileOutputStream(new File(klasor(), "hata_" + System.currentTimeMillis() + ".txt"))) { f.write(metin.getBytes(StandardCharsets.UTF_8)); }
        catch (Exception ignored) { }
        hatalariGonder();
    }
    void hatalariGonder() {
        File[] dosyalar = klasor().listFiles();
        if (dosyalar == null || dosyalar.length == 0) return;
        new Thread(() -> {
            for (File f : dosyalar) {
                try { istek("/hata", new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)); f.delete(); }
                catch (Exception e) { return; }      // PC kapalı: sonra tekrar
            }
        }).start();
    }
}
