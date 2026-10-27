# Kübit anahtarlık kasası v2: Kübit kafası gibi sekizgen gövde, ekran = yüz, altta çukur yuvada 2 Cherry MX tuş, anten şeklinde halka.
# Katmanlar (önden arkaya): 1,3" OLED + MX tuş plakası | raf: önünde Beetle ESP32 ve HW-373 | arkasında 1000 mAh LiPo | kapak.
# Eksenler: x = genişlik (merkez 0), y = boy (0 = iç alt duvar, USB-C tarafı), z = derinlik (0 = ön yüz, 30 = arka).
# Dikkat: z ön yüzden içeri baktığı için ÖNDEN bakınca +x SOLDA kalır.
# Çalıştır: python kasa.py -> govde, kapak, raf, kep, halka .stl (hepsi baskı yönünde, destek gerekmez)
# Her şey tık diye oturur: MX tuşlar gövdeye, keycap tuşa, ekran ve kartlar kancalara, raf ve kapak duvara, halka kızağa.
import math
import numpy as np
import trimesh
from manifold3d import Manifold, CrossSection, JoinType, set_circular_segments

set_circular_segments(48)
M = Manifold

# ---- ölçüler (cetvelle ölçüp burayı düzelt) ----
TOL = 0.3
T = 30.0                       # toplam kalınlık (istenen)
WALL, FRONT, LID = 2.4, 2.2, 2.0
IW, IL = 42.0, 67.0            # iç genişlik / boy
PAH_KOSE = 5.5                 # sekizgen: dış köşe kesiği (Kübit'in kafası gibi)
EDGE_CH = 1.4                  # ön/arka kenar pahı
OLED_W, OLED_H = 35.4, 33.5    # 1,3" SH1106 kartı
OLED_GLASS, OLED_PCB = 1.6, 1.6
OLED_HOLE_IN = 2.3             # köşe deliklerinin kenara uzaklığı (M2)
OLED_WIN_W, OLED_WIN_H = 30.4, 16.0
OLED_WIN_FROM_TOP = 15.0       # kartın üst kenarından ekran ortasına (TAHMİN, ölç)
OLED_UST_BOSLUK = 2.0          # kart üstü ile iç duvar arası (header kabloları + sekizgen köşe)
BEETLE_W, BEETLE_L = 35.0, 34.0      # TAHMİN, ölç
HW_W, HW_L = 17.5, 28.0              # HW-373 (USB-C kısa kenarda) TAHMİN, ölç
USBC_W, USBC_H = 9.6, 3.8
BAT_W, BAT_L, BAT_T = 34.0, 51.0, 5.8  # Robotistan 1000 mAh
# Cherry MX: plaka deliği 14x14, plaka 1,5 mm; tuş aralığı 19,05 mm; keycap 18x18
MX_DELIK, MX_PLAKA, MX_ARALIK, KEP = 14.0, 1.5, 19.05, 18.0
KEP_H = 4.5                    # alçak profil keycap yüksekliği (MX şaftı 3,6 mm geçer)
MX_ALT = 8.6                   # plakanın arkasında kalan gövde + bacaklar
TUS_Y = 17.0                   # tuş merkezlerinin y'si (önden bakınca Tuş 2 solda = +x, Tuş 1 sağda = -x)
KUYU = 3.5                     # tuş yuvasının derinliği
KUYU_W, KUYU_H = MX_ARALIK + KEP + 1.2, KEP + 1.2

SHELF_Z, SHELF_T = 20.4, 1.6   # rafın ön yüzü, kalınlığı
LIP_H, LIP_T = 1.8, 1.2

OW, OL = IW + 2 * WALL, IL + 2 * WALL
oled_top = IL - OLED_UST_BOSLUK
oled_cy = oled_top - OLED_H / 2
plaka_z = KUYU                 # tuş plakasının ön yüzü


def box(x0, x1, y0, y1, z0, z1):
    return M.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def cyl(d, z0, z1, x, y):
    return M.cylinder(z1 - z0, d / 2).translate([x, y, z0])


def yuvarlak_kutu(w, l, r, z0, z1, cy):
    r = max(0.01, min(r, w / 2 - 0.01, l / 2 - 0.01))
    return M.extrude(CrossSection.square([w - 2 * r, l - 2 * r], center=True).offset(r, JoinType.Round), z1 - z0).translate([0, cy, z0])


