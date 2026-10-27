package com.asrin.kubik;

import android.app.Activity;
import android.content.ContentValues;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.graphics.ImageFormat;
import android.media.Image;
import android.media.ImageReader;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.provider.MediaStore;
import android.util.Size;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;

/**
 * Kübit'in kamera deklanşörü: Kübit "KAMERA:AC" deyince açılır, "KAMERA:FOTO:<sn>" ile (geri sayımlı) çeker,
 * "KAMERA:CEVIR" ön/arka değiştirir, "KAMERA:KAPAT" kapatır. Fotoğraflar Resimler/Kübit klasörüne kaydedilir.
 */
public class KubitKamera extends Activity {
    static KubitKamera aktif;
    private TextureView goruntu; private TextView yazi;
    private CameraDevice kamera; private CameraCaptureSession oturum; private ImageReader okuyucu;
    private HandlerThread arkaIs; private Handler arka; private final Handler ana = new Handler();
    private boolean on; private int sensorYonu;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        aktif = this;
        if (Build.VERSION.SDK_INT >= 27) { setShowWhenLocked(true); setTurnScreenOn(true); }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        FrameLayout kok = new FrameLayout(this); kok.setBackgroundColor(Color.BLACK);
        goruntu = new TextureView(this); kok.addView(goruntu);
        yazi = new TextView(this); yazi.setTextColor(Color.WHITE); yazi.setTextSize(96); yazi.setGravity(Gravity.CENTER); kok.addView(yazi);
        TextView ipucu = new TextView(this); ipucu.setText("Kübit: sol tuş çek · sağ tuş 3 sn · iki tuş çevir"); ipucu.setTextColor(0xCCFFFFFF); ipucu.setPadding(24, 24, 24, 24);
        kok.addView(ipucu, new FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM));
        setContentView(kok);
        arkaIs = new HandlerThread("kamera"); arkaIs.start(); arka = new Handler(arkaIs.getLooper());
        if (checkSelfPermission(android.Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) requestPermissions(new String[]{android.Manifest.permission.CAMERA}, 1);
        else hazirsaAc();
    }
    @Override public void onRequestPermissionsResult(int k, String[] i, int[] s) { if (s.length > 0 && s[0] == PackageManager.PERMISSION_GRANTED) hazirsaAc(); else finish(); }
    private void hazirsaAc() {
        if (goruntu.isAvailable()) ac();
        else goruntu.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            public void onSurfaceTextureAvailable(SurfaceTexture t, int w, int h) { ac(); }
            public void onSurfaceTextureSizeChanged(SurfaceTexture t, int w, int h) { }
            public boolean onSurfaceTextureDestroyed(SurfaceTexture t) { return true; }
            public void onSurfaceTextureUpdated(SurfaceTexture t) { }
        });
    }

    private void ac() {
        CameraManager cm = getSystemService(CameraManager.class);
        try {
            String secilen = null;
            for (String id : cm.getCameraIdList()) {
                Integer yon = cm.getCameraCharacteristics(id).get(CameraCharacteristics.LENS_FACING);
                if (yon != null && yon == (on ? CameraCharacteristics.LENS_FACING_FRONT : CameraCharacteristics.LENS_FACING_BACK)) { secilen = id; break; }
            }
            if (secilen == null) secilen = cm.getCameraIdList()[0];
            CameraCharacteristics oz = cm.getCameraCharacteristics(secilen);
            Integer sy = oz.get(CameraCharacteristics.SENSOR_ORIENTATION); sensorYonu = sy == null ? 90 : sy;
            StreamConfigurationMap harita = oz.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            Size foto = Collections.max(Arrays.asList(harita.getOutputSizes(ImageFormat.JPEG)), Comparator.comparingLong(s -> (long) s.getWidth() * s.getHeight()));
            Size onizleme = new Size(1280, 720);
            for (Size s : harita.getOutputSizes(SurfaceTexture.class)) if (s.getWidth() <= 1920 && (long) s.getWidth() * s.getHeight() > (long) onizleme.getWidth() * onizleme.getHeight() && s.getWidth() * 9 == s.getHeight() * 16) onizleme = s;
            if (okuyucu != null) okuyucu.close();
            okuyucu = ImageReader.newInstance(foto.getWidth(), foto.getHeight(), ImageFormat.JPEG, 2);
            okuyucu.setOnImageAvailableListener(r -> { try (Image g = r.acquireLatestImage()) { if (g != null) kaydet(g); } }, arka);
            SurfaceTexture st = goruntu.getSurfaceTexture(); st.setDefaultBufferSize(onizleme.getWidth(), onizleme.getHeight());
            Surface yuzey = new Surface(st);
            cm.openCamera(secilen, new CameraDevice.StateCallback() {
                @Override public void onOpened(CameraDevice c) {
                    kamera = c;
                    try {
                        c.createCaptureSession(Arrays.asList(yuzey, okuyucu.getSurface()), new CameraCaptureSession.StateCallback() {
                            @Override public void onConfigured(CameraCaptureSession o) {
                                oturum = o;
                                try { CaptureRequest.Builder r = c.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW); r.addTarget(yuzey);
                                      r.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE); o.setRepeatingRequest(r.build(), null, arka); }
                                catch (CameraAccessException ignored) { }
                            }
                            @Override public void onConfigureFailed(CameraCaptureSession o) { }
                        }, arka);
                    } catch (CameraAccessException ignored) { }
                }
                @Override public void onDisconnected(CameraDevice c) { c.close(); }
                @Override public void onError(CameraDevice c, int e) { c.close(); }
            }, arka);
        } catch (SecurityException | CameraAccessException ignored) { }
    }
    private void kapat() {
        try { if (oturum != null) oturum.close(); if (kamera != null) kamera.close(); } catch (Exception ignored) { }
        oturum = null; kamera = null;
    }

    void foto(int gecikme) {
        ana.post(() -> {
            if (gecikme > 0) { for (int i = gecikme; i > 0; i--) { int n = i; ana.postDelayed(() -> yazi.setText(String.valueOf(n)), (gecikme - i) * 1000L); }
                               ana.postDelayed(() -> { yazi.setText(""); cek(); }, gecikme * 1000L); }
            else cek();
        });
    }
    private void cek() {
        if (kamera == null || oturum == null) return;
        try {
            CaptureRequest.Builder r = kamera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE); r.addTarget(okuyucu.getSurface());
            int ekran = getWindowManager().getDefaultDisplay().getRotation() * 90;
            r.set(CaptureRequest.JPEG_ORIENTATION, on ? (sensorYonu + ekran) % 360 : (sensorYonu - ekran + 360) % 360);
            oturum.capture(r.build(), null, arka);
            ana.post(() -> { yazi.setBackgroundColor(Color.WHITE); ana.postDelayed(() -> yazi.setBackgroundColor(Color.TRANSPARENT), 120); });   // flaş efekti
        } catch (CameraAccessException ignored) { }
    }
    private void kaydet(Image g) {
        ByteBuffer b = g.getPlanes()[0].getBuffer(); byte[] veri = new byte[b.remaining()]; b.get(veri);
        String ad = "Kubit_" + new java.text.SimpleDateFormat("yyyyMMdd_HHmmss", java.util.Locale.ROOT).format(new java.util.Date()) + ".jpg";
        try {
            if (Build.VERSION.SDK_INT >= 29) {
                ContentValues d = new ContentValues(); d.put(MediaStore.Images.Media.DISPLAY_NAME, ad); d.put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg");
                d.put(MediaStore.Images.Media.RELATIVE_PATH, Environment.DIRECTORY_PICTURES + "/Kübit");
                Uri u = getContentResolver().insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, d);
                try (OutputStream o = getContentResolver().openOutputStream(u)) { o.write(veri); }
            } else try (FileOutputStream o = new FileOutputStream(new File(getExternalFilesDir(Environment.DIRECTORY_PICTURES), ad))) { o.write(veri); }
            if (KubitServis.aktif != null) KubitServis.aktif.disaridanYaz("!Foto kaydedildi");
        } catch (Exception e) { if (KubitServis.aktif != null) KubitServis.aktif.disaridanYaz("!Foto kaydedilemedi"); }
    }
    void cevir() { ana.post(() -> { on = !on; kapat(); ac(); }); }

    @Override protected void onDestroy() { kapat(); if (okuyucu != null) okuyucu.close(); arkaIs.quitSafely(); if (aktif == this) aktif = null; super.onDestroy(); }
}
