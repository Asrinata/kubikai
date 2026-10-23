'use strict';
// Cube Connect: 24 bitlik giriş kodu, Code arka planındaki gibi birbirine bağlı yıldızlarla çizilir.
// Köşelerde 4 "küp" çapa var (sol üstteki en büyüğü, yönü o belirler); arada 6x5 ızgarada yıldız varsa bit 1.
// 24 bit veri + 6 bit sağlama. Kamera okuması: parlak lekeler -> en büyük 4'ü çapa -> perspektif -> hücreleri oku.
window.kubikCube = (() => {
  const COLS = 6, ROWS = 5, BITS = 24, CHECK = 6;
  const checksum = data => Math.imul(data ^ 0x5bd1e995, 0x27d4eb2d) >>> (32 - CHECK);
  const toBits = code => { const value = code * 2 ** CHECK + checksum(code), bits = [];
    for (let i = COLS * ROWS - 1; i >= 0; i--) bits.push(Math.floor(value / 2 ** i) % 2); return bits; };
  const fromBits = bits => { let value = 0; for (const b of bits) value = value * 2 + b;
    const code = Math.floor(value / 2 ** CHECK); return checksum(code) === value % 2 ** CHECK ? code : null; };
  // kod uzayı: 0..1 kare; çapalar köşelerde, hücreler içeride
  const ANCHORS = [[0, 0], [1, 0], [1, 1], [0, 1]];
  const cell = i => [(i % COLS + 1) / (COLS + 1), (Math.floor(i / COLS) + 1) / (ROWS + 1)];

  function draw(canvas, code, time = 0) {
    const ctx = canvas.getContext('2d'), size = canvas.width, pad = size * .1, span = size - 2 * pad;
    const at = ([u, v]) => [pad + u * span, pad + v * span], bits = toBits(code);
    ctx.fillStyle = '#07080e'; ctx.fillRect(0, 0, size, size);
    // sönük süs yıldızları (okuma eşiğinin altında kalır)
    for (let i = 0; i < 70; i++) { const x = (Math.sin(i * 91.7) * .5 + .5) * size, y = (Math.sin(i * 47.3 + 1) * .5 + .5) * size;
      ctx.globalAlpha = .12 + .08 * Math.sin(time / 700 + i); ctx.fillStyle = '#8fa7ff'; ctx.beginPath(); ctx.arc(x, y, size / 500, 0, 7); ctx.fill(); }
    // yıldızlar hücre ortasından biraz kayık: ızgara gibi değil takımyıldız gibi görünsün (okuyucu bu kaymayı tolere eder)
    const jitter = i => [Math.sin(i * 12.9898 + code % 997) * .09 / (COLS + 1), Math.sin(i * 78.233 + code % 991) * .09 / (ROWS + 1)];
    const lit = bits.map((b, i) => b ? at(cell(i).map((v, k) => v + jitter(i)[k])) : null).filter(Boolean), corners = ANCHORS.map(at);
    // takımyıldız çizgileri: yakın yıldızları ve çapaları bağla
    ctx.lineWidth = size / 300; ctx.strokeStyle = '#8fa7ff';
    const points = [...corners, ...lit];
    for (let i = 0; i < points.length; i++) for (let j = i + 1; j < points.length; j++) {
      const d = Math.hypot(points[i][0] - points[j][0], points[i][1] - points[j][1]);
      if (d < span * .26) { ctx.globalAlpha = .18 + (1 - d / (span * .26)) * .4; ctx.beginPath(); ctx.moveTo(...points[i]); ctx.lineTo(...points[j]); ctx.stroke(); }
    }
    // veri yıldızları: hafif parıldar ama hep eşiğin üstünde
    lit.forEach(([x, y], i) => {
      const glow = ctx.createRadialGradient(x, y, 0, x, y, size * .045);
      glow.addColorStop(0, 'rgba(184,198,255,.35)'); glow.addColorStop(1, 'rgba(184,198,255,0)');
      ctx.globalAlpha = 1; ctx.fillStyle = glow; ctx.beginPath(); ctx.arc(x, y, size * .045, 0, 7); ctx.fill();
      ctx.globalAlpha = .88 + .12 * Math.sin(time / 300 + i * 1.7); ctx.fillStyle = '#f2f5ff';
      ctx.beginPath(); ctx.arc(x, y, size * .017, 0, 7); ctx.fill();
    });
    // çapalar: dönen küp (izometrik altıgen); sol üstteki büyük
    corners.forEach(([x, y], i) => {
      const r = size * (i === 0 ? .062 : .042), turn = time / 2400 + i;
      // üç yüz üç ayrı açık tonda: hepsi okuma eşiğinin üstünde, çapa tek leke olarak kalır
      ctx.globalAlpha = 1;
      ['#f2f5ff', '#d4dcfb', '#b9c6f7'].forEach((shade, k) => {
        const a = turn + k * 2 * Math.PI / 3, p = j => [x + Math.cos(a + j * Math.PI / 3) * r, y + Math.sin(a + j * Math.PI / 3) * r];
        ctx.fillStyle = shade; ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(...p(0)); ctx.lineTo(...p(1)); ctx.lineTo(...p(2)); ctx.closePath(); ctx.fill();
      });
    });
    ctx.globalAlpha = 1;
  }

  // ---- okuma
  function homography(src, dst) {     // src[i] -> dst[i], 4 nokta; 8x8 doğrusal sistem
    const A = [], b = [];
    src.forEach(([x, y], i) => { const [u, v] = dst[i];
      A.push([x, y, 1, 0, 0, 0, -u * x, -u * y]); b.push(u);
      A.push([0, 0, 0, x, y, 1, -v * x, -v * y]); b.push(v); });
    for (let c = 0; c < 8; c++) {
      let p = c; for (let r = c + 1; r < 8; r++) if (Math.abs(A[r][c]) > Math.abs(A[p][c])) p = r;
      [A[c], A[p]] = [A[p], A[c]]; [b[c], b[p]] = [b[p], b[c]];
      if (Math.abs(A[c][c]) < 1e-9) return null;
      for (let r = 0; r < 8; r++) if (r !== c) { const f = A[r][c] / A[c][c]; for (let k = c; k < 8; k++) A[r][k] -= f * A[c][k]; b[r] -= f * b[c]; }
    }
    const h = b.map((v, i) => v / A[i][i]);
    return ([x, y]) => { const w = h[6] * x + h[7] * y + 1; return [(h[0] * x + h[1] * y + h[2]) / w, (h[3] * x + h[4] * y + h[5]) / w]; };
  }

  function decode(image, debug) {    // image: ImageData
    const {width: w, height: h, data} = image, lum = new Uint8Array(w * h);
    let peak = 0;
    for (let i = 0; i < w * h; i++) { const v = (data[i * 4] * 2 + data[i * 4 + 1] * 5 + data[i * 4 + 2]) >> 3; lum[i] = v; if (v > peak) peak = v; }
    if (peak < 90) return null;
    const threshold = Math.max(70, peak * .62), seen = new Uint8Array(w * h), blobs = [], stack = [];
    for (let start = 0; start < w * h; start++) {
      if (seen[start] || lum[start] < threshold) continue;
      let area = 0, sx = 0, sy = 0; stack.push(start); seen[start] = 1;
      while (stack.length) { const p = stack.pop(), x = p % w, y = (p - x) / w; area++; sx += x; sy += y;
        for (const q of [p - 1, p + 1, p - w, p + w]) if (q >= 0 && q < w * h && !seen[q] && lum[q] >= threshold && Math.abs((q % w) - x) <= 1) { seen[q] = 1; stack.push(q); } }
      if (area >= 2) blobs.push({area, x: sx / area, y: sy / area});
    }
    blobs.sort((a, b) => b.area - a.area);
    if (debug) debug.blobs = blobs.slice(0, 8);
    if (blobs.length < 4) return null;
    const anchors = blobs.slice(0, 4);
    if (anchors[3].area < 1.5 * (blobs[4]?.area || 0)) return null;     // çapalar yıldızlardan belirgin büyük olmalı
    const cx = anchors.reduce((s, a) => s + a.x, 0) / 4, cy = anchors.reduce((s, a) => s + a.y, 0) / 4;
    const angle = a => Math.atan2(a.y - cy, a.x - cx);
    const first = anchors[0], rest = anchors.slice(1).sort((a, b) => ((angle(a) - angle(first) + 2 * Math.PI) % (2 * Math.PI)) - ((angle(b) - angle(first) + 2 * Math.PI) % (2 * Math.PI)));
    const map = homography(ANCHORS, [first, ...rest].map(a => [a.x, a.y]));
    if (!map) return null;
    const spacing = Math.hypot(...[0, 1].map(k => map([1 / (COLS + 1), 0])[k] - map([0, 0])[k])), reach = Math.max(1, Math.round(spacing * .18));
    const bits = [];
    for (let i = 0; i < COLS * ROWS; i++) {
      const [x, y] = map(cell(i)).map(Math.round); let best = 0;
      for (let dy = -reach; dy <= reach; dy++) for (let dx = -reach; dx <= reach; dx++) {
        const px = x + dx, py = y + dy; if (px >= 0 && py >= 0 && px < w && py < h) best = Math.max(best, lum[py * w + px]); }
      bits.push(best >= threshold ? 1 : 0);
    }
    if (debug) debug.bits = bits;
    return fromBits(bits);
  }

  // Kameradan sürekli okuma; okununca onFound(code). stop() döndürür.
  async function scan(video, onFound) {
    const stream = await navigator.mediaDevices.getUserMedia({video: {facingMode: 'environment', width: {ideal: 1280}}, audio: false});
    video.srcObject = stream; video.setAttribute('playsinline', ''); await video.play();
    const canvas = document.createElement('canvas'), ctx = canvas.getContext('2d', {willReadFrequently: true});
    let running = true, last = null, same = 0;
    (function tick() {
      if (!running) return;
      if (video.videoWidth) {
        const scale = 420 / Math.max(video.videoWidth, video.videoHeight);
        canvas.width = Math.round(video.videoWidth * scale); canvas.height = Math.round(video.videoHeight * scale);
        ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
        const code = decode(ctx.getImageData(0, 0, canvas.width, canvas.height));
        same = code !== null && code === last ? same + 1 : 0; last = code;
        if (same >= 1) { running = false; stream.getTracks().forEach(t => t.stop()); onFound(code); return; }   // iki kare aynı okunsun
      }
      setTimeout(tick, 120);
    })();
    return () => { running = false; stream.getTracks().forEach(t => t.stop()); };
  }
  return {draw, decode, scan, toBits, fromBits, BITS};
})();