def sekizgen(w, l, pah, cy):
    """Köşeleri kesik dikdörtgen (Kübit'in kafası), merkez (0, cy)."""
    x, y, p = w / 2, l / 2, pah
    return CrossSection([[(-x + p, -y), (x - p, -y), (x, -y + p), (x, y - p), (x - p, y), (-x + p, y), (-x, y - p), (-x, -y + p)]]).translate([0, cy])


def katman(kesit, z0, z1):
    return M.extrude(kesit, z1 - z0).translate([0, 0, z0])


DIS = sekizgen(OW, OL, PAH_KOSE, IL / 2)
IC = DIS.offset(-WALL, JoinType.Miter)


def pahli_govde(z0, z1, on, arka):
    """Sekizgen gövde, ön/arka kenarları pahlı (konveks katmanların zarfı)."""
    kucuk = DIS.offset(-EDGE_CH, JoinType.Miter)
    k = []
    if on: k.append(katman(kucuk, z0, z0 + 0.01))
    k.append(katman(DIS, z0 + (EDGE_CH if on else 0), z1 - (EDGE_CH if arka else 0)))
    if arka: k.append(katman(kucuk, z1 - 0.01, z1))
    return M.batch_hull(k)


# ================================================================ gövde
body_z1 = T - LID
govde = pahli_govde(0, body_z1, True, False) - katman(IC, FRONT, body_z1 + 1)

# aksesuar kızağı: üstte kırlangıç kuyruğu yuva, arkadan açık, önden kapalı. Halka, anten vb. arkadan kayarak takılır.
ust = IL + WALL                                     # gövdenin dış üst yüzü
KIZAK_Z0, KIZAK_Z1 = 6.0, body_z1                   # yuva önde 6 mm'de durur, arkada açık
KIZAK_BOYUN, KIZAK_TABAN, KIZAK_DERIN = 5.0, 8.0, 3.0
KIZAK_YUVA_Y = ust + 4.0                            # yuvanın ağzı (kızak bloğunun üstü)
KIZAK_TIK_Z = body_z1 - 2.5                         # tık noktası (arkaya yakın)


def kirlangic(boyun, taban, derin, y_agiz, z0, z1):
    """Kırlangıç kuyruğu prizma: ağızda dar (boyun), dipte geniş (taban); z boyunca uzanır."""
    k = CrossSection([[(-taban / 2, y_agiz - derin), (taban / 2, y_agiz - derin), (boyun / 2, y_agiz), (-boyun / 2, y_agiz)]])
    return M.extrude(k, z1 - z0).translate([0, 0, z0])


kizak_blok = box(-7, 7, ust - 0.5, KIZAK_YUVA_Y, 4.0, body_z1)
govde = govde + kizak_blok - kirlangic(KIZAK_BOYUN, KIZAK_TABAN, KIZAK_DERIN, KIZAK_YUVA_Y, KIZAK_Z0, body_z1 + 1)     - M.sphere(0.9).translate([0, KIZAK_YUVA_Y - KIZAK_DERIN, KIZAK_TIK_Z])   # tık çukuru

# ekran penceresi (dışa doğru pahlı) + çevresinde ince süs oluğu
win_y = oled_top - OLED_WIN_FROM_TOP
pencere = M.batch_hull([yuvarlak_kutu(OLED_WIN_W, OLED_WIN_H, 1.2, FRONT - 0.6, FRONT + 0.1, win_y),
                        yuvarlak_kutu(OLED_WIN_W + 2.4, OLED_WIN_H + 2.4, 2.0, -0.1, 0.0, win_y)])
oluk = yuvarlak_kutu(OLED_WIN_W + 7, OLED_WIN_H + 7, 3.5, -0.1, 0.4, win_y) - yuvarlak_kutu(OLED_WIN_W + 6, OLED_WIN_H + 6, 3.0, -0.2, 0.5, win_y)
govde = govde - pencere - oluk

