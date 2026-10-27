"""Kubit ekrani: dogrudan Bluetooth LE. Socket, HTTP, UDP veya firewall kurali acmaz.

Kubit-Ekran.bat ile baslat. Firmware s24+, paylasim cihaz ustunde acilmali.
"""
from __future__ import annotations
import asyncio
import base64
import binascii
import queue
import sys
import threading
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / '_deps'))
SERVIS = '8f1d0001-6b75-4269-7421-4b7562697400'
EKRAN = '8f1d0004-6b75-4269-7421-4b7562697400'


def packbits_ac(veri: bytes) -> bytes | None:
    """Yalniz tam 1024 bayt uretilen, tasma/eksigi olmayan kareleri kabul et."""
    out = bytearray(); i = 0
    while i < len(veri):
        n = veri[i]; i += 1
        if n == 128:
            continue
        if n < 128:
            adet = n + 1
            if i + adet > len(veri) or len(out) + adet > 1024:
                return None
            out.extend(veri[i:i + adet]); i += adet
        else:
            adet = 257 - n
            if i >= len(veri) or len(out) + adet > 1024:
                return None
            out.extend(bytes([veri[i]]) * adet); i += 1
    return bytes(out) if len(out) == 1024 else None


class KareTopla:
    def __init__(self):
        self.no = None; self.parcalar = []; self.bas = 0.0; self.son_no = None

    def besle(self, veri: bytes, simdi: float | None = None) -> bytes | None:
        simdi = time.monotonic() if simdi is None else simdi
        if not veri.startswith(b'EKR:') or len(veri) > 190:
            return None
        try:
            baslik, parca = veri[4:].split(b':', 1)
            no, i, toplam = map(int, baslik.split(b'/'))
            if not (0 <= no <= 255 and 1 <= toplam <= 9 and 0 <= i < toplam and 0 < len(parca) <= 160):
                return None
            # Yalniz base64 karakterleri; parcalarin kendisi dordun kati olmak zorunda degil.
            if any(c not in b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=' for c in parca):
                return None
            if no == self.son_no and simdi - self.bas < 2:
                return None
            if self.no is not None and no != self.no and simdi - self.bas < 2:
                fark = (no - self.no) & 255
                if fark > 128:  # gecikmis eski kare parcasi
                    return None
            if no != self.no or len(self.parcalar) != toplam or simdi - self.bas > 2:
                self.no = no; self.parcalar = [None] * toplam; self.bas = simdi
            if self.parcalar[i] is not None and self.parcalar[i] != parca:
                self.parcalar = []; self.no = None; return None
            self.parcalar[i] = parca
            if any(p is None for p in self.parcalar):
                return None
            sonuc = packbits_ac(base64.b64decode(b''.join(self.parcalar), validate=True))
            self.son_no = no; self.parcalar = []; self.no = None
            return sonuc
        except (ValueError, TypeError, binascii.Error):
            return None


def en_yeni(q: queue.Queue, deger):
    try:
        q.put_nowait(deger)
    except queue.Full:
        try: q.get_nowait()
        except queue.Empty: pass
        try: q.put_nowait(deger)
        except queue.Full: pass


class BluetoothAlici:
    def __init__(self, kareler: queue.Queue, olaylar: queue.Queue):
        self.kareler = kareler; self.olaylar = olaylar
        self.dur = threading.Event(); self.degistir = threading.Event()
        self.hedef = None

    def sec(self, adres: str):
        self.hedef = adres; self.degistir.set()

    def durum(self, metin):
        en_yeni(self.olaylar, ('durum', metin))

    def baslat(self):
        threading.Thread(target=self._calis, name='KubitEkranBLE', daemon=True).start()

    def _calis(self):
        try:
            asyncio.run(self._dongu())
        except ImportError:
            self.durum('Bluetooth destegi eksik. Kubit-Ekran.bat dosyasini ac.')
        except Exception as e:
            self.durum('Bluetooth baslatilamadi: ' + str(e)[:100])

    async def _bekle(self, saniye):
        for _ in range(int(saniye * 4)):
            if self.dur.is_set() or self.degistir.is_set(): return
            await asyncio.sleep(.25)

    async def _dongu(self):
        from bleak import BleakScanner, BleakClient
        while not self.dur.is_set():
            self.degistir.clear()
            try:
                self.durum('Kubit araniyor... Bluetooth acik ve Kubit uyanik olmali.')
                bulunan = await BleakScanner.discover(timeout=5, service_uuids=[SERVIS], return_adv=True)
                cihazlar = {dev.address: dev for dev, adv in bulunan.values() if SERVIS in [u.lower() for u in adv.service_uuids]}
                en_yeni(self.olaylar, ('cihazlar', [(d.address, d.name or 'Kubit') for d in cihazlar.values()]))
                if not cihazlar:
                    self.durum('Kubit bulunamadi. Uyandir; Bluetooth kapaliysa ac. Yeniden araniyor...')
                    await self._bekle(3); continue
                if self.hedef not in cihazlar:
                    if len(cihazlar) == 1: self.hedef = next(iter(cihazlar))
                    else:
                        self.durum('Birden fazla Kubit var. Listeden kendi cihazini secip Baglan de.')
                        await self._bekle(8); continue
                dev = cihazlar[self.hedef]; self.degistir.clear()
                self.durum('Baglaniyor: ' + (dev.name or 'Kubit'))
                async with BleakClient(dev, timeout=20, winrt={'use_cached_services': False}) as client:
                    if not client.services.get_characteristic(EKRAN):
                        self.durum('Bu Kubit eski yazilimda. Dogrudan ekran icin s24 veya ustunu yukle.')
                        await self._bekle(8); continue
                    toplayici = KareTopla(); son = [time.monotonic()]
                    def al(_kar, veri):
                        kare = toplayici.besle(bytes(veri))
                        if kare is not None:
                            son[0] = time.monotonic(); en_yeni(self.kareler, kare)
                    await client.start_notify(EKRAN, al)
                    self.durum('Bluetooth bagli. Kubit > Araclar > Bilgisayar > Ekrani paylas: acik')
                    while client.is_connected and not self.dur.is_set() and not self.degistir.is_set():
                        await asyncio.sleep(.25)
                        if time.monotonic() - son[0] > 8:
                            self.durum('Kare bekleniyor. Kubit ekraninda Ekrani paylas secenegini ac.')
                            son[0] = time.monotonic()
            except Exception as e:
                self.durum('Baglanti yenileniyor: ' + str(e)[:100])
                await self._bekle(3)


KONTROLLER = '''KUBITCRAFT
Sol/sag basili: yuru
Ayni tusa cift tik: blok kir / dusmana vur
Tek tik: o yone secili blok koy
Iki tus kisa: zipla / suda yuz
Iki tus 0.35-1 saniye: ayaginin altini kaz
Iki tus 1 saniye: oyun menusu
Menude sag: gez; sol veya iki tus kisa: sec
Envanter: esyayi sec; uretim: + isareti hazir tarif
Kaydet ve cik: dunyayi sakla

IZLEYICI
+ / -: buyut / kucult
F11: tam ekran; Esc: tam ekrandan cik
T: hep ustte
PNG kaydet: o anki kareyi kaydet
Bu pencere yalniz goruntu alir; cihazdaki paylasim kapatilinca akis durur.
'''


def pencere():
    import tkinter as tk
    from tkinter import ttk, filedialog
    kareler = queue.Queue(maxsize=2); olaylar = queue.Queue(maxsize=16)
    alici = BluetoothAlici(kareler, olaylar)
    kok = tk.Tk(); kok.title('Kubit Ekran — dogrudan Bluetooth'); kok.configure(bg='#10141e')
    bar = tk.Frame(kok, bg='#10141e'); bar.pack(fill='x', padx=12, pady=10)
    cihaz = ttk.Combobox(bar, state='readonly', width=32); cihaz.pack(side='left', padx=(0,8))
    adresler = []
    def baglan():
        i = cihaz.current()
        if 0 <= i < len(adresler): alici.sec(adresler[i])
        else: alici.degistir.set()
    ttk.Button(bar, text='Ara / Baglan', command=baglan).pack(side='left')
    resim = tk.Label(kok, bg='#000000', bd=0); resim.pack(padx=12, pady=(0,8))
    alt = tk.Label(kok, text='Baslatiliyor...', font=('Segoe UI',10), fg='#b5c2d8', bg='#10141e', wraplength=640)
    alt.pack(padx=12, pady=(0,10))
    panel = tk.Frame(kok, bg='#10141e'); panel.pack(pady=(0,12))
    d = {'olcek':5, 'ustte':False, 'son':0.0, 'toplam':0, 't':time.monotonic(), 'kare':None}
    kucuk = tk.PhotoImage(width=128, height=64)
    def ciz(kare):
        if kare is None: return
        satirlar = ['{' + ' '.join('#cfe0ff' if kare[y*16+(x>>3)] & (0x80>>(x&7)) else '#000000' for x in range(128)) + '}' for y in range(64)]
        kucuk.put(' '.join(satirlar)); buyuk = kucuk.zoom(d['olcek']); resim.configure(image=buyuk); resim.image=buyuk
    def olcek(adim):
        d['olcek'] = max(2,min(12,d['olcek']+adim)); ciz(d['kare'])
    def kaydet():
        if d['kare'] is None: return
        yol = filedialog.asksaveasfilename(parent=kok, defaultextension='.png', filetypes=[('PNG','*.png')], initialfile='Kubit-ekran.png')
        if yol: kucuk.write(yol, format='png')
    def rehber():
        w=tk.Toplevel(kok); w.title('Kontroller'); t=tk.Text(w,width=65,height=25,font=('Consolas',11)); t.pack(); t.insert('1.0',KONTROLLER); t.configure(state='disabled')
    def ustte(_=None):
        d['ustte']=not d['ustte']; kok.attributes('-topmost',d['ustte'])
    def kapat(): alici.dur.set(); kok.destroy()
    ttk.Button(panel,text='−',command=lambda:olcek(-1)).pack(side='left',padx=3)
    ttk.Button(panel,text='+',command=lambda:olcek(1)).pack(side='left',padx=3)
    ttk.Button(panel,text='PNG kaydet',command=kaydet).pack(side='left',padx=3)
    ttk.Button(panel,text='Kontroller',command=rehber).pack(side='left',padx=3)
    ttk.Button(panel,text='Hep ustte',command=ustte).pack(side='left',padx=3)
    kok.bind('+',lambda _:olcek(1)); kok.bind('-',lambda _:olcek(-1)); kok.bind('t',ustte)
    kok.bind('<F11>',lambda _:kok.attributes('-fullscreen',not kok.attributes('-fullscreen')))
    kok.bind('<Escape>',lambda _:kok.attributes('-fullscreen',False)); kok.protocol('WM_DELETE_WINDOW',kapat)
    def tik():
        nonlocal adresler
        while not olaylar.empty():
            tur, veri=olaylar.get_nowait()
            if tur=='durum': alt.configure(text=veri)
            else:
                onceki=alici.hedef; adresler=[a for a,n in veri]; cihaz['values']=[n+' · '+a for a,n in veri]
                if onceki in adresler: cihaz.current(adresler.index(onceki))
                elif len(adresler)==1: cihaz.current(0)
        kare=None
        while not kareler.empty(): kare=kareler.get_nowait()
        if kare is not None:
            d['kare']=kare; d['son']=time.monotonic(); d['toplam']+=1; ciz(kare)
            alt.configure(text='Canli · dogrudan Bluetooth · Wi-Fi ve ag izni gerekmez')
        elif d['son'] and time.monotonic()-d['son']>4:
            alt.configure(text='Goruntu durdu. Kubit uyuyor, paylasim kapali veya baglanti kesildi.')
        kok.after(35,tik)
    ciz(bytes(1024)); alici.baslat(); tik(); kok.mainloop()


if __name__ == '__main__':
    pencere()
