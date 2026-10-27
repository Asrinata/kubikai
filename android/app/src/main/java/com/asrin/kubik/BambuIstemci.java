package com.asrin.kubik;

import org.json.JSONObject;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.security.cert.X509Certificate;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

/**
 * Bambu Lab yazıcının yerel MQTT'si (port 8883, kullanıcı "bblp", şifre: yazıcının erişim kodu, konu device/<seri>/report).
 * Yazıcı kendi imzalı sertifika kullanır; yalnız ev ağındaki bu IP'ye bağlanıldığı için doğrulanmaz. Kütüphanesiz küçük MQTT 3.1.1.
 * Gelen kısmi JSON güncellemeleri birleştirilir, Kübit'e B<durum>|<yüzde>|<kalan dk>|<dosya>|<nozül>|<tabla>|<katman>|<toplam> gider.
 */
class BambuIstemci extends Thread {
    private final KubitServis s;
    private final String ip, seri, kod;
    private volatile boolean dur;
    private volatile OutputStream cikis;
    private final JSONObject baski = new JSONObject();
    private String sonSatir = "", sonDurum = "";
    private long sonGonderim;

    /**
     * Ağdaki Bambu yazıcıyı bulur: yazıcılar birkaç saniyede bir UDP 2021'e SSDP duyurusu yayınlar
     * ("Location: <ip>", "USN: <seri>", "DevName.bambu.com: <ad>"). En çok 12 sn dinler; {ip, seri, ad} ya da null.
     */
    static String[] bul(android.content.Context ctx) {
        android.net.wifi.WifiManager.MulticastLock kilit = ((android.net.wifi.WifiManager) ctx.getApplicationContext().getSystemService(android.content.Context.WIFI_SERVICE)).createMulticastLock("kubit");
        kilit.acquire();
        try (java.net.DatagramSocket d = new java.net.DatagramSocket(null)) {
            d.setReuseAddress(true); d.setBroadcast(true); d.bind(new InetSocketAddress(2021)); d.setSoTimeout(12000);
            byte[] t = new byte[2048]; long son = System.currentTimeMillis() + 12000;
            while (System.currentTimeMillis() < son) {
                java.net.DatagramPacket p = new java.net.DatagramPacket(t, t.length); d.receive(p);
                String ip = null, seri = null, ad = "";
                for (String satir : new String(p.getData(), 0, p.getLength(), StandardCharsets.UTF_8).split("\r?\n")) {
                    int i = satir.indexOf(':'); if (i < 0) continue;
                    String k = satir.substring(0, i).trim().toLowerCase(java.util.Locale.ROOT), v = satir.substring(i + 1).trim();
                    if (k.equals("location")) ip = v; else if (k.equals("usn")) seri = v; else if (k.equals("devname.bambu.com")) ad = v;
                }
                if (seri != null) return new String[]{ip != null ? ip : p.getAddress().getHostAddress(), seri, ad};
            }
        } catch (Exception ignored) {
        } finally { kilit.release(); }
        return null;
    }

    BambuIstemci(KubitServis s, String ip, String seri, String kod) { this.s = s; this.ip = ip; this.seri = seri; this.kod = kod; setDaemon(true); }
    void durdur() { dur = true; interrupt(); }

    @Override public void run() {
        while (!dur) {
            try { baglanVeOku(); } catch (Exception ignored) { }
            cikis = null;
            try { Thread.sleep(15000); } catch (InterruptedException e) { return; }
        }
    }

