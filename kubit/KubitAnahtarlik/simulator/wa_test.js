const fs=require('fs'),path=require('path'),assert=require('assert'),motor=require('./motor.js');
(async()=>{
 const out=process.argv[2];if(out)fs.mkdirSync(out,{recursive:true});const depo=new Map(),sent=[];let kare;
 const m=await motor(fs.readFileSync(path.join(__dirname,'build/kubit.wasm')),{kare:p=>kare=p,parlak(){},bildir:s=>sent.push(s),log(){},depo:{get:k=>depo.get(k),set:(k,v)=>depo.set(k,v),del:k=>depo.delete(k)}});await m.ac(false);
 const s=[[30,0,'kisi-kayit'],[31,0,'kisi-numara-dogrulama'],[32,0,'pc-gonderim-sonucu'],[33,0,'telefon-taslagi'],[34,0,'tam-mesaj-inceleme-iptal'],[35,0,'belirsiz-sonuc-gec-ack'],[36,0,'klavye-tut-yaz'],[36,1,'klavye-tut-sil'],[37,0,'menu-uzun-geri'],[38,0,'kisiler'],[39,0,'mesaj-onizleme'],[39,3,'mesaj-onay']];
 for(const[a,b,ad]of s){assert.equal(m.qa(a,b),1,ad);if(out&&kare)fs.writeFileSync(path.join(out,ad+'.pgm'),Buffer.concat([Buffer.from('P5\n128 64\n255\n'),Buffer.from(kare.map(v=>v?255:0))]));console.log('OK '+ad)}
 const komut=sent.find(x=>x.startsWith('WA1:'));assert(komut&&komut.length<=180);assert(sent.some(x=>x.startsWith('WA:HAZIRLA:')));
 if(out)fs.writeFileSync(path.join(out,'wa-komut.json'),JSON.stringify({komut}));
 console.log('12 senaryo ve PC/telefon komutlari gecti. Disariya mesaj gonderilmedi.');
})().catch(e=>{console.error(e);process.exit(1)});
