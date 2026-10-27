# Kübit simülatörünü derler: gerçek KubitAnahtarlik.ino -> WebAssembly -> ../deneme.html (tek dosya, çift tıklayıp aç).
# python simulator/derle.py      (gerekenler: Arduino kütüphaneleri, pip install ziglang)
import base64, os, re, shutil, subprocess, sys
BURA = os.path.dirname(os.path.abspath(__file__)); PROJE = os.path.dirname(BURA)
GFX = os.path.expanduser(r'~\Documents\Arduino\libraries\Adafruit_GFX_Library')
YAPI = os.path.join(BURA, 'build'); os.makedirs(YAPI, exist_ok=True)

# 1) Arduino'nun yaptığını taklit et: en başa Arduino.h, ilk fonksiyondan önce prototipler (varsayılan argümanlı olanlar hariç, Arduino da atlar).
#    arduino-cli --preprocess bu bilgisayarda dakikalarca bekleyebiliyor; bu adım anında biter.
kaynak = open(os.path.join(PROJE, 'KubitAnahtarlik.ino'), encoding='utf-8').read().split('\n')
TANIM = re.compile(r'^(?!(?:if|for|while|switch|return|else|struct|class|enum|namespace|typedef|extern)\b)'
                   r'([A-Za-z_][\w:<>,\s\*&]*?[\s\*&])([A-Za-z_]\w*)\s*\(([^()]*)\)\s*\{')
proto, ilk = [], None
for i, satir in enumerate(kaynak):
    m = TANIM.match(satir)
    if not m or '=' in m.group(3): continue
    if ilk is None: ilk = i
    proto.append('%s%s(%s);' % (m.group(1), m.group(2), m.group(3)))
kaynak[ilk:ilk] = proto + ['#line %d' % (ilk + 1)]
open(os.path.join(YAPI, 'KubitAnahtarlik.ino.cpp'), 'w', encoding='utf-8').write('#include <Arduino.h>\n#line 1 "KubitAnahtarlik.ino"\n' + '\n'.join(kaynak))
for f in os.listdir(YAPI):                            # eski başlıklar kalmasın
    if f.endswith('.h'): os.remove(os.path.join(YAPI, f))
for f in os.listdir(PROJE):                           # wifi_ayar.h ASLA: şifre simülatöre (ve html'e) girmesin
    if f.endswith('.h') and f != 'wifi_ayar.h': shutil.copy(os.path.join(PROJE, f), YAPI)

# 2) zig ile wasm32-wasi (reactor: main yok, JS setup/loop'u çağırır)
wasm = os.path.join(YAPI, 'kubit.wasm')
r = subprocess.run([sys.executable, '-m', 'ziglang', 'c++', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-Os', '-s', '-fno-exceptions',
                    '-std=gnu++17', '-w', *(['-DKUBIT_QA'] if '--qa' in sys.argv else []), '-I', os.path.join(BURA, 'stub'), '-I', GFX, os.path.join(BURA, 'sim.cpp'), '-o', wasm], capture_output=True, text=True)
if r.returncode: sys.exit('derleme hatasi:\n' + r.stderr[-6000:])

# 3) html'e göm
sablon = open(os.path.join(BURA, 'sablon.html'), encoding='utf-8').read()
surum = next(l for l in open(os.path.join(PROJE, 'KubitAnahtarlik.ino'), encoding='utf-8') if 'const int SURUM' in l).split('=')[1].split(';')[0].strip()
html = sablon.replace('__WASM__', base64.b64encode(open(wasm, 'rb').read()).decode()).replace('__SURUM__', surum).replace('__MOTOR__', open(os.path.join(BURA, 'motor.js'), encoding='utf-8').read())
open(os.path.join(PROJE, 'deneme.html'), 'w', encoding='utf-8').write(html)
print('tamam: deneme.html (%d KB wasm, surum %s)' % (os.path.getsize(wasm) // 1024, surum))

# 4) Kübik APK'sındaki telefon oynatıcısı (assets/kubit): motor.js + wasm (file:// fetch çalışmadığı için base64 betik)
APK = os.path.join(os.path.expanduser('~'), 'Desktop', 'chatgpt', 'Kubik', 'android', 'app', 'src', 'main', 'assets', 'kubit')
if '--apk' in sys.argv and os.path.isdir(APK):
    shutil.copy(os.path.join(BURA, 'motor.js'), APK)
    open(os.path.join(APK, 'wasm.js'), 'w').write('const KUBIT_WASM="' + base64.b64encode(open(wasm, 'rb').read()).decode() + '";\n')
    print('tamam: APK assets/kubit')
