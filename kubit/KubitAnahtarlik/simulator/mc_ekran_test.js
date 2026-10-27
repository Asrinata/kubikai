const assert=require('assert'), fs=require('fs'), path=require('path'), motor=require('./motor.js');
(async()=>{
  const out=process.argv[2]; if(out)fs.mkdirSync(out,{recursive:true});
  const depo=new Map(), bildirim=[]; let kare;
  const m=await motor(fs.readFileSync(path.join(__dirname,'build/kubit.wasm')),{
    kare:p=>kare=p,parlak(){},bildir:s=>bildirim.push(s),log(){},
    depo:{get:k=>depo.get(k),set:(k,v)=>depo.set(k,v),del:k=>depo.delete(k)}
  });
  await m.ac(false);
  const S=[
    [10,0,'mc-hayatta-kal'],[10,1,'mc-uretim'],[10,2,'mc-envanter'],
    [11,0,'mc-anakaya'],[11,1,'mc-kazma'],[12,0,'mc-kayit-donus'],[12,1,'mc-bozuk-kayit'],
    [13,0,'mc-yaratici'],[14,0,'mc-menu-basisi'],[15,0,'mc-bitis'],[16,0,'mc-eksik-malzeme'],
    [17,0,'mc-kontrol-yuru'],[17,1,'mc-kontrol-kaz'],[17,2,'mc-kontrol-insa'],[17,3,'mc-kontrol-menu'],[17,4,'mc-kontrol-hedef'],
    [17,5,'mc-gorevler'],[17,6,'mc-baslik'],[18,0,'mc-sifirlama-iptal'],
    [20,0,'ekran-telefonsuz'],[21,0,'ekran-pc-kopunca-telefon'],[22,0,'ekran-telefon-kopunca-pc'],
  ];
  for(const[a,b,ad]of S){
    assert.equal(m.qa(a,b),1,ad);
    if(out&&kare)fs.writeFileSync(path.join(out,ad+'.pgm'),Buffer.concat([Buffer.from('P5\n128 64\n255\n'),Buffer.from(kare.map(v=>v?255:0))]));
    console.log('OK '+ad);
  }
  const n=bildirim.length; assert.equal(m.qa(23,0),1); assert.equal(bildirim.length,n,'Paylasim kapaliyken paket gitmemeli');
  const paket=bildirim.filter(s=>s.startsWith('EKR:')); assert(paket.length>0,'Firmware gercek kare uretmeli');
  const beklenen=Buffer.alloc(1024);
  for(let y=0;y<64;y++)for(let x=0;x<128;x++)if((x+y)%3===0)beklenen[y*16+(x>>3)]|=0x80>>(x&7);
  if(out)fs.writeFileSync(path.join(out,'firmware-ekran.json'),JSON.stringify({paket,beklenen:beklenen.toString('hex')}));
  console.log((S.length+1)+' senaryo gecti; '+paket.length+' gercek firmware ekran parcasi uretildi.');
})().catch(e=>{console.error(e);process.exit(1)});
