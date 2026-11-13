'use strict';
// Klik sürprizleri (easter egg). Yalnız Klik markasında. Hepsi kullanıcı tetikler, hiçbiri sunucuya bir şey yollamaz.
//  Yazı kutusuna komut (gönderilmez):  /snake  /pong  /matrix  /gravity  /roll  /cimbom  /sürpriz
//  Yazı kutusu dışında klavyeden:      snake · pong · matrix · Konami (↑↑↓↓←→←→BA)
//  Fare/dokunma: boş yere çift tık = bit dalgası · büyük logoya 5 tık = piksellere dağılır
//                alttaki "hata yapabilir" yazısına 3 tık · telefonu salla · 520 sn boşta = ekran koruyucu
// Bulunanlar localStorage'da sayılır (kubik.surpriz). prefers-reduced-motion: dönme/yerçekimi kısa ve sakin.
(() => {
  const kok = document.documentElement;
  const klik = () => kok.dataset.brand === 'klik';
  const max = () => kok.dataset.plan === 'max';
  const sakin = matchMedia('(prefers-reduced-motion: reduce)').matches;
  const MAX_RENK = ['#c46bff', '#8a63ff', '#6b5cff', '#3fa0ff', '#18d2ff'];
  const GS_RENK = ['#fdb912', '#e30613', '#fdb912', '#a90432'];
  const TUM = ['snake', 'pong', 'matrix', 'gravity', 'roll', 'cimbom', 'dalga', 'logo', 'dipnot', 'salla', 'koruyucu', 'konami'];
  const AD = {snake: 'Yılan', pong: 'Pong', matrix: 'Bit yağmuru', gravity: 'Yerçekimi', roll: 'Takla', cimbom: 'Cimbom',
              dalga: 'Bit dalgası', logo: 'Piksel logo', dipnot: 'Dipnot', salla: 'Salla', koruyucu: 'Ekran koruyucu', konami: 'Konami'};
  const oku = () => { try { return JSON.parse(localStorage.getItem('kubik.surpriz') || '[]'); } catch { return []; } };
  const renkler = () => max() ? MAX_RENK : ['#f1f1f3', '#b8b8c0', '#8b8b94'];
  const sec = a => a[(Math.random() * a.length) | 0];

  let bildirim;
  function haber(metin) {
    bildirim ??= Object.assign(document.createElement('div'), {className: 'surpriz-haber', role: 'status'});
    if (!bildirim.isConnected) document.body.append(bildirim);
    bildirim.textContent = metin;
    bildirim.classList.remove('gor');
    void bildirim.offsetWidth;
    bildirim.classList.add('gor');
  }
  function buldu(id) {
    const l = oku();
    if (!l.includes(id)) {
      l.push(id);
      try { localStorage.setItem('kubik.surpriz', JSON.stringify(l)); } catch { /* gizli sekme */ }
    }
    haber(`${AD[id]} · sürpriz ${l.length}/${TUM.length}`);
  }

  // --- parçacık katmanı (tüm efektler aynı tuvali kullanır) ---
  let tuval, ctx, parca = [], dongu = 0, dpr = 1;
  function katman() {
    if (tuval) return;
    tuval = Object.assign(document.createElement('canvas'), {className: 'surpriz-katman'});
    tuval.setAttribute('aria-hidden', 'true');
    document.body.append(tuval);
    ctx = tuval.getContext('2d');
    const boyut = () => { dpr = Math.min(devicePixelRatio || 1, 2); tuval.width = innerWidth * dpr; tuval.height = innerHeight * dpr; };
    boyut();
    addEventListener('resize', boyut);
  }
  function oynat() { katman(); if (!dongu) dongu = requestAnimationFrame(ciz); }
  function ciz() {
    dongu = 0;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, innerWidth, innerHeight);
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    parca = parca.filter(p => p.omur > 0);
    for (const p of parca) {
      if (p.ev) {                                   // eve dönen piksel (logo)
        p.t++;
        if (p.t > 38) { p.x += (p.ev[0] - p.x) * 0.14; p.y += (p.ev[1] - p.y) * 0.14; p.vx = p.vy = 0; }
        if (p.t > 95) p.omur = 0;
      } else p.omur--;
      p.x += p.vx; p.y += p.vy;
      p.vx *= p.surt ?? 0.96; p.vy = p.vy * (p.surt ?? 0.96) + (p.agirlik ?? 0);
      ctx.globalAlpha = p.ev ? 1 : Math.min(1, p.omur / 30);
      ctx.fillStyle = p.renk;
      if (p.harf) { ctx.font = `600 ${p.boy}px 'Space Grotesk',monospace`; ctx.fillText(p.harf, p.x, p.y); }
      else ctx.fillRect(p.x - p.boy / 2, p.y - p.boy / 2, p.boy, p.boy);
    }
    ctx.globalAlpha = 1;
    if (parca.length) dongu = requestAnimationFrame(ciz);
  }
  function dalga(x, y, guc = 1) {               // bit halkası
    const n = Math.round(46 * guc), r = renkler();
    for (let i = 0; i < n; i++) {
      const a = (i / n) * Math.PI * 2, h = (3.2 + Math.random() * 1.6) * guc;
      parca.push({x, y, vx: Math.cos(a) * h, vy: Math.sin(a) * h, omur: 55 + Math.random() * 20, harf: Math.random() < 0.5 ? '0' : '1',
                  boy: 11 + Math.random() * 4, renk: sec(r), surt: 0.95});
    }
    oynat();
  }
  function konfeti(renk, n = 140) {
    for (let i = 0; i < n; i++) {
      const sol = i % 2 === 0;
      parca.push({x: sol ? -10 : innerWidth + 10, y: innerHeight * (0.55 + Math.random() * 0.4),
                  vx: (sol ? 1 : -1) * (5 + Math.random() * 9), vy: -(9 + Math.random() * 10), omur: 140 + Math.random() * 60,
                  boy: 5 + Math.random() * 6, renk: sec(renk), surt: 0.985, agirlik: 0.32});
    }
    oynat();
  }

  // --- büyük logo piksellere dağılıp geri toplanır ---
  function logoPatlat(el) {
    const r = el.getBoundingClientRect(), url = getComputedStyle(el).backgroundImage.match(/url\("?(.*?)"?\)/)?.[1];
    if (!url || !r.width) return;
    const img = new Image();
    img.onload = () => {
      const c = document.createElement('canvas'), w = Math.round(r.width), h = Math.round(r.height);
      c.width = w; c.height = h;
      const g = c.getContext('2d');
      g.drawImage(img, 0, 0, w, h);
      const d = g.getImageData(0, 0, w, h).data, adim = 2;
      for (let y = 0; y < h; y += adim) for (let x = 0; x < w; x += adim) {
        const i = (y * w + x) * 4;
        if (d[i + 3] < 120) continue;
        const ex = r.left + x, ey = r.top + y, a = Math.atan2(ey - (r.top + h / 2), ex - (r.left + w / 2)), hz = 4 + Math.random() * 7;
        parca.push({x: ex, y: ey, vx: Math.cos(a) * hz + (Math.random() - .5) * 3, vy: Math.sin(a) * hz + (Math.random() - .5) * 3,
                    ev: [ex, ey], t: 0, omur: 1, boy: adim, renk: `rgb(${d[i]},${d[i + 1]},${d[i + 2]})`, surt: 0.9});
      }
      el.style.visibility = 'hidden';
      oynat();
      setTimeout(() => { el.style.visibility = ''; }, 95 * 16.7);
    };
    img.src = url;
  }

  // --- tam ekran oyun kabuğu ---
  let oyun = null;
  function oyunAc(baslik, ipucu) {
    oyunKapat();
    const kap = Object.assign(document.createElement('div'), {className: 'surpriz-oyun'});
    kap.innerHTML = '<div class="so-ust"><b></b><span class="so-skor"></span><button type="button" class="so-kapat" aria-label="Kapat">×</button></div><canvas></canvas><p class="so-ipucu"></p>';
    kap.querySelector('b').textContent = baslik;
    kap.querySelector('.so-ipucu').textContent = ipucu;
    kap.querySelector('.so-kapat').addEventListener('click', oyunKapat);
    document.body.append(kap);
    const cv = kap.querySelector('canvas');
    oyun = {kap, cv, ctx: cv.getContext('2d'), skor: kap.querySelector('.so-skor'), dur: [], tus: null};
    return oyun;
  }
  function oyunKapat() {
    if (!oyun) return;
    oyun.dur.forEach(f => f());
    oyun.kap.remove();
    oyun = null;
  }
  function tuvalBoyut(o, w, h) {
    const d = Math.min(devicePixelRatio || 1, 2);
    o.cv.width = w * d; o.cv.height = h * d;
    o.cv.style.width = w + 'px'; o.cv.style.height = h + 'px';
    o.ctx.setTransform(d, 0, 0, d, 0, 0);
    o.kap.querySelector('.so-ust').style.width = w + 'px';
  }

  function yilan() {
    const o = oyunAc('Yılan', 'Ok tuşları / WASD / kaydır · Boşluk: yeniden · Esc: çık');
    const H = 20, N = Math.max(12, Math.min(24, Math.floor(Math.min(innerWidth - 32, innerHeight - 170) / H)));
    tuvalBoyut(o, N * H, N * H);
    let govde, yon, siradaki, yem, bitti, skor, rekor = +(localStorage.getItem('kubik.yilan') || 0);
    const yemKoy = () => { do yem = [(Math.random() * N) | 0, (Math.random() * N) | 0]; while (govde.some(p => p[0] === yem[0] && p[1] === yem[1])); };
    const basla = () => { govde = [[5, 8], [4, 8], [3, 8]]; yon = siradaki = [1, 0]; skor = 0; bitti = false; yemKoy(); };
    basla();
    const renk = renkler();
    const adim = () => {
      if (bitti) return ciz2();
      yon = siradaki;
      const bas = [(govde[0][0] + yon[0] + N) % N, (govde[0][1] + yon[1] + N) % N];
      if (govde.some(p => p[0] === bas[0] && p[1] === bas[1])) {
        bitti = true;
        if (skor > rekor) { rekor = skor; try { localStorage.setItem('kubik.yilan', rekor); } catch { /* */ } }
      } else {
        govde.unshift(bas);
        if (bas[0] === yem[0] && bas[1] === yem[1]) { skor++; yemKoy(); } else govde.pop();
      }
      ciz2();
    };
    const ciz2 = () => {
      const g = o.ctx;
      g.fillStyle = '#050506'; g.fillRect(0, 0, N * H, N * H);
      g.fillStyle = '#111114';
      for (let x = 0; x < N; x++) for (let y = 0; y < N; y++) g.fillRect(x * H + H / 2 - 1, y * H + H / 2 - 1, 2, 2);
      govde.forEach((p, i) => { g.fillStyle = max() ? renk[Math.min(renk.length - 1, (i / 3) | 0)] : (i ? '#b8b8c0' : '#fff'); g.fillRect(p[0] * H + 2, p[1] * H + 2, H - 4, H - 4); });
      g.fillStyle = (Date.now() / 300 | 0) % 2 ? '#fff' : (max() ? '#18d2ff' : '#8b8b94');
      g.font = "700 15px 'Space Grotesk',monospace"; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.fillText('1', yem[0] * H + H / 2, yem[1] * H + H / 2 + 1);
      o.skor.textContent = `${skor} · rekor ${rekor}`;
      if (bitti) { g.fillStyle = 'rgba(0,0,0,.65)'; g.fillRect(0, 0, N * H, N * H); g.fillStyle = '#fff'; g.font = "600 22px 'Space Grotesk'"; g.fillText('Oyun bitti · Boşluk', N * H / 2, N * H / 2); }
    };
    const zaman = setInterval(adim, 95);
    o.dur.push(() => clearInterval(zaman));
    const cevir = d => { if (d[0] !== -yon[0] || d[1] !== -yon[1]) siradaki = d; };
    o.tus = e => {
      const d = {ArrowUp: [0, -1], w: [0, -1], ArrowDown: [0, 1], s: [0, 1], ArrowLeft: [-1, 0], a: [-1, 0], ArrowRight: [1, 0], d: [1, 0]}[e.key];
      if (d) { cevir(d); return true; }
      if (e.key === ' ' && bitti) { basla(); return true; }
    };
    let bas0;
    o.cv.addEventListener('pointerdown', e => { bas0 = [e.clientX, e.clientY]; if (bitti) basla(); });
    o.cv.addEventListener('pointerup', e => {
      if (!bas0) return;
      const dx = e.clientX - bas0[0], dy = e.clientY - bas0[1];
      if (Math.max(Math.abs(dx), Math.abs(dy)) > 20) cevir(Math.abs(dx) > Math.abs(dy) ? [Math.sign(dx), 0] : [0, Math.sign(dy)]);
    });
    buldu('snake');
  }

  function pong() {
    const o = oyunAc('Pong', 'Fare / parmakla raketi oynat · 5 olan kazanır · Esc: çık');
    const W = Math.min(720, innerWidth - 32), H = Math.min(440, Math.round(W * 0.6));
    tuvalBoyut(o, W, H);
    const R = 10, RH = 70;
    let ben = H / 2, cpu = H / 2, top, puan = [0, 0], bekle = 40;
    const servis = yon => { top = {x: W / 2, y: H / 2, vx: 5 * yon, vy: (Math.random() - .5) * 6}; bekle = 40; };
    servis(1);
    const renk = renkler();
    o.cv.addEventListener('pointermove', e => { ben = e.clientY - o.cv.getBoundingClientRect().top; });
    o.tus = e => { if (e.key === 'ArrowUp') { ben -= 30; return true; } if (e.key === 'ArrowDown') { ben += 30; return true; } };
    let son = 0;
    const kare = () => {
      son = requestAnimationFrame(kare);
      const g = o.ctx;
      ben = Math.max(RH / 2, Math.min(H - RH / 2, ben));
      cpu += Math.max(-4.2, Math.min(4.2, top.y - cpu));
      if (bekle-- <= 0 && puan[0] < 5 && puan[1] < 5) {
        top.x += top.vx; top.y += top.vy;
        if (top.y < R || top.y > H - R) top.vy *= -1;
        for (const [px, py, s] of [[20, ben, 1], [W - 20, cpu, -1]]) {
          if (Math.abs(top.x - px) < R + 4 && Math.abs(top.y - py) < RH / 2 + R && Math.sign(top.vx) === -s) {
            top.vx = -top.vx * 1.06; top.vy += (top.y - py) * 0.12;
            dalga(o.cv.getBoundingClientRect().left + top.x, o.cv.getBoundingClientRect().top + top.y, 0.35);
          }
        }
        if (top.x < 0) { puan[1]++; servis(1); }
        if (top.x > W) { puan[0]++; servis(-1); }
      }
      g.fillStyle = '#050506'; g.fillRect(0, 0, W, H);
      g.fillStyle = '#1e1e23';
      for (let y = 6; y < H; y += 22) g.fillRect(W / 2 - 1, y, 2, 12);
      g.fillStyle = max() ? renk[0] : '#f1f1f3'; g.fillRect(14, ben - RH / 2, 10, RH);
      g.fillStyle = max() ? renk[4] : '#8b8b94'; g.fillRect(W - 24, cpu - RH / 2, 10, RH);
      g.fillStyle = '#fff'; g.fillRect(top.x - R / 2 - 2, top.y - R / 2 - 2, R + 4, R + 4);
      g.font = "600 46px 'Space Grotesk'"; g.textAlign = 'center'; g.fillStyle = '#2a2a30';
      g.fillText(puan[0], W / 2 - 60, 60); g.fillText(puan[1], W / 2 + 60, 60);
      o.skor.textContent = `Sen ${puan[0]} · ${puan[1]} Klik`;
      if (puan[0] >= 5 || puan[1] >= 5) {
        g.fillStyle = '#fff'; g.font = "600 26px 'Space Grotesk'";
        g.fillText(puan[0] >= 5 ? 'Kazandın! Tıkla: rövanş' : 'Klik kazandı · Tıkla: rövanş', W / 2, H / 2);
      }
    };
    o.cv.addEventListener('click', () => { if (puan[0] >= 5 || puan[1] >= 5) { puan = [0, 0]; servis(1); } });
    kare();
    o.dur.push(() => cancelAnimationFrame(son));
    buldu('pong');
  }

  function matrix(id = 'matrix') {
    const cv = Object.assign(document.createElement('canvas'), {className: 'surpriz-yagmur'});
    document.body.append(cv);
    const g = cv.getContext('2d'), d = Math.min(devicePixelRatio || 1, 2), A = 16;
    cv.width = innerWidth * d; cv.height = innerHeight * d;
    g.scale(d, d);
    const sut = Math.ceil(innerWidth / A), y = Array.from({length: sut}, () => Math.random() * -60), renk = renkler();
    const bas = performance.now();
    let son;
    const kare = t => {
      g.fillStyle = 'rgba(0,0,0,.12)'; g.fillRect(0, 0, innerWidth, innerHeight);
      g.font = "600 14px 'Space Grotesk',monospace"; g.textAlign = 'center';
      for (let i = 0; i < sut; i++) {
        g.fillStyle = Math.random() < 0.08 ? '#fff' : (max() ? renk[(i / sut * renk.length) | 0] : '#8b8b94');
        g.fillText(Math.random() < 0.5 ? '0' : '1', i * A + A / 2, y[i] * A);
        if (y[i] * A > innerHeight && Math.random() > 0.975) y[i] = 0;
        y[i] += 0.55;
      }
      if (t - bas < 9000) son = requestAnimationFrame(kare);
      else { cv.classList.add('git'); setTimeout(() => cv.remove(), 700); }
    };
    son = requestAnimationFrame(kare);
    cv.addEventListener('click', () => { cancelAnimationFrame(son); cv.remove(); });
    buldu(id);
  }

  function yercekimi() {
    const els = [...document.querySelectorAll('.suggestions button, .ayar-cip, .welcome h1, .welcome p, .welcome .eyebrow, .sidebar button, .mode-switch button, .composer-footnote, .topbar > *, .auth-card > *, .auth-story h1, .auth-story p, .auth-story .eyebrow')]
      .filter(e => e.offsetParent && e.getBoundingClientRect().bottom < innerHeight);
    if (!els.length) return;
    const s = els.map(el => ({el, y: 0, vy: -Math.random() * 4, x: 0, vx: (Math.random() - .5) * 3, r: 0, vr: (Math.random() - .5) * 4,
                              alt: innerHeight - el.getBoundingClientRect().bottom - 2}));
    let son, n = 0;
    const kare = () => {
      for (const p of s) {
        p.vy += sakin ? 0.6 : 0.9; p.y += p.vy; p.x += p.vx; p.r += p.vr;
        if (p.y > p.alt) { p.y = p.alt; p.vy *= -0.38; p.vx *= 0.8; p.vr *= 0.6; }
        p.el.style.transform = `translate(${p.x}px,${p.y}px) rotate(${p.r}deg)`;
      }
      if (++n < 330) son = requestAnimationFrame(kare); else geri();
    };
    const geri = () => {
      cancelAnimationFrame(son);
      removeEventListener('pointerdown', geri, true);
      for (const p of s) {
        p.el.style.transition = 'transform .7s cubic-bezier(.3,1.4,.5,1)';
        p.el.style.transform = '';
        setTimeout(() => { p.el.style.transition = ''; }, 750);
      }
    };
    son = requestAnimationFrame(kare);
    setTimeout(() => addEventListener('pointerdown', geri, true), 300);
    buldu('gravity');
  }

  function takla() {
    document.body.classList.remove('surpriz-takla');
    void document.body.offsetWidth;
    document.body.classList.add('surpriz-takla');
    setTimeout(() => document.body.classList.remove('surpriz-takla'), 1300);
    buldu('roll');
  }

  function cimbom() {
    konfeti(GS_RENK, 180);
    const y = Object.assign(document.createElement('div'), {className: 'surpriz-cimbom', textContent: 'CİMBOM'});
    document.body.append(y);
    setTimeout(() => y.remove(), 2600);
    buldu('cimbom');
  }

  function liste() {
    const l = oku();
    haber(`Sürpriz ${l.length}/${TUM.length}: ` + (l.length ? l.map(k => AD[k]).join(', ') : 'henüz yok. Yazı kutusuna /snake yazmayı dene.'));
  }

  // --- ekran koruyucu: 520 sn hareketsizlikte logo köşelere çarparak gezer ---
  let bosZaman, koruyucu;
  function koruyucuAc() {
    if (koruyucu || oyun || document.hidden || document.querySelector('dialog[open]') || !klik()) return;
    koruyucu = Object.assign(document.createElement('div'), {className: 'surpriz-koruyucu'});
    const logo = Object.assign(document.createElement('img'), {alt: '', src: new URL(`logo/${max() ? 'klik-max' : 'klik'}.svg`, document.querySelector('script[src*="surpriz.js"]').src).href});
    koruyucu.append(logo);
    document.body.append(koruyucu);
    let x = Math.random() * (innerWidth - 90), y = Math.random() * (innerHeight - 90), vx = 1.6, vy = 1.3, son;
    const kare = () => {
      x += vx; y += vy;
      const sx = x <= 0 || x >= innerWidth - 90, sy = y <= 0 || y >= innerHeight - 90;
      if (sx) vx = -vx;
      if (sy) vy = -vy;
      if (sx && sy) { dalga(x + 45, y + 45, 1.4); buldu('koruyucu'); }
      else if (sx || sy) logo.animate([{filter: 'brightness(2.2)'}, {filter: 'none'}], 400);
      x = Math.max(0, Math.min(innerWidth - 90, x)); y = Math.max(0, Math.min(innerHeight - 90, y));
      logo.style.transform = `translate(${x}px,${y}px)`;
      son = requestAnimationFrame(kare);
    };
    son = requestAnimationFrame(kare);
    koruyucu.dur = () => cancelAnimationFrame(son);
  }
  function hareket() {
    if (koruyucu) { koruyucu.dur(); koruyucu.remove(); koruyucu = null; }
    clearTimeout(bosZaman);
    bosZaman = setTimeout(koruyucuAc, 520000);
  }

  const KOMUT = {snake: yilan, yilan, 'yılan': yilan, pong, matrix: () => matrix(), gravity: yercekimi, yercekimi, roll: takla, takla,
                 cimbom, galatasaray: cimbom, 'sürpriz': liste, surpriz: liste, egg: liste};

  function kur() {
    if (!klik()) return;
    // yazı kutusuna /komut: gönderilmez, sürpriz çalışır (app.js'in submit'inden önce, belge yakalama aşamasında)
    document.addEventListener('submit', e => {
      if (e.target.id !== 'chatForm') return;
      const p = document.getElementById('prompt'), k = p?.value.trim().toLocaleLowerCase('tr').match(/^\/(\S+)$/)?.[1];
      if (!k || !KOMUT[k]) return;
      e.preventDefault();
      e.stopImmediatePropagation();
      p.value = '';
      p.dispatchEvent(new Event('input', {bubbles: true}));
      KOMUT[k]();
    }, true);

    // klavye: oyun tuşları, Esc, yazı alanı dışında kelime ve Konami
    const KONAMI = 'ArrowUp ArrowUp ArrowDown ArrowDown ArrowLeft ArrowRight ArrowLeft ArrowRight b a';
    let tamp = '', tuslar = [];
    addEventListener('keydown', e => {
      hareket();
      if (oyun) {
        if (e.key === 'Escape') { oyunKapat(); e.preventDefault(); return; }
        if (oyun.tus?.(e)) { e.preventDefault(); e.stopPropagation(); }
        return;
      }
      tuslar = [...tuslar, e.key.length === 1 ? e.key.toLowerCase() : e.key].slice(-10);
      if (tuslar.join(' ') === KONAMI) { tuslar = []; matrix('konami'); return; }
      if (e.target.closest?.('input, textarea, select, [contenteditable]') || e.ctrlKey || e.metaKey || e.altKey) return;
      if (e.key.length !== 1) return;
      tamp = (tamp + e.key.toLowerCase()).slice(-12);
      for (const k of ['snake', 'pong', 'matrix']) if (tamp.endsWith(k)) { tamp = ''; KOMUT[k](); }
    }, true);

    // boş yere çift tık: bit dalgası
    addEventListener('dblclick', e => {
      if (oyun || e.target.closest('button, a, input, textarea, select, label, dialog, .message, .composer, pre, code, [contenteditable]')) return;
      getSelection()?.removeAllRanges();
      dalga(e.clientX, e.clientY);
      buldu('dalga');
    });

    // büyük logo (karşılama): 5 hızlı tık
    let tik = [];
    document.addEventListener('click', e => {
      const logo = e.target.closest('.welcome .cube.large');
      if (logo) {
        const t = performance.now();
        tik = [...tik.filter(x => t - x < 1500), t];
        if (tik.length >= 5) { tik = []; logoPatlat(logo); buldu('logo'); }
      }
      const not = e.target.closest('.composer-footnote');
      if (not && e.detail === 3) {
        const s = not.querySelector('span') || not, eski = s.textContent;
        s.textContent = 'Klik hata yapabilir. Sen de. Birlikte daha az yaparız.';
        const r = s.getBoundingClientRect();
        dalga(r.left + r.width / 2, r.top + r.height / 2, 0.5);
        setTimeout(() => { s.textContent = eski; }, 4000);
        buldu('dipnot');
      }
    });

    // telefonu salla: ekranın ortasından bit patlaması
    let sonSalla = 0;
    addEventListener('devicemotion', e => {
      const a = e.accelerationIncludingGravity || e.acceleration;
      if (!a || Math.hypot(a.x || 0, a.y || 0, a.z || 0) < 28 || Date.now() - sonSalla < 1500) return;
      sonSalla = Date.now();
      dalga(innerWidth / 2, innerHeight / 2, 1.8);
      buldu('salla');
    });

    ['pointermove', 'pointerdown', 'wheel', 'touchstart'].forEach(t => addEventListener(t, hareket, {passive: true}));
    hareket();

    console.log('%c\n  ▖ ▖▌ ▘▌ \n  ▙▘▌ ▌▙▘   Klik\n  ▌▌▙▖▌▛▖\n', `font:600 14px monospace;color:${max() ? '#a597ff' : '#f1f1f3'}`);
    console.log('%cMeraklı mısın? Bu sitede 12 sürpriz saklı. İpucu: yazı kutusuna /sürpriz yaz. ↑↑↓↓←→←→BA da bir şey yapar.', 'color:#8b8b94');
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
