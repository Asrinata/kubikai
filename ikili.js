'use strict';
// Klik Code arka planı: 1 ve 0 ızgarası. Fare/parmak gezdikçe etrafı aydınlanır, yakındaki bitler daha sık döner.
// Yalnız Klik markasında ve Code modunda (body.code-theme) çalışır; sekme gizliyken ve modlar dışında hiç çizmez.
// Free: beyaz/gri. Max ve üstü (data-plan=max): ışık mor→mavi. prefers-reduced-motion: bitler dönmez, yalnız ışık.
(() => {
  const kok = document.documentElement;
  const azHareket = matchMedia('(prefers-reduced-motion: reduce)');
  const ARALIK = 15, YAZI = 10.5, ISIK = 190, TABAN = 0.05;
  const MAX_RENK = [[196, 107, 255], [107, 92, 255], [24, 210, 255]];
  let tuval, ctx, w = 0, h = 0, dpr = 1, sut = 0, sat = 0, bit, parlak, glif = {};
  let fare = {x: -9999, y: -9999, gx: -9999, gy: -9999, son: 0}, dongu = 0, sonKare = 0, max = false;

  const aktif = () => kok.dataset.brand === 'klik' && document.body.classList.contains('code-theme') && !document.hidden;

  function renk(t) {          // 0..1 -> mor→mavi
    const k = t * (MAX_RENK.length - 1), i = Math.min(Math.floor(k), MAX_RENK.length - 2), f = k - i;
    return MAX_RENK[i].map((c, j) => Math.round(c + (MAX_RENK[i + 1][j] - c) * f));
  }

  function glifYap(karakter, rgb) {      // her karakter + renk bir kez çizilir, sonra drawImage (hızlı)
    const c = document.createElement('canvas'), s = Math.ceil(ARALIK * dpr);
    c.width = c.height = s;
    const g = c.getContext('2d');
    g.font = `500 ${YAZI * dpr}px 'Space Grotesk', ui-monospace, Consolas, monospace`;
    g.textAlign = 'center';
    g.textBaseline = 'middle';
    g.fillStyle = `rgb(${rgb.join(',')})`;
    g.fillText(karakter, s / 2, s / 2 + dpr);
    return c;
  }

  function glifler() {
    glif = {beyaz: ['0', '1'].map(k => glifYap(k, [241, 241, 243])), renk: []};
    for (let i = 0; i < 8; i++) glif.renk.push(['0', '1'].map(k => glifYap(k, renk(i / 7))));
  }

  function boyut() {
    dpr = Math.min(devicePixelRatio || 1, 2);
    w = innerWidth;
    h = innerHeight;
    tuval.width = Math.ceil(w * dpr);
    tuval.height = Math.ceil(h * dpr);
    sut = Math.ceil(w / ARALIK) + 1;
    sat = Math.ceil(h / ARALIK) + 1;
    bit = new Uint8Array(sut * sat).map(() => Math.random() < 0.5 ? 1 : 0);
    parlak = new Float32Array(sut * sat).fill(TABAN);
    glifler();
  }

  function kare(zaman) {
    dongu = 0;
    if (!aktif()) { ctx && ctx.clearRect(0, 0, tuval.width, tuval.height); return; }
    const hareketli = zaman - fare.son < 1500;
    if (zaman - sonKare < (hareketli ? 1000 / 40 : 1000 / 10)) { dongu = requestAnimationFrame(kare); return; }
    sonKare = zaman;
    fare.gx += (fare.x - fare.gx) * 0.22;           // ışık imleci yumuşak takip eder
    fare.gy += (fare.y - fare.gy) * 0.22;
    const sakin = azHareket.matches, s = ARALIK * dpr, R2 = ISIK * ISIK;
    if (!sakin) for (let n = Math.ceil(sut * sat * 0.0025); n--;) bit[(Math.random() * bit.length) | 0] ^= 1;
    ctx.clearRect(0, 0, tuval.width, tuval.height);
    if (fare.gx > -1000) {                          // imlecin altında yumuşak hale, bitler bunun üstünde parlar
      const r = ISIK * 1.3 * dpr, hale = ctx.createRadialGradient(fare.gx * dpr, fare.gy * dpr, 0, fare.gx * dpr, fare.gy * dpr, r);
      hale.addColorStop(0, max ? 'rgba(107,92,255,.13)' : 'rgba(255,255,255,.055)');
      hale.addColorStop(1, 'rgba(0,0,0,0)');
      ctx.fillStyle = hale;
      ctx.fillRect(fare.gx * dpr - r, fare.gy * dpr - r, r * 2, r * 2);
    }
    let hala = false;
    for (let y = 0; y < sat; y++) {
      const py = y * ARALIK, dy = py - fare.gy;
      for (let x = 0; x < sut; x++) {
        const i = y * sut + x, px = x * ARALIK, dx = px - fare.gx, d2 = dx * dx + dy * dy;
        let hedef = TABAN;
        if (d2 < R2) {
          const isik = 1 - Math.sqrt(d2) / ISIK;
          hedef = TABAN + isik * isik * 0.8;
          if (!sakin && Math.random() < isik * 0.12) bit[i] ^= 1;      // ışığın içinde bitler "hesaplar"
        }
        const p = parlak[i] += (hedef - parlak[i]) * 0.18;           // iz bırakarak söner
        if (p > TABAN + 0.01) hala = true;
        ctx.globalAlpha = p;
        const g = max && p > TABAN + 0.04 ? glif.renk[Math.min(7, ((px / w) * 8) | 0)] : glif.beyaz;
        ctx.drawImage(g[bit[i]], px * dpr - s / 2, py * dpr - s / 2);
      }
    }
    ctx.globalAlpha = 1;
    if (hareketli || hala || !sakin) dongu = requestAnimationFrame(kare);
  }

  function baslat() {
    max = kok.dataset.plan === 'max';
    if (aktif() && !dongu) dongu = requestAnimationFrame(kare);
  }

  function imlec(x, y) {
    fare.x = x;
    fare.y = y;
    if (fare.gx < -1000) { fare.gx = x; fare.gy = y; }
    fare.son = performance.now();
    baslat();
  }

  function kur() {
    tuval = document.createElement('canvas');
    tuval.id = 'ikili';
    tuval.setAttribute('aria-hidden', 'true');
    document.body.prepend(tuval);
    ctx = tuval.getContext('2d');
    boyut();
    addEventListener('pointermove', e => {
      imlec(e.clientX, e.clientY);
      const kart = e.target.closest?.('.suggestions button');   // öneri kartında imleç ışığı (marka.css --mx/--my)
      if (kart) { const r = kart.getBoundingClientRect(); kart.style.setProperty('--mx', `${e.clientX - r.left}px`); kart.style.setProperty('--my', `${e.clientY - r.top}px`); }
    }, {passive: true});
    addEventListener('pointerdown', e => imlec(e.clientX, e.clientY), {passive: true});
    document.addEventListener('pointerleave', () => { fare.x = fare.y = -9999; });
    let zamanlayici;
    addEventListener('resize', () => { clearTimeout(zamanlayici); zamanlayici = setTimeout(() => { boyut(); baslat(); }, 150); });
    document.addEventListener('visibilitychange', baslat);
    const izle = new MutationObserver(baslat);           // Code moduna girince / marka-plan değişince
    izle.observe(document.body, {attributes: true, attributeFilter: ['class']});
    izle.observe(kok, {attributes: true, attributeFilter: ['data-brand', 'data-plan']});
    document.fonts?.ready.then(() => { glifler(); baslat(); });
    baslat();
  }

  if (document.readyState === 'loading') addEventListener('DOMContentLoaded', kur); else kur();
})();
