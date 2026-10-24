package com.asrin.kubik;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.le.ScanResult;
import android.companion.AssociationRequest;
import android.companion.BluetoothLeDeviceFilter;
import android.companion.CompanionDeviceManager;
import android.content.IntentSender;
import android.location.Location;
import android.location.LocationManager;
import android.provider.Settings;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.webkit.JavascriptInterface;
import android.webkit.PermissionRequest;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

/**
 * Kübik Android: siteyi (GitHub Pages) açan ince kabuk. Normal giriş sitedeki gibi;
 * ek olarak kamera izni verir ki Profil > Cube Connect ile bilgisayardaki yıldız kodu okutulabilsin.
 * Kullanıcı ajanına "KubikApp" eklenir: site uygulamada olduğunu buradan anlar.
 */
public class MainActivity extends Activity {
    private static final String SITE = "https://asrinata.github.io/kubikai/";
    private static final int IZIN_KAMERA = 1001, DOSYA = 1002, ESLE = 1003, IZIN_KUBIT = 1004;
    private WebView web;
    private ValueCallback<Uri[]> dosyaCb;
    private PermissionRequest kameraIstegi;

    @Override
    protected void onCreate(Bundle durum) {
        super.onCreate(durum);
        web = new WebView(this);
        setContentView(web);
        WebSettings a = web.getSettings();
        a.setJavaScriptEnabled(true);
        a.setDomStorageEnabled(true);
        a.setMediaPlaybackRequiresUserGesture(false);
        a.setAllowFileAccess(false);
        a.setTextZoom(100);
        a.setUserAgentString(a.getUserAgentString() + " KubikApp/1.0");
        web.addJavascriptInterface(new Kopru(), "KubikApp");   // yalnız Kübik sitesi yüklenir (dış linkler tarayıcıda açılır)
        web.setBackgroundColor(Color.parseColor("#f8f7f3"));
        web.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest istek) {
                Uri adres = istek.getUrl();
                if ("asrinata.github.io".equals(adres.getHost()) && adres.getPath() != null && adres.getPath().startsWith("/kubikai")) return false;
                try { startActivity(new Intent(Intent.ACTION_VIEW, adres)); } catch (Exception ignored) { }   // dış linkler tarayıcıda
                return true;
            }
        });
        web.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onPermissionRequest(PermissionRequest istek) {
                runOnUiThread(() -> {
                    for (String kaynak : istek.getResources()) if (!PermissionRequest.RESOURCE_VIDEO_CAPTURE.equals(kaynak)) { istek.deny(); return; }
                    if (checkSelfPermission(android.Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) istek.grant(istek.getResources());
                    else { kameraIstegi = istek; requestPermissions(new String[]{android.Manifest.permission.CAMERA}, IZIN_KAMERA); }
                });
            }

            @Override
            public boolean onShowFileChooser(WebView v, ValueCallback<Uri[]> cb, FileChooserParams p) {
                if (dosyaCb != null) dosyaCb.onReceiveValue(null);
                dosyaCb = cb;
                try { startActivityForResult(p.createIntent(), DOSYA); }
                catch (Exception e) { dosyaCb = null; return false; }
                return true;
            }
        });
        Uri gelen = getIntent().getData();
        web.loadUrl(gelen != null ? gelen.toString() : SITE);
        if (eksikIzinler().isEmpty()) KubitServis.baslat(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        konumKaydet();
    }

    /** Kübit'in hava durumu için son bilinen yaklaşık konum (arka planda konum izni istemeden). */
    private void konumKaydet() {
        if (getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE).getString(KubitServis.ADRES, null) == null) return;
        if (checkSelfPermission(android.Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) return;
        LocationManager lm = getSystemService(LocationManager.class);
        Location en = null;
        for (String saglayici : lm.getProviders(true)) {
            try { Location l = lm.getLastKnownLocation(saglayici); if (l != null && (en == null || l.getTime() > en.getTime())) en = l; }
            catch (SecurityException ignored) { }
        }
        if (en != null) getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE).edit()
                .putString(KubitServis.ENLEM, String.format(java.util.Locale.ROOT, "%.2f", en.getLatitude()))     // ~1 km: hava için yeter
                .putString(KubitServis.BOYLAM, String.format(java.util.Locale.ROOT, "%.2f", en.getLongitude())).apply();
    }

    /** Sitedeki Profil > "Kübit anahtarlığını eşle" butonu buradan çağırır. */
    private class Kopru {
        @JavascriptInterface public void kubitEsle() { runOnUiThread(MainActivity.this::kubitEsle); }
        @JavascriptInterface public void kubitAnahtar(String anahtar) {   // yönetici paneli > Kübit anahtarı
            if (anahtar != null && anahtar.startsWith("kbt_"))
                getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE).edit().putString(KubitServis.ANAHTAR, anahtar).apply();
        }
        @JavascriptInterface public void kubitBildirimIzni() {   // bildirimler ve müzik kumandası için sistem ayarı
            runOnUiThread(() -> startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)));
        }
        @JavascriptInterface public boolean kubitEsli() { return getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE).getString(KubitServis.ADRES, null) != null; }
    }

    // Android'in eşli cihaz penceresi: Kübit'i listeler, kullanıcı seçer. Eşli cihaz izni uygulamanın
    // arka planda bağlı kalmasına ve Kübit'teki tuşla Kübik'i öne getirmesine izin verir.
    private void kubitEsle() {
        CompanionDeviceManager cdm = getSystemService(CompanionDeviceManager.class);
        AssociationRequest istek = new AssociationRequest.Builder().setSingleDevice(true)
                .addDeviceFilter(new BluetoothLeDeviceFilter.Builder().setNamePattern(Pattern.compile("Kubit")).build()).build();
        cdm.associate(istek, new CompanionDeviceManager.Callback() {
            @Override public void onDeviceFound(IntentSender secici) {
                try { startIntentSenderForResult(secici, ESLE, null, 0, 0, 0); }
                catch (IntentSender.SendIntentException e) { Toast.makeText(MainActivity.this, "Eşleşme açılamadı", Toast.LENGTH_LONG).show(); }
            }
            @Override public void onFailure(CharSequence hata) {
                Toast.makeText(MainActivity.this, "Kübit bulunamadı. Açık ve yakında mı? Tuş 1'e basıp tekrar dene.", Toast.LENGTH_LONG).show();
            }
        }, null);
    }

    private List<String> eksikIzinler() {
        List<String> eksik = new ArrayList<>();
        if (Build.VERSION.SDK_INT >= 31 && checkSelfPermission(android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED)
            eksik.add(android.Manifest.permission.BLUETOOTH_CONNECT);
        if (Build.VERSION.SDK_INT >= 33 && checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            eksik.add(android.Manifest.permission.POST_NOTIFICATIONS);
        if (checkSelfPermission(android.Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED)
            eksik.add(android.Manifest.permission.ACCESS_COARSE_LOCATION);
        return eksik;
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent.getData() != null) web.loadUrl(intent.getData().toString());
    }

    @Override
    public void onRequestPermissionsResult(int kod, String[] izinler, int[] sonuc) {
        if (kod == IZIN_KUBIT) {
            if (Build.VERSION.SDK_INT < 31 || checkSelfPermission(android.Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED) {
                konumKaydet();
                KubitServis.baslat(this);
                Toast.makeText(this, "Kübit eşlendi!", Toast.LENGTH_SHORT).show();
            } else Toast.makeText(this, "Bluetooth izni olmadan Kübit bağlanamaz", Toast.LENGTH_LONG).show();
            return;
        }
        if (kod == IZIN_KAMERA && kameraIstegi != null) {
            if (sonuc.length > 0 && sonuc[0] == PackageManager.PERMISSION_GRANTED) kameraIstegi.grant(kameraIstegi.getResources());
            else kameraIstegi.deny();
            kameraIstegi = null;
        }
    }

    @Override
    protected void onActivityResult(int kod, int sonuc, Intent veri) {
        if (kod == ESLE) {
            Object cihaz = sonuc == RESULT_OK && veri != null ? veri.getParcelableExtra(CompanionDeviceManager.EXTRA_DEVICE) : null;
            String adres = cihaz instanceof ScanResult ? ((ScanResult) cihaz).getDevice().getAddress()
                    : cihaz instanceof BluetoothDevice ? ((BluetoothDevice) cihaz).getAddress() : null;
            if (adres == null) return;
            getSharedPreferences(KubitServis.AYARLAR, MODE_PRIVATE).edit().putString(KubitServis.ADRES, adres).apply();
            stopService(new Intent(this, KubitServis.class));        // yeni adresle baştan bağlansın
            List<String> eksik = eksikIzinler();
            if (eksik.isEmpty()) onRequestPermissionsResult(IZIN_KUBIT, new String[0], new int[0]);
            else requestPermissions(eksik.toArray(new String[0]), IZIN_KUBIT);
            return;
        }
        if (kod == DOSYA && dosyaCb != null) {
            dosyaCb.onReceiveValue(WebChromeClient.FileChooserParams.parseResult(sonuc, veri));
            dosyaCb = null;
        }
    }

    @Override
    public void onBackPressed() {
        if (web.canGoBack()) web.goBack(); else super.onBackPressed();
    }
}
