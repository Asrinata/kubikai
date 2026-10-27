// node simulator/arayuz_test.js <kare-klasoru> (once: python simulator/derle.py --qa)
const assert = require('assert'), fs = require('fs'), path = require('path'), motor = require('./motor.js');
(async () => {
  const hedef = process.argv[2]; if (hedef) fs.mkdirSync(hedef, { recursive: true });
  const depo = new Map(); let kare;
  const m = await motor(fs.readFileSync(path.join(__dirname, 'build', 'kubit.wasm')), {
    kare: p => { kare = p; }, parlak() {}, bildir() {}, log() {},
    depo: { get: k => depo.get(k), set: (k, v) => depo.set(k, v), del: k => depo.delete(k) },
  });
  await m.ac(false);
  const senaryolar = [
    [0, 0, 'radar-harita'], [0, 1, 'radar-rota'], [0, 2, 'radar-telemetri'], [0, 3, 'radar-bos'],
    [1, 0, 'iss-harita'], [1, 1, 'iss-koordinat'], [1, 2, 'iss-mesafe'], [1, 3, 'iss-eski-veri'],
    [2, 0, 'wifi-aglar'], [2, 1, 'wifi-bagli'], [2, 2, 'wifi-yanlis-sifre'], [2, 3, 'wifi-iptal'], [2, 4, 'wifi-klavye'],
    [3, 0, 'web-okuma'], [3, 1, 'web-linkler'], [3, 2, 'web-uzun-sayfa'], [3, 3, 'web-gecmis'],
    [4, 0, 'gecersiz-paket'], [5, 0, 'telefonsuz-kurulum'],
  ];
  for (const [a, b, ad] of senaryolar) {
    assert.equal(m.qa(a, b), 1, ad);
    if (hedef && kare) fs.writeFileSync(path.join(hedef, ad + '.pgm'), Buffer.concat([Buffer.from('P5\n128 64\n255\n'), Buffer.from(kare.map(v => v ? 255 : 0))]));
    console.log('OK ' + ad);
  }
  console.log(senaryolar.length + ' senaryo gecti. Ag taklidi kullanilir; fiziksel Wi-Fi ve BLE cihazda denenmelidir.');
})().catch(e => { console.error(e); process.exit(1); });
