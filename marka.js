'use strict';
// Marka (kubik/klik) ve "hesaplar kapalı" ayarını sayfaya uygular. Ayarlar admin > Geliştirici sekmesinden gelir (/api/status).
// Klik'te arayüzdeki "Kübik" yazıları "Klik" olur; sohbet mesajları ve kullanıcının yazdıkları değişmez.
(() => {
  const MAX_PLANLAR = ['max', 'maxplus', 'super', 'ultra', 'ultramax', 'hyper'];
  const HARIC = '#messages, #conversationList, textarea, input, pre, code, [data-marka-yok]';
  const kok = document.documentElement;
  const betik = document.currentScript?.src || location.href;     // async içinde currentScript null olur
  let klikUygulandi = false;

  const cevir = t => t.replace(/Kübik/g, 'Klik').replace(/KÜBİK/g, 'KLİK');
  function metinDegistir(n) {
    if (/Kübik|KÜBİK/.test(n.nodeValue) && !n.parentElement?.closest(HARIC)) n.nodeValue = cevir(n.nodeValue);
  }
  function agacDegistir(el) {
    const w = document.createTreeWalker(el, NodeFilter.SHOW_TEXT);
    const liste = [];
    while (w.nextNode()) liste.push(w.currentNode);
    liste.forEach(metinDegistir);
    const nitelikler = ['placeholder', 'title', 'aria-label'];
    el.querySelectorAll?.('[placeholder*="Kübik"],[title*="Kübik"],[aria-label*="Kübik"]').forEach(x =>
      nitelikler.forEach(a => { const v = x.getAttribute(a); if (v) x.setAttribute(a, cevir(v)); }));
  }
  function klikYap() {
    if (klikUygulandi) return;
    klikUygulandi = true;
    document.title = cevir(document.title);
    agacDegistir(document.body);
    new MutationObserver(kayitlar => kayitlar.forEach(k => k.addedNodes.forEach(n => {
      if (n.nodeType === 3) metinDegistir(n); else if (n.nodeType === 1) agacDegistir(n);
    }))).observe(document.body, {childList: true, subtree: true});
    const ikon = document.querySelector('link[rel="icon"]') || Object.assign(document.createElement('link'), {rel: 'icon'});
    ikon.href = new URL('logo/klik.svg', betik).href;
    document.head.append(ikon);
  }
  function hesaplarKapali() {
    const alan = document.querySelector('.auth-form-area');
    if (!alan || alan.querySelector('.hesap-kapali-not')) return;
    document.querySelector('#authForm')?.setAttribute('hidden', '');
    const not = document.createElement('div');
    not.className = 'hesap-kapali-not';
    const h = document.createElement('h2'); h.textContent = 'Hesaplar şu an kapalı';
    const p = document.createElement('p'); p.textContent = 'Giriş ve kayıt geçici olarak kapalı. Oturum açmadan sohbet edebilirsin.';
    const b = document.createElement('button'); b.type = 'button'; b.className = 'primary'; b.textContent = 'Oturum açmadan devam et';
    b.onclick = () => document.querySelector('#guestContinue')?.click();
    not.append(h, p, b);
    alan.append(not);
  }
  const iste = async yol => (await kubikTransport.fetch('/api' + yol, {headers: {'X-Kubik-Request': '1'}})).json();

  async function uygula() {
    let s;
    try { s = await iste('/status'); } catch { return; }
    kok.dataset.brand = s.brand === 'klik' ? 'klik' : 'kubik';
    kok.dataset.hesap = s.accounts_enabled === false ? 'kapali' : 'acik';
    if (s.brand === 'klik') klikYap();
    if (s.accounts_enabled === false) hesaplarKapali();
    try { const me = await iste('/me'); kok.dataset.plan = MAX_PLANLAR.includes(me.tier) ? 'max' : 'normal'; } catch { /* girişsiz */ }
  }
  window.kubikMarka = {uygula};
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', uygula); else uygula();
  setInterval(async () => {      // giriş/çıkış ya da plan değişince logo rengi güncellensin
    try { const me = await iste('/me'); kok.dataset.plan = MAX_PLANLAR.includes(me.tier) ? 'max' : 'normal'; }
    catch { kok.dataset.plan = 'normal'; }
  }, 60000);
})();