# OLED köşe ayakları (cam kalınlığı) + karta geçen hizalama pimleri
for sx in (-1, 1):
    for sy in (-1, 1):
        hx, hy = sx * (OLED_W / 2 - OLED_HOLE_IN), oled_cy + sy * (OLED_H / 2 - OLED_HOLE_IN)
        govde = govde + cyl(4.4, FRONT - 0.1, FRONT + OLED_GLASS, hx, hy) + cyl(2.0, FRONT, FRONT + OLED_GLASS + OLED_PCB + 1.0, hx, hy)


def kanca(x_ic, yon, y, uzun, z_kok, z_tut, tirnak=0.5):
    """Esneyen tık kancası. Direk x_ic'de kartın kenarına değer (yon: kartın içine doğru +1/-1), z_kok'taki yüzeye bağlıdır.
    Ucundaki tırnak z_tut düzleminde kartın üst yüzüne tirnak kadar taşar; takarken kart pahlı yüzden kayıp tık diye geçer."""
    s = 1 if z_tut > z_kok else -1                  # direğin uzandığı yön
    dis = x_ic - yon * 0.15                          # kartla direk arasında 0,15 mm boşluk
    z_uc = z_tut + s * 1.2
    direk = box(min(dis, dis - yon * 1.2), max(dis, dis - yon * 1.2), y - uzun / 2, y + uzun / 2, min(z_kok, z_uc), max(z_kok, z_uc))
    tirnak_ = M.batch_hull([box(min(dis, dis + yon * tirnak), max(dis, dis + yon * tirnak), y - uzun / 2, y + uzun / 2, min(z_tut, z_tut + s * 0.3), max(z_tut, z_tut + s * 0.3)),
                            box(min(dis, dis + yon * 0.01), max(dis, dis + yon * 0.01), y - uzun / 2, y + uzun / 2, min(z_tut, z_uc), max(z_tut, z_uc))])
    return direk + tirnak_


oled_arka = FRONT + OLED_GLASS + OLED_PCB + 0.1
for sx in (-1, 1):                                   # kart kenarının ortasında, içe bakan tırnak
    govde = govde + kanca(sx * OLED_W / 2, -sx, oled_cy, 7, FRONT - 0.1, oled_arka)

# tuş yuvası: önden açık çukur; tabanı gövdeyle tek parça 1,5 mm MX plakası, tuşlar önden tık diye geçer.
# Baskıda plaka yuvanın üstünde 19 mm'lik bir köprü olur, desteksiz basılır.
kuyu = yuvarlak_kutu(KUYU_W, KUYU_H, 2.0, -0.1, KUYU + 0.01, TUS_Y)
cerceve = yuvarlak_kutu(KUYU_W + 6, KUYU_H + 6, 3.0, FRONT - 0.1, KUYU + MX_PLAKA, TUS_Y)
govde = govde + cerceve - kuyu
for sx in (-1, 1):
    d = (MX_DELIK + 0.1) / 2
    govde = govde - box(sx * MX_ARALIK / 2 - d, sx * MX_ARALIK / 2 + d, TUS_Y - d, TUS_Y + d, KUYU - 1, KUYU + MX_PLAKA + 1)
# tuş işaretleri: önden soldaki tuşun (+x) altında 2 nokta (Tuş 2), sağdakinin (-x) altında 1 nokta (Tuş 1)
for sx, n in ((1, 2), (-1, 1)):
    for k in range(n):
        govde = govde - cyl(1.3, -1, 0.5, sx * MX_ARALIK / 2 + (k - (n - 1) / 2) * 2.4, TUS_Y - KUYU_H / 2 - 2.6)

# raf çıkıntıları (uzun duvarlarda, alt yüzü 45° pahlı -> desteksiz)
for sx in (-1, 1):
    x_in = sx * IW / 2
    govde = govde + M.batch_hull([box(min(x_in, x_in - sx * 1.5), max(x_in, x_in - sx * 1.5), 8, IL - 8, SHELF_Z - 1.6, SHELF_Z),
                                  box(min(x_in, x_in + sx * 0.01), max(x_in, x_in + sx * 0.01), 8, IL - 8, SHELF_Z - 3.1, SHELF_Z)])

