# Kübit PC ajanı: telefondaki Kübik uygulaması (Kübit'in köprüsü) ev ağında bu programı bulur.
#  - Hata raporları: Kübit çökünce rapor buraya gelir, masaüstündeki "Kübit Hata Raporları" klasörüne yazılır; adresler
#    o sürümün .elf dosyasıyla fonksiyon/satıra çevrilir (KubitAnahtarlik\build-yayin\elf_arsiv\kubit_s<sürüm>.elf).
#  - Kumanda: ses, medya, kilitle, uyut, ekran görüntüsü, sunum ileri/geri; CPU/RAM durumu. PIN ister (ayar.json).
#  - Ekran paylaşımı: Kübit'te Bilgisayar > Ekrani paylas açılınca kareler telefon üzerinden UDP 47823'e gelir, pencere kendiliğinden açılır.
# Yalnız stdlib. Açılışta kendiliğinden başlatmak için: KubitPC-Kur.bat
import ctypes, json, os, random, re, socket, subprocess, sys, threading, time
from ctypes import wintypes
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

BURA = os.path.dirname(os.path.abspath(__file__))
if sys.stderr is None:                                # pyw (penceresiz): hatalar kaybolmasın, ajan.log'a yazılsın
    sys.stdout = sys.stderr = open(os.path.join(BURA, 'ajan.log'), 'a', encoding='utf-8', buffering=1)
    print('\n=== basladi', time.strftime('%d.%m.%Y %H:%M:%S'))
AYAR = os.path.join(BURA, 'ayar.json')
HTTP_PORT, KESIF_PORT = 47821, 47822
PROJE = os.path.join(os.path.dirname(BURA), 'KubitAnahtarlik')
ADDR2LINE = os.path.expandvars(r'%LOCALAPPDATA%\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-addr2line.exe')
KURU = os.environ.get('KUBITPC_KURU') == '1'           # test: komutları uygulamaz, sadece yazar

def ayar():
    if os.path.exists(AYAR):
        return json.load(open(AYAR, encoding='utf-8'))
    a = {'pin': '%04d' % random.randint(0, 9999), 'ad': socket.gethostname()}
    json.dump(a, open(AYAR, 'w', encoding='utf-8'), indent=1)
    return a
A = ayar()

def masaustu():
    yol = ctypes.create_unicode_buffer(260)
    ctypes.windll.shell32.SHGetFolderPathW(None, 0x10, None, 0, yol)   # CSIDL_DESKTOPDIRECTORY (OneDrive'a taşınmış olsa da doğru yer)
    return yol.value or os.path.join(os.path.expanduser('~'), 'Desktop')
KLASOR = os.path.join(masaustu(), 'Kübit Hata Raporları')

def bildirim(baslik, metin):                          # Windows bildirimi (modülsüz WinRT)
    ps = ('[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null;'
          '$x = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent([Windows.UI.Notifications.ToastTemplateType]::ToastText02);'
          '$t = $x.GetElementsByTagName("text"); $t.Item(0).AppendChild($x.CreateTextNode("%s")) > $null; $t.Item(1).AppendChild($x.CreateTextNode("%s")) > $null;'
          '[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier("Kübit").Show([Windows.UI.Notifications.ToastNotification]::new($x))'
          % (baslik.replace('"', "'"), metin.replace('"', "'")))
    subprocess.Popen(['powershell', '-NoProfile', '-WindowStyle', 'Hidden', '-Command', ps], creationflags=0x08000000)

