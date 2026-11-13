'use strict';
// Klik: Düşünme ve Web araması, yazı kutusunun altında hep açık duran kaydırıcılarla seçilir.
// Asıl değer hâlâ #effort / #searchEffort select'lerinde (app.js onları okur); kaydırıcı yalnız onları sürer.
// Plan/sunucu yüzünden kapalı seçeneklere (option.disabled) kaydırıcı gitmez, aşağıdaki ilk açık olana döner.
// "Ayarlar" düğmesi eskisi gibi tüm ayarlar penceresini açar. Dosya ekleme (+) gönder düğmesinin yanına taşınır.
// Eski Kübik markasında hiçbir şey değişmez.
(() => {
  const kok = document.documentElement;
  const klik = () => kok.dataset.brand === 'klik';

  function satir(id, baslik) {
    const sec = document.getElementById(id);
    const kutu = document.createElement('label');
    kutu.className = 'ayar-satir';
    const ust = document.createElement('span');
    ust.className = 'ayar-ust';
    const ad = document.createElement('span');
    ad.textContent = baslik;
    const deger = document.createElement('b');
    ust.append(ad, deger);
    const kay = document.createElement('input');
    kay.type = 'range';
    kay.min = 0;
    kay.step = 1;
    kutu.append(ust, kay);

    function yenile() {
      const s = [...sec.options];
      kay.max = s.length - 1;
      kay.value = sec.selectedIndex;
      const metin = sec.selectedOptions[0]?.textContent || '';     // "Web · Hızlı" -> "Hızlı", "Web kapalı" -> "Kapalı"
      deger.textContent = id === 'searchEffort' ? metin.replace(/^Web\s*[·:-]?\s*/i, '').replace(/^./, c => c.toLocaleUpperCase('tr')) : metin;
      kay.setAttribute('aria-valuetext', deger.textContent);
      const kapali = s.filter(o => o.disabled).map(o => o.textContent);
      kutu.title = kapali.length ? 'Planında kapalı: ' + kapali.join(', ') : '';
      const oran = sec.selectedIndex / Math.max(1, s.length - 1);   // dolgu yuvarlağın ortasına kadar; yuvarlak ucu kapatır
      kay.style.setProperty('--dolu', `calc(11px + (100% - 22px) * ${oran})`);
    }
    kay.addEventListener('input', () => {
      const s = [...sec.options];
      let i = +kay.value;
      while (i > 0 && s[i].disabled) i--;           // kapalı seçenek: aşağıdaki ilk açık olana in
      if (sec.selectedIndex !== i) {
        sec.selectedIndex = i;
        sec.dispatchEvent(new Event('change', {bubbles: true}));
      }
      yenile();
    });
    sec.addEventListener('change', yenile);
    return {kutu, yenile};
  }

  function kur() {
    const dugme = document.getElementById('openSettings');
    if (!klik() || !dugme || !document.getElementById('effort')) return;
    const yer = document.createElement('div');
    yer.className = 'ayar-satirlar';
    const dusun = satir('effort', 'Düşünme');
    const web = satir('searchEffort', 'Web');
    yer.append(dusun.kutu, web.kutu);
    dugme.parentElement.prepend(yer);
    const ek = document.getElementById('attachFile');
    if (ek) { ek.title = ek.textContent.trim(); ek.setAttribute('aria-label', ek.title); dugme.before(ek); }
    const tazele = () => { dusun.yenile(); web.yenile(); };
    tazele();
    // plan/sunucu durumu (kapalı seçenekler) sonradan gelir; değer değişmese de görünüm güncellenmeli
    addEventListener('kubik:enter', () => setTimeout(tazele, 300));
    ['effort', 'searchEffort'].forEach(id => new MutationObserver(tazele)
      .observe(document.getElementById(id), {subtree: true, attributes: true, attributeFilter: ['disabled']}));
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
