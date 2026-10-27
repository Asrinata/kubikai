import asyncio
import base64
import json
import pathlib
import sys
import types
import unittest
from unittest.mock import patch
import queue
from kubit_ekran import KareTopla, packbits_ac, BluetoothAlici, SERVIS, EKRAN

def paketler(raw, no=5):
    # Bagimsiz, yalniz literal blok kullanan PackBits kodlayici.
    pb=b''.join(bytes([len(raw[i:i+128])-1])+raw[i:i+128] for i in range(0,len(raw),128))
    b=base64.b64encode(pb); parca=[b[i:i+160] for i in range(0,len(b),160)]
    return [f'EKR:{no}/{i}/{len(parca)}:'.encode()+p for i,p in enumerate(parca)]

class PaketTest(unittest.TestCase):
    def test_gercek_firmware(self):
        fixture=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else None
        if not fixture or not fixture.exists(): self.skipTest('Firmware fixture verilmedi')
        v=json.loads(fixture.read_text()); k=KareTopla(); out=None
        for p in v['paket']: out=k.besle(p.encode()) or out
        self.assertEqual(out,bytes.fromhex(v['beklenen']))
    def test_literal_karisik_kare(self):
        raw=bytes(range(256))*4; k=KareTopla(); out=None
        for p in paketler(raw): out=k.besle(p)
        self.assertEqual(out,raw)
    def test_sirasi_degisen_parcalar(self):
        raw=bytes(range(256))*4; k=KareTopla(); out=None
        for p in reversed(paketler(raw)): out=k.besle(p)
        self.assertEqual(out,raw)
    def test_eksik_kare(self):
        k=KareTopla()
        for p in paketler(bytes(1024))[:-1]: self.assertIsNone(k.besle(p))
    def test_sonra_gelen_eski_kare(self):
        raw=bytes(range(256))*4; eski=paketler(raw,10); yeni=paketler(raw[::-1],11); k=KareTopla()
        k.besle(eski[0],0); k.besle(yeni[0],.1); self.assertIsNone(k.besle(eski[1],.2))
        out=None
        for p in yeni[1:]: out=k.besle(p,.3)
        self.assertEqual(out,raw[::-1])
    def test_numara_sarmasi(self):
        k=KareTopla()
        for no in [255,0]:
            out=None
            for p in paketler(bytes(1024),no): out=k.besle(p)
            self.assertEqual(out,bytes(1024))
    def test_bozuk_basliklar(self):
        k=KareTopla()
        for p in [b'EKR:1/0/999:AAAA',b'EKR:1/-1/2:AAAA',b'EKR:1/2/2:AAAA',b'EKR:a/b/c:AAAA',b'EKR:1/0/1:!!',b'EKR:300/0/1:AAAA']:
            self.assertIsNone(k.besle(p))
    def test_tekrar_acma_ve_tasma(self):
        self.assertEqual(packbits_ac(bytes([129,0])*8),bytes(1024))
        self.assertIsNone(packbits_ac(bytes([129,0])*9))
        self.assertIsNone(packbits_ac(bytes([127])+b'kisa'))
        self.assertIsNone(packbits_ac(bytes([129])))
    def test_baglanmadan_yazma_yok(self):
        """Ekran yolu sadece notify okur; yazma/port acma gerektirmez."""
        kareler=queue.Queue(2); olaylar=queue.Queue(16); a=BluetoothAlici(kareler,olaylar)
        dev=types.SimpleNamespace(address='test-adres',name='Kubit')
        class Tarayici:
            @staticmethod
            async def discover(**kw): return {'test':(dev,types.SimpleNamespace(service_uuids=[SERVIS]))}
        class Istemci:
            def __init__(self,*args,**kwargs): self.services=types.SimpleNamespace(get_characteristic=lambda u:u==EKRAN); self.is_connected=True
            async def __aenter__(self): return self
            async def __aexit__(self,*args): pass
            async def start_notify(self,uuid,cb):
                for p in paketler(bytes(1024)): cb(None,p)
                a.dur.set()
        with patch.dict(sys.modules,{'bleak':types.SimpleNamespace(BleakScanner=Tarayici,BleakClient=Istemci)}): asyncio.run(a._dongu())
        self.assertEqual(kareler.get_nowait(),bytes(1024))
    def test_coklu_cihaz_icin_secim_gerekir(self):
        a=BluetoothAlici(queue.Queue(2),queue.Queue(16))
        class Tarayici:
            @staticmethod
            async def discover(**kw):
                return {str(i):(types.SimpleNamespace(address=str(i),name='Kubit'),types.SimpleNamespace(service_uuids=[SERVIS])) for i in range(2)}
        def baglanmak_yasak(*args,**kwargs): raise AssertionError('Birden fazla cihazda kendiliginden baglandi')
        async def bekle(_): a.dur.set()
        a._bekle=bekle
        with patch.dict(sys.modules,{'bleak':types.SimpleNamespace(BleakScanner=Tarayici,BleakClient=baglanmak_yasak)}): asyncio.run(a._dongu())
        self.assertIsNone(a.hedef)
    def test_izleyici_gizli_pencerede_acilir(self):
        import tkinter as tk
        from kubit_ekran import pencere
        gercek=tk.Tk
        def gizli():
            r=gercek(); r.withdraw(); r.after(150,r.destroy); return r
        with patch.object(tk,'Tk',gizli),patch.object(BluetoothAlici,'baslat',lambda self:None): pencere()

if __name__=='__main__':
    unittest.main(argv=[sys.argv[0]],verbosity=2)