    private static void uzunluk(ByteArrayOutputStream b, int n) { do { int x = n % 128; n /= 128; if (n > 0) x |= 128; b.write(x); } while (n > 0); }
    private static void metin(ByteArrayOutputStream b, String m) { byte[] v = m.getBytes(StandardCharsets.UTF_8); b.write(v.length >> 8); b.write(v.length & 255); b.write(v, 0, v.length); }
    private synchronized void paket(int tur, byte[] govde) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream(); b.write(tur); uzunluk(b, govde.length); b.write(govde, 0, govde.length);
        if (cikis != null) { cikis.write(b.toByteArray()); cikis.flush(); }
    }
    void pushall() {
        new Thread(() -> {
            try { ByteArrayOutputStream g = new ByteArrayOutputStream(); metin(g, "device/" + seri + "/request");
                  byte[] v = "{\"pushing\":{\"sequence_id\":\"0\",\"command\":\"pushall\"}}".getBytes(StandardCharsets.UTF_8); g.write(v, 0, v.length); paket(0x30, g.toByteArray()); }
            catch (Exception ignored) { }
        }).start();
    }

    private void baglanVeOku() throws Exception {
        SSLContext ctx = SSLContext.getInstance("TLS");
        ctx.init(null, new TrustManager[]{new X509TrustManager() {
            public void checkClientTrusted(X509Certificate[] c, String a) { }
            public void checkServerTrusted(X509Certificate[] c, String a) { }
            public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
        }}, null);
        try (SSLSocket k = (SSLSocket) ctx.getSocketFactory().createSocket()) {
            k.connect(new InetSocketAddress(ip, 8883), 5000); k.setSoTimeout(30000); k.startHandshake();
            cikis = k.getOutputStream();
            DataInputStream giris = new DataInputStream(k.getInputStream());
            ByteArrayOutputStream c = new ByteArrayOutputStream();          // CONNECT
            metin(c, "MQTT"); c.write(4); c.write(0xC2); c.write(0); c.write(60);
            metin(c, "kubit" + (System.nanoTime() % 100000)); metin(c, "bblp"); metin(c, kod);
            paket(0x10, c.toByteArray());
            ByteArrayOutputStream a = new ByteArrayOutputStream(); a.write(0); a.write(1); metin(a, "device/" + seri + "/report"); a.write(0);
            paket(0x82, a.toByteArray());                                     // SUBSCRIBE
            pushall();
            while (!dur) {
                int tur;
                try { tur = giris.readUnsignedByte(); }
                catch (SocketTimeoutException e) { paket(0xC0, new byte[0]); continue; }   // PINGREQ
                int n = 0, carpan = 1, bayt;
                do { bayt = giris.readUnsignedByte(); n += (bayt & 127) * carpan; carpan *= 128; } while ((bayt & 128) != 0);
                byte[] govde = new byte[n]; giris.readFully(govde);
                if ((tur & 0xF0) == 0x20 && n >= 2 && govde[1] != 0) throw new IOException("MQTT reddetti: erisim kodu?");   // CONNACK
                if ((tur & 0xF0) != 0x30) continue;                         // yalnız PUBLISH
                int konu = ((govde[0] & 255) << 8) | (govde[1] & 255), bas = 2 + konu + (((tur >> 1) & 3) > 0 ? 2 : 0);
                isle(new String(govde, bas, n - bas, StandardCharsets.UTF_8));
            }
        }
    }

    private void isle(String json) {
        try {
            JSONObject p = new JSONObject(json).optJSONObject("print");
            if (p == null) return;
            for (java.util.Iterator<String> i = p.keys(); i.hasNext(); ) { String a = i.next(); baski.put(a, p.get(a)); }   // kısmi güncellemeleri birleştir
            String durum = baski.optString("gcode_state", "IDLE");
            String dosya = baski.optString("subtask_name", baski.optString("gcode_file", ""));
            String satir = "B" + durum + "|" + baski.optInt("mc_percent", -1) + "|" + baski.optInt("mc_remaining_time", 0) + "|" + KubitServis.temizle(dosya, 30) + "|"
                    + Math.round(baski.optDouble("nozzle_temper", 0)) + "|" + Math.round(baski.optDouble("bed_temper", 0)) + "|" + baski.optInt("layer_num", 0) + "|" + baski.optInt("total_layer_num", 0);
            boolean degisti = !durum.equals(sonDurum);
            if (degisti && "FINISH".equals(durum) && !sonDurum.isEmpty()) s.uyariBildirimi("Baskı bitti", dosya);
            if (degisti && "FAILED".equals(durum) && !sonDurum.isEmpty()) s.uyariBildirimi("Baskı hatası", dosya);
            sonDurum = durum;
            if (!satir.equals(sonSatir) && (degisti || System.currentTimeMillis() - sonGonderim > 5000)) { sonSatir = satir; sonGonderim = System.currentTimeMillis(); s.disaridanYaz(satir); }
        } catch (Exception ignored) { }
    }
}