# ---------------------------------------------------------------- hata raporları
def coz(rapor):
    """Rapordaki PC ve geri izleme adreslerini fonksiyon:satır'a çevirir (eşleşen sürümün .elf'i varsa)."""
    m = re.search(r'Surum: (\d+)', rapor)
    adresler = re.findall(r'0x4[0-9a-fA-F]{7}', rapor)
    if not m or not adresler or not os.path.exists(ADDR2LINE): return ''
    elf = os.path.join(PROJE, 'build-yayin', 'elf_arsiv', 'kubit_s%s.elf' % m.group(1))
    if not os.path.exists(elf): return '\n(Bu sürümün .elf dosyası yok: %s, adresler çözülemedi)\n' % elf
    try:
        r = subprocess.run([ADDR2LINE, '-pfiaC', '-e', elf] + adresler, capture_output=True, text=True, timeout=20, creationflags=0x08000000)
        satirlar = [s.replace(PROJE + os.sep, '') for s in r.stdout.splitlines() if s.strip()]
        return '\nNerede çöktü (en üstteki çökme noktası):\n' + '\n'.join('  ' + s for s in satirlar) + '\n'
    except Exception as e:
        return '\n(adres çözme hatası: %s)\n' % e

son_hata = [0.0]
def hata_kaydet(govde):
    if time.time() - son_hata[0] < 2: return False    # dakikada en çok ~30 rapor
    son_hata[0] = time.time()
    os.makedirs(KLASOR, exist_ok=True)
    neden = re.search(r'Neden: (\w+)', govde)
    ad = time.strftime('%Y-%m-%d_%H-%M-%S') + '_' + (neden.group(1) if neden else 'HATA') + '.txt'
    metin = ('Kübit hata raporu\nPC zamanı: %s\n\n' % time.strftime('%d.%m.%Y %H:%M:%S')) + govde.strip() + '\n' + coz(govde)
    metin += ('\nNe demek?\n  PANIC: program geçersiz bir işlem yaptı (çoğu zaman boş işaretçi ya da dizi taşması).\n'
              '  WATCHDOG: bir görev çok uzun süre takıldı.\n  DONMA: ana döngü 20 sn ilerlemedi, bekçi yeniden başlattı.\n'
              '  BROWNOUT: pil/gerilim düştü (çoğu zaman pil bitmek üzere).\n')
    open(os.path.join(KLASOR, ad), 'w', encoding='utf-8').write(metin)
    bildirim('Kübit hata raporu', (neden.group(1) if neden else 'Hata') + ' - masaüstündeki klasöre yazıldı')
    print('hata raporu yazıldı:', ad)
    return True

# ---------------------------------------------------------------- eşleşme: telefon ister, PC'de "izin ver?" penceresi çıkar, evetse PIN telefona gider
user32 = ctypes.windll.user32
eslesme_kilidi = threading.Lock()
def eslesme_sor(telefon):
    if not eslesme_kilidi.acquire(blocking=False): return False      # aynı anda tek pencere
    try:
        if KURU: print('eslesme istegi:', telefon); return True
        # MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SETFOREGROUND
        return user32.MessageBoxW(None, '"%s" telefonu Kübit üzerinden bu bilgisayarı kumanda etmek istiyor.\n\nİzin verilsin mi?' % telefon[:40],
                                  'Kübit eşleşme', 0x4 | 0x20 | 0x40000 | 0x10000) == 6
    finally: eslesme_kilidi.release()

# ---------------------------------------------------------------- kumanda
def tus(*vk):
    if KURU: print('tus', vk); return
    for v in vk: user32.keybd_event(v, 0, 0, 0)
    for v in reversed(vk): user32.keybd_event(v, 0, 2, 0)
KOMUT = {
    'ses+': lambda: tus(0xAF), 'ses-': lambda: tus(0xAE), 'sessiz': lambda: tus(0xAD),
    'oynat': lambda: tus(0xB3), 'sonraki': lambda: tus(0xB0), 'onceki': lambda: tus(0xB1),
    'kilitle': lambda: KURU or user32.LockWorkStation(),
    'ekran': lambda: tus(0x5B, 0x2C),                  # Win+PrtScn: Resimler\Ekran Görüntüleri
    'uyku': lambda: KURU or subprocess.Popen(['rundll32.exe', 'powrprof.dll,SetSuspendState', '0,1,0']),
    'sunum_ileri': lambda: tus(0x22), 'sunum_geri': lambda: tus(0x21),   # PageDown / PageUp: PowerPoint, Slaytlar, PDF
}

