# Kübit anahtarlık kasası: 1,3" OLED (önde) + Beetle ESP32 ve HW-373 (ortada, raf üstünde) + 1000 mAh LiPo (arkada).
# Eksenler: x = genişlik (merkez 0), y = boy (0 = iç alt duvar, USB-C tarafı), z = derinlik (0 = ön yüz, 30 = arka).
# Çalıştır: python kasa.py  -> govde, kapak, raf, tus x2 .stl (hepsi baskı yönünde, destek gerekmez)
import math
import numpy as np
import trimesh
from manifold3d import Manifold, CrossSection, set_circular_segments

set_circular_segments(48)
M = Manifold

# ---- ölçüler (cetvelle ölçüp burayı düzelt) ----
TOL = 0.3
T = 30.0                       # toplam kalınlık (istenen)
WALL, FRONT, LID = 2.4, 2.2, 2.0
IW, IL = 40.0, 65.6            # iç genişlik / boy
CORNER_R, EDGE_CH = 6.0, 1.4   # köşe yarıçapı, ön/arka kenar pahı
OLED_W, OLED_H = 35.4, 33.5    # 1,3" SH1106 kartı
OLED_GLASS = 1.6               # cam + bant kalınlığı (kart ön yüzü camın arkasında)
OLED_PCB = 1.6
OLED_HOLE_IN = 2.3             # köşe deliklerinin kenara uzaklığı (M2)
OLED_WIN_W, OLED_WIN_H = 30.4, 16.0
OLED_WIN_FROM_TOP = 15.0       # kartın üst kenarından ekran ortasına (TAHMİN, ölç)
BEETLE_W, BEETLE_L = 35.0, 34.0      # TAHMİN, ölç
HW_W, HW_L = 17.5, 28.0              # HW-373 (USB-C kısa kenarda) TAHMİN, ölç
USBC_W, USBC_H = 9.6, 3.8
BAT_W, BAT_L, BAT_T = 34.0, 51.0, 5.8  # Robotistan 1000 mAh
SW = 6.0                       # 6x6 mm tuş
SW_H = 5.0                     # tuşun gövde tabanından düğme ucuna yüksekliği
BTN_X, BTN_Y = 12.0, 15.0      # tuş merkezleri (x=±12)

SHELF_Z = 20.4                 # rafın ön yüzü
SHELF_T = 1.6
LIP_H, LIP_T = 1.8, 1.2

OW, OL = IW + 2 * WALL, IL + 2 * WALL
oled_top = IL - 1.0            # kartın üst kenarı (header kabloları için 1 mm)
oled_cy = oled_top - OLED_H / 2


def box(x0, x1, y0, y1, z0, z1):
    return M.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def rrect(w, l, r):
    r = max(0.01, min(r, w / 2 - 0.01, l / 2 - 0.01))
    return CrossSection.square([w - 2 * r, l - 2 * r], center=True).offset(r, join_type=__import__('manifold3d').JoinType.Round)


def rbox(w, l, r, z0, z1, cy):
    return M.extrude(rrect(w, l, r), z1 - z0).translate([0, cy, z0])


def cyl(d, z0, z1, x, y):
    return M.cylinder(z1 - z0, d / 2).translate([x, y, z0])


def chamfered_body(z0, z1, inset_front, inset_back):
    """Yuvarlak köşeli kutu, ön ve arka kenarları pahlı (konveks gövdelerin zarfı)."""
    cy = IL / 2
    layers = []
    if inset_front: layers.append(rbox(OW - 2 * EDGE_CH, OL - 2 * EDGE_CH, CORNER_R - EDGE_CH, z0, z0 + 0.01, cy))
    layers.append(rbox(OW, OL, CORNER_R, z0 + (EDGE_CH if inset_front else 0), z1 - (EDGE_CH if inset_back else 0), cy))
    if inset_back: layers.append(rbox(OW - 2 * EDGE_CH, OL - 2 * EDGE_CH, CORNER_R - EDGE_CH, z1 - 0.01, z1, cy))
    return M.batch_hull(layers)


def detent(x, y, z, r):
    return M.sphere(r).translate([x, y, z])


# ================================================================ gövde
body_z1 = T - LID
govde = chamfered_body(0, body_z1, True, False)
cavity = rbox(IW, IL, CORNER_R - WALL, FRONT, body_z1 + 1, IL / 2)
govde = govde - cavity

# anahtarlık halkası: üst ortada, ön yüzle aynı tabandan (desteksiz)
ring = M.batch_hull([cyl(14, 0, 10, 0, IL + WALL + 5), box(-9, 9, IL, IL + WALL, 0, 10)])
govde = govde + ring - cyl(5.5, -1, 11, 0, IL + WALL + 5.5)

# ekran penceresi (dışa doğru pahlı)
win_y = oled_top - OLED_WIN_FROM_TOP
pencere = M.batch_hull([
    rbox(OLED_WIN_W, OLED_WIN_H, 1.2, FRONT - 0.6, FRONT + 0.1, win_y),
    rbox(OLED_WIN_W + 2 * 1.2, OLED_WIN_H + 2 * 1.2, 2.0, -0.1, 0.0, win_y)])
