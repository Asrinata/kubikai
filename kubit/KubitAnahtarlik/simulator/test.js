// Duman testi: her oyunu gerçek firmware'le 90 sanal saniye rastgele tuşlarla oynatır, sonra menülerde dolaşır. Çökme (wasm trap) olursa söyler.
// node simulator/test.js   (önce python simulator/derle.py)
const fs = require('fs'), path = require('path'), kubitMotor = require('./motor.js');
(async () => {
  const depo = new Map(); let kare = 0, bildirim = [];
  const m = await kubitMotor(fs.readFileSync(path.join(__dirname, 'build', 'kubit.wasm')), {
    kare: () => kare++, parlak() {}, bildir: s => bildirim.push(s), log() {},
    depo: { get: k => depo.get(k), set: (k, v) => depo.set(k, v), del: k => depo.delete(k) },
  });
  let tohum = 1; const rnd = n => { tohum = (tohum * 1103515245 + 12345) % 2147483648; return tohum % n; };
  const oyna = (sure, durdur) => {                      // rastgele bas/bırak; ara sıra iki tuş birden
    const bitis = m.vt() + sure; let sonraki = 0, ol = 0;
    while (m.vt() < bitis) {
      if (m.vt() >= sonraki) { const r = rnd(10); m.tus(25, r < 4 || r === 9); m.tus(26, (r >= 4 && r < 8) || r === 9); sonraki = m.vt() + 40 + rnd(600); }
      ol = m.loop(); if (ol) return ol;
      if (durdur && durdur()) return 0;
    }
    return 0;
  };
  let hata = 0;
  const oyunlar = (await m.ac(false), m.oyunlar());
  m.bagla(true); m.gonder('S54|Kubik 3.0|UYANIK|' + Math.floor(Date.now() / 1000));
  for (let i = 0; i < oyunlar.length; i++) {
    try {
      m.oyun(i); const k0 = kare; const ol = oyna(90000);
      console.log((ol ? 'olay ' + ol : 'ok  ') + '  ' + oyunlar[i] + '  (' + (kare - k0) + ' kare)');
      if (ol) await m.ac(ol === 1);
    } catch (e) { hata++; console.log('COKTU  ' + oyunlar[i] + ': ' + e.message); await m.ac(false); }
  }
  for (const [ad, k] of [['Flappy', 2], ['Geometry', 3], ['Racer', 4], ['Menu', 0], ['Ayarlar', 5]]) {
    try { m.git(k); const ol = oyna(120000); console.log((ol ? 'olay ' + ol : 'ok  ') + '  ' + ad); if (ol) await m.ac(ol === 1); }
    catch (e) { hata++; console.log('COKTU  ' + ad + ': ' + e.message); await m.ac(false); }
  }
  console.log('rekor/ayar anahtarlari:', [...depo.keys()].join(' '));
  console.log('telefona giden komutlar:', [...new Set(bildirim)].slice(0, 15).join(', '));
  if (hata) { console.log(hata + ' COKME'); process.exit(1); }
  console.log('hepsi tamam');
})();
