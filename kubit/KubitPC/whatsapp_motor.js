// PC'de tutulan WhatsApp Web oturumu. Yalniz acik cihaz onayindan gelen tekil komutu gonderir.
const fs=require('fs'), path=require('path'), crypto=require('crypto');
async function sinir(p,ms){let t;try{return await Promise.race([p,new Promise((_,reject)=>{t=setTimeout(()=>reject(Error('timeout')),ms)})]);}finally{clearTimeout(t)}}

class Gonderici {
  constructor(client, dosya, emit) { this.client=client; this.dosya=dosya; this.emit=emit; this.hazir=false; this.isler={};
    if(fs.existsSync(dosya))this.isler=JSON.parse(fs.readFileSync(dosya,'utf8')); }
  sakla(){ const tmp=this.dosya+'.tmp'; fs.writeFileSync(tmp,JSON.stringify(this.isler));fs.renameSync(tmp,this.dosya); }
  async gonder(k){
    const id=String(k.id||''), no=String(k.no||''), text=String(k.text||'');
    const done=(status,message)=>this.emit({type:'sonuc',id,status,message});
    if(!/^\d{1,10}$/.test(id)||id==='0'||!/^[1-9]\d{7,14}$/.test(no)||!text.trim()||text.length>120||/[\r\n]/.test(text))return done('ERR','Gecersiz numara veya mesaj');
    const oz=crypto.createHash('sha256').update(no+'\n'+text).digest('hex');
    if(this.isler[id]){
      const eski=this.isler[id];
      if(eski.oz!==oz)return done('ERR','Istek kimligi cakisti');
      return done(eski.status==='SENT'?'SENT':'UNKNOWN',eski.status==='SENT'?'WhatsApp a daha once iletildi':'Onceki sonuc belirsiz. Tekrar otomatik gonderilmedi');
    }
    if(!this.hazir)return done('ERR','PC WhatsApp oturumunu esle');
    if(Object.values(this.isler).some(x=>x.oz===oz&&x.status!=='SENT'&&Date.now()-x.t<120000))return done('UNKNOWN','Onceki ayni mesaj belirsiz. WhatsApp i kontrol et');
    try{
      const kisi=await sinir(this.client.getNumberId(no),10000);
      if(!kisi)return done('ERR','Bu numara WhatsApp ta bulunamadi');
      this.isler[id]={oz,status:'PENDING',t:Date.now()};this.sakla();
      // Kayit PENDING iken tekrar gelirse yeniden yollama; cift gonderimi onle.
      await sinir(this.client.sendMessage(kisi._serialized,text,{sendSeen:false}),20000);
      this.isler[id].status='SENT';this.sakla();done('SENT','WhatsApp a iletildi. Teslim bilgisi yok');
    }catch(e){
      if(this.isler[id]){this.isler[id].status='UNKNOWN';try{this.sakla()}catch{}done('UNKNOWN','Sonuc belirsiz. WhatsApp i kontrol et');}
      else done('ERR','WhatsApp baglantisi hazir degil');
    }
  }
}

async function main(){
  const {Client,LocalAuth}=require('whatsapp-web.js'), QR=require('qrcode');
  const veri=path.join(__dirname,'wa-oturum');fs.mkdirSync(veri,{recursive:true});
  const emit=x=>process.stdout.write(JSON.stringify(x)+'\n');
  const aday=[process.env.KUBIT_CHROME,'C:/Program Files/Google/Chrome/Application/chrome.exe','C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'].filter(Boolean);
  const exe=aday.find(p=>fs.existsSync(p));if(!exe)throw Error('Chrome veya Edge bulunamadi');
  const client=new Client({authStrategy:new LocalAuth({dataPath:veri,clientId:'kubit'}),puppeteer:{headless:true,executablePath:exe},takeoverOnConflict:false});
  const g=new Gonderici(client,path.join(veri,'istekler.json'),emit);
  client.on('qr',async qr=>{g.hazir=false;emit({type:'qr',png:(await QR.toDataURL(qr,{width:260,margin:2})).split(',')[1]});emit({type:'durum',ready:false,message:'WhatsApp > Bagli cihazlar ile QR kodu esle'});});
  client.on('ready',()=>{g.hazir=true;emit({type:'durum',ready:true,message:'WhatsApp hazir. Gonderimi Kubit ekraninda onayla.'});});
  client.on('disconnected',()=>{g.hazir=false;emit({type:'durum',ready:false,message:'WhatsApp oturumu kapandi; kopruyu yeniden ac'});});
  client.on('auth_failure',()=>emit({type:'durum',ready:false,message:'WhatsApp eslemesi yenilenmeli'}));
  const rl=require('readline').createInterface({input:process.stdin});
  let zincir=Promise.resolve();
  rl.on('line',line=>{if(line.length>1024)return;try{const k=JSON.parse(line);if(k.type==='send')zincir=zincir.then(()=>g.gonder(k)).catch(()=>emit({type:'durum',ready:false,message:'Istek islenemedi'}));}catch{}});
  rl.on('close',async()=>{await client.destroy();process.exit(0)});
  process.on('SIGINT',async()=>{await client.destroy();process.exit(0)});
  await client.initialize();
}
module.exports={Gonderici};
if(require.main===module)main().catch(()=>{process.stdout.write(JSON.stringify({type:'durum',ready:false,message:'WhatsApp baslatilamadi. Kurulumu ve Chrome/Edge i kontrol et.'})+'\n');process.exitCode=1});