govde = govde - pencere

# OLED: köşelerde ayak (cam kalınlığı kadar) + karta geçen pim (havyayla ucu ezilerek sabitlenir)
for sx in (-1, 1):
    for sy in (-1, 1):
        hx, hy = sx * (OLED_W / 2 - OLED_HOLE_IN), oled_cy + sy * (OLED_H / 2 - OLED_HOLE_IN)
        govde = govde + cyl(4.4, FRONT - 0.1, FRONT + OLED_GLASS, hx, hy) + cyl(2.0, FRONT, FRONT + OLED_GLASS + OLED_PCB + 1.4, hx, hy)

# tuş yuvaları: önden Ø6.4 delik, içeride başlık flanşı için Ø9.6, sonra 6x6 tuş gövdesi
for sx in (-1, 1):
    x = sx * BTN_X
    govde = govde + rbox(12.4, 12.4, 1.0, FRONT - 0.1, FRONT + 1.2 + SW_H - 1.0, BTN_Y).translate([x, 0, 0])
    govde = govde - cyl(6.4, -1, FRONT + 0.01, x, BTN_Y) - cyl(9.6, FRONT, FRONT + 1.2, x, BTN_Y) \
        - box(x - SW / 2 - 0.2, x + SW / 2 + 0.2, BTN_Y - SW / 2 - 0.2, BTN_Y + SW / 2 + 0.2, FRONT + 1.2, FRONT + 10)
    # tuş yanına nokta işareti: sol 1 nokta (Tuş 1), sağ 2 nokta (Tuş 2)
    for k in range(1 if sx < 0 else 2):
        govde = govde - cyl(1.4, -1, 0.5, x + (k - (0 if sx < 0 else 0.5)) * 2.6, BTN_Y - 9.2)

# raf çıkıntıları (uzun duvarlarda), alt yüzü 45° pahlı -> desteksiz
for sx in (-1, 1):
    x_in = sx * IW / 2
    cikinti = M.batch_hull([box(min(x_in, x_in - sx * 1.5), max(x_in, x_in - sx * 1.5), 2, IL - 2, SHELF_Z - 1.6, SHELF_Z),
                            box(min(x_in, x_in + sx * 0.01), max(x_in, x_in + sx * 0.01), 2, IL - 2, SHELF_Z - 3.1, SHELF_Z)])
    govde = govde + cikinti

# USB-C deliği (alt duvar), dışta fiş başlığı için havşa
usb_z = SHELF_Z - 1.6 - USBC_H / 2 + 0.2
usb = M.extrude(rrect(USBC_W, USBC_H, 1.6), WALL + 2).rotate([90, 0, 0]).translate([0, 1, usb_z])
usb_h = M.extrude(rrect(13.0, 7.5, 3.0), 1.3).rotate([90, 0, 0]).translate([0, -WALL + 1.2, usb_z])
govde = govde - usb - usb_h

# kapak tırnağı için çentik (alt kenarda) + kapak tık noktaları (uzun duvarlarda çukur)
govde = govde - box(-5, 5, -WALL - 1, -WALL + 0.9, body_z1 - 1.0, body_z1 + 1)
for sx in (-1, 1):
    for y in (IL * 0.25, IL * 0.75):
        govde = govde - detent(sx * IW / 2, y, body_z1 - LIP_H / 2, 0.9)

# ================================================================ kapak (dış yüzü tabanda basılır)
kapak = chamfered_body(body_z1, T, False, True)
lip_o = rbox(IW - 0.3, IL - 0.3, CORNER_R - WALL - 0.15, body_z1 - LIP_H, body_z1 + 0.01, IL / 2)
lip_i = rbox(IW - 0.3 - 2 * LIP_T, IL - 0.3 - 2 * LIP_T, max(0.5, CORNER_R - WALL - 0.15 - LIP_T), body_z1 - LIP_H - 1, body_z1 + 0.02, IL / 2)
kapak = kapak + (lip_o - lip_i)
for sx in (-1, 1):
    for y in (IL * 0.25, IL * 0.75):
        kapak = kapak + (detent(sx * (IW / 2 - 0.15), y, body_z1 - LIP_H / 2, 0.75) - box(-IW / 2 + 0.6, IW / 2 - 0.6, y - 2, y + 2, body_z1 - LIP_H - 1, body_z1 + 1))
# Kübik küpü logosu (dış yüze 0.6 mm oyma): üç eşkenar dörtgen
cx, cy, s = 0, IL / 2, 9.0
yukari = CrossSection([[(0, 0), (s * math.cos(math.radians(30)), s / 2), (0, s), (-s * math.cos(math.radians(30)), s / 2)]])
for ang in (0, 120, 240):
    parca = M.extrude(yukari.offset(-0.45), 0.7).rotate([0, 0, ang]).translate([cx, cy, T - 0.6])
    kapak = kapak - parca