# USB-C deliği (alt duvar) ve fiş başlığı için havşa
usb_z = SHELF_Z - 1.6 - USBC_H / 2 + 0.2
govde = govde - M.extrude(CrossSection.square([USBC_W - 3.2, USBC_H - 3.2 + 0.01], center=True).offset(1.6, JoinType.Round), WALL + 2).rotate([90, 0, 0]).translate([0, 1, usb_z])
govde = govde - M.extrude(CrossSection.square([13.0 - 6, 7.5 - 6], center=True).offset(3.0, JoinType.Round), 1.3).rotate([90, 0, 0]).translate([0, -WALL + 1.2, usb_z])

# rafın tık çukurları (uzun duvarlarda)
RAF_TIK_Y = (IL * 0.35, IL * 0.65)
for sx in (-1, 1):
    for y in RAF_TIK_Y:
        govde = govde - M.sphere(0.9).translate([sx * IW / 2, y, SHELF_Z + SHELF_T / 2])

# kapak tırnağı için çentik + kapak tık noktaları
govde = govde - box(-5, 5, -WALL - 1, -WALL + 0.9, body_z1 - 1.0, body_z1 + 1)
for sx in (-1, 1):
    for y in (IL * 0.3, IL * 0.7):
        govde = govde - M.sphere(0.9).translate([sx * IW / 2, y, body_z1 - LIP_H / 2])

# ================================================================ kapak (dış yüzü tabanda basılır)
kapak = pahli_govde(body_z1, T, False, True)
lip_o = katman(IC.offset(-0.15, JoinType.Miter), body_z1 - LIP_H, body_z1 + 0.01)
lip_i = katman(IC.offset(-0.15 - LIP_T, JoinType.Miter), body_z1 - LIP_H - 1, body_z1 + 0.02)
kapak = kapak + (lip_o - lip_i)
for sx in (-1, 1):
    for y in (IL * 0.3, IL * 0.7):
        kapak = kapak + (M.sphere(0.75).translate([sx * (IW / 2 - 0.15), y, body_z1 - LIP_H / 2]) - box(-IW / 2 + 0.6, IW / 2 - 0.6, y - 2, y + 2, body_z1 - LIP_H - 1, body_z1 + 1))
s = 9.0                                              # Kübik küpü logosu (0.6 mm oyma)
rombus = CrossSection([[(0, 0), (s * math.cos(math.radians(30)), s / 2), (0, s), (-s * math.cos(math.radians(30)), s / 2)]])
for ang in (0, 120, 240):
    kapak = kapak - M.extrude(rombus.offset(-0.45, JoinType.Miter), 0.7).rotate([0, 0, ang]).translate([0, IL / 2, T - 0.6])

# ================================================================ raf: önünde Beetle ve HW-373, arkasında pil
raf = katman(IC.offset(-0.3, JoinType.Miter), SHELF_Z, SHELF_Z + SHELF_T)
hw_y0, hw_y1 = 0.6, 0.6 + HW_L
for sx in (-1, 1):
    x0 = HW_W / 2 + TOL
    raf = raf + box(min(sx * x0, sx * (x0 + 1.2)), max(sx * x0, sx * (x0 + 1.2)), hw_y0 + 4, hw_y1 - 6, SHELF_Z - 1.6, SHELF_Z)
raf = raf + box(-HW_W / 2, HW_W / 2, hw_y1 + TOL, hw_y1 + TOL + 1.2, SHELF_Z - 1.6, SHELF_Z)
b_y0 = hw_y1 + TOL + 1.2 + 0.4
b_cy = b_y0 + BEETLE_L / 2
for sx in (-1, 1):
    for sy in (-1, 1):
        kx, ky = sx * (BEETLE_W / 2 + TOL), b_cy + sy * (BEETLE_L / 2 + TOL)
        raf = raf + box(min(kx, kx + sx * 1.2), max(kx, kx + sx * 1.2), min(ky, ky - sy * 5), max(ky, ky - sy * 5), SHELF_Z - 1.6, SHELF_Z)
        raf = raf + box(min(kx, kx - sx * 5), max(kx, kx - sx * 5), min(ky, ky + sy * 1.2), max(ky, ky + sy * 1.2), SHELF_Z - 1.6, SHELF_Z)
raf = raf - box(HW_W / 2 + TOL + 1.6, HW_W / 2 + TOL + 8.5, hw_y1 - 10, hw_y1 + 1, SHELF_Z - 2, SHELF_Z + SHELF_T + 1)   # pil kablo yarığı
for sx in (-1, 1):                                   # raf duvardaki çukurlara tık diye oturur
    for y in RAF_TIK_Y:
        raf = raf + M.sphere(0.75).translate([sx * (IW / 2 - 0.3 - 0.15), y, SHELF_Z + SHELF_T / 2])
