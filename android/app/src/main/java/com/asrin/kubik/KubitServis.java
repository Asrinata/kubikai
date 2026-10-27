package com.asrin.kubik;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.IntentFilter;
import android.database.Cursor;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.ToneGenerator;
import android.os.BatteryManager;
import android.provider.CalendarContract;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ServiceInfo;
import android.location.Address;
import android.location.Geocoder;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.MediaMetadata;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Locale;
import java.util.UUID;

/**
 * Kübit anahtarlığıyla köprü. Eşlenen cihaza BLE ile bağlı kalır: bağlantı koparsa (Kübit yeniden başladı, uyudu) doğrudan bağlanmayı
 * artan aralıklarla dener; Android'in autoConnect'i Kübit yeniden başlayınca çoğu telefonda "bağlı" sanıp takılıyordu.
 * Bekçi: 75 sn boyunca hiçbir yazma/bildirim başarılı olmazsa bağlantıyı yeniden kurar. Bildirimde "Şimdi bağlan" ve otomatik bağlan ayarı var.
 * Kübit'e yazılan satırların ilk harfi türünü söyler (S durum, H hava, O özet, N bildirim, M müzik, A/C/E cevap, ! mesaj);
 * Kübit'ten gelen komutlar: AC, YENILE, CMD:x, SOR:x, BUL, BULDUR, MUZ:x, ARAMA:RED/SES, ALARM:SS:DD, BIP, TEL:YENILE, YEDEK:GERI ve Y yedek satırları.
 * Telefondan ek satırlar: P pil|şarj, R arayan (boş: bitti), T takvim başlık|yerel epoch, D adım|hedef, Z yedekten geri yükleme. Protokolün diğer ucu KubitAnahtarlik.ino.
 */
public class KubitServis extends Service {
    static final String AYARLAR = "kubit", ADRES = "adres", ANAHTAR = "anahtar", ENLEM = "enlem", BOYLAM = "boylam", OTO = "oto_baglan";
    private static final String EYLEM_BAGLAN = "com.asrin.kubik.KUBIT_BAGLAN", EYLEM_OTO = "com.asrin.kubik.KUBIT_OTO", EYLEM_ALARM = "com.asrin.kubik.KUBIT_ALARM";
    // Kübit'in kodundaki UUID'lerle aynı olmalı (KubitAnahtarlik.ino)
    private static final UUID SERVIS = UUID.fromString("8f1d0001-6b75-4269-7421-4b7562697400");
    private static final UUID DURUM = UUID.fromString("8f1d0002-6b75-4269-7421-4b7562697400");
    private static final UUID TUS = UUID.fromString("8f1d0003-6b75-4269-7421-4b7562697400");
    private static final UUID CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final String SUNUCU = "https://kubik-ai.asrinatayilmazok.workers.dev";
    private static final String KANAL = "kubit";
    private static final long ARALIK_MS = 30_000, HAVA_ARALIK_MS = 30 * 60_000;

    static KubitServis aktif;                       // bildirim dinleyicisi buradan Kübit'e iletir
    private final Handler ana = new Handler(Looper.getMainLooper());
    private final ArrayDeque<String> kuyruk = new ArrayDeque<>();   // Android aynı anda tek GATT yazmasına izin verir
    private boolean yaziyor;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic durumKar;
    private String sonBildirim = "", havaSatiri, yer = "";
    private long havaZamani;
    private Ringtone zil;
    private int eskiSes = -1;
    private long sonBasari, bekleme = 3000;          // son başarılı BLE işi; yeniden deneme aralığı (3 sn -> 20 sn)
    private boolean bagli;
    private String bildirimYazi = "Bağlanıyor…";
    private int sonPil = -1; private boolean sonSarj; private long sonAdim = -1, takvimZamani;
    private SensorManager sensorler; private java.util.ArrayList<String> yedekSatir;
    private Notification aramaBildirimi; private boolean sesKapatildi;
    PcKopru pc; private BambuIstemci yazici; private StringBuilder hataMetni; private boolean uyuyor;
    private final java.util.Map<Character, String[]> seviyeParca = new java.util.HashMap<>();
    private final BroadcastReceiver pilAlici = new BroadcastReceiver() {
        @Override public void onReceive(Context c, Intent i) {
            int seviye = i.getIntExtra(BatteryManager.EXTRA_LEVEL, -1), olcek = i.getIntExtra(BatteryManager.EXTRA_SCALE, 100), durum = i.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
            int yuzde = seviye < 0 ? -1 : seviye * 100 / Math.max(1, olcek); boolean sarj = durum == BatteryManager.BATTERY_STATUS_CHARGING || durum == BatteryManager.BATTERY_STATUS_FULL;
            if (yuzde != sonPil || sarj != sonSarj) { sonPil = yuzde; sonSarj = sarj; pilGonder(); }
        }
    };
    private final SensorEventListener adimDinle = new SensorEventListener() {   // adım sayacı açılıştan beri sayar: gün başındaki değer çıkarılır
        @Override public void onSensorChanged(SensorEvent e) {
            long toplam = (long) e.values[0]; int gun = java.util.Calendar.getInstance().get(java.util.Calendar.DAY_OF_YEAR);
            SharedPreferences a = ayar();
            if (a.getInt("adim_gun", -1) != gun || toplam < a.getLong("adim_taban", 0)) a.edit().putInt("adim_gun", gun).putLong("adim_taban", toplam).apply();
            long adim = toplam - a.getLong("adim_taban", toplam);
            if (Math.abs(adim - sonAdim) >= 10 || sonAdim < 0) { sonAdim = adim; yaz("D" + adim + "|8000"); }
        }
        @Override public void onAccuracyChanged(Sensor s, int d) { }
    };

    static void baslat(Context ctx) {
        if (ctx.getSharedPreferences(AYARLAR, MODE_PRIVATE).getString(ADRES, null) == null) return;
        Intent i = new Intent(ctx, KubitServis.class);
        if (Build.VERSION.SDK_INT >= 26) ctx.startForegroundService(i); else ctx.startService(i);
    }

    SharedPreferences ayar() { return getSharedPreferences(AYARLAR, MODE_PRIVATE); }