class FILETIME(ctypes.Structure): _fields_ = [('lo', wintypes.DWORD), ('hi', wintypes.DWORD)]
class MEMSTAT(ctypes.Structure):
    _fields_ = [('len', wintypes.DWORD), ('yuk', wintypes.DWORD)] + [(n, ctypes.c_ulonglong) for n in ('tf', 'af', 'tp', 'ap', 'tv', 'av', 'ae')]
onceki = [None]
def cpu():
    bos, cek, kul = FILETIME(), FILETIME(), FILETIME()
    ctypes.windll.kernel32.GetSystemTimes(ctypes.byref(bos), ctypes.byref(cek), ctypes.byref(kul))
    t = [(f.hi << 32) | f.lo for f in (bos, cek, kul)]
    p, onceki[0] = onceki[0], t
    if not p: return 0
    toplam = (t[1] - p[1]) + (t[2] - p[2])
    return int(100 * (toplam - (t[0] - p[0])) / toplam) if toplam else 0
def ram():
    m = MEMSTAT(); m.len = ctypes.sizeof(MEMSTAT); ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)); return int(m.yuk)

# ---------------------------------------------------------------- sıcaklık, GPU, FPS (Kübit'in "Canlı monitör"ü)
# CPU sıcaklığı: SicaklikUyari'nın SYSTEM servisi (ProgramData\PCSicaklik\cpu.json; HVCI yüzünden yönetici şart, bu ajan normal kullanıcı)
# GPU: nvidia-smi. FPS: FPS_KUR.bat'ın kurduğu PresentMon servisi (ProgramData\KubitPC\fps.json). Yoksa -1.
def _json_taze(yol, sn):
    try:
        if time.time() - os.path.getmtime(yol) > sn: return None
        return json.load(open(yol, encoding='utf-8-sig'))   # PowerShell 5.1 BOM'lu yazar
    except Exception: return None
_gpu = {'t': 0, 'v': (-1, -1)}
def gpu():
    if time.time() - _gpu['t'] > 2:
        _gpu['t'] = time.time()
        try:
            r = subprocess.run(['nvidia-smi', '--query-gpu=temperature.gpu,utilization.gpu', '--format=csv,noheader,nounits'], capture_output=True, text=True, timeout=3, creationflags=0x08000000)
            c, y = r.stdout.splitlines()[0].split(','); _gpu['v'] = (int(c), int(y))
        except Exception: _gpu['v'] = (-1, -1)
    return _gpu['v']
def sensorler():
    c = _json_taze(r'C:\ProgramData\PCSicaklik\cpu.json', 30); f = _json_taze(r'C:\ProgramData\KubitPC\fps.json', 3)
    gc, gy = gpu()
    return {'cpu_c': round(c['cpu']) if c and c.get('cpu') else -1, 'gpu_c': gc, 'gpu': gy,
            'fps': round(f['fps']) if f and f.get('fps') else -1, 'fps_uyg': (f or {}).get('uygulama', '')}

# ---------------------------------------------------------------- yakınlık kilidi: Kübit yayınını dinle, uzaklaşınca Windows'u kilitle
# Kübit (Bilgisayar > Yakinlik kilidi) açıkken telefona bağlıyken de yayın yapar. Sinyal 10 sn boyunca eşiğin altında kalırsa ya da 20 sn
# hiç görülmezse kilitler. Kübit uyurken/kapanırken telefon "yakinlik_uyku" der: tekrar görülene kadar kilitlemez. Gereken paket: bleak.
KUBIT_UUID = '8f1d0001-6b75-4269-7421-4b7562697400'
YK = {'acik': bool(A.get('yakinlik')), 'son': 0.0, 'rssi': None, 'zayif': None, 'kilitledi': False, 'uyku': False}
def yakinlik_ayarla(acik):
    YK['acik'] = acik; YK['son'] = 0.0; YK['zayif'] = None; YK['kilitledi'] = False; YK['uyku'] = False   # Kübit bir kez görülmeden saymaya başlama
    A['yakinlik'] = acik; json.dump(A, open(AYAR, 'w', encoding='utf-8'), indent=1)
    print('yakinlik kilidi:', 'acik' if acik else 'kapali')
