'use strict';
// Klik: ayarlar yazı kutusunun altında çip olarak durur (Düşünme · Web · Verimlilik · Multitask; dil çipi yok).
// Bir çipe tıklayınca yazı kutusunun altında o ayarın kaydırıcısı açılır; aynı çipe yeniden tıklamak kapatır.
// Asıl değerler hâlâ select'lerde (#effort, #searchEffort, #quality, #multitask, #language); app.js onları okur,
// kaydırıcı yalnız onları sürer ve 'change' yollar. Kapalı seçeneğe (option.disabled) gitmez, aşağıdaki ilk açık olana iner.
// "Ayarlar" düğmesi gizlenir; PC erişimi (yalnız masaüstü uygulaması) için ayrı bir çip eski pencereyi açar.
// Dosya ekleme (+) gönder düğmesinin yanına taşınır. Eski Kübik markasında hiçbir şey değişmez.
(() => {
  const kok = document.documentElement;
  const AYARLAR = [
    {id: 'effort', ad: () => document.querySelector('#effort')?.closest('label')?.querySelector('span')?.textContent || 'Düşünme'},
    {id: 'searchEffort', ad: () => 'Web', kisalt: t => t.replace(/^Web\s*[·:-]?\s*/i, '').replace(/^./, c => c.toLocaleUpperCase('tr'))},
    {id: 'quality', ad: () => document.querySelector('[data-i18n=quality]')?.textContent || 'Verimlilik'},
    {id: 'multitask', ad: () => 'Multitask', kisalt: t => t.replace(/^.*\((\d+)\s*(?:YZ|AI)\).*$/i, '$1 agent')},   // 'Az (2 YZ)' -> '2 agent'
  ];
  const kisa = t => t.replace(/\s*[·—-]?\s*🔒.*$/u, '').replace(/\s*\(.*?\)\s*/g, '').trim();   // 'Derin · 🔒 Max+ Pack' -> 'Derin'
  let acik = null;

  function kur() {
    const dugme = document.getElementById('openSettings');
    const kontroller = dugme?.parentElement;
    if (kok.dataset.brand !== 'klik' || !kontroller || !document.getElementById('effort')) return;

    const serit = document.createElement('div');          // açılan kaydırıcı
    serit.className = 'ayar-serit';
    serit.hidden = true;
    const ust = document.createElement('div');
    ust.className = 'ayar-ust';
    const ad = document.createElement('span');
    const deger = document.createElement('b');
    ust.append(ad, deger);
    const kay = document.createElement('input');
    kay.type = 'range';
    kay.min = 0;
    kay.step = 1;
    const uclar = document.createElement('div');
    uclar.className = 'ayar-uclar';
    serit.append(ust, kay, uclar);
    kontroller.before(serit);

    const cipler = document.createElement('div');          // çip satırı
    cipler.className = 'ayar-cipler';
    const cip = {};
    for (const a of AYARLAR) {
      const sec = document.getElementById(a.id);
      if (!sec) continue;
      const b = document.createElement('button');
      b.type = 'button';
      b.className = 'ayar-cip';
      b.setAttribute('aria-expanded', 'false');
      b.addEventListener('click', () => goster(acik === a ? null : a));
      cipler.append(b);
      cip[a.id] = b;
      sec.addEventListener('change', tazele);
      new MutationObserver(tazele).observe(sec, {subtree: true, attributes: true, attributeFilter: ['disabled']});
    }
    const pc = document.getElementById('pcSettings');
    if (pc) {                                             // PC erişimi: yalnız masaüstü uygulamasında görünür, eski pencereyi açar
      const b = document.createElement('button');
      b.type = 'button';
      b.className = 'ayar-cip';
      b.dataset.pc = '1';
      b.textContent = 'PC erişimi';
      b.hidden = pc.hidden;
      b.addEventListener('click', () => dugme.click());
      new MutationObserver(() => { b.hidden = pc.hidden; }).observe(pc, {attributes: true, attributeFilter: ['hidden']});
      cipler.append(b);
    }
    kontroller.prepend(cipler);
    const ek = document.getElementById('attachFile');
    if (ek) { ek.title = ek.textContent.trim(); ek.setAttribute('aria-label', ek.title); dugme.before(ek); }

    function metin(a, sec) {
      const t = sec.selectedOptions[0]?.textContent || '';
      return a.kisalt ? a.kisalt(t) : t;
    }
    function tazele() {
      for (const a of AYARLAR) {
        const sec = document.getElementById(a.id), b = cip[a.id];
        if (!sec || !b) continue;
        b.replaceChildren(Object.assign(document.createElement('span'), {textContent: a.ad()}),
                          Object.assign(document.createElement('b'), {textContent: kisa(metin(a, sec))}));
        b.classList.toggle('tavan', sec.selectedIndex === sec.options.length - 1);   // en üst seviye: mor parlar
      }
      const tavan = AYARLAR.filter(a => cip[a.id]);
      kontroller.closest('.composer')?.classList.toggle('hepsi-tavan', tavan.length > 0 && tavan.every(a => cip[a.id].classList.contains('tavan')));
      serit.classList.toggle('tavan', !!acik && !!cip[acik.id]?.classList.contains('tavan'));
      if (!acik) return;
      const sec = document.getElementById(acik.id), s = [...sec.options];
      ad.textContent = acik.ad();
      deger.textContent = kisa(metin(acik, sec));
      kay.max = s.length - 1;
      kay.value = sec.selectedIndex;
      kay.setAttribute('aria-label', acik.ad());
      kay.setAttribute('aria-valuetext', deger.textContent);
      const oran = sec.selectedIndex / Math.max(1, s.length - 1);   // dolgu yuvarlağın ortasına kadar; yuvarlak ucu kapatır
      kay.style.setProperty('--dolu', `calc(var(--yuvarlak, 22px) / 2 + (100% - var(--yuvarlak, 22px)) * ${oran})`);
      uclar.replaceChildren(...s.map((o, i) => Object.assign(document.createElement('i'), {
        className: (o.disabled ? 'kapali ' : '') + (i === sec.selectedIndex ? 'secili' : ''),
        textContent: kisa(acik.kisalt ? acik.kisalt(o.textContent) : o.textContent),
        title: o.textContent + (o.disabled ? ' (planında kapalı)' : ''),
      })));
    }

    function goster(a) {
      acik = a;
      serit.hidden = !a;
      for (const [id, b] of Object.entries(cip)) b.setAttribute('aria-expanded', String(!!a && a.id === id));
      tazele();
      if (a) kay.focus({preventScroll: true});
    }
    function sec(i) {
      const sel = document.getElementById(acik.id), s = [...sel.options];
      while (i > 0 && s[i].disabled) i--;           // kapalı seçenek: aşağıdaki ilk açık olana in
      if (sel.selectedIndex !== i) {
        sel.selectedIndex = i;
        sel.dispatchEvent(new Event('change', {bubbles: true}));
      }
      tazele();
    }
    uclar.addEventListener('click', e => {          // seviye adına dokun: o seviyeye geç (telefonda barı sürüklemekten kolay)
      const i = [...uclar.children].indexOf(e.target.closest('i'));
      if (i >= 0 && acik && !e.target.closest('i').classList.contains('kapali')) sec(i);
    });
    kay.addEventListener('input', () => sec(+kay.value));
    addEventListener('keydown', e => { if (e.key === 'Escape' && acik) goster(null); });
    addEventListener('kubik:enter', () => setTimeout(tazele, 300));
    tazele();
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
