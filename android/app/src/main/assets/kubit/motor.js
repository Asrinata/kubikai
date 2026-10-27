// Kübit wasm motoru: deneme.html ve test.js ortak kullanır. Her açılışta (yeniden başlama, uykudan uyanma) yeni bir örnek kurulur,
// yani RAM cihazdaki gibi sıfırlanır; kayıtlar (NVS) h.depo'da kalır.
// h: { kare(px, vt), parlak(v), bildir(satir), log(yazi), depo: { get(k) -> Uint8Array|undefined, set(k, u8), del(k) } }
async function kubitMotor(bytes, h) {
  const modul = await WebAssembly.compile(bytes);
  const dec = new TextDecoder(), enc = new TextEncoder();
  const tus = { 25: false, 26: false };               // GPIO25 = Tuş 1 (sağ), GPIO26 = Tuş 2 (sol)
  let ex, mem, cagriVt = 0, olay = 0;
  const bayt = (p, n) => new Uint8Array(mem.buffer, p, n);
  const cstr = p => { const b = new Uint8Array(mem.buffer); let e = p; while (b[e]) e++; return dec.decode(b.subarray(p, e)); };
  const env = {
    js_tus: (pin, vt) => (vt - cagriVt > 7000 ? 0 : tus[pin] ? 1 : 0),   // bir çağrı içinde 7 sn'den uzun bekleyen döngü kilitlenmesin: tuş bırakılmış sayılır
    js_kare: (p, vt) => h.kare(bayt(p, 8192).slice(), vt),
    js_parlak: v => h.parlak(v),
    js_ters: v => h.ters && h.ters(v),
    js_bildir: (p, n) => h.bildir(dec.decode(bayt(p, n))),
    js_log: (p, n) => h.log(dec.decode(bayt(p, n))),
    js_olay: t => { olay = t; },
    js_kayit_al: (k, buf, max) => { const v = h.depo.get(cstr(k)); if (!v) return -1; if (!max) return v.length; const n = Math.min(v.length, max); bayt(buf, n).set(v.subarray(0, n)); return n; },
    js_kayit_yaz: (k, b, n) => h.depo.set(cstr(k), bayt(b, n).slice()),
    js_kayit_sil: k => h.depo.del(cstr(k)),
  };
  const wasi = {
    fd_write: (fd, iov, n, yazilan) => { const d = new DataView(mem.buffer); let t = 0;
      for (let i = 0; i < n; i++) { const p = d.getUint32(iov + i * 8, true), l = d.getUint32(iov + i * 8 + 4, true); h.log(dec.decode(bayt(p, l))); t += l; }
      d.setUint32(yazilan, t, true); return 0; },
    fd_close: () => 0, fd_seek: () => 0,
  };
  const cagir = f => {                                 // 0: normal, 1: derin uykuya girdi, 2: yeniden başladı
    cagriVt = ex.sim_vt(); olay = 0;
    try { f(); } catch (e) { if (olay) return olay; throw e; }
    return 0;
  };
  const m = {
    async ac(uyandi) {
      ex = (await WebAssembly.instantiate(modul, { env, wasi_snapshot_preview1: wasi })).exports;
      mem = ex.memory; ex._initialize();
      return cagir(() => ex.sim_setup(uyandi ? 1 : 0, (Math.random() * 4294967295) >>> 0));
    },
    loop: () => cagir(ex.sim_loop),
    vt: () => ex.sim_vt(),
    tus: (pin, b) => { tus[pin] = b; },
    bagla: b => cagir(() => ex.sim_bagla(b ? 1 : 0)),
    gonder(satir) { const b = enc.encode(satir).slice(0, 2000); bayt(ex.sim_tampon(), b.length).set(b); return cagir(() => ex.sim_gonder(b.length)); },
    oyunlar: () => Array.from({ length: ex.sim_oyun_sayisi() }, (_, i) => cstr(ex.sim_oyun_adi(i))),
    oyun: i => cagir(() => ex.sim_oyun(i)),
    git: k => cagir(() => ex.sim_git(k)),
    ozel: tur => cagir(() => ex.sim_ozel(tur.charCodeAt(0))),
    qa(a, b) { if (!ex.sim_qa) throw new Error('Once derle.py --qa'); let sonuc; cagir(() => { sonuc = ex.sim_qa(a, b); }); return sonuc; },
  };
  return m;
}
if (typeof module !== 'undefined') module.exports = kubitMotor;