def yakinlik_dongusu():
    try:
        import asyncio
        from bleak import BleakScanner
    except ImportError:
        print('yakinlik kilidi icin bleak gerekli: py -m pip install bleak'); return
    esik = A.get('yakinlik_esik', -85)
    def gordum(cihaz, ilan):
        if KUBIT_UUID not in [u.lower() for u in (ilan.service_uuids or [])] and (ilan.local_name or '') != 'Kubit': return
        YK['son'] = time.time(); YK['uyku'] = False
        YK['rssi'] = ilan.rssi if YK['rssi'] is None else YK['rssi'] * 0.7 + ilan.rssi * 0.3   # yumuşat
        if YK['rssi'] > esik + 5: YK['kilitledi'] = False                                  # geri döndü: bir sonraki uzaklaşmada yine kilitle
    async def tara():
        async with BleakScanner(gordum):
            while True:
                await asyncio.sleep(1)
                if not YK['acik'] or YK['uyku'] or YK['kilitledi'] or not YK['son']: YK['zayif'] = None; continue
                simdi = time.time(); zayif = YK['rssi'] is not None and YK['rssi'] < esik
                YK['zayif'] = (YK['zayif'] or simdi) if zayif else None
                if simdi - YK['son'] > 20 or (YK['zayif'] and simdi - YK['zayif'] > 10):
                    print('yakinlik: Kubit uzaklasti (rssi %s), kilitleniyor' % (round(YK['rssi']) if YK['rssi'] is not None else '-'))
                    YK['kilitledi'] = True; YK['rssi'] = None
                    if not KURU: user32.LockWorkStation()
    while True:
        try: asyncio.run(tara())
        except Exception as e: print('yakinlik taramasi:', e); time.sleep(10)
KOMUT.update({'yakinlik_ac': lambda: yakinlik_ayarla(True), 'yakinlik_kapat': lambda: yakinlik_ayarla(False),
              'yakinlik_uyku': lambda: YK.update(uyku=True)})

class Sunucu(BaseHTTPRequestHandler):
    def yanit(self, kod, veri):
        b = json.dumps(veri, ensure_ascii=False).encode('utf-8')
        self.send_response(kod); self.send_header('Content-Type', 'application/json'); self.send_header('Content-Length', str(len(b))); self.end_headers(); self.wfile.write(b)
    def pin_tamam(self, q): return q.get('pin', [''])[0] == A['pin']
    def do_GET(self):
        u = urlparse(self.path); q = parse_qs(u.query)
        if self.client_address[0] not in gorulen: gorulen.add(self.client_address[0]); print('istek geldi:', self.client_address[0], u.path)
        if u.path == '/durum':
            if not self.pin_tamam(q): return self.yanit(403, {'hata': 'PIN'})
            return self.yanit(200, dict({'ad': A.get('ad', socket.gethostname()), 'cpu': cpu(), 'ram': ram()}, **sensorler()))
        self.yanit(404, {})
    def do_POST(self):
        u = urlparse(self.path); q = parse_qs(u.query)
        n = min(int(self.headers.get('Content-Length', 0)), 8192)
        govde = self.rfile.read(n).decode('utf-8', 'replace')
        if u.path == '/eslestir':
            if eslesme_sor(q.get('ad', ['telefon'])[0]): return self.yanit(200, {'pin': A['pin'], 'ad': A.get('ad', socket.gethostname())})
            return self.yanit(403, {'hata': 'reddedildi'})
        if u.path == '/hata': return self.yanit(200 if hata_kaydet(govde) else 429, {'tamam': True})
        if u.path == '/komut':
            if not self.pin_tamam(q): return self.yanit(403, {'hata': 'PIN'})
            k = q.get('k', [''])[0].replace(' ', '+')   # URL'de + boşluğa dönüşebilir
            if k not in KOMUT: return self.yanit(400, {'hata': 'komut'})
            KOMUT[k](); print('komut:', k); return self.yanit(200, {'tamam': True})
        self.yanit(404, {})
    def log_message(self, *a): pass

