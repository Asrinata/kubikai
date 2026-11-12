'use strict';
// "Google ile devam et": Client ID admin > Geliştirici'den gelir (/api/status). Boşsa ya da hesaplar kapalıysa düğme hiç çıkmaz.
// Google'ın verdiği kimlik jetonu /api/auth/google'a gider; sunucu Google'a doğrulatır, hesap açar ya da giriş yapar.
// app.js'teki api(), enter() ve toast() ortak.
(() => {
  let kuruldu = false;

  async function kur() {
    if (kuruldu) return;
    let s;
    try { s = await (await kubikTransport.fetch('/api/status', {headers: {'X-Kubik-Request': '1'}})).json(); } catch { return; }
    if (!s.google_client_id || s.accounts_enabled === false) return;
    const form = document.querySelector('#authForm');
    if (!form) return;
    kuruldu = true;
    const kutu = document.createElement('div');
    kutu.className = 'google-giris';
    const ayrac = document.createElement('div');
    ayrac.className = 'google-ayrac';
    ayrac.textContent = 'veya';
    const yer = document.createElement('div');
    yer.id = 'googleButon';
    kutu.append(ayrac, yer);
    (document.querySelector('#authToggle') || form.lastElementChild).before(kutu);
    const betik = document.createElement('script');
    betik.src = 'https://accounts.google.com/gsi/client';
    betik.async = true;
    betik.onload = () => {
      google.accounts.id.initialize({client_id: s.google_client_id, callback: girisYap, ux_mode: 'popup', itp_support: true});
      const koyu = document.documentElement.dataset.brand !== 'kubik';
      google.accounts.id.renderButton(yer, {theme: koyu ? 'filled_black' : 'outline', size: 'large', shape: 'pill',
        text: 'continue_with', locale: 'tr', logo_alignment: 'center', width: Math.min(Math.max(yer.clientWidth, 240), 400)});
    };
    betik.onerror = () => kutu.remove();       // reklam engelleyici vb. Google'ı engellerse düğmesiz devam
    document.head.append(betik);
  }

  async function girisYap(yanit) {
    const hata = document.querySelector('#authError');
    if (hata) hata.textContent = '';
    try {
      const anahtar = document.querySelector('#accessKey')?.value || '';
      const sonuc = await api('/auth/google', {method: 'POST', body: {credential: yanit.credential, access_key: anahtar}});
      if (sonuc.new && typeof toast === 'function') toast('Hoş geldin! Hesabın açıldı: ' + sonuc.username);
      await enter();
    } catch (e) {
      if (hata) hata.textContent = e.message;
    }
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
