# Hızlı derleme: Arduino'yu atlar, sadece Kübit kodunu derleyip bağlar (kütüphaneler ve çekirdek build-yayin'de hazır bekler).
#   python hizli.py            -> build-yayin/KubitAnahtarlik.ino.bin (yerel: wifi_ayar.h dahil)
#   python hizli.py --yayin    -> wifi_ayar.h OLMADAN (GitHub'a giden sürüm; yayinla.sh bunu kullanır)
#   python hizli.py --yukle    -> derle + USB'den COM4'e yükle
# Komutlar build-ayrinti.log'dan okunur. Kütüphane/çekirdek değişirse ya da log yoksa bir kere tam derleme yapar (~1.5 dk).
import os, re, shlex, shutil, subprocess, sys, time
BURA = os.path.dirname(os.path.abspath(__file__))
YAPI = os.path.join(BURA, 'build-yayin'); TASLAK = os.path.join(YAPI, 'sketch'); LOG = os.path.join(BURA, 'build-ayrinti.log')
CLI = os.path.expandvars(r'%LOCALAPPDATA%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
FQBN = 'esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200'
yayin, yukle = '--yayin' in sys.argv, '--yukle' in sys.argv
bas = time.time()

def kopyala(hedef):                                  # .ino ve başlıklar; yayında wifi_ayar.h ASLA
    os.makedirs(hedef, exist_ok=True)
    for f in os.listdir(hedef):
        if f.endswith('.h'): os.remove(os.path.join(hedef, f))
    for f in os.listdir(BURA):
        if f.endswith('.h') and not (yayin and f == 'wifi_ayar.h'): shutil.copy(os.path.join(BURA, f), hedef)

def tam_derleme():
    print('tam derleme (bir kere, ~1.5 dk)...')
    kaynak = os.path.join(BURA, 'build-yayin-src', 'KubitAnahtarlik'); kopyala(kaynak)
    shutil.copy(os.path.join(BURA, 'KubitAnahtarlik.ino'), kaynak)
    with open(LOG, 'w', encoding='utf-8') as f:
        if subprocess.run([CLI, 'compile', '-v', '--fqbn', FQBN, '--build-path', YAPI, kaynak], stdout=f, stderr=subprocess.STDOUT).returncode:
            sys.exit('DUR: tam derleme hatasi, ' + LOG)

def komut(desen):
    for satir in open(LOG, encoding='utf-8', errors='replace'):
        if desen(satir): return shlex.split(satir.strip())
    return None

if not os.path.exists(LOG): tam_derleme()
derle = komut(lambda s: 'xtensa-esp32-elf-g++' in s and ' -MMD -c ' in s and 'sketch\\\\KubitAnahtarlik.ino.cpp"' in s)
bagla = komut(lambda s: 'xtensa-esp32-elf-g++' in s and 'KubitAnahtarlik.ino.map' in s)
resim = komut(lambda s: 'esptool' in s and 'elf2image' in s and 'KubitAnahtarlik.ino.elf' in s and 'bootloader' not in s)
if not (derle and bagla and resim): tam_derleme(); sys.exit('komutlar kaydedildi, tekrar calistir')

# 1) Arduino'nun ön işlemesinin aynısı: Arduino.h + prototipler (varsayılan argümanlılar hariç) -> build-yayin/sketch
kaynak = open(os.path.join(BURA, 'KubitAnahtarlik.ino'), encoding='utf-8').read().split('\n')
TANIM = re.compile(r'^(?!(?:if|for|while|switch|return|else|struct|class|enum|namespace|typedef|extern)\b)'
                   r'([A-Za-z_][\w:<>,\s\*&]*?[\s\*&])([A-Za-z_]\w*)\s*\(([^()]*)\)\s*\{')
proto, ilk = [], None
for i, s in enumerate(kaynak):
    m = TANIM.match(s)
    if not m or '=' in m.group(3): continue
    if ilk is None: ilk = i
    proto.append('%s%s(%s);' % (m.group(1), m.group(2), m.group(3)))
kaynak[ilk:ilk] = proto + ['#line %d "KubitAnahtarlik.ino"' % (ilk + 1)]
kopyala(TASLAK)
open(os.path.join(TASLAK, 'KubitAnahtarlik.ino.cpp'), 'w', encoding='utf-8').write('#include <Arduino.h>\n#line 1 "KubitAnahtarlik.ino"\n' + '\n'.join(kaynak))

# 2) derle, bağla, .bin yap (eski çıktılar önce silinir: bir adım sessizce atlanırsa eski .bin yeni sürüm sanılmasın)
for eski in ('KubitAnahtarlik.ino.elf', 'KubitAnahtarlik.ino.bin'):
    if os.path.exists(os.path.join(YAPI, eski)): os.remove(os.path.join(YAPI, eski))
for ad, k in (('derleniyor', derle), ('baglaniyor', bagla), ('bin', resim)):
    r = subprocess.run(k, capture_output=True, text=True, errors='replace')
    if r.returncode:
        hatalar = [s for s in (r.stdout + r.stderr).split('\n') if 'error' in s]
        sys.exit('DUR: %s hatasi\n%s' % (ad, '\n'.join(hatalar[:15]) or (r.stdout + r.stderr)[-3000:]))
binyol = os.path.join(YAPI, 'KubitAnahtarlik.ino.bin')
if not os.path.exists(binyol): sys.exit('DUR: .bin olusmadi')
print('Sketch uses %d bytes (%d%%) - %.0f sn' % (os.path.getsize(binyol), os.path.getsize(binyol) * 100 // 1966080, time.time() - bas))

if yukle:
    esptool = resim[0]
    sys.exit(subprocess.run([esptool, '--chip', 'esp32', '--port', 'COM4', '--baud', '115200', 'write-flash', '0x10000', binyol]).returncode)