# kartları tık diye tutan kancalar: HW-373 iki yanında, Beetle iki yan kenarın ortasında (kart 1,6 mm)
KART = 1.6
for sx in (-1, 1):
    raf = raf + kanca(sx * HW_W / 2, -sx, (hw_y0 + hw_y1) / 2, 6, SHELF_Z + 0.01, SHELF_Z - KART - 0.1, 0.45)
    raf = raf + kanca(sx * BEETLE_W / 2, -sx, b_cy, 6, SHELF_Z + 0.01, SHELF_Z - KART - 0.1, 0.45)


# ================================================================ keycap (isteğe bağlı; Cherry MX artı şaftlı, üst yüzü tabanda basılır)
kep = M.batch_hull([yuvarlak_kutu(KEP, KEP, 2.0, 0, 0.01, 0), yuvarlak_kutu(KEP - 3, KEP - 3, 2.0, KEP_H, KEP_H + 0.01, 0)])
kep = kep - M.batch_hull([yuvarlak_kutu(KEP - 2.4, KEP - 2.4, 1.2, -0.01, 0.01, 0), yuvarlak_kutu(KEP - 5, KEP - 5, 1.2, KEP_H - 1.4, KEP_H - 1.39, 0)])
kep = kep + cyl(5.6, 0, KEP_H - 0.6, 0, 0) - box(-2.05, 2.05, -0.65, 0.65, -1, 3.7) - box(-0.6, 0.6, -2.05, 2.05, -1, 3.7)   # MX artı yuvası: 4.1 x 1.3 / 1.2, 3.7 derin
kep = kep - M.sphere(60).translate([0, 0, KEP_H + 59.6])                     # hafif çukur üst yüz


# ================================================================ aksesuar: oval halka (kızağa arkadan takılır; ön ucu tabanda basılır)
dil = kirlangic(KIZAK_BOYUN - 0.35, KIZAK_TABAN - 0.35, KIZAK_DERIN - 0.15, KIZAK_YUVA_Y, KIZAK_Z0 + 0.2, body_z1)   # 0,15-0,2 mm boşluk
dil = dil + M.sphere(0.75).translate([0, KIZAK_YUVA_Y - KIZAK_DERIN + 0.15 + 0.35, KIZAK_TIK_Z])   # tık tırnağı
halka_z0, halka_z1 = KIZAK_Z0 + 0.2, KIZAK_Z0 + 8.2
halka_y = KIZAK_YUVA_Y + 7.0                        # oval halka: 19 x 13 dış, 11 x 5,5 delik
halka = dil + box(-2.4, 2.4, KIZAK_YUVA_Y - 0.01, halka_y - 4, halka_z0, halka_z1)     + M.cylinder(halka_z1 - halka_z0, 1).scale([9.5, 6.5, 1]).translate([0, halka_y, halka_z0])     - M.cylinder(20, 1).scale([5.5, 2.75, 1]).translate([0, halka_y, halka_z0 - 5])


# ================================================================ kontrol ve dışa aktarım
def kaydet(ad, m, flip=False):
    mesh = m.to_mesh()
    tm = trimesh.Trimesh(vertices=np.array(mesh.vert_properties)[:, :3], faces=np.array(mesh.tri_verts), process=False)
    if flip:
        tm.apply_transform(trimesh.transformations.rotation_matrix(math.pi, [1, 0, 0]))
    tm.apply_translation([0, 0, -tm.bounds[0][2]])
    tm.export(f'{ad}.stl')
    b = tm.bounds
    print(f'{ad:12s} {b[1][0]-b[0][0]:6.1f} x {b[1][1]-b[0][1]:6.1f} x {b[1][2]-b[0][2]:5.1f} mm  hacim {m.volume()/1000:5.1f} cm3  kapali={tm.is_watertight} gen={m.genus()}')


