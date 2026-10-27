"""Kubit <-> Bluetooth <-> yerel WhatsApp Web motoru. Gelen ag portu acmaz."""
import asyncio, base64, json, os, queue, shutil, subprocess, sys, threading, time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'_deps'))
from kubit_ekran import en_yeni, SERVIS
KOMUT='8f1d0005-6b75-4269-7421-4b7562697400'
SONUC='8f1d0006-6b75-4269-7421-4b7562697400'

def komut_coz(v):
    s=v.decode('ascii')
    if not s.startswith('WA1:'): raise ValueError('tur')
    k=s[4:].split('|',2)
    if len(k)!=3 or not k[0].isdigit() or not 0<int(k[0])<=4294967295 or not k[1].isdigit() or not 8<=len(k[1])<=15 or k[1][0]=='0' or not k[2].strip() or len(k[2])>120 or '\n' in k[2] or '\r' in k[2]: raise ValueError('veri')
    return {'type':'send','id':k[0],'no':k[1],'text':k[2]}

class Kopru:
    def __init__(self):
        self.olay=queue.Queue(32);self.ack=queue.Queue(32);self.dur=threading.Event();self.hazir=False;self.p=None
    def durum(self,s): en_yeni(self.olay,{'type':'durum','message':s})
    def baslat(self):
        exe=shutil.which('node')
        if not exe: self.durum('Node.js kurulu olmali');return
        self.p=subprocess.Popen([exe,str(Path(__file__).with_name('whatsapp_motor.js'))],cwd=Path(__file__).parent,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,text=True,encoding='utf-8',creationflags=0x08000000)
        threading.Thread(target=self.motor_oku,daemon=True).start();threading.Thread(target=lambda:asyncio.run(self.ble()),daemon=True).start()
    def motor_oku(self):
        try:
            for line in self.p.stdout:
                try:
                    j=json.loads(line)
                    if j.get('type')=='durum' and 'ready' in j:
                        self.hazir=bool(j['ready']);en_yeni(self.ack,'F0|'+('READY' if self.hazir else 'WAIT')+'|PC oturumu')
                    if j.get('type')=='sonuc':
                        en_yeni(self.ack,'F'+str(j['id'])+'|'+str(j['status'])+'|'+str(j['message']).replace('|',' ')[:100])
                    en_yeni(self.olay,j)
                except (ValueError,KeyError): pass
        finally:
            self.hazir=False;en_yeni(self.ack,'F0|WAIT|WhatsApp motoru durdu');self.durum('WhatsApp motoru durdu; kopruyu yeniden ac')
    async def ble(self):
        from bleak import BleakScanner,BleakClient
        while not self.dur.is_set():
            try:
                d=await BleakScanner.discover(timeout=5,service_uuids=[SERVIS],return_adv=True)
                aday=[dev for dev,adv in d.values() if SERVIS in [u.lower()for u in adv.service_uuids]]
                if len(aday)!=1:
                    self.durum('Kubit i uyandir. Birden fazla cihaz varsa digerini uyut.');await asyncio.sleep(3);continue
                async with BleakClient(aday[0],timeout=20,winrt={'use_cached_services':False})as c:
                    if not c.services.get_characteristic(KOMUT): self.durum('Kubit e s25 yazilimi yuklenmeli');await asyncio.sleep(5);continue
                    def al(_,v):
                        try:
                            k=komut_coz(bytes(v))
                            if not self.hazir: en_yeni(self.ack,'F'+k['id']+'|ERR|PC WhatsApp oturumunu esle');return
                            self.p.stdin.write(json.dumps(k,ensure_ascii=True)+'\n');self.p.stdin.flush()
                        except (ValueError,UnicodeError,BrokenPipeError): self.durum('Gonderim istegi islenemedi')
                    await c.start_notify(KOMUT,al)
                    await c.write_gatt_char(SONUC,('F0|'+('READY' if self.hazir else 'WAIT')+'|PC oturumu').encode(),response=True)
                    self.durum('Kubit Bluetooth bagli. Gonderim cihazdan onaylanir.')
                    while c.is_connected and not self.dur.is_set():
                        while not self.ack.empty(): await c.write_gatt_char(SONUC,self.ack.get_nowait().encode('ascii','replace'),response=True)
                        await asyncio.sleep(.1)
            except Exception: self.durum('Bluetooth yeniden baglaniyor');await asyncio.sleep(3)
    def kapat(self):
        self.dur.set()
        if self.p and self.p.stdin:
            try:self.p.stdin.close()
            except OSError:pass

def pencere():
    import tkinter as tk
    b=Kopru();r=tk.Tk();r.title('Kubit WhatsApp koprusu');r.configure(bg='#10141e')
    tk.Label(r,text='Kubit ile WhatsApp',font=('Segoe UI',20),fg='white',bg='#10141e').pack(padx=24,pady=16)
    qr=tk.Label(r,bg='#10141e');qr.pack(padx=24)
    t=tk.Label(r,text='Baslatiliyor...',font=('Segoe UI',11),wraplength=420,fg='#bfcee6',bg='#10141e');t.pack(padx=24,pady=16)
    tk.Label(r,text='PC acik kalir. Ilk esleme/oturum yenilemesi gerekir.\nGondermek icin Kubit ekranindaki onayi kullan.',wraplength=420,fg='#bfcee6',bg='#10141e').pack(padx=24,pady=12)
    tk.Label(r,text='Resmi olmayan WhatsApp Web koprusu. WhatsApp tarafindan desteklenmez; hesap kisitlamasi riski vardir.',wraplength=420,fg='#e7bc78',bg='#10141e').pack(padx=24,pady=(0,16))
    def tik():
        while not b.olay.empty():
            j=b.olay.get_nowait()
            if j.get('type')=='qr':
                im=tk.PhotoImage(data=j['png']);qr.configure(image=im);qr.image=im
            elif j.get('message'):t.configure(text=j['message'])
            if j.get('ready'):qr.configure(image='');qr.image=None
        r.after(100,tik)
    def kapat():b.kapat();r.destroy()
    r.protocol('WM_DELETE_WINDOW',kapat);b.baslat();tik();r.mainloop()
if __name__=='__main__':pencere()
