package com.asrin.kubik;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInstaller;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.widget.Toast;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/**
 * Uygulama kendini GitHub'dan günceller: v1.0.2 sürümündeki kubik_surum.txt (versionCode) yüklü olandan büyükse
 * Kubik.apk indirilir ve Android'in kurulum onayı açılır (PackageInstaller; ilk seferde "bilinmeyen kaynak" izni istenir).
 * Yayınlarken: android\apk-yayinla.ps1 (APK + kubik_surum.txt birlikte yüklenir).
 */
class Guncelleyici {
    static final String TABAN = "https://github.com/Asrinata/kubikai/releases/download/v1.0.2/";
    interface Sonuc { void bitti(String mesaj); }

    static int yuklu(Context c) {
        try { return (int) (Build.VERSION.SDK_INT >= 28 ? c.getPackageManager().getPackageInfo(c.getPackageName(), 0).getLongVersionCode()
                                                       : c.getPackageManager().getPackageInfo(c.getPackageName(), 0).versionCode); }
        catch (Exception e) { return 0; }
    }
    static String yukluAd(Context c) {
        try { return c.getPackageManager().getPackageInfo(c.getPackageName(), 0).versionName; } catch (Exception e) { return "?"; }
    }

    /** GitHub'daki sürüm (yoksa -1). Ana iş parçacığında çağrılmamalı. */
    static int uzaktaki() {
        try {
            HttpURLConnection c = (HttpURLConnection) new URL(TABAN + "kubik_surum.txt").openConnection();
            c.setConnectTimeout(8000); c.setReadTimeout(8000); c.setUseCaches(false);
            if (c.getResponseCode() != 200) return -1;
            try (InputStream in = c.getInputStream()) { byte[] b = new byte[32]; int n = in.read(b); return Integer.parseInt(new String(b, 0, Math.max(0, n), StandardCharsets.UTF_8).trim()); }
        } catch (Exception e) { return -1; }
    }

    /** Uygulama açılınca: en çok 6 saatte bir sessizce bakar, yeni sürüm varsa sorar. */
    static void sessizKontrol(Activity a) {
        SharedPreferences p = a.getSharedPreferences(KubitServis.AYARLAR, Context.MODE_PRIVATE);
        if (System.currentTimeMillis() - p.getLong("guncelleme_bakildi", 0) < 6 * 3600_000L) return;
        p.edit().putLong("guncelleme_bakildi", System.currentTimeMillis()).apply();
        new Thread(() -> {
            int u = uzaktaki();
            if (u > yuklu(a)) a.runOnUiThread(() -> {
                if (a.isFinishing()) return;
                new AlertDialog.Builder(a).setTitle("Kübik güncellemesi var").setMessage("Yeni sürüm hazır. Şimdi güncellensin mi?")
                        .setPositiveButton("Güncelle", (d, w) -> indirVeKur(a, m -> Toast.makeText(a, m, Toast.LENGTH_LONG).show()))
                        .setNegativeButton("Sonra", null).show();
            });
        }).start();
    }

    /** Elle "Güncellemeleri denetle": sonucu sonuc'a yazar. */
    static void denetle(Activity a, Sonuc sonuc) {
        new Thread(() -> {
            int u = uzaktaki();
            if (u < 0) sonuc.bitti("GitHub'a ulaşılamadı");
            else if (u <= yuklu(a)) sonuc.bitti("Güncel ✓ (" + yukluAd(a) + ")");
            else { sonuc.bitti("Yeni sürüm indiriliyor…"); indirVeKur(a, sonuc); }
        }).start();
    }

    static void indirVeKur(Activity a, Sonuc sonuc) {
        if (Build.VERSION.SDK_INT >= 26 && !a.getPackageManager().canRequestPackageInstalls()) {
            a.runOnUiThread(() -> {
                Toast.makeText(a, "Bir kerelik: Kübik'e \"bilinmeyen uygulama yükleme\" izni ver, sonra tekrar Güncelle'ye bas", Toast.LENGTH_LONG).show();
                a.startActivity(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:" + a.getPackageName())));
            });
            sonuc.bitti("Kurulum izni gerekiyor");
            return;
        }
        new Thread(() -> {
            PackageInstaller pi = a.getPackageManager().getPackageInstaller();
            try {
                HttpURLConnection c = (HttpURLConnection) new URL(TABAN + "Kubik.apk").openConnection();
                c.setConnectTimeout(10000); c.setReadTimeout(30000);
                if (c.getResponseCode() != 200) { sonuc.bitti("İndirilemedi: HTTP " + c.getResponseCode()); return; }
                long toplam = c.getContentLengthLong();
                PackageInstaller.SessionParams sp = new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                if (Build.VERSION.SDK_INT >= 31) sp.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_NOT_REQUIRED);
                int id = pi.createSession(sp);
                try (PackageInstaller.Session s = pi.openSession(id)) {
                    try (InputStream in = c.getInputStream(); OutputStream o = s.openWrite("kubik.apk", 0, toplam)) {
                        byte[] b = new byte[64 * 1024]; long bitti = 0; int son = -1;
                        for (int n; (n = in.read(b)) > 0; ) {
                            o.write(b, 0, n); bitti += n;
                            int y = toplam > 0 ? (int) (100 * bitti / toplam) : 0;
                            if (y / 10 != son) { son = y / 10; sonuc.bitti("İndiriliyor %" + y); }
                        }
                        s.fsync(o);
                    }
                    Intent i = new Intent(a, Alici.class);
                    s.commit(PendingIntent.getBroadcast(a, 7, i, PendingIntent.FLAG_UPDATE_CURRENT | (Build.VERSION.SDK_INT >= 31 ? PendingIntent.FLAG_MUTABLE : 0)).getIntentSender());
                }
                sonuc.bitti("Kuruluyor…");
            } catch (Exception e) { sonuc.bitti("Güncelleme hatası: " + e.getMessage()); }
        }).start();
    }

    /** Kurulum sonucu: onay gerekiyorsa Android'in onay ekranını açar. */
    public static class Alici extends BroadcastReceiver {
        @Override public void onReceive(Context c, Intent i) {
            int durum = i.getIntExtra(PackageInstaller.EXTRA_STATUS, -99);
            if (durum == PackageInstaller.STATUS_PENDING_USER_ACTION) {
                Intent onay = i.getParcelableExtra(Intent.EXTRA_INTENT);
                if (onay != null) c.startActivity(onay.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            } else if (durum != PackageInstaller.STATUS_SUCCESS)
                Toast.makeText(c, "Güncelleme kurulamadı: " + i.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE), Toast.LENGTH_LONG).show();
        }
    }
}