# ---------------------------------------------------------------- Kübit ekranı penceresi: telefon kareleri UDP 47823'e yollar ("KBE1" + 1024 bayt, satır satır 1 bit)
EKRAN_PORT, OLCEK, ACIK, KOYU = 47823, 5, '#cfe0ff', '#000000'
gorulen = set()                                       # günlük için: hangi adreslerden kare geldi
def ekran_dinle(q):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(('', EKRAN_PORT))
    while True:
        veri, adres = s.recvfrom(2048)
        if adres[0] not in gorulen: gorulen.add(adres[0]); print('ekran karesi geldi:', adres[0], len(veri), 'bayt')
        if len(veri) == 1028 and veri[:4] == b'KBE1':
            if q.full():
                try: q.get_nowait()
                except Exception: pass
            q.put(veri[4:])

def pencere():
    import queue, tkinter as tk
    q = queue.Queue(maxsize=2)
    threading.Thread(target=ekran_dinle, args=(q,), daemon=True).start()
    kok = tk.Tk(); kok.withdraw(); kok.title('Kübit ekranı'); kok.configure(bg='#0b0c10'); kok.resizable(False, False)
    kucuk = tk.PhotoImage(width=128, height=64)
    resim = tk.Label(kok, bg='#0b0c10', bd=0); resim.pack(padx=14, pady=(14, 4))
    alt = tk.Label(kok, text='', fg='#8d93a5', bg='#0b0c10', font=('Segoe UI', 9)); alt.pack(pady=(0, 10))
    d = {'son': 0.0, 'kapali': False, 'ustte': False, 'kare': 0}
    def kapat(): d['kapali'] = True; kok.withdraw()      # akış durana kadar yeniden açılmaz
    def ustte(_=None): d['ustte'] = not d['ustte']; kok.attributes('-topmost', d['ustte'])
    kok.protocol('WM_DELETE_WINDOW', kapat); kok.bind('t', ustte)
    def ciz(kare):
        satirlar = []
        for y in range(64):
            r = kare[y * 16:(y + 1) * 16]
            satirlar.append('{' + ' '.join(ACIK if r[x >> 3] & (0x80 >> (x & 7)) else KOYU for x in range(128)) + '}')
        kucuk.put(' '.join(satirlar))
        buyuk = kucuk.zoom(OLCEK); resim.configure(image=buyuk); resim.image = buyuk
    def tik():
        simdi = time.time(); kare = None
        while not q.empty(): kare = q.get_nowait()      # en yenisini çiz
        if kare is not None:
            if simdi - d['son'] > 10: d['kapali'] = False   # yeni oturum: kapatılmış olsa da yeniden aç
            d['son'] = simdi; d['kare'] += 1
            if not d['kapali']:
                if kok.state() == 'withdrawn': kok.deiconify(); kok.lift()
                ciz(kare); alt.configure(text='canlı · T: hep üstte ' + ('açık' if d['ustte'] else 'kapalı'))
        elif d['son'] and simdi - d['son'] > 5 and kok.state() != 'withdrawn':
            alt.configure(text='bağlantı bekleniyor… (Kübit > Araçlar > Bilgisayar > Ekranı paylaş)')
        kok.after(30, tik)
    tik(); kok.mainloop()

def kesif():                                          # telefon "KUBITPC?" yayınlar, biz adımızla cevap veririz
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); s.bind(('', KESIF_PORT))
    while True:
        veri, adres = s.recvfrom(256)
        if veri.startswith(b'KUBITPC?'): s.sendto(('KUBITPC|%s|%d' % (A.get('ad', socket.gethostname()), HTTP_PORT)).encode(), adres)

if __name__ == '__main__':
    threading.Thread(target=kesif, daemon=True).start()
    print('Kübit PC ajanı hazır. Telefonda Kübit > Ayarlar > Bilgisayarı eşle (PIN: %s)\nHata raporları: %s' % (A['pin'], KLASOR))
    threading.Thread(target=ThreadingHTTPServer(('0.0.0.0', HTTP_PORT), Sunucu).serve_forever, daemon=True).start()
    threading.Thread(target=yakinlik_dongusu, daemon=True).start()
    pencere()                                         # Tk ana iş parçacığında çalışmalı