    @Override
    public void onCreate() {
        super.onCreate();
        aktif = this;
        NotificationManager nm = getSystemService(NotificationManager.class);
        nm.createNotificationChannel(new NotificationChannel(KANAL, "Kübit anahtarlığı", NotificationManager.IMPORTANCE_LOW));
        if (Build.VERSION.SDK_INT >= 29) startForeground(1, bildirim("Bağlanıyor…"), ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE);
        else startForeground(1, bildirim("Bağlanıyor…"));
        baglan();
        ana.post(dongu);
        registerReceiver(pilAlici, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        adimBaslat();
        pc = new PcKopru(this);
        yaziciBaslat();
    }

    /** Başka sınıflardan (PC köprüsü, yazıcı, kamera, ayar ekranı) Kübit'e satır yaz. */
    void disaridanYaz(String satir) { ana.post(() -> yaz(satir)); }
    void yaziciBaslat() {
        if (yazici != null) { yazici.durdur(); yazici = null; }
        SharedPreferences a = ayar(); String ip = a.getString("bambu_ip", ""), seri = a.getString("bambu_seri", ""), kod = a.getString("bambu_kod", "");
        if (!ip.isEmpty() && !seri.isEmpty() && !kod.isEmpty()) { yazici = new BambuIstemci(this, ip, seri, kod); yazici.start(); }
    }
    boolean bagliMi() { return bagli && durumKar != null; }
    /** Ayar ekranındaki editörden: bölümü Kübit'e L satırlarıyla gönder (150 karakterlik parçalar). */
    void seviyeYolla(char tur, String veri) {
        ayar().edit().putString("sv_" + tur, veri).apply();
        int parca = (veri.length() + 149) / 150;
        ana.post(() -> { for (int p = 0; p < parca; p++) yaz("L" + tur + "|" + p + "|" + parca + "|" + veri.substring(p * 150, Math.min(veri.length(), p * 150 + 150))); });
    }
    private static final String UYARI_KANAL = "kubit_uyari";
    void uyariBildirimi(String baslik, String metin) { uyariBildirimi(baslik, metin, null); }
    private void uyariBildirimi(String baslik, String metin, PendingIntent dokun) {
        NotificationManager nm = getSystemService(NotificationManager.class);
        nm.createNotificationChannel(new NotificationChannel(UYARI_KANAL, "Kübit uyarıları", NotificationManager.IMPORTANCE_HIGH));
        Notification.Builder b = new Notification.Builder(this, UYARI_KANAL).setSmallIcon(android.R.drawable.stat_notify_error).setContentTitle(baslik).setContentText(metin).setAutoCancel(true);
        if (dokun != null) b.setContentIntent(dokun);
        nm.notify((int) (System.currentTimeMillis() % 100000) + 10, b.build());
    }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        String e = intent == null ? null : intent.getAction();
        if (EYLEM_BAGLAN.equals(e)) { bekleme = 3000; yenidenBaglan(0); }
        else if (EYLEM_ALARM.equals(e)) { zilCal(); bildir("⏰ Kübit alarmı"); }
        else if (EYLEM_OTO.equals(e)) {
            boolean oto = !otoAcik();
            ayar().edit().putBoolean(OTO, oto).apply();
            if (oto && !bagli) yenidenBaglan(0);
            bildir(bildirimYazi);
        }
        return START_STICKY;
    }

    private boolean otoAcik() { return ayar().getBoolean(OTO, true); }
    @Override public IBinder onBind(Intent intent) { return null; }

    @Override
    public void onDestroy() {
        aktif = null;
        ana.removeCallbacksAndMessages(null);
        zilDurdur();
        kapat();
        try { unregisterReceiver(pilAlici); } catch (Exception ignored) { }
        if (sensorler != null) sensorler.unregisterListener(adimDinle);
        if (yazici != null) yazici.durdur();
        super.onDestroy();
    }

    private Notification bildirim(String yazi) {
        PendingIntent ac = PendingIntent.getActivity(this, 0, new Intent(this, MainActivity.class), PendingIntent.FLAG_IMMUTABLE);
        PendingIntent baglanPI = PendingIntent.getService(this, 1, new Intent(this, KubitServis.class).setAction(EYLEM_BAGLAN), PendingIntent.FLAG_IMMUTABLE);
        PendingIntent otoPI = PendingIntent.getService(this, 2, new Intent(this, KubitServis.class).setAction(EYLEM_OTO), PendingIntent.FLAG_IMMUTABLE);
        return new Notification.Builder(this, KANAL).setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
                .setContentTitle("Kübit").setContentText(yazi).setContentIntent(ac).setOngoing(true)
                .addAction(new Notification.Action.Builder(null, "Şimdi bağlan", baglanPI).build())
                .addAction(new Notification.Action.Builder(null, "Otomatik bağlan: " + (otoAcik() ? "Açık" : "Kapalı"), otoPI).build())
                .build();
    }

    private void bildir(String yazi) { bildirimYazi = yazi; getSystemService(NotificationManager.class).notify(1, bildirim(yazi)); }

    // ---------------------------------------------------------------- BLE
    private void baglan() {
        String adres = ayar().getString(ADRES, null);
        BluetoothAdapter bt = ((BluetoothManager) getSystemService(BLUETOOTH_SERVICE)).getAdapter();
        if (adres == null || bt == null) { stopSelf(); return; }
        kapat();
        try { gatt = bt.getRemoteDevice(adres).connectGatt(this, false, geri, BluetoothDevice.TRANSPORT_LE); }   // doğrudan: Kübit yayındaysa saniyeler içinde bağlanır
        catch (SecurityException e) { bildir("Bluetooth izni yok · Kübik'i açıp izin ver"); }
    }

    private void kapat() {
        bagli = false; durumKar = null; kuyruk.clear(); yaziyor = false;
        try { if (gatt != null) { gatt.disconnect(); gatt.close(); } } catch (SecurityException ignored) { }
        gatt = null;
    }

    private final Runnable tekrarDene = this::baglan;
    private void yenidenBaglan(long gecikme) {
        ana.removeCallbacks(tekrarDene);
        ana.postDelayed(tekrarDene, gecikme);
    }

