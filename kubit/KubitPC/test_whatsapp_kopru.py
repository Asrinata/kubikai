import json, pathlib, sys, unittest
from unittest.mock import patch
from whatsapp_kopru import komut_coz,Kopru,pencere
class Test(unittest.TestCase):
    def test_firmware_paketi(self):
        p=pathlib.Path(sys.argv[1])
        v=komut_coz(json.loads(p.read_text())['komut'].encode())
        self.assertEqual(v['no'],'15550000000');self.assertEqual(v['text'],'Merhaba | Kubit testi')
    def test_bozuk_numara_ve_komut(self):
        for x in [b'WA1:0|15550000000|test',b'WA1:1|05550000000|test',b'WA1:1|15550000000|',b'WA1:1|15550000000|a\nb',b'WA1:1|15550000000|'+b'a'*121]:
            with self.assertRaises(ValueError):komut_coz(x)
    def test_gui_auth_baslatmadan(self):
        import tkinter as tk
        real=tk.Tk
        def gizli():
            r=real();r.withdraw();r.after(150,r.destroy);return r
        with patch.object(tk,'Tk',gizli),patch.object(Kopru,'baslat',lambda self:None):pencere()
if __name__=='__main__':unittest.main(argv=[sys.argv[0]],verbosity=2)
