package com.asrin.kubik;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;

import org.json.JSONObject;

/**
 * "Kübit" uygulama simgesi: PC PIN'i, Bambu yazıcı, unutma uyarısı; Süper Kübit / Geometry bölüm editörleri ve
 * Kübit'in gerçek yazılımıyla (wasm) telefonda oynatıcı. Sayfa assets/kubit/index.html.
 */
public class KubitAyar extends Activity {
    private WebView w;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        w = new WebView(this);
        w.getSettings().setJavaScriptEnabled(true);
        w.addJavascriptInterface(new Kopru(), "Kubit");
        w.loadUrl("file:///android_asset/kubit/index.html");
        w.setBackgroundColor(0xFF0B0C10);
        setContentView(w);
        getWindow().setStatusBarColor(0xFF0B0C10); getWindow().setNavigationBarColor(0xFF0B0C10);
        KubitServis.baslat(this);
        Guncelleyici.sessizKontrol(this);
    }
    @Override protected void onDestroy() { w.destroy(); super.onDestroy(); }

    private SharedPreferences ayar() { return getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE); }

    class Kopru {
        @JavascriptInterface public String ayarlar() {
            SharedPreferences a = ayar(); JSONObject j = new JSONObject();
            try { for (String k : new String[]{"pc_pin", "pc_ad", "bambu_ip", "bambu_seri", "bambu_kod", "bambu_ad"}) j.put(k, a.getString(k, "")); j.put("unutma", a.getBoolean("unutma", true)); }
            catch (Exception ignored) { }
            return j.toString();
        }
        @JavascriptInterface public void ayarKaydet(String json) {
            try {
                JSONObject j = new JSONObject(json); SharedPreferences.Editor e = ayar().edit();
                for (String k : new String[]{"bambu_ip", "bambu_seri", "bambu_kod"}) e.putString(k, j.optString(k, ""));
                e.putBoolean("unutma", j.optBoolean("unutma", true)).apply();
                if (KubitServis.aktif != null) KubitServis.aktif.yaziciBaslat();
            } catch (Exception ignored) { }
        }
        @JavascriptInterface public String durum() {
            KubitServis s = KubitServis.aktif;
            return "{\"eslendi\":" + (ayar().getString(KubitServis.ADRES, null) != null) + ",\"servis\":" + (s != null) + ",\"bagli\":" + (s != null && s.bagliMi()) + "}";
        }
        /** Kübit'in Wi-Fi'ı (güncelleme ve tarayıcı için): "W<ad>\x1f<şifre>" Bluetooth'tan gider, Kübit kendi hafızasına yazar. */
        @JavascriptInterface public String wifiGonder(String ad, String sifre) {
            KubitServis s = KubitServis.aktif;
            if (ad == null || ad.trim().isEmpty()) return "Ağ adını yaz";
            if ((ad + sifre).getBytes(java.nio.charset.StandardCharsets.UTF_8).length > 160) return "Ad ya da şifre çok uzun";
            if (s == null || !s.bagliMi()) return "Kübit bağlı değil: önce bağlansın";
            s.disaridanYaz("W" + ad.trim() + "\u001f" + sifre);
            return "Gönderildi ✓ Kübit'te Ayarlar > Wi-Fi'da görünür";
        }
        /** Telefonun bağlı olduğu ağın adı (konum izni yoksa boş). */
        @JavascriptInterface public String telefonWifi() {
            try {
                String a = ((android.net.wifi.WifiManager) getApplicationContext().getSystemService(WIFI_SERVICE)).getConnectionInfo().getSSID();
                return a == null || a.contains("unknown") ? "" : a.replace("\"", "");
            } catch (Exception e) { return ""; }
        }
        @JavascriptInterface public String surum() { return Guncelleyici.yukluAd(KubitAyar.this); }
        @JavascriptInterface public void guncelle() { Guncelleyici.denetle(KubitAyar.this, m -> sonuc("guncelSonuc", m)); }
        @JavascriptInterface public void pcEsle() {
            KubitServis s = KubitServis.aktif;
            if (s == null) { sonuc("pcSonuc", "Kübit servisi çalışmıyor"); return; }
            new Thread(() -> sonuc("pcSonuc", s.pc.eslestir())).start();
        }
        @JavascriptInterface public void yaziciBul() {
            new Thread(() -> {
                String[] y = BambuIstemci.bul(KubitAyar.this);
                if (y == null) { sonuc("yaziciSonuc", null); return; }
                ayar().edit().putString("bambu_ip", y[0]).putString("bambu_seri", y[1]).putString("bambu_ad", y[2]).apply();
                try { sonuc("yaziciSonuc", new JSONObject().put("ip", y[0]).put("seri", y[1]).put("ad", y[2]).toString()); } catch (Exception ignored) { }
            }).start();
        }
        private void sonuc(String f, String m) { w.post(() -> w.evaluateJavascript(f + "(" + (m == null ? "null" : JSONObject.quote(m)) + ")", null)); }
        @JavascriptInterface public String seviye(String tur) { return ayar().getString("sv_" + tur, ""); }
        @JavascriptInterface public void seviyeSakla(String tur, String veri) { ayar().edit().putString("sv_" + tur, veri).apply(); }
        @JavascriptInterface public String seviyeGonder(String tur, String veri) {
            KubitServis s = KubitServis.aktif;
            if (s == null || !s.bagliMi()) { seviyeSakla(tur, veri); return "Kübit bağlı değil, telefonda kaydedildi."; }
            s.seviyeYolla(tur.charAt(0), veri);
            return "Gönderildi ✓ Kübit'te: Oyunlar > " + ("m".equals(tur) ? "Süper Kübit (haritada sola uzun bas)" : "Geometry > * Ozel bolum");
        }
    }
}