    private final BluetoothGattCallback geri = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt g, int durum, int yeni) {
            try {
                if (yeni == BluetoothProfile.STATE_CONNECTED) {
                    ana.post(() -> { bagli = true; bekleme = 3000; sonBasari = System.currentTimeMillis(); ana.removeCallbacks(unutmaUyar); });
                    bildir("Bağlı");
                    if (!g.requestMtu(185)) g.discoverServices();
                } else if (yeni == BluetoothProfile.STATE_DISCONNECTED) {
                    ana.post(() -> {
                        if (g != gatt) return;                 // eski bağlantının geç gelen haberi
                        boolean idi = bagli;
                        kapat();
                        if (idi && !uyuyor) unutmaBekle();
                        uyuyor = false;
                        if (otoAcik()) { yenidenBaglan(bekleme); bekleme = Math.min(20_000, bekleme * 2); bildir("Kübit'e bağlanılıyor…"); }
                        else bildir("Bağlı değil · Şimdi bağlan'a dokun");
                    });
                }
            } catch (SecurityException ignored) { }
        }

        @Override
        public void onMtuChanged(BluetoothGatt g, int mtu, int durum) {
            try { g.discoverServices(); } catch (SecurityException ignored) { }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt g, int durum) {
            BluetoothGattService s = g.getService(SERVIS);
            if (s == null) {                                  // Kübit yeniden başladıysa Android eski servis listesini tutabiliyor: önbelleği sil, yeniden bağlan
                try { g.getClass().getMethod("refresh").invoke(g); } catch (Exception ignored) { }
                ana.post(() -> { kapat(); yenidenBaglan(1000); });
                return;
            }
            BluetoothGattCharacteristic tus = s.getCharacteristic(TUS);
            try {
                g.setCharacteristicNotification(tus, true);
                BluetoothGattDescriptor d = tus.getDescriptor(CCCD);
                d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                g.writeDescriptor(d);   // bitince onDescriptorWrite yazmaya başlar
            } catch (SecurityException ignored) { }
            ana.post(() -> { durumKar = s.getCharacteristic(DURUM); yaziyor = true; });   // tanımlayıcı yazılana kadar bekle
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt g, BluetoothGattDescriptor d, int durum) {
            ana.post(() -> { yaziyor = false; durumGonder(); if (havaSatiri != null) yaz(havaSatiri); telefonDurumu(); siradaki(); });
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt g, BluetoothGattCharacteristic k, int durum) {
            ana.post(() -> {
                yaziyor = false;
                if (durum == BluetoothGatt.GATT_SUCCESS) { sonBasari = System.currentTimeMillis(); siradaki(); }
                else { kapat(); yenidenBaglan(1000); }        // yazılamıyorsa bağlantı ölü
            });
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt g, BluetoothGattCharacteristic k) {
            String komut = new String(k.getValue(), StandardCharsets.UTF_8);
            ana.post(() -> { sonBasari = System.currentTimeMillis(); komutIsle(komut); });
        }
    };

    /** Kübit'e bir satır yaz (ana iş parçacığında çağrılır). */
    private void yaz(String satir) {
        if (durumKar == null) return;
        kuyruk.add(satir);
        siradaki();
    }

    private void siradaki() {
        if (yaziyor || durumKar == null || gatt == null || kuyruk.isEmpty()) return;
        try {
            durumKar.setValue(kuyruk.peek().getBytes(StandardCharsets.UTF_8));
            durumKar.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
            yaziyor = gatt.writeCharacteristic(durumKar);
            if (yaziyor) kuyruk.poll(); else ana.postDelayed(this::siradaki, 200);   // meşgulse az sonra tekrar
        } catch (SecurityException ignored) { }
    }

    private void komutIsle(String komut) {
        if ("AC".equals(komut)) startActivity(new Intent(this, MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT));
        else if ("YENILE".equals(komut)) { durumGonder(); havaZamani = 0; havaGuncelle(); }
        else if (komut.startsWith("CMD:")) kumanda(komut.substring(4));
        else if (komut.startsWith("SOR:")) sor(komut.substring(4));
        else if (komut.startsWith("WA:HAZIRLA:")) whatsappHazirla(komut.substring(11));
        else if ("BUL".equals(komut)) zilCal();
        else if ("BULDUR".equals(komut)) zilDurdur();
        else if (komut.startsWith("MUZ:")) muzik(komut.substring(4));
        else if ("ARAMA:RED".equals(komut)) aramaReddet();
        else if ("ARAMA:SES".equals(komut)) aramaSessiz();
        else if (komut.startsWith("ALARM:")) alarmKur(komut.substring(6));
        else if ("BIP".equals(komut)) bip();
        else if ("TEL:YENILE".equals(komut)) { sonAdim = -1; takvimZamani = 0; telefonDurumu(); }
        else if ("YEDEK:GERI".equals(komut)) yedekGeri();
        else if ("Y+".equals(komut)) yedekSatir = new java.util.ArrayList<>();
        else if ("Y.".equals(komut)) yedekKaydet();
        else if (komut.startsWith("Y") && yedekSatir != null) yedekSatir.add(komut.substring(1));
        else if ("UYUYOR".equals(komut)) { uyuyor = true; if (ayar().getBoolean("yakinlik", false)) pc.komut("yakinlik_uyku"); }   // uyurken PC'yi kilitletme
        else if (komut.startsWith("PC:yakinlik_")) { ayar().edit().putBoolean("yakinlik", komut.endsWith("_ac")).apply(); pc.komut(komut.substring(3)); }
        else if (komut.startsWith("EKR:")) ekranParca(komut.substring(4));
        else if (komut.startsWith("RADAR:")) radar(komut.substring(6));
        else if (komut.startsWith("ROTA:")) rota(komut.substring(5));
        else if ("UZAY:ISS".equals(komut)) iss();
        else if ("UZAY:ISSYOL".equals(komut)) issYol();
        else if (komut.startsWith("PC:")) { if ("durum".equals(komut.substring(3))) pc.durum(); else pc.komut(komut.substring(3)); }
        else if (komut.startsWith("KAMERA:")) kamera(komut.substring(7));
        else if ("YAZICI:YENILE".equals(komut)) { if (yazici != null) yazici.pushall(); else yaz("BYOK|0|0|Kubit uygulamasi > Yazici|0|0|0|0"); }
        else if (komut.startsWith("L") && komut.length() > 2 && komut.charAt(2) == '|') seviyeParcasi(komut.substring(1));
        else if (komut.matches("X\\+\\d+")) hataMetni = new StringBuilder();
        else if (komut.matches("X\\.\\d+")) { if (hataMetni != null) pc.hataKaydet(hataMetni.toString()); hataMetni = null; yaz("x" + komut.substring(2)); }
        else if (komut.startsWith("X") && hataMetni != null) hataMetni.append(komut.substring(1)).append('\n');
    }

    // WhatsApp yeni sohbet: Android'de bildirimden acilan, kullanicinin Gonder'e bastigi taslak.
    private void whatsappHazirla(String veri) {
        String[] p = veri.split("\\|", 3);
        if (p.length != 3 || !p[0].matches("[1-9][0-9]{0,9}") || !p[1].matches("[1-9][0-9]{7,14}") || p[2].trim().isEmpty() || p[2].length() > 120) return;
        String id = p[0];
        try {
            String pkg = "com.whatsapp";
            try { getPackageManager().getPackageInfo(pkg, 0); }
            catch (android.content.pm.PackageManager.NameNotFoundException e) { pkg = "com.whatsapp.w4b"; getPackageManager().getPackageInfo(pkg, 0); }
            if (Build.VERSION.SDK_INT >= 33 && checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS) != android.content.pm.PackageManager.PERMISSION_GRANTED) { yaz("F" + id + "|ERR|Telefonda bildirim izni ver"); return; }
            NotificationManager nm = getSystemService(NotificationManager.class);
            NotificationChannel ch = nm.getNotificationChannel(UYARI_KANAL);
            if (!nm.areNotificationsEnabled() || (ch != null && ch.getImportance() == NotificationManager.IMPORTANCE_NONE)) { yaz("F" + id + "|ERR|Kubit bildirimlerini telefonda ac"); return; }
            android.net.Uri u = android.net.Uri.parse("https://wa.me/" + p[1]).buildUpon().appendQueryParameter("text", p[2]).build();
            Intent i = new Intent(Intent.ACTION_VIEW, u).setPackage(pkg).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pi = PendingIntent.getActivity(this, id.hashCode() & 0x7fffffff, i, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
            uyariBildirimi("Kübit → WhatsApp · +" + p[1], "Mesaj hazır. Dokun, WhatsApp'ta Gönder'e bas.", pi);
            yaz("F" + id + "|READY|Telefonda bildirime dokun; WhatsApp ta Gonder e bas");
        } catch (android.content.pm.PackageManager.NameNotFoundException e) { yaz("F" + id + "|ERR|Telefonda WhatsApp kurulu degil"); }
        catch (Exception e) { yaz("F" + id + "|ERR|WhatsApp taslagi hazirlanamadi"); }
    }

    // ---------------------------------------------------------------- unutma uyarısı: Kübit beklenmedik koptuysa (uyumadı, kapanmadı)
    private final Runnable unutmaUyar = () -> {
        if (bagli || !ayar().getBoolean("unutma", true)) return;
        String enlem = ayar().getString(ENLEM, null), boylam = ayar().getString(BOYLAM, null);
        PendingIntent harita = enlem == null ? null : PendingIntent.getActivity(this, 5, new Intent(Intent.ACTION_VIEW, android.net.Uri.parse("geo:" + enlem + "," + boylam + "?q=" + enlem + "," + boylam + "(Kubit)")), PendingIntent.FLAG_IMMUTABLE);
        uyariBildirimi("Kübit'i unuttun mu?", "Anahtarlıkla bağlantı koptu" + (harita != null ? " · son konum için dokun" : ""), harita);
    };
    private void unutmaBekle() { ana.removeCallbacks(unutmaUyar); ana.postDelayed(unutmaUyar, 25_000); }

    // ---------------------------------------------------------------- kamera deklanşörü
    private void kamera(String k) {
        KubitKamera kam = KubitKamera.aktif;
        if ("AC".equals(k)) { if (kam == null) startActivity(new Intent(this, KubitKamera.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)); }
        else if ("KAPAT".equals(k)) { if (kam != null) kam.finish(); }
        else if ("CEVIR".equals(k)) { if (kam != null) kam.cevir(); }
        else if (k.startsWith("FOTO:")) {
            int gecikme; try { gecikme = Integer.parseInt(k.substring(5)); } catch (Exception e) { gecikme = 0; }
            if (kam != null) kam.foto(gecikme);
            else { startActivity(new Intent(this, KubitKamera.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)); int g2 = gecikme; ana.postDelayed(() -> { if (KubitKamera.aktif != null) KubitKamera.aktif.foto(g2); }, 2500); }
        }
    }

    // ---------------------------------------------------------------- keşif: uçak radarı (adsb.lol + adsbdb) ve ISS (wheretheiss.at); konum hava durumundakiyle aynı
    private static String duz(String s) {             // Kübit'in yazı tipi: aksanları at (Türkçe harfleri Kübit kendisi çevirir)
        return s == null ? "" : java.text.Normalizer.normalize(s, java.text.Normalizer.Form.NFD).replaceAll("\\p{M}", "");
    }
    private void radar(String kmYazi) {
        String enlem = ayar().getString(ENLEM, null), boylam = ayar().getString(BOYLAM, null);
        if (enlem == null) { yaz("!Konum yok: Kubik'i bir kez ac"); yaz("U0"); return; }
        arkada(() -> {
            try {
                double km = Math.max(5, Math.min(400, Double.parseDouble(kmYazi))), la0 = Double.parseDouble(enlem), lo0 = Double.parseDouble(boylam);
                JSONObject j = istek("https://api.adsb.lol/v2/point/" + enlem + "/" + boylam + "/" + Math.round(km / 1.852 + 1), null);
                org.json.JSONArray ac = j.optJSONArray("ac");
                java.util.List<double[]> sira = new java.util.ArrayList<>(); java.util.List<JSONObject> u = new java.util.ArrayList<>();
                for (int i = 0; ac != null && i < ac.length(); i++) {
                    JSONObject a = ac.getJSONObject(i);
                    if (!a.has("lat")) continue;
                    double dy = (a.getDouble("lat") - la0) * 111.32, dx = (a.getDouble("lon") - lo0) * 111.32 * Math.cos(Math.toRadians(la0)), d = Math.hypot(dx, dy);
                    if (d > km) continue;
                    sira.add(new double[]{d, u.size(), dx, dy}); u.add(a);
                }
                sira.sort((p, q) -> Double.compare(p[0], q[0]));
                int n = Math.min(20, sira.size());
                java.util.List<String> satirlar = new java.util.ArrayList<>();
                if (n == 0) satirlar.add("U0");
                for (int i = 0; i < n; i++) {
                    double[] s = sira.get(i); JSONObject a = u.get((int) s[1]);
                    Object alt = a.opt("alt_baro");
                    int irtifa = alt instanceof Number ? (int) Math.round(((Number) alt).doubleValue() * 0.3048) : 0;   // "ground" = yerde
                    String cs = a.optString("flight", "").trim(); if (cs.isEmpty()) cs = a.optString("r", a.optString("hex", "?"));
                    satirlar.add("U" + n + "|" + i + "|" + temizle(cs, 10) + "|" + temizle(a.optString("t", "?"), 6) + "|" + irtifa + "|" + Math.round(a.optDouble("gs", 0) * 1.852) + "|"
                            + Math.round(a.optDouble("track", a.optDouble("true_heading", 0))) + "|" + Math.round(s[2] * 10) + "|" + Math.round(s[3] * 10) + "|"
                            + temizle(a.optString("r", ""), 10) + "|" + a.optString("hex", "") + "|" + Math.round(a.optDouble("baro_rate", 0) * 0.3048));
                }
                ana.post(() -> { for (String s : satirlar) yaz(s); });
            } catch (Exception e) { ana.post(() -> { yaz("!Radar: " + temizle(e.getMessage(), 30)); yaz("U0"); }); }
        });
    }
    private void rota(String g) {                    // ROTA:<çağrı>|<hex>
        String[] p = g.split("\\|", 2); String cs = p[0], hex = p.length > 1 ? p[1] : "";
        arkada(() -> {
            String k = "", ks = "", v = "", vs = "", sirket = "", model = "";
            try {
                JSONObject r = istek("https://api.adsbdb.com/v0/callsign/" + cs, null).getJSONObject("response").getJSONObject("flightroute");
                JSONObject o = r.getJSONObject("origin"), d = r.getJSONObject("destination");
                k = o.optString("iata_code", o.optString("icao_code")); ks = o.optString("municipality", o.optString("name")); v = d.optString("iata_code", d.optString("icao_code")); vs = d.optString("municipality", d.optString("name"));
                JSONObject a = r.optJSONObject("airline"); if (a != null) sirket = a.optString("name");
            } catch (Exception ignored) { }
            try { if (!hex.isEmpty()) { JSONObject a = istek("https://api.adsbdb.com/v0/aircraft/" + hex, null).getJSONObject("response").getJSONObject("aircraft");
                  model = (a.optString("manufacturer") + " " + a.optString("type")).trim(); if (sirket.isEmpty()) sirket = a.optString("registered_owner"); } }
            catch (Exception ignored) { }
            String satir = "V" + temizle(cs, 10) + "|" + temizle(k, 4) + "|" + temizle(duz(ks), 16) + "|" + temizle(v, 4) + "|" + temizle(duz(vs), 16) + "|" + temizle(duz(sirket), 21) + "|" + temizle(duz(model), 21);
            ana.post(() -> yaz(satir));
        });
    }
    private void iss() {
        arkada(() -> {
            try {
                JSONObject j = istek("https://api.wheretheiss.at/v1/satellites/25544", null);
                String satir = "I" + String.format(Locale.ROOT, "%.2f|%.2f|%.0f|%.0f|%d|", j.getDouble("latitude"), j.getDouble("longitude"), j.getDouble("altitude"), j.getDouble("velocity"),
                        "daylight".equals(j.optString("visibility")) ? 1 : 0) + ayar().getString(ENLEM, "") + "|" + ayar().getString(BOYLAM, "");
                ana.post(() -> yaz(satir));
            } catch (Exception ignored) { }
        });
    }
    private void issYol() {                          // önümüzdeki 90 dk: 10'ar noktalık iki istek (servis en çok 10 zaman alır)
        arkada(() -> {
            StringBuilder s = new StringBuilder("J"); long t = System.currentTimeMillis() / 1000;
            try {
                for (int parti = 0; parti < 2; parti++) {
                    StringBuilder z = new StringBuilder();
                    for (int i = 1; i <= 10; i++) z.append(z.length() > 0 ? "," : "").append(t + (parti * 10 + i) * 270);
                    org.json.JSONArray a = new org.json.JSONArray(oku((HttpURLConnection) new URL("https://api.wheretheiss.at/v1/satellites/25544/positions?timestamps=" + z).openConnection()));
                    for (int i = 0; i < a.length(); i++) s.append(Math.round(a.getJSONObject(i).getDouble("latitude"))).append(',').append(Math.round(a.getJSONObject(i).getDouble("longitude"))).append(';');
                }
                String satir = s.toString(); ana.post(() -> yaz(satir));
            } catch (Exception ignored) { }
        });
    }

    // ---------------------------------------------------------------- ekran paylaşımı: "EKR:<no>/<parça>/<toplam>:<base64 PackBits>" -> PC penceresi
    private int ekranNo = -1; private String[] ekranParcalar;
    private void ekranParca(String g) {
        try {
            int a = g.indexOf('/'), b = g.indexOf('/', a + 1), c = g.indexOf(':', b + 1);
            int no = Integer.parseInt(g.substring(0, a)), i = Integer.parseInt(g.substring(a + 1, b)), t = Integer.parseInt(g.substring(b + 1, c));
            if (no != ekranNo || ekranParcalar == null || ekranParcalar.length != t) { ekranNo = no; ekranParcalar = new String[t]; }   // yeni kare: yarım kalan eskisi atılır
            ekranParcalar[i] = g.substring(c + 1);
            for (String p : ekranParcalar) if (p == null) return;
            byte[] pb = android.util.Base64.decode(String.join("", ekranParcalar), android.util.Base64.DEFAULT), kare = new byte[1024];
            int k = 0;
            for (int j = 0; j < pb.length && k < 1024; ) {              // PackBits aç
                int n = pb[j++];
                if (n >= 0) { for (int m = 0; m <= n && j < pb.length && k < 1024; m++) kare[k++] = pb[j++]; }
                else if (n != -128 && j < pb.length) { byte v = pb[j++]; for (int m = 0; m < 1 - n && k < 1024; m++) kare[k++] = v; }
            }
            ekranParcalar = null;
            if (k == 1024) pc.ekranYolla(kare);
        } catch (Exception ignored) { }
    }

    // ---------------------------------------------------------------- Kübit'ten gelen bölüm (editörden "Telefona gönder")
    private void seviyeParcasi(String g) {
        String[] p = g.split("\\|", 4); if (p.length < 4) return;
        char tur = p[0].charAt(0); int parca = Integer.parseInt(p[1]), toplam = Integer.parseInt(p[2]);
        String[] d = seviyeParca.get(tur);
        if (parca == 0 || d == null || d.length != toplam) { d = new String[toplam]; seviyeParca.put(tur, d); }
        d[parca] = p[3];
        for (String x : d) if (x == null) return;
        ayar().edit().putString("sv_" + tur, String.join("", d)).apply(); seviyeParca.remove(tur);
        bildir(tur == 'm' ? "Süper Kübit bölümü telefona geldi" : "Geometry bölümü telefona geldi");
    }

    // ---------------------------------------------------------------- telefon durumu: pil, adım, takvim
    private void pilGonder() { if (sonPil >= 0) yaz("P" + sonPil + "|" + (sonSarj ? 1 : 0)); }
    private void telefonDurumu() { pilGonder(); if (sonAdim >= 0) yaz("D" + sonAdim + "|8000"); takvimGonder(); }
    private void adimBaslat() {
        if (Build.VERSION.SDK_INT >= 29 && checkSelfPermission(android.Manifest.permission.ACTIVITY_RECOGNITION) != android.content.pm.PackageManager.PERMISSION_GRANTED) return;
        sensorler = getSystemService(SensorManager.class);
        Sensor s = sensorler == null ? null : sensorler.getDefaultSensor(Sensor.TYPE_STEP_COUNTER);
        if (s != null) sensorler.registerListener(adimDinle, s, SensorManager.SENSOR_DELAY_NORMAL);
    }
    private void takvimGonder() {                    // önümüzdeki 24 saatteki ilk etkinlik (tüm gün olanlar hariç)
        if (System.currentTimeMillis() - takvimZamani < 5 * 60_000) return;
        takvimZamani = System.currentTimeMillis();
        if (checkSelfPermission(android.Manifest.permission.READ_CALENDAR) != android.content.pm.PackageManager.PERMISSION_GRANTED) return;
        arkada(() -> {
            long simdi = System.currentTimeMillis();
            android.net.Uri.Builder b = CalendarContract.Instances.CONTENT_URI.buildUpon();
            android.content.ContentUris.appendId(b, simdi); android.content.ContentUris.appendId(b, simdi + 86_400_000L);
            String satir = "T";
            try (Cursor c = getContentResolver().query(b.build(), new String[]{CalendarContract.Instances.TITLE, CalendarContract.Instances.BEGIN},
                    CalendarContract.Instances.ALL_DAY + "=0 AND " + CalendarContract.Instances.BEGIN + ">=" + simdi, null, CalendarContract.Instances.BEGIN + " ASC")) {
                if (c != null && c.moveToFirst()) { long bas = c.getLong(1); satir = "T" + temizle(c.getString(0), 30) + "|" + (bas + java.util.TimeZone.getDefault().getOffset(bas)) / 1000; }
            } catch (Exception ignored) { }
            String s = satir; ana.post(() -> yaz(s));
        });
    }

    // ---------------------------------------------------------------- gelen arama (bildirim dinleyicisinden), alarm, bip
    void aramaGeldi(Notification n, String kim) { aramaBildirimi = n; ana.post(() -> yaz("R" + temizle(kim.isEmpty() ? "Bilinmeyen" : kim, 30))); }
    void aramaBitti() {
        aramaBildirimi = null;
        if (sesKapatildi) { try { getSystemService(AudioManager.class).adjustStreamVolume(AudioManager.STREAM_RING, AudioManager.ADJUST_UNMUTE, 0); } catch (Exception ignored) { } sesKapatildi = false; }
        ana.post(() -> yaz("R"));
    }
    private void aramaReddet() {                     // arama bildirimindeki "Reddet" düğmesine basar (ek izin gerekmez)
        Notification n = aramaBildirimi;
        if (n == null || n.actions == null) return;
        for (Notification.Action a : n.actions) {
            String ad = a.title == null ? "" : a.title.toString().toLowerCase(Locale.ROOT);
            if (ad.contains("reddet") || ad.contains("decline") || ad.contains("reject") || ad.contains("kapat") || ad.contains("dismiss") || ad.contains("meşgul")) {
                try { a.actionIntent.send(); } catch (Exception ignored) { }
                return;
            }
        }
    }
    private void aramaSessiz() { try { getSystemService(AudioManager.class).adjustStreamVolume(AudioManager.STREAM_RING, AudioManager.ADJUST_MUTE, 0); sesKapatildi = true; } catch (Exception ignored) { } }
    private void alarmKur(String ssdd) {             // Kübit'ten: en yakın SS:DD'de telefon çalar (bulma zili gibi)
        try {
            String[] p = ssdd.split(":"); java.util.Calendar c = java.util.Calendar.getInstance();
            c.set(java.util.Calendar.HOUR_OF_DAY, Integer.parseInt(p[0])); c.set(java.util.Calendar.MINUTE, Integer.parseInt(p[1])); c.set(java.util.Calendar.SECOND, 0);
            if (c.getTimeInMillis() <= System.currentTimeMillis()) c.add(java.util.Calendar.DAY_OF_MONTH, 1);
            PendingIntent pi = PendingIntent.getService(this, 3, new Intent(this, KubitServis.class).setAction(EYLEM_ALARM), PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
            AlarmManager am = getSystemService(AlarmManager.class);
            if (Build.VERSION.SDK_INT < 31 || am.canScheduleExactAlarms()) am.setAlarmClock(new AlarmManager.AlarmClockInfo(c.getTimeInMillis(), pi), pi);
            else am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, c.getTimeInMillis(), pi);
            yaz("!Alarm " + ssdd + " kuruldu");
        } catch (Exception e) { yaz("!Alarm kurulamadi"); }
    }
    private void bip() { try { new ToneGenerator(AudioManager.STREAM_NOTIFICATION, 90).startTone(ToneGenerator.TONE_PROP_BEEP2, 400); } catch (Exception ignored) { } }

    // ---------------------------------------------------------------- yedek: Kübit'in kayıtları telefonda bir dosyada durur
    private java.io.File yedekDosya() { return new java.io.File(getFilesDir(), "kubit_yedek.txt"); }
    private void yedekKaydet() {
        if (yedekSatir == null) return;
        try (java.io.FileOutputStream f = new java.io.FileOutputStream(yedekDosya())) { f.write(String.join("\n", yedekSatir).getBytes(StandardCharsets.UTF_8)); bildir("Kübit yedeği alındı (" + yedekSatir.size() + " satır)"); }
        catch (Exception e) { yaz("!Yedek kaydedilemedi"); }
        yedekSatir = null;
    }
    private void yedekGeri() {
        java.io.File f = yedekDosya();
        if (!f.exists()) { yaz("!Telefonda yedek yok"); return; }
        try {
            String icerik = new String(java.nio.file.Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8);
            for (String s : icerik.split("\n")) if (!s.isEmpty()) yaz("Z" + s);
            yaz("Z.");
        } catch (Exception e) { yaz("!Yedek okunamadi"); }
    }

    // ---------------------------------------------------------------- periyodik: durum, özet, hava
    private final Runnable dongu = new Runnable() {
        @Override public void run() {
            takvimGonder();
            if (bagli && System.currentTimeMillis() - sonBasari > 75_000) { kapat(); if (otoAcik()) yenidenBaglan(0); }   // bekçi: sessiz ölü bağlantı
            durumGonder(); havaGuncelle(); pc.hatalariGonder(); ana.postDelayed(this, ARALIK_MS);
        }
    };

    /** Yerel saat, saniye (Kübit saati gösterir; saat dilimi telefondan). */
    private static long yerelSaat() {
        long simdi = System.currentTimeMillis();
        return (simdi + java.util.TimeZone.getDefault().getOffset(simdi)) / 1000;
    }

    private static String oku(HttpURLConnection c) throws Exception {
        int kod = c.getResponseCode();
        InputStream in = kod < 400 ? c.getInputStream() : c.getErrorStream();
        ByteArrayOutputStream tampon = new ByteArrayOutputStream();
        byte[] parca = new byte[4096];
        for (int n; in != null && (n = in.read(parca)) > 0; ) tampon.write(parca, 0, n);   // readAllBytes Android 13 öncesinde yok
        if (kod >= 400) throw new Exception(new JSONObject(tampon.size() > 0 ? tampon.toString("UTF-8") : "{}").optString("detail", "HTTP " + kod));
        return tampon.toString("UTF-8");
    }

    private JSONObject istek(String yol, String govde) throws Exception {
        boolean dis = yol.startsWith("http");
        HttpURLConnection c = (HttpURLConnection) new URL(dis ? yol : SUNUCU + yol).openConnection();
        c.setConnectTimeout(8000); c.setReadTimeout(15000);
        String anahtar = ayar().getString(ANAHTAR, null);
        if (anahtar != null && !dis) c.setRequestProperty("Authorization", "Bearer " + anahtar);   // anahtar yalnız Kübik'e gider
        if (govde != null) {
            c.setRequestMethod("POST"); c.setDoOutput(true);
            c.setRequestProperty("Content-Type", "application/json");
            c.getOutputStream().write(govde.getBytes(StandardCharsets.UTF_8));
        }
        return new JSONObject(oku(c));
    }

    private void arkada(Runnable is) { new Thread(is).start(); }

    private void durumGonder() {
        if (durumKar == null) return;
        boolean anahtarVar = ayar().getString(ANAHTAR, null) != null;
        arkada(() -> {
            String satir, ozet = null;
            try {
                JSONObject j = istek("/api/status", null);
                String hal = j.optBoolean("model_ready") ? "UYANIK" : j.optBoolean("hibernating") ? "UYKUDA" : "ACILIYOR";
                int derece = j.isNull("pi_temp") ? -1 : j.optInt("pi_temp", -1);
                satir = "S" + derece + "|" + j.optString("model", "Kübik") + "|" + hal + "|" + yerelSaat();
                if (anahtarVar) {
                    try {
                        JSONObject o = istek("/api/kubit/ozet", null);
                        ozet = "O" + o.optInt("yeni_kayit") + "|" + o.optInt("isaret") + "|" + o.optInt("mesaj_bugun") + "|" + o.optInt("aktif") + "|" + o.optLong("acik_sn", -1) + "|" + o.optInt("kup");
                    } catch (Exception ignored) { }
                }
            } catch (Exception e) { satir = "S-1|Sunucu|ULASILAMIYOR|" + yerelSaat(); }
            String s = satir, o = ozet;
            ana.post(() -> {
                yaz(s);
                if (o != null) yaz(o);
                String[] p = s.substring(1).split("\\|");
                String yeni = "Bağlı · " + (p[0].equals("-1") ? "" : "Pi " + p[0] + "°C · ") + p[1] + " " + p[2].toLowerCase(Locale.ROOT);
                if (!yeni.equals(sonBildirim)) { sonBildirim = yeni; bildir(yeni); }
            });
        });
    }

    /** Hava: konum uygulama açıkken kaydedilir (MainActivity), Open-Meteo anahtar istemez. */
    private void havaGuncelle() {
        String enlem = ayar().getString(ENLEM, null), boylam = ayar().getString(BOYLAM, null);
        if (enlem == null || System.currentTimeMillis() - havaZamani < HAVA_ARALIK_MS) return;
        havaZamani = System.currentTimeMillis();
        arkada(() -> {
            try {
                JSONObject j = istek("https://api.open-meteo.com/v1/forecast?latitude=" + enlem + "&longitude=" + boylam
                        + "&current=temperature_2m,weather_code&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max&timezone=auto&forecast_days=1", null);
                JSONObject simdi = j.getJSONObject("current"), gun = j.getJSONObject("daily");
                if (yer.isEmpty() && Geocoder.isPresent()) {
                    try {
                        List<Address> a = new Geocoder(this, new Locale("tr")).getFromLocation(Double.parseDouble(enlem), Double.parseDouble(boylam), 1);
                        if (a != null && !a.isEmpty()) yer = a.get(0).getSubAdminArea() != null ? a.get(0).getSubAdminArea() : String.valueOf(a.get(0).getAdminArea());
                    } catch (Exception ignored) { }
                }
                String satir = "H" + simdi.getInt("weather_code") + "|" + Math.round(simdi.getDouble("temperature_2m")) + "|"
                        + Math.round(gun.getJSONArray("temperature_2m_max").getDouble(0)) + "|" + Math.round(gun.getJSONArray("temperature_2m_min").getDouble(0)) + "|"
                        + gun.getJSONArray("precipitation_probability_max").optInt(0, 0) + "|" + temizle(yer, 20);
                ana.post(() -> { havaSatiri = satir; yaz(satir); });
            } catch (Exception e) { havaZamani = 0; }
        });
    }

    // ---------------------------------------------------------------- kumanda ve Kübik'e soru
    private void kumanda(String ad) {
        if (ayar().getString(ANAHTAR, null) == null) { yaz("!Anahtar yok: panel > Guvenlik"); return; }
        arkada(() -> {
            String sonuc;
            try { sonuc = istek("/api/kubit/komut", new JSONObject().put("komut", ad).toString()).optString("message", "Tamam"); }
            catch (Exception e) { sonuc = "Hata: " + e.getMessage(); }
            String m = "!" + sonuc;
            ana.post(() -> { yaz(m); ana.postDelayed(this::durumGonder, 4000); });
        });
    }

    private void sor(String soru) {
        if (ayar().getString(ANAHTAR, null) == null) { cevapGonder("Kubit anahtari yok. Kubik'te panel > Guvenlik > Yeni anahtar."); return; }
        arkada(() -> {
            String cevap;
            try {
                String id = istek("/api/kubit/sor", new JSONObject().put("soru", soru).toString()).getString("id");
                cevap = "Kubik cevap vermedi.";
                for (int i = 0; i < 100; i++) {                   // en fazla ~5 dk
                    Thread.sleep(3000);
                    JSONObject j = istek("/api/kubit/sor/" + id, null);
                    String durum = j.optString("state");
                    if ("done".equals(durum)) { cevap = j.optString("text"); break; }
                    if (!"queued".equals(durum) && !"running".equals(durum)) { cevap = "Hata: " + j.optString("status"); break; }
                }
            } catch (Exception e) { cevap = "Hata: " + e.getMessage(); }
            String c = cevap;
            ana.post(() -> cevapGonder(c));
        });
    }

    /** Uzun cevap 60 karakterlik parçalarla gider (185 baytlık pakete sığsın): A ilk parça, C devamı, E bitti. */
    private void cevapGonder(String metin) {
        String temiz = metin.replaceAll("(?s)<secenekler.*?</secenekler>", "").replaceAll("[*#`_>]", "").replaceAll("\\s+", " ").trim();
        if (temiz.isEmpty()) temiz = "(bos cevap)";
        for (int i = 0; i < temiz.length() && i < 1400; i += 60)
            yaz((i == 0 ? "A" : "C") + temiz.substring(i, Math.min(temiz.length(), i + 60)));
        yaz("E");
    }

    // ---------------------------------------------------------------- telefonu bul
    private void zilCal() {
        zilDurdur();
        AudioManager ses = getSystemService(AudioManager.class);
        eskiSes = ses.getStreamVolume(AudioManager.STREAM_ALARM);
        ses.setStreamVolume(AudioManager.STREAM_ALARM, ses.getStreamMaxVolume(AudioManager.STREAM_ALARM), 0);   // sessizde bile duyulsun
        zil = RingtoneManager.getRingtone(this, RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM));
        if (zil == null) return;
        zil.setAudioAttributes(new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_ALARM).build());
        if (Build.VERSION.SDK_INT >= 28) zil.setLooping(true);
        zil.play();
        ana.postDelayed(this::zilDurdur, 30_000);
    }

    private void zilDurdur() {
        if (zil != null) { zil.stop(); zil = null; }
        if (eskiSes >= 0) { getSystemService(AudioManager.class).setStreamVolume(AudioManager.STREAM_ALARM, eskiSes, 0); eskiSes = -1; }
    }

    // ---------------------------------------------------------------- müzik (bildirim erişimi gerekir)
    private MediaController calan() {
        try {
            List<MediaController> l = getSystemService(MediaSessionManager.class).getActiveSessions(new ComponentName(this, KubitBildirim.class));
            return l.isEmpty() ? null : l.get(0);
        } catch (SecurityException e) { return null; }
    }

    private void muzik(String ad) {
        MediaController m = calan();
        if (m == null) { yaz("M0|Muzik yok ya da|bildirim erisimi kapali"); return; }
        boolean caliyor = m.getPlaybackState() != null && m.getPlaybackState().getState() == PlaybackState.STATE_PLAYING;
        if ("oynat".equals(ad)) { if (caliyor) m.getTransportControls().pause(); else m.getTransportControls().play(); }
        else if ("sonraki".equals(ad)) m.getTransportControls().skipToNext();
        else if ("onceki".equals(ad)) m.getTransportControls().skipToPrevious();
        else if ("ses+".equals(ad) || "ses-".equals(ad)) getSystemService(AudioManager.class).adjustStreamVolume(AudioManager.STREAM_MUSIC, "ses+".equals(ad) ? AudioManager.ADJUST_RAISE : AudioManager.ADJUST_LOWER, 0);
        else if ("begen".equals(ad)) ozelEylem(m, true, "like", "favorite", "heart", "thumb", "begen");
        else if ("karistir".equals(ad)) ozelEylem(m, false, "shuffle", "karistir");
        ana.postDelayed(this::muzikDurumu, "durum".equals(ad) ? 0 : 700);
    }

    private void muzikDurumu() {
        MediaController m = calan();
        if (m == null) return;
        MediaMetadata md = m.getMetadata();
        boolean caliyor = m.getPlaybackState() != null && m.getPlaybackState().getState() == PlaybackState.STATE_PLAYING;
        String sarki = md == null ? "" : md.getString(MediaMetadata.METADATA_KEY_TITLE);
        String sanatci = md == null ? "" : md.getString(MediaMetadata.METADATA_KEY_ARTIST);
        PlaybackState ps = m.getPlaybackState();
        long poz = 0; if (ps != null) { poz = ps.getPosition(); if (caliyor) poz += (long) ((android.os.SystemClock.elapsedRealtime() - ps.getLastPositionUpdateTime()) * ps.getPlaybackSpeed()); }
        long sure = md == null ? 0 : md.getLong(MediaMetadata.METADATA_KEY_DURATION);
        yaz("M" + (caliyor ? 1 : 0) + "|" + temizle(sarki, 40) + "|" + temizle(sanatci, 40) + "|" + poz / 1000 + "|" + Math.max(0, sure / 1000) + "|" + (begenildi ? 1 : 0) + "|" + (karisik ? 1 : 0));
    }
    private boolean begenildi, karisik;   // ponytail: uygulamanın gerçek durumu okunamıyor, Kübit'ten basılanı sayar
    /** Beğen / karıştır: Spotify gibi uygulamalar bunları özel eylem olarak sunar; adında anahtar kelime geçeni çalıştır. */
    private void ozelEylem(MediaController m, boolean begen, String... anahtar) {
        PlaybackState ps = m.getPlaybackState();
        if (ps != null && ps.getCustomActions() != null) for (PlaybackState.CustomAction e : ps.getCustomActions()) {
            String ad = (e.getAction() + " " + e.getName()).toLowerCase(Locale.ROOT);
            for (String k : anahtar) if (ad.contains(k)) {
                m.getTransportControls().sendCustomAction(e, null);
                if (begen) begenildi = !begenildi; else karisik = !karisik;
                return;
            }
        }
        yaz("!Uygulama desteklemiyor");
    }

    static String temizle(String s, int en) {
        String t = s == null ? "" : s.replace('|', '/').replace('\n', ' ').trim();
        return t.length() > en ? t.substring(0, en) : t;
    }

    /** Bildirim dinleyicisinden: "N<uygulama>|<başlık>|<metin>". */
    void bildirimGonder(String uygulama, String baslik, String metin) {
        ana.post(() -> yaz("N" + temizle(uygulama, 20) + "|" + temizle(baslik, 30) + "|" + temizle(metin, 60)));
    }
}
