// Gercek WhatsApp'a baglanmaz; sahte istemci ile cift gonderim ve hata akisini dener.
const assert=require('assert'),fs=require('fs'),path=require('path'),{Gonderici}=require('./whatsapp_motor.js');
(async()=>{
 const base=process.argv[2];if(!base)throw Error('Test klasoru gerekli');fs.mkdirSync(base,{recursive:true});
 let n=0;const yeni=(name,fail=false)=>{
   const events=[],file=path.join(base,name+'.json'); if(fs.existsSync(file))fs.unlinkSync(file);
   const c={getNumberId:async no=>no==='15550000001'?null:{_serialized:no+'@c.us'},sendMessage:async()=>{n++;if(fail)throw Error('disconnect');return{id:{_serialized:'fake'}}}};
   const g=new Gonderici(c,file,e=>events.push(e));return{g,c,events,file};};
 const k={id:'123',no:'15550000000',text:'Kubit test | yerel'};
 let t=yeni('normal');t.g.hazir=true;await t.g.gonder(k);assert.equal(n,1);assert.equal(t.events.at(-1).status,'SENT');
 await t.g.gonder(k);assert.equal(n,1);assert.equal(t.events.at(-1).status,'SENT');
 await t.g.gonder({...k,text:'Degisti'});assert.equal(n,1);assert.equal(t.events.at(-1).status,'ERR');
 let events=[];let tekrar=new Gonderici(t.c,t.file,e=>events.push(e));tekrar.hazir=true;await tekrar.gonder(k);assert.equal(n,1);assert.equal(events.at(-1).status,'SENT');
 t=yeni('notready');await t.g.gonder(k);assert.equal(n,1);assert.equal(t.events.at(-1).status,'ERR');
 t.g.hazir=true;await t.g.gonder({...k,no:'15550000001'});assert.equal(n,1);assert.equal(t.events.at(-1).status,'ERR');
 await t.g.gonder({...k,no:'05500000000'});assert.equal(n,1);assert.equal(t.events.at(-1).status,'ERR');
 t=yeni('belirsiz',true);t.g.hazir=true;await t.g.gonder(k);assert.equal(n,2);assert.equal(t.events.at(-1).status,'UNKNOWN');
 await t.g.gonder(k);await t.g.gonder({...k,id:'124'});assert.equal(n,2);assert.equal(t.events.at(-1).status,'UNKNOWN');
 console.log('9 WhatsApp motor senaryosu gecti. Gercek mesaj gonderilmedi.');
})().catch(e=>{console.error(e);process.exit(1)});
