package com.asrin.kubik;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.webkit.PermissionRequest;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/**
 * Kübik Android: siteyi (GitHub Pages) açan ince kabuk. Normal giriş sitedeki gibi;
 * ek olarak kamera izni verir ki Profil > Cube Connect ile bilgisayardaki yıldız kodu okutulabilsin.
 * Kullanıcı ajanına "KubikApp" eklenir: site uygulamada olduğunu buradan anlar.
 */
public class MainActivity extends Activity {
    private static final String SITE = "https://asrinata.github.io/kubikai/";
    private static final int IZIN_KAMERA = 1001, DOSYA = 1002;
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
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent.getData() != null) web.loadUrl(intent.getData().toString());
    }

    @Override
    public void onRequestPermissionsResult(int kod, String[] izinler, int[] sonuc) {
        if (kod == IZIN_KAMERA && kameraIstegi != null) {
            if (sonuc.length > 0 && sonuc[0] == PackageManager.PERMISSION_GRANTED) kameraIstegi.grant(kameraIstegi.getResources());
            else kameraIstegi.deny();
            kameraIstegi = null;
        }
    }

    @Override
    protected void onActivityResult(int kod, int sonuc, Intent veri) {
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
