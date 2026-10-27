package com.asrin.kubik;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/** Telefon yeniden başlayınca Kübit köprüsü kendiliğinden açılsın. */
public class Acilis extends BroadcastReceiver {
    @Override
    public void onReceive(Context ctx, Intent intent) {
        if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) KubitServis.baslat(ctx);
    }
}