# ================================================================ raf: önünde Beetle ve HW-373, arkasında pil
raf = box(-IW / 2 + 0.3, IW / 2 - 0.3, 0.3, IL - 0.3, SHELF_Z, SHELF_Z + SHELF_T)
# HW-373: USB-C alt duvara dayalı; yanlarda ve arkada durdurucu
hw_y0, hw_y1 = 0.6, 0.6 + HW_L
for sx in (-1, 1):
    x0 = HW_W / 2 + TOL
    raf = raf + box(min(sx * x0, sx * (x0 + 1.2)), max(sx * x0, sx * (x0 + 1.2)), hw_y0 + 4, hw_y1 - 6, SHELF_Z - 1.6, SHELF_Z)
raf = raf + box(-HW_W / 2, HW_W / 2, hw_y1 + TOL, hw_y1 + TOL + 1.2, SHELF_Z - 1.6, SHELF_Z)
# Beetle: köşelerde L durdurucular
b_y0 = hw_y1 + TOL + 1.2 + 0.4
b_cy = b_y0 + BEETLE_L / 2
for sx in (-1, 1):
    for sy in (-1, 1):
        kx, ky = sx * (BEETLE_W / 2 + TOL), b_cy + sy * (BEETLE_L / 2 + TOL)
        raf = raf + box(min(kx, kx + sx * 1.2), max(kx, kx + sx * 1.2), min(ky, ky - sy * 5), max(ky, ky - sy * 5), SHELF_Z - 1.6, SHELF_Z)
        raf = raf + box(min(kx, kx - sx * 5), max(kx, kx - sx * 5), min(ky, ky + sy * 1.2), max(ky, ky + sy * 1.2), SHELF_Z - 1.6, SHELF_Z)
# tuşların arkasını tutan direkler (bacaklara değmesin diye ince, ortada)
sw_back = FRONT + 1.2 + SW_H + 0.2
for sx in (-1, 1):
    raf = raf + cyl(3.4, sw_back, SHELF_Z + 0.01, sx * BTN_X, BTN_Y)
# pil kabloları için yarık (HW-373'ün yanında)
raf = raf - box(HW_W / 2 + TOL + 1.6, HW_W / 2 + TOL + 8.5, hw_y1 - 10, hw_y1 + 1, SHELF_Z - 2, SHELF_Z + SHELF_T + 1)

# ================================================================ tuş başlığı (flanş tabanda basılır)
tus = cyl(9.0, 0, 1.0, 0, 0) + cyl(6.0, 1.0, 1.0 + FRONT + 1.4, 0, 0)
tus = tus + M.sphere(3.0).scale([1, 1, 0.45]).translate([0, 0, 1.0 + FRONT + 1.4])

# ================================================================ kontrol ve dışa aktarım
def kaydet(ad, m, flip=False):
    mesh = m.to_mesh()
    tm = trimesh.Trimesh(vertices=np.array(mesh.vert_properties)[:, :3], faces=np.array(mesh.tri_verts), process=False)
    if flip:
        tm.apply_transform(trimesh.transformations.rotation_matrix(math.pi, [1, 0, 0]))
    tm.apply_translation([0, 0, -tm.bounds[0][2]])
    tm.export(f'{ad}.stl')
    b = tm.bounds
    print(f'{ad:6s} {b[1][0]-b[0][0]:6.1f} x {b[1][1]-b[0][1]:6.1f} x {b[1][2]-b[0][2]:5.1f} mm  hacim {m.volume()/1000:5.1f} cm3  su_gecirmez={tm.is_watertight} gen={m.genus()}')


if __name__ == '__main__':
    # çakışma kontrolleri: bileşen hacimleri parçalara girmemeli
    pil = box(-BAT_W / 2, BAT_W / 2, IL / 2 - BAT_L / 2, IL / 2 + BAT_L / 2, SHELF_Z + SHELF_T + 0.05, SHELF_Z + SHELF_T + 0.05 + BAT_T)
    beetle = box(-BEETLE_W / 2, BEETLE_W / 2, b_cy - BEETLE_L / 2, b_cy + BEETLE_L / 2, SHELF_Z - 7, SHELF_Z - 0.01)
    hw = box(-HW_W / 2, HW_W / 2, hw_y0, hw_y1, SHELF_Z - 1.6 - USBC_H, SHELF_Z - 0.01)
    oled = box(-OLED_W / 2, OLED_W / 2, oled_cy - OLED_H / 2, oled_cy + OLED_H / 2, FRONT + OLED_GLASS + 0.01, FRONT + OLED_GLASS + OLED_PCB + 2)
    for ad, parca in (('pil', pil), ('beetle', beetle), ('hw373', hw), ('oled', oled)):
        for pad, p in (('govde', govde), ('raf', raf), ('kapak', kapak)):
            v = (parca ^ p).volume()
            if v > 0.5 and not (ad == 'oled' and pad == 'govde'):
                print(f'UYARI: {ad} ile {pad} çakışıyor ({v:.1f} mm3)')
    assert pil.bounding_box()[5] <= body_z1 + 0.01, 'pil kapağa taşıyor'
    assert b_cy + BEETLE_L / 2 + TOL <= IL, 'Beetle boya sığmıyor'
    kaydet('govde', govde)
    kaydet('kapak', kapak, flip=True)
    kaydet('raf', raf)
    kaydet('tus', tus)