if __name__ == '__main__':
    pil = box(-BAT_W / 2, BAT_W / 2, IL / 2 - BAT_L / 2, IL / 2 + BAT_L / 2, SHELF_Z + SHELF_T + 0.05, SHELF_Z + SHELF_T + 0.05 + BAT_T)
    def kart(w, l, cy, yukseklik):                  # 1,6 mm kart + kenarlardan 1 mm içeride bileşenler
        return box(-w / 2, w / 2, cy - l / 2, cy + l / 2, SHELF_Z - KART, SHELF_Z - 0.01) +                box(-w / 2 + 1, w / 2 - 1, cy - l / 2 + 1, cy + l / 2 - 1, SHELF_Z - yukseklik, SHELF_Z - KART)
    beetle = kart(BEETLE_W, BEETLE_L, b_cy, 7)
    hw = kart(HW_W, HW_L, (hw_y0 + hw_y1) / 2, KART + USBC_H)
    oled = box(-OLED_W / 2, OLED_W / 2, oled_cy - OLED_H / 2, oled_cy + OLED_H / 2, FRONT + OLED_GLASS + 0.01, FRONT + OLED_GLASS + OLED_PCB + 2)
    mx1 = box(-15.6 / 2, 15.6 / 2, TUS_Y - 7.8, TUS_Y + 7.8, plaka_z + MX_PLAKA + 0.01, plaka_z + MX_PLAKA + MX_ALT)   # plakanın arkasındaki gövde
    mx = mx1.translate([-MX_ARALIK / 2, 0, 0]) + mx1.translate([MX_ARALIK / 2, 0, 0])
    sorun = 0
    for ad, parca in (('pil', pil), ('beetle', beetle), ('hw373', hw), ('oled', oled), ('mx', mx), ('halka', halka - dil)):
        for pad, p in (('govde', govde), ('raf', raf), ('kapak', kapak)):
            if ad == pad or (ad == 'oled' and pad == 'govde'):
                continue
            v = (parca ^ p).volume()
            if v > 0.5:
                print(f'UYARI: {ad} ile {pad} çakışıyor ({v:.1f} mm3)'); sorun += 1
    for ad, parca in (('beetle', beetle), ('oled', oled), ('pil', pil)):   # sekizgen köşelere sığıyor mu
        v = (parca - katman(IC, -50, 50)).volume()
        if v > 0.5:
            print(f'UYARI: {ad} iç duvara taşıyor ({v:.1f} mm3)'); sorun += 1
    print('çakışma yok' if not sorun else f'{sorun} sorun')
    kaydet('govde', govde)
    kaydet('kapak', kapak, flip=True)
    kaydet('raf', raf, flip=True)                   # raf plakası tablaya, kancalar yukarı
    kaydet('kep', kep, flip=True)
    kaydet('halka', halka)
    v = (dil ^ govde).volume()                          # tırnak dışında kızak boşluklu olmalı
    print(f'halka dili ile gövde kesişimi {v:.2f} mm3 (kilitli konumda 0 olmalı: tırnak çukura oturur)')

    # Blender görseli için montajlı parçalar (render/ altında; baskı için değil)
    import os
    os.makedirs('render', exist_ok=True)
    def montaj(ad, m):
        mesh = m.to_mesh()
        trimesh.Trimesh(vertices=np.array(mesh.vert_properties)[:, :3], faces=np.array(mesh.tri_verts), process=False).export(f'render/{ad}.stl')
    montaj('govde', govde); montaj('kapak', kapak); montaj('raf', raf); montaj('halka', halka)
    MX_UST = 6.6                                      # tuş gövdesinin plakadan öne çıkan kısmı
    for sx, ad in ((-1, 'kep_sol'), (1, 'kep_sag')):
        x = sx * MX_ARALIK / 2
        montaj(ad, kep.rotate([180, 0, 0]).translate([x, TUS_Y, plaka_z - MX_UST + 0.5]))
        montaj('mx_' + ad[4:], box(x - 7, x + 7, TUS_Y - 7, TUS_Y + 7, plaka_z - MX_UST, plaka_z))
    montaj('oled_cam', box(-OLED_WIN_W / 2 - 1.5, OLED_WIN_W / 2 + 1.5, win_y - OLED_WIN_H / 2 - 1.5, win_y + OLED_WIN_H / 2 + 1.5, FRONT, FRONT + OLED_GLASS))
    print('montaj: render/')
