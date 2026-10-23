"""Kübik masaüstü uygulaması: web sürümünü (GitHub Pages → Cloudflare → Pi) bir pencerede açar ve
isteğe bağlı olarak Kübik'e bu bilgisayarda iş yaptırır (PC erişimi).

PC erişimi modları (yetkiyi her zaman BU dosya denetler; web sayfası kendi başına yetki yükseltemez):
  kapali : hiçbir şey
  klasor : seçilen klasörde dosya listele/oku/yaz
  onay   : klasor + komut çalıştırma, her komut için Windows onay penceresi
  bypass : klasor + komut, onaysız
  tam    : her yer + komut + ekran/fare/klavye (computer use), ekranın etrafı kırmızı çerçeve
Yetkiyi artırmak her zaman Windows onay penceresiyle olur.
"""
import base64
import ctypes
import io
import json
import math
import os
import subprocess
import sys
import threading
from ctypes import wintypes
from pathlib import Path

import webview

VERSION = '1.0.2'
SITE = 'https://asrinata.github.io/kubikai/'
CONFIG = Path(os.environ.get('APPDATA', Path.home())) / 'KubikMasaustu' / 'ayar.json'
LEVEL = {'kapali': 0, 'klasor': 1, 'onay': 2, 'bypass': 3, 'tam': 4}
NAMES = {'kapali': 'Kapalı', 'klasor': 'Sadece klasör', 'onay': 'Klasör + onaylı komut', 'bypass': 'Bypass (onaysız)',
         'tam': 'Tam kontrol (ekran, fare, klavye)'}
user32 = ctypes.windll.user32


def ask(text):
    """Windows'un kendi onay penceresi (web sayfası bunu atlayamaz)."""
    MB_YESNO, MB_ICONWARNING, MB_TOPMOST, MB_SETFOREGROUND = 0x4, 0x30, 0x40000, 0x10000
    return user32.MessageBoxW(None, text, 'Kübik izin istiyor', MB_YESNO | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND) == 6


