package com.asrin.kubik;

import android.app.Notification;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;

/**
 * Telefon bildirimlerini (WhatsApp, Discord, gelen arama…) Kübit'e iletir. Kullanıcı Ayarlar > Bildirim erişimi'nden
 * açar; aynı izin müzik kumandasının etkin medya oturumlarını görmesini de sağlar. Bildirimler yalnız Bluetooth ile
 * anahtarlığa gider, hiçbir yere kaydedilmez.
 */
public class KubitBildirim extends NotificationListenerService {
    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        KubitServis servis = KubitServis.aktif;
        Notification n = sbn.getNotification();
        if (servis == null || getPackageName().equals(sbn.getPackageName())) return;
        boolean arama = Notification.CATEGORY_CALL.equals(n.category);
        if (arama) {                                  // gelen arama: Kübit'te tam ekran, sol tuş reddeder
            CharSequence kim = n.extras.getCharSequence(Notification.EXTRA_TITLE);
            servis.aramaGeldi(n, kim == null ? "" : kim.toString());
            return;
        }
        if (!arama && (sbn.isOngoing() || (n.flags & Notification.FLAG_GROUP_SUMMARY) != 0)) return;   // müzik, indirme, özet bildirimleri değil
        Bundle e = n.extras;
        CharSequence baslik = e.getCharSequence(Notification.EXTRA_TITLE), metin = e.getCharSequence(Notification.EXTRA_TEXT);
        if (baslik == null && metin == null) return;
        String uygulama = sbn.getPackageName();
        try {
            PackageManager pm = getPackageManager();
            ApplicationInfo ai = pm.getApplicationInfo(uygulama, 0);
            uygulama = String.valueOf(pm.getApplicationLabel(ai));
        } catch (PackageManager.NameNotFoundException ignored) { }
        servis.bildirimGonder(uygulama, baslik == null ? "" : baslik.toString(), metin == null ? "" : metin.toString());
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn) {
        KubitServis servis = KubitServis.aktif;
        if (servis != null && Notification.CATEGORY_CALL.equals(sbn.getNotification().category)) servis.aramaBitti();
    }
}
