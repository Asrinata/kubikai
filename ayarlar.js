'use strict';
// Klik: composer'daki "Ayarlar" düğmesi küçük bir panel açar; Düşünme ve Web araması kaydırıcıyla seçilir.
// Asıl değer hâlâ #effort / #searchEffort select'lerinde (app.js onları okur); kaydırıcı yalnız onları sürer.
// Plan/sunucu yüzünden kapalı seçeneklere (option.disabled) kaydırıcı gitmez, en yakın açık olana döner.
// "Tüm ayarlar" eski ayar penceresini açar. Eski Kübik markasında hiçbir şey değişmez.
(() => {
  const kok = document.documentElement;
  const klik = () => kok.dataset.brand === 'klik';
  let pano, gecir = false;

  function satir(id, baslik) {
    const sec = document.getElementById(id);
    const kutu = document.createElement('div');
    kutu.className = 'ayar-satir';
    const ust = document.createElement('div');
    ust.className = 'ayar-ust';
    const ad = document.createElement('span');
    ad.textContent = baslik;
    const deger = document.createElement('b');
    ust.append(ad, deger);
    const kay = document.createElement('input');
    kay.type = 'range';
    kay.min = 0;
    kay.step = 1;
    kay.setAttribute('aria-label', baslik);
    const cizgi = document.createElement('div');
    cizgi.className = 'ayar-cizgi';
    kutu.append(ust, kay, cizgi);

    function yenile() {
      const s = [...sec.options];
      kay.max = s.length - 1;
      kay.value = sec.selectedIndex;
      deger.textContent = sec.selectedOptions[0]?.textContent || '';
      kay.setAttribute('aria-valuetext', deger.textContent);
      kay.style.setProperty('--dolu', `${(sec.selectedIndex / Math.max(1, s.length - 1)) * 100}%`);
      cizgi.replaceChildren(...s.map((o, i) => {
        const n = document.createElement('i');
        n.className = (o.disabled ? 'kapali ' : '') + (i <= sec.selectedIndex ? 'dolu' : '');
        n.title = o.textContent + (o.disabled ? ' (planında kapalı)' : '');
        return n;
      }));
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
    return {kutu, yenile};
  }

  function kur() {
    const dugme = document.getElementById('openSettings');
    if (!dugme || !document.getElementById('effort')) return;
    pano = document.createElement('div');
    pano.className = 'ayar-pano';
    pano.hidden = true;
    pano.setAttribute('role', 'dialog');
    pano.setAttribute('aria-label', 'Hızlı ayarlar');
    const dusun = satir('effort', 'Düşünme');
    const web = satir('searchEffort', 'Web araması');
    const tum = document.createElement('button');
    tum.type = 'button';
    tum.className = 'ayar-tum';
    tum.textContent = 'Tüm ayarlar';
    tum.addEventListener('click', () => { kapat(); gecir = true; dugme.click(); gecir = false; });
    pano.append(dusun.kutu, web.kutu, tum);
    dugme.closest('.composer').append(pano);

    const ac = () => { dusun.yenile(); web.yenile(); pano.hidden = false; dugme.setAttribute('aria-expanded', 'true'); pano.querySelector('input').focus({preventScroll: true}); };
    function kapat() { pano.hidden = true; dugme.setAttribute('aria-expanded', 'false'); }
    // app.js'in onclick'i hedefte çalışır; yakalama aşamasında durdurup paneli açıyoruz (Klik'te)
    addEventListener('click', e => {
      if (!klik()) return;
      if (e.target.closest('#openSettings') && !gecir) {
        e.stopPropagation();
        e.preventDefault();
        pano.hidden ? ac() : kapat();
      } else if (!pano.hidden && !e.target.closest('.ayar-pano')) kapat();
    }, true);
    addEventListener('keydown', e => { if (e.key === 'Escape' && !pano.hidden) { kapat(); dugme.focus(); } });
    ['effort', 'searchEffort'].forEach(id => document.getElementById(id).addEventListener('change', () => { if (!pano.hidden) { dusun.yenile(); web.yenile(); } }));
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