def load():
    try:
        value = json.loads(CONFIG.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        value = {}
    value.setdefault('mode', 'kapali')
    value.setdefault('folder', str(Path.home() / 'Documents' / 'Kübik'))
    if value['mode'] not in LEVEL:
        value['mode'] = 'kapali'
    return value


def save(value):
    CONFIG.parent.mkdir(parents=True, exist_ok=True)
    CONFIG.write_text(json.dumps(value, ensure_ascii=False, indent=1), encoding='utf-8')


# ---------- kırmızı çerçeve (tam kontrolde)
class Border:
    def __init__(self, on_stop):
        self.on_stop, self.root, self.thread = on_stop, None, None

    def show(self):
        if self.thread and self.thread.is_alive():
            return
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def hide(self):
        if self.root:
            self.root.after(0, self.root.destroy)

    def _run(self):
        import tkinter as tk
        root = self.root = tk.Tk()
        left, top = user32.GetSystemMetrics(76), user32.GetSystemMetrics(77)      # sanal ekran (tüm monitörler)
        width, height = user32.GetSystemMetrics(78), user32.GetSystemMetrics(79)
        if user32.GetSystemMetrics(80) == 1:                                     # tek ekran: görev çubuğunun altında kalmasın
            area = wintypes.RECT()
            user32.SystemParametersInfoW(0x30, 0, ctypes.byref(area), 0)          # SPI_GETWORKAREA
            left, top, width, height = area.left, area.top, area.right - area.left, area.bottom - area.top
        root.overrideredirect(True)
        root.attributes('-topmost', True)
        root.geometry(f'{width}x{height}+{left}+{top}')
        root.configure(bg='magenta')
        root.attributes('-transparentcolor', 'magenta')                         # boş alanlar tıklamayı geçirir
        canvas = tk.Canvas(root, bg='magenta', highlightthickness=0)
        canvas.pack(fill='both', expand=True)
        # parlayan çerçeve: dıştan içe açık kırmızıdan koyuya katmanlar, nefes alır gibi yanıp söner
        layers = [canvas.create_rectangle(i * 2 + 1, i * 2 + 1, width - i * 2 - 1, height - i * 2 - 1, width=2, outline='#e01b24')
                  for i in range(7)]
        corner = 46
        for x, y, dx, dy in ((0, 0, 1, 1), (width, 0, -1, 1), (0, height, 1, -1), (width, height, -1, -1)):   # köşe parantezleri
            canvas.create_line(x + dx * 16, y + dy * (16 + corner), x + dx * 16, y + dy * 16, x + dx * (16 + corner), y + dy * 16,
                               fill='#ffffff', width=4, capstyle='round', joinstyle='round')
        bar = tk.Frame(root, bg='#1b0b0e', highlightbackground='#e01b24', highlightthickness=2)
        bar.place(relx=.5, y=10, anchor='n')
        dot = tk.Label(bar, text='●', bg='#1b0b0e', fg='#ff3b47', font=('Segoe UI', 11))
        dot.pack(side='left', padx=(12, 4), pady=6)
        tk.Label(bar, text='Kübik bilgisayarını kullanıyor', bg='#1b0b0e', fg='white', font=('Segoe UI', 10, 'bold')).pack(side='left', pady=6)
        stop = tk.Label(bar, text='  DURDUR  ', bg='#e01b24', fg='white', font=('Segoe UI', 10, 'bold'), cursor='hand2')
        stop.pack(side='left', padx=(12, 6), pady=6)
        stop.bind('<Button-1>', lambda e: self.on_stop())
        stop.bind('<Enter>', lambda e: stop.configure(bg='#ff3b47'))
        stop.bind('<Leave>', lambda e: stop.configure(bg='#e01b24'))

        def pulse(step=0):
            glow = (math.sin(step / 9) + 1) / 2                                 # 0..1
            for index, layer in enumerate(layers):
                fade = index / len(layers)
                red = int(150 + 105 * glow * (1 - fade))
                canvas.itemconfigure(layer, outline=f'#{red:02x}{int(30 * (1 - fade) * glow):02x}{int(40 * (1 - fade) * glow):02x}')
            dot.configure(fg='#ff3b47' if step // 12 % 2 else '#7a1218')
            root.after(60, pulse, step + 1)
        pulse()
        root.mainloop()
        self.root = None


# ---------- fare / klavye (yalnız tam kontrolde)
class Input:
    KEYS = {'enter': 0x0D, 'tab': 0x09, 'esc': 0x1B, 'backspace': 0x08, 'delete': 0x2E, 'space': 0x20, 'up': 0x26,
            'down': 0x28, 'left': 0x25, 'right': 0x27, 'home': 0x24, 'end': 0x23, 'ctrl': 0x11, 'alt': 0x12,
            'shift': 0x10, 'win': 0x5B, **{f'f{n}': 0x6F + n for n in range(1, 13)}}

    @staticmethod
    def click(x, y, button='left'):
        user32.SetCursorPos(int(x), int(y))
        down, up = (0x0002, 0x0004) if button == 'left' else (0x0008, 0x0010)
        user32.mouse_event(down, 0, 0, 0, 0)
        user32.mouse_event(up, 0, 0, 0, 0)

    @staticmethod
    def type_text(text):
        class KEYBDINPUT(ctypes.Structure):
            _fields_ = [('wVk', wintypes.WORD), ('wScan', wintypes.WORD), ('dwFlags', wintypes.DWORD),
                        ('time', wintypes.DWORD), ('dwExtraInfo', ctypes.c_size_t)]

        class INPUT(ctypes.Structure):
            class _U(ctypes.Union):
                _fields_ = [('ki', KEYBDINPUT), ('pad', ctypes.c_byte * 32)]
            _anonymous_ = ('u',)
            _fields_ = [('type', wintypes.DWORD), ('u', _U)]
        for char in text:
            for flags in (0x0004, 0x0004 | 0x0002):                              # KEYEVENTF_UNICODE (+ KEYUP)
                event = INPUT(type=1)
                event.ki = KEYBDINPUT(0, ord(char), flags, 0, 0)
                user32.SendInput(1, ctypes.byref(event), ctypes.sizeof(INPUT))

    @classmethod
    def key(cls, combo):
        codes = [cls.KEYS.get(part.strip().lower()) or ord(part.strip().upper()[:1]) for part in combo.split('+') if part.strip()]
        for code in codes:
            user32.keybd_event(code, 0, 0, 0)
        for code in reversed(codes):
            user32.keybd_event(code, 0, 2, 0)


# ---------- web sayfasının çağırdığı köprü
class Api:
    def __init__(self):
        self._config = load()
        self._window = None
        self._scale = 1.0
        self._border = Border(lambda: self._set_mode('onay', confirm=False))
        if self._config['mode'] == 'tam':
            self._config['mode'] = 'onay'          # tam kontrol her açılışta yeniden onaylanır
            save(self._config)

    def pc_durum(self):
        folder = Path(self._config['folder'])
        return {'app': VERSION, 'mode': self._config['mode'], 'mode_name': NAMES[self._config['mode']],
                'folder': folder.name, 'folder_path': str(folder), 'modes': NAMES}

    def _set_mode(self, mode, confirm=True):
        if mode not in LEVEL:
            return self.pc_durum()
        if confirm and LEVEL[mode] > LEVEL[self._config['mode']]:
            warning = '\n\nTam kontrolde Kübik ekranını görebilir, fare ve klavyeyi kullanabilir. Ekranın etrafında kırmızı çerçeve olur; durdurmak için üstteki DURDUR yazısına tıkla.' if mode == 'tam' else ''
            privacy = ('\n\nKübik’in okuduğu dosyalar, komut çıktıları ve ekran görüntüsü açıklamaları sohbet mesajı olarak '
                       'Kübik sunucusuna gönderilir ve sohbet geçmişinde saklanır. Kişisel ya da gizli dosyaların olduğu klasörü seçme.')
            if not ask(f'Kübik’e şu yetki verilsin mi?\n\n{NAMES[mode]}\nKlasör: {self._config["folder"]}{privacy}{warning}'):
                return self.pc_durum()
        self._config['mode'] = mode
        save(self._config)
        (self._border.show if mode == 'tam' else self._border.hide)()
        if self._window and not confirm:
            self._window.evaluate_js('window.kubikPcChanged && window.kubikPcChanged()')
        return self.pc_durum()

    def pc_mod(self, mode):
        return self._set_mode(mode)

    def pc_klasor(self):
        picked = self._window.create_file_dialog(webview.FileDialog.FOLDER, directory=self._config['folder']) if self._window else None
        if picked:
            self._config['folder'] = picked[0] if isinstance(picked, (list, tuple)) else picked
            save(self._config)
        return self.pc_durum()

    def _path(self, relative):
        base = Path(self._config['folder'])
        base.mkdir(parents=True, exist_ok=True)
        target = (base / str(relative or '.')).resolve()
        if self._config['mode'] != 'tam' and not target.is_relative_to(base.resolve()):
            raise PermissionError('Bu yol seçili klasörün dışında.')
        return target

    def pc_islem(self, task):
        """Kübik'in istediği tek işlem. Yetki ve onay burada denetlenir."""
        try:
            mode, level, op = self._config['mode'], LEVEL[self._config['mode']], str(task.get('islem', ''))
            if level == 0:
                return {'ok': False, 'hata': 'PC erişimi kapalı.'}
            if op == 'listele':
                target = self._path(task.get('yol'))
                items = sorted(target.iterdir(), key=lambda p: (p.is_file(), p.name.lower()))[:200]
                return {'ok': True, 'sonuc': '\n'.join(('📁 ' if p.is_dir() else '') + p.name for p in items) or '(boş)'}
            if op == 'oku':
                target = self._path(task.get('yol'))
                return {'ok': True, 'sonuc': target.read_text(encoding='utf-8', errors='replace')[:20000]}
            if op == 'yaz':
                target = self._path(task.get('yol'))
                content = str(task.get('icerik', ''))
                if mode == 'onay' and not ask(f'Kübik bu dosyayı yazmak istiyor:\n\n{target}\n({len(content)} karakter)'):
                    return {'ok': False, 'hata': 'Kullanıcı reddetti.'}
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(content, encoding='utf-8')
                return {'ok': True, 'sonuc': f'{target.name} yazıldı.'}
            if op == 'calistir':
                if level < 2:
                    return {'ok': False, 'hata': 'Komut çalıştırma bu modda kapalı.'}
                command = str(task.get('komut', ''))[:2000]
                if mode == 'onay' and not ask(f'Kübik bu komutu çalıştırmak istiyor:\n\n{command}\n\nKlasör: {self._config["folder"]}'):
                    return {'ok': False, 'hata': 'Kullanıcı reddetti.'}
                result = subprocess.run(['powershell', '-NoProfile', '-Command', command], cwd=self._path('.'),
                                        capture_output=True, text=True, timeout=120, creationflags=0x08000000)
                return {'ok': result.returncode == 0, 'sonuc': (result.stdout + result.stderr)[-8000:] or '(çıktı yok)'}
            if op in ('ekran', 'tikla', 'klavye', 'tus'):
                if mode != 'tam':
                    return {'ok': False, 'hata': 'Ekran ve fare/klavye yalnız Tam kontrol modunda.'}
                if op == 'ekran':
                    from PIL import ImageGrab
                    image = ImageGrab.grab(all_screens=True)
                    self._scale = image.width / 1280 if image.width > 1280 else 1.0
                    image = image.convert('RGB').resize((int(image.width / self._scale), int(image.height / self._scale)))
                    buffer = io.BytesIO()
                    image.save(buffer, 'JPEG', quality=70)
                    return {'ok': True, 'sonuc': f'Ekran görüntüsü {image.width}x{image.height}', 'resim': base64.b64encode(buffer.getvalue()).decode()}
                if op == 'tikla':
                    Input.click(float(task['x']) * self._scale, float(task['y']) * self._scale, task.get('tus', 'left'))
                    return {'ok': True, 'sonuc': 'Tıklandı.'}
                if op == 'klavye':
                    Input.type_text(str(task.get('metin', ''))[:2000])
                    return {'ok': True, 'sonuc': 'Yazıldı.'}
                Input.key(str(task.get('tus', '')))
                return {'ok': True, 'sonuc': 'Tuşa basıldı.'}
            return {'ok': False, 'hata': f'Bilinmeyen işlem: {op}'}
        except Exception as error:
            return {'ok': False, 'hata': str(error)[:500]}


def main():
    api = Api()
    window = webview.create_window('Kübik', SITE, js_api=api, width=1280, height=820, min_size=(420, 600),
                                   background_color='#f8f7f3')
    api._window = window
    if api._config['mode'] == 'tam':
        api._border.show()
    webview.start(private_mode=False, storage_path=str(CONFIG.parent / 'web'))


if __name__ == '__main__':
    main()
