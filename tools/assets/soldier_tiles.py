"""Soldier 32x32 tiles: floor, walls (autotile), exit, button, doors, keys, guillotine,
water, box, sunk box, shadows, effects."""
from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pixel import Canvas, hash2, dither, periodic_noise  # noqa: E402
from soldier_pal import PAL, SHADOW  # noqa: E402

T = 32


def C(fill=None):
    return Canvas(PAL, T, T, fill)


# --------------------------------------------------------------------------- floor
def floor(variant: int) -> Canvas:
    c = C("g4")
    seed = 11 + variant
    for y in range(T):
        for x in range(T):
            r = hash2(x, y, seed)
            if r < 0.045:
                c.px(x, y, "g3")
            elif r > 0.975:
                c.px(x, y, "g5")
    # a few larger pores
    for i in range(5):
        x, y = int(hash2(i, 1, seed) * 28) + 2, int(hash2(i, 2, seed) * 28) + 2
        c.px(x, y, "g2")
        c.px(x + 1, y, "g3")
    # slab bevel
    c.hline(0, T - 1, 0, "g5")
    c.vline(0, 0, T - 1, "g5")
    c.hline(0, T - 1, T - 1, "g2")
    c.vline(T - 1, 0, T - 1, "g2")
    c.px(0, T - 1, "g3")
    c.px(T - 1, 0, "g3")
    if variant == 1:  # hairline crack
        pts = [(5, 20), (8, 18), (10, 18), (13, 15), (15, 15), (17, 12), (19, 11)]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            c.line(x0, y0, x1, y1, "g2")
        c.px(9, 19, "g5"); c.px(14, 16, "g5"); c.px(18, 13, "g5")
        c.line(13, 15, 13, 18, "g2")
    elif variant == 2:  # bolted plate
        c.frame(3, 3, 26, 26, "g3")
        c.hline(4, 28, 4, "g5")
        c.vline(4, 4, 28, "g5")
        for bx, by in ((6, 6), (25, 6), (6, 25), (25, 25)):
            c.px(bx, by, "g6"); c.px(bx + 1, by, "g3"); c.px(bx, by + 1, "g3"); c.px(bx + 1, by + 1, "g2")
    elif variant == 3:  # oil stain + scuffs
        c.ellipse(19, 18, 5, 3, "g3")
        c.ellipse(20, 18, 3, 2, "g2")
        c.px(16, 17, "g3"); c.px(24, 19, "g3"); c.px(17, 21, "g3")
        c.line(5, 8, 10, 6, "g3")
        c.line(6, 10, 9, 9, "g5")
    return c


# --------------------------------------------------------------------------- walls
WALL_PAL = dict(m0="mortar0", m1="mortar1", b=["br2", "br3", "br3", "br4"], hi="br5", lo="br1", lo2="br0",
                face=["br1", "br2"], glint=None)
DARK_PAL = dict(m0="dk0", m1="dk1", b=["dk2", "dk2", "dk3", "dk3"], hi="dk4", lo="dk1", lo2="dk0",
                face=["dk1", "dk1"], glint="dkglow")


def _brick_id(x, y):
    row = y // 8
    off = 8 if row % 2 else 0
    col = ((x + off) % 32) // 16
    return row % 4, col


def wall(mask: int, p=WALL_PAL, seed=3) -> Canvas:
    """mask bits: 1=N, 2=E, 4=S, 8=W neighbour is the same kind of wall."""
    c = C(p["m0"])
    for y in range(T):
        for x in range(T):
            row = y // 8
            off = 8 if row % 2 else 0
            lx = (x + off) % 16
            ly = y % 8
            bid = _brick_id(x, y)
            if ly == 7 or lx == 15:
                c.px(x, y, p["m0"])
                continue
            tone = p["b"][int(hash2(*bid, seed) * 4)]
            col = tone
            if ly == 0:
                col = p["hi"] if tone != p["b"][0] else p["b"][2]
            elif lx == 0:
                col = p["b"][3] if tone != p["b"][3] else p["hi"]
            elif ly == 6 or lx == 14:
                col = p["lo"]
            c.px(x, y, col)
            r = hash2(x, y, seed + 7)
            if 1 <= ly <= 5 and 1 <= lx <= 13:
                if r < 0.06:
                    c.px(x, y, p["lo"])
                elif r > 0.97:
                    c.px(x, y, p["hi"])
    # mortar highlight pixels under bricks
    for y in range(T):
        for x in range(T):
            if y % 8 == 7 and hash2(x, y, seed + 2) < 0.25:
                c.px(x, y, p["m1"])
    if p["glint"]:
        # faint luminous veins in the dark stone
        veins = [[(4, 3), (7, 5), (9, 4)], [(20, 12), (23, 13), (24, 16)], [(10, 22), (12, 21), (15, 23)],
                 [(27, 26), (29, 28)]]
        for v in veins:
            for (x0, y0), (x1, y1) in zip(v, v[1:]):
                c.line(x0, y0, x1, y1, "dk4")
            c.px(*v[0], p["glint"])
        for x, y in ((14, 6), (3, 17), (26, 4), (19, 27)):
            c.px(x, y, "dk4")
    N, E, S, W = mask & 1, mask & 2, mask & 4, mask & 8
    if not S:  # front face (3/4 view)
        for y in range(T - 7, T):
            for x in range(T):
                fy = y - (T - 7)
                if fy == 0:
                    col = p["lo2"]
                elif (x + (4 if fy > 3 else 0)) % 8 == 0 and fy > 0:
                    col = p["lo2"]
                elif fy == 3 or fy == 6:
                    col = p["lo2"]
                else:
                    col = p["face"][(x // 8 + (fy > 3)) % 2]
                    if fy in (1, 4):
                        col = p["b"][0]
                c.px(x, y, col)
        c.hline(0, T - 1, T - 1, "ink")
    if not N:
        c.hline(0, T - 1, 0, "ink")
        c.hline(0, T - 1, 1, p["hi"])
        c.hline(0, T - 1, 2, p["b"][3])
    if not W:
        c.vline(0, 0, T - 1, "ink")
        c.vline(1, 1 if not N else 0, (T - 8) if not S else T - 1, p["b"][3])
    if not E:
        c.vline(T - 1, 0, T - 1, "ink")
        c.vline(T - 2, 1 if not N else 0, (T - 8) if not S else T - 1, p["lo"])
    if not N and not W:
        c.px(0, 0, None); c.px(1, 1, "ink")
    if not N and not E:
        c.px(T - 1, 0, None); c.px(T - 2, 1, "ink")
    if not S and not W:
        c.px(0, T - 1, None); c.px(1, T - 2, "ink")
    if not S and not E:
        c.px(T - 1, T - 1, None); c.px(T - 2, T - 2, "ink")
    return c


def dark_shimmer(frame: int, n=8) -> Canvas:
    """Transparent overlay: a faint diagonal glint sweeping across a dark wall."""
    c = C()
    pos = -16 + frame * (64 / n)
    for y in range(T):
        for x in range(T):
            d = (x + y) / 2 - pos
            if abs(d) < 1.0 and hash2(x, y, 5) < 0.5:
                c.px(x, y, (122, 216, 255, 90))
            elif abs(d) < 2.5 and hash2(x, y, 6) < 0.15:
                c.px(x, y, (122, 216, 255, 50))
    return c


def reveal_fx(frame: int) -> Canvas:
    """Overlay played when a dark wall turns into a plain wall: flash ring + dust."""
    c = C()
    r = 3 + frame * 3
    alpha = max(0, 230 - frame * 40)
    if frame < 5:
        c.ring(15.5, 15.5, r - 2, r, (200, 244, 255, alpha))
        c.ring(15.5, 15.5, max(0, r - 5), r - 2, (122, 216, 255, alpha // 2))
    for i in range(14):
        ang = hash2(i, 1, 9) * 2 * math.pi
        sp = 0.6 + hash2(i, 2, 9)
        d = frame * 3.2 * sp + 2
        x = 15.5 + math.cos(ang) * d
        y = 15.5 + math.sin(ang) * d + frame * 0.6
        col = "g6" if i % 3 else "g7"
        if frame < 6:
            c.px(int(x), int(y), col)
            if frame < 3:
                c.px(int(x) + 1, int(y), "g5")
    return c


# --------------------------------------------------------------------------- shadows
def shadow(kind: str) -> Canvas:
    c = C()
    depth = 6
    for y in range(T):
        for x in range(T):
            v = 0.0
            if "n" in kind and y < depth:
                v = max(v, 1 - y / depth)
            if "w" in kind and x < depth:
                v = max(v, 1 - x / depth)
            if kind == "nw":
                v = max(0, 1 - math.hypot(x, y) / depth)
            if v > 0:
                a = int(30 + 100 * v) if v > 0.15 else 0
                if a:
                    c.px(x, y, (8, 6, 16, a))
    return c


# --------------------------------------------------------------------------- exit
def exit_tile(frame: int, n=8) -> Canvas:
    c = floor(0)
    t = frame / n
    # recessed round pad
    c.ellipse(15.5, 15.5, 13, 13, "g2")
    c.ellipse(15.5, 15.5, 12, 12, "g1")
    c.ellipse(15.5, 16.0, 11, 11, "g0")
    # glowing ring (pulsing)
    pulse = 0.5 + 0.5 * math.sin(2 * math.pi * t)
    c.ring(15.5, 15.5, 9.0, 11.0, "gr1")
    c.ring(15.5, 15.5, 9.5, 10.5, "gr2" if pulse < 0.5 else "gr3")
    # rotating bright segment
    for k in range(3):
        a0 = 2 * math.pi * (t + k / 3)
        for s in range(-3, 4):
            a = a0 + s * 0.09
            x = 15.5 + math.cos(a) * 10
            y = 15.5 + math.sin(a) * 10
            c.px(int(round(x - 0.5)), int(round(y - 0.5)), "gr4" if abs(s) < 2 else "gr3")
    # inner glow
    c.ellipse(15.5, 15.5, 7, 7, "gr0")
    for y in range(T):
        for x in range(T):
            d = math.hypot(x - 15.5, y - 15.5)
            if d < 7 and dither((7 - d) / 7 * (0.55 + 0.35 * pulse), x, y):
                c.px(x, y, "gr1")
    # up-arrow / "E" glyph
    glyph = [".GGGG.",
             ".G....",
             ".GGG..",
             ".G....",
             ".GGGG."]
    c.stamp(glyph, 13, 13, {"G": "gr4" if pulse > 0.3 else "gr3"})
    # rising sparks
    for i in range(4):
        ph = (t + i / 4) % 1.0
        x = 9 + int(hash2(i, 0, 3) * 14)
        y = int(22 - ph * 14)
        if ph < 0.85:
            c.px(x, y, "gr4" if ph < 0.5 else "gr3")
    # rim bolts
    for a in range(8):
        ang = a * math.pi / 4 + math.pi / 8
        c.px(int(15.5 + math.cos(ang) * 12.4), int(15.5 + math.sin(ang) * 12.4), "g5")
    return c


# --------------------------------------------------------------------------- button
def button(state: int) -> Canvas:
    """state 0 = up, 1 = half, 2 = down (pressed)."""
    c = floor(0)
    # housing recess
    c.rect(6, 6, 20, 20, "g1")
    c.frame(5, 5, 22, 22, "g2")
    c.hline(5, 26, 26, "g6")
    c.vline(26, 5, 26, "g6")
    # hazard corners
    for (x, y) in ((5, 5), (23, 5), (5, 23), (23, 23)):
        for i in range(4):
            for j in range(4):
                if (i + j) % 4 < 2 and (abs(i - (0 if x == 5 else 3)) + abs(j - (0 if y == 5 else 3)) < 3):
                    c.px(x + i, y + j, "yel2")
    inset = [0, 1, 2][state]
    x0, y0, s = 8 + inset, 8 + inset, 16 - inset * 2
    top = ["g6", "g5", "g4"][state]
    c.rect(x0, y0, s, s, top)
    if state < 2:
        c.rect(x0, y0 + s, s, 2 - state, "g2")  # visible side
    c.hline(x0, x0 + s - 1, y0, "g7" if state == 0 else "g6")
    c.vline(x0, y0, y0 + s - 1, "g7" if state == 0 else "g5")
    c.hline(x0, x0 + s - 1, y0 + s - 1, "g3")
    c.vline(x0 + s - 1, y0, y0 + s - 1, "g3")
    # indicator lamp
    lamp = ["red2", "yel2", "grn2"][state]
    lamp_hi = ["red3", "yel3", "grn3"][state]
    c.ellipse(15.5, 15.5, 3, 3, "ink2")
    c.ellipse(15.5, 15.5, 2, 2, lamp)
    c.px(15, 14, lamp_hi)
    if state == 2:
        for (x, y) in ((15, 11), (16, 11), (15, 20), (16, 20), (11, 15), (11, 16), (20, 15), (20, 16)):
            c.px(x, y, (143, 232, 115, 120))
    return c


# --------------------------------------------------------------------------- doors
DOOR_COLS = {
    None: ("g2", "g3", "g5", "g6"),
    "red": ("red0", "red1", "red2", "red3"),
    "green": ("grn0", "grn1", "grn2", "grn3"),
    "blue": ("blu0", "blu1", "blu2", "blu3"),
    "yellow": ("yel0", "yel1", "yel2", "yel3"),
    "white": ("g5", "g6", "g8", "white"),
    "black": ("ink", "ink2", "g0", "g2"),
}


def door(stage: int, colour=None, stages=5) -> Canvas:
    """Retracting blast barrier. stage 0 = closed (raised), stages-1 = open (flush)."""
    k0, k1, k2, k3 = DOOR_COLS[colour]
    c = floor(0)
    # frame in the floor (always visible)
    c.rect(2, 2, 28, 28, "g1")
    c.frame(2, 2, 28, 28, "ink2")
    c.hline(2, 29, 30, "g6")
    c.vline(30, 2, 30, "g6")
    # colour coded rim lights
    for x in (4, 27):
        for y in (4, 27):
            c.px(x, y, k2)
    c.hline(8, 23, 3, k1)
    c.hline(8, 23, 28, k1)
    h = stages - 1
    lift = (h - stage) / h  # 1 = fully raised
    if stage == h:
        # open: flush grated plate
        c.rect(4, 5, 24, 22, "g2")
        for y in range(6, 26, 3):
            c.hline(5, 26, y, "g1")
            c.hline(5, 26, y + 1, "g3")
        c.hline(8, 23, 3, k2)
        c.hline(8, 23, 28, k2)
        for x in (4, 27):
            for y in (4, 27):
                c.px(x, y, k3)
        return c
    face = int(round(7 * lift))  # front face height
    top_y = 4 - int(round(3 * lift))
    th = 24  # top face height
    # block body
    x0, x1 = 3, 28
    ytop = top_y
    ybot = ytop + th
    # front face
    for y in range(ybot, ybot + face):
        for x in range(x0, x1 + 1):
            col = "g1" if (y - ybot) % 3 else "g0"
            if (x - x0) % 6 == 0:
                col = "g0"
            c.px(x, y, col)
    if face:
        c.hline(x0, x1, ybot + face - 1, "ink")
        # coloured hazard band on the face
        if face >= 3:
            for x in range(x0, x1 + 1):
                c.px(x, ybot + 1, k2 if ((x // 3) % 2) else "ink2")
    # top plate
    tone = ["g4", "g4", "g3", "g3"][min(3, stage)]
    c.rect(x0, ytop, x1 - x0 + 1, th, tone)
    c.hline(x0, x1, ytop, "g6" if stage == 0 else "g5")
    c.vline(x0, ytop, ybot - 1, "g5")
    c.vline(x1, ytop, ybot - 1, "g2")
    c.hline(x0, x1, ybot - 1, "g2")
    # X brace in key colour (like the original)
    for i in range(th - 6):
        fx = x0 + 3 + int(i * (x1 - x0 - 6) / (th - 7))
        fy = ytop + 3 + i
        for dx in (0, 1):
            c.px(fx + dx, fy, k2)
            c.px(x1 - 3 - (fx - x0 - 3) - dx, fy, k2)
            c.px(fx + dx, fy + 1, k0)
            c.px(x1 - 3 - (fx - x0 - 3) - dx, fy + 1, k0)
    # centre hub with highlight
    cx, cy = (x0 + x1) // 2, ytop + th // 2
    c.rect(cx - 2, cy - 2, 5, 5, k1)
    c.frame(cx - 3, cy - 3, 7, 7, "ink2")
    c.px(cx - 1, cy - 1, k3)
    c.px(cx, cy - 1, k2)
    # rivets
    for rx, ry in ((x0 + 2, ytop + 2), (x1 - 2, ytop + 2), (x0 + 2, ybot - 3), (x1 - 2, ybot - 3)):
        c.px(rx, ry, "g7"); c.px(rx, ry + 1, "g2")
    # outline
    c.frame(x0 - 1, ytop - 1, x1 - x0 + 3, th + face + 1, "ink")
    if stage > 0:  # darker as it sinks
        for y in range(T):
            for x in range(T):
                if x0 <= x <= x1 and ytop <= y < ybot and dither(0.18 * stage, x, y):
                    c.px(x, y, (15, 14, 23, 70))
    return c


# --------------------------------------------------------------------------- keys
KEY_ART = [
    "........oooo........",
    "......ooBBBBoo......",
    ".....oBLLLBBBBo.....",
    "....oBLBooooBBDo....",
    "....oBBo....oBDo....",
    "....oBBo....oBDo....",
    "....oBBDooooBDDo....",
    ".....oBBDDDDDDo.....",
    "......ooBBDDoo......",
    "........oBDo........",
    "........oBDo........",
    "........oBDo........",
    "........oBDoooo.....",
    "........oBDBBBDo....",
    "........oBDoooo.....",
    "........oBDo........",
    "........oBDooo......",
    "........oBDBBDo.....",
    "........oBDDDo......",
    ".........ooo........",
]


def key(colour: str) -> Canvas:
    k0, k1, k2, k3 = DOOR_COLS[colour]
    outline = "ink" if colour != "black" else "ink"
    cm = {"o": outline, "B": k2, "L": k3, "D": k1}
    if colour == "black":
        cm = {"o": "ink", "B": "g1", "L": "g4", "D": "g0"}
    if colour == "white":
        cm = {"o": "ink", "B": "g8", "L": "white", "D": "g5"}
    c = C()
    # soft drop shadow
    sh = C()
    sh.stamp(KEY_ART, 7, 7, {ch: (8, 6, 16, 90) for ch in "oBLD"})
    c.paste(sh, 2, 2)
    c.stamp(KEY_ART, 6, 5, cm)
    return c


def key_rotated(colour: str) -> Canvas:
    """Key tilted 45 degrees (nicer as a floor item)."""
    base = key(colour)
    from PIL import Image
    im = base.img().rotate(-35, resample=Image.NEAREST, center=(16, 16))
    c = C()
    c.a = np.array(im)
    return c


# --------------------------------------------------------------------------- guillotine
def guillotine_base() -> Canvas:
    c = floor(0)
    # slot in the floor where the blade lands
    c.rect(4, 21, 24, 3, "ink2")
    c.hline(4, 27, 24, "g5")
    # hazard stripes either side of slot
    for x in range(4, 28):
        col = "yel2" if ((x // 2) % 2) else "ink2"
        c.px(x, 19, col)
        c.px(x, 26, col)
    # post bases
    for x in (0, 27):
        c.rect(x, 17, 5, 12, "g1")
        c.frame(x, 17, 5, 12, "ink")
    return c


GH = 48  # guillotine overlays are 32x48: the bottom 32 px cover the cell, the top 16 px the cell above


def guillotine_top(stage: int) -> Canvas:
    """Transparent 32x48 overlay: posts, beam and blade. stage 0 = open (blade up) .. 3 = closed.
    Draw it at (cell_x, cell_y - 16), after the player."""
    c = Canvas(PAL, T, GH)
    y0 = 2
    # posts (3/4 view, tall)
    for x in (0, 27):
        c.rect(x, y0, 5, 43 - y0, "wd2")
        c.vline(x + 1, y0, 42, "wd4")
        c.vline(x + 3, y0, 42, "wd1")
        c.frame(x, y0, 5, 43 - y0, "ink")
        for yy in (10, 24, 38):
            c.px(x + 2, yy, "g6")
            c.px(x + 2, yy + 1, "g2")
    # beam
    c.rect(0, y0, 32, 6, "wd3")
    c.hline(0, 31, y0 + 1, "wd5")
    c.hline(0, 31, y0 + 5, "wd1")
    c.frame(0, y0, 32, 7, "ink")
    c.rect(13, y0 + 2, 6, 3, "g3")
    c.px(14, y0 + 2, "g6")
    # blade
    by = [9, 17, 26, 32][stage]
    bh = 9
    for y in range(by, by + bh):
        for x in range(5, 27):
            edge = by + bh - 1 - int((x - 5) * 3 / 21)
            if y > edge:
                continue
            col = "g5"
            if y == edge:
                col = "white"
            elif y == edge - 1:
                col = "g8"
            elif y - by < 2:
                col = "g3"
            elif x in (5, 26):
                col = "g3"
            elif (x + y) % 7 == 0:
                col = "g6"
            c.px(x, y, col)
    # weight bar + rope
    c.rect(5, by - 2, 22, 2, "g2")
    c.hline(5, 26, by - 2, "g4")
    c.vline(15, y0 + 7, by - 3, "wd1")
    c.vline(16, y0 + 7, by - 3, "wd4")
    c.outline("ink")
    if stage == 3:
        for (x, y) in ((6, 42), (9, 43), (24, 41), (27, 40)):
            c.px(x, y, "fire1")
    if stage in (1, 2):  # motion streaks
        for x in (8, 13, 19, 24):
            c.vline(x, by - 6, by - 3, (231, 232, 234, 90))
    return c


def guillotine(stage: int) -> Canvas:
    """Full 32x48 composite (base in the bottom 32 px)."""
    c = Canvas(PAL, T, GH)
    c.paste(guillotine_base(), 0, GH - T)
    c.paste(guillotine_top(stage))
    return c


# --------------------------------------------------------------------------- water
def water(frame: int, n=8) -> Canvas:
    """Seamless (in space and time) water: slow dark swells + undulating crest dashes."""
    c = C()
    t = frame / n
    tau = 2 * math.pi
    low = periodic_noise(T, T, [(1, 1, 1.0, 1), (1, -1, 0.8, -1), (0, 1, 0.4, 1)], seed=77, t=t)
    for y in range(T):
        for x in range(T):
            v = low[y, x]
            col = "wa2"
            if v < -9:
                col = "wa1"
            u = y / 8 + 0.16 * math.sin(tau * (x / 16 + t)) + 0.07 * math.sin(tau * (x / 32 * 3 - t))
            row = math.floor(u)
            fr = u - row
            ph = (row % 4) * 0.27 + (row % 2) * 0.5
            dash = math.sin(tau * (2 * x / 32 + ph - t)) + 0.5 * math.sin(tau * (x / 32 + ph * 2 + t))
            if fr < 0.13 and dash > 0.15:
                col = "wa4" if dash < 1.0 else "wa5"
            elif fr < 0.13 and dash > -0.4:
                col = "wa3"
            elif 0.13 <= fr < 0.26 and dash > 0.4:
                col = "wa1"
            c.px(x, y, col)
    return c


def BAYER(x, y):
    from pixel import BAYER4
    return BAYER4[y % 4, x % 4]


# --------------------------------------------------------------------------- box
def box_canvas(c: Canvas, ox=0, oy=0, wet=False, shadow=True):
    """Draw the crate (3/4 view) onto canvas c at offset."""
    x0, y0, w, h, face = 4 + ox, 3 + oy, 24, 21, 6
    if shadow:
        for y in range(y0 + 3, y0 + h + face + 2):
            for x in range(x0 + 3, x0 + w + 2):
                c.px(x, y, (8, 6, 16, 90))
    pal = ["wd1", "wd2", "wd3", "wd4", "wd5"]
    if wet:
        pal = ["wd0", "wd1", "wd2", "wd2", "wd3"]
    d0, d1, m, l1, l2 = pal
    # top face planks
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            plank = (y - y0) // 5
            ly = (y - y0) % 5
            col = m if plank % 2 == 0 else l1
            if ly == 4:
                col = d1
            elif ly == 0:
                col = l2 if plank % 2 else l1
            if hash2(x, y, 41) < 0.08 and 0 < ly < 4:
                col = d1
            c.px(x, y, col)
    # metal frame band around the top
    c.frame(x0, y0, w, h, "g3")
    c.hline(x0, x0 + w - 1, y0, "g6")
    c.vline(x0, y0, y0 + h - 1, "g5")
    # diagonal brace
    for i in range(h - 4):
        x = x0 + 2 + int(i * (w - 5) / (h - 5))
        y = y0 + h - 3 - i
        c.px(x, y, d0)
        c.px(x + 1, y, d1)
        c.px(x, y - 1, l2)
    # corner brackets
    for cx, cy in ((x0, y0), (x0 + w - 4, y0), (x0, y0 + h - 4), (x0 + w - 4, y0 + h - 4)):
        c.rect(cx, cy, 4, 4, "g4")
        c.px(cx + (1 if cx == x0 else 2), cy + (1 if cy == y0 else 2), "g7")
    # front face
    for y in range(y0 + h, y0 + h + face):
        for x in range(x0, x0 + w):
            col = d1 if (x - x0) % 6 else d0
            if y == y0 + h:
                col = d0
            c.px(x, y, col)
    c.hline(x0, x0 + w - 1, y0 + h + 2, "yel1" if not wet else "yel0")
    for x in range(x0, x0 + w):
        if (x // 2) % 2 == 0:
            c.px(x, y0 + h + 2, "ink2")
    # stencil mark on top
    c.stamp(["ooo.o.o", "o.o..o.", "ooo.o.o"], x0 + 9, y0 + 8, {"o": d0})
    # outline
    c.frame(x0 - 1, y0 - 1, w + 2, h + face + 2, "ink")
    return c


def box() -> Canvas:
    c = C()
    box_canvas(c)
    return c


def sunk_box(frame: int) -> Canvas:
    c = water(frame)
    # crate top flush with the water, slightly smaller, dark and wet
    x0, y0, w, h = 3, 3, 26, 26
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            plank = (y - y0) // 5
            ly = (y - y0) % 5
            col = "wd2" if plank % 2 == 0 else "wd3"
            if ly == 4:
                col = "wd1"
            if hash2(x, y, 43) < 0.1:
                col = "wd1"
            c.px(x, y, col)
    c.frame(x0, y0, w, h, "g2")
    for cx, cy in ((x0, y0), (x0 + w - 4, y0), (x0, y0 + h - 4), (x0 + w - 4, y0 + h - 4)):
        c.rect(cx, cy, 4, 4, "g3")
    c.frame(x0 - 1, y0 - 1, w + 2, h + 2, "wa0")
    # water lapping at the edges (animated)
    for i in range(0, 26, 1):
        ph = math.sin(2 * math.pi * (frame / 8 + i / 13))
        if ph > 0.6:
            c.px(x0 + i, y0 - 1, "wa5")
            c.px(x0 + 25 - i, y0 + h, "wa5")
        if ph < -0.7:
            c.px(x0 - 1, y0 + i, "wa4")
            c.px(x0 + w, y0 + 25 - i, "wa4")
    # wet shine
    c.line(x0 + 3, y0 + 2, x0 + 8, y0 + 2, "wd4")
    return c


def box_sink(frame: int, n=6) -> Canvas:
    """Overlay drawn on a water tile: crate sinking with splash. frame 0..n-1."""
    c = C()
    t = frame / (n - 1)
    sub = int(round(t * 8))
    # crate pushed down: draw crate into temp then crop rows by waterline
    tmp = C()
    box_canvas(tmp, 0, sub, wet=frame > 1, shadow=False)
    waterline = 29 - int(t * 4)
    for y in range(T):
        for x in range(T):
            px = tmp.a[y, x]
            if px[3] and y < waterline:
                c.a[y, x] = px
            elif px[3] and y < waterline + 3:
                c.px(x, y, (10, 31, 61, 160))
    # splash ring + droplets
    if frame >= 1:
        r = 8 + frame * 2.5
        a = max(0, 220 - frame * 35)
        c.ring(15.5, 18, r - 1.5, r, (198, 241, 247, a), aspect=0.75)
        for i in range(10):
            ang = hash2(i, 3, 11) * 2 * math.pi
            d = r + 1 + hash2(i, 4, 11) * 3
            x = 15.5 + math.cos(ang) * d
            y = 18 + math.sin(ang) * d * 0.75 - (2 if frame < 3 else 0)
            if frame < 5:
                c.px(int(x), int(y), "wa6" if i % 2 else "wa5")
    return c


# --------------------------------------------------------------------------- misc
def dust_puff(frame: int) -> Canvas:
    """Small dust puff at the feet (used when pushing / landing)."""
    c = C()
    for i in range(6):
        ang = math.pi + hash2(i, 0, 21) * math.pi
        d = 2 + frame * 2.2
        x = 15.5 + math.cos(ang) * d * 1.6
        y = 27 + math.sin(ang) * d * 0.5
        a = max(0, 200 - frame * 50)
        c.ellipse(x, y, 1.2 if frame < 2 else 1.8, 1.0, (205, 208, 213, a))
    return c


def water_edge(side: str) -> Canvas:
    """Overlay for a water tile whose neighbour on `side` (n/e/s/w) is not water:
    the bank casts a shadow (n, w) or catches a foam highlight (s, e)."""
    c = C()
    for i in range(T):
        if side in "nw":
            for d in range(4):
                a = [150, 105, 65, 30][d]
                x, y = (i, d) if side == "n" else (d, i)
                c.px(x, y, (6, 14, 30, a))
            x, y = (i, 4) if side == "n" else (4, i)
            if hash2(i, 0, 31 if side == "n" else 32) < 0.35:
                c.px(x, y, (198, 241, 247, 110))
        else:
            x, y = (i, T - 1) if side == "s" else (T - 1, i)
            c.px(x, y, (198, 241, 247, 150))
            x2, y2 = (i, T - 2) if side == "s" else (T - 2, i)
            if hash2(i, 1, 33) < 0.5:
                c.px(x2, y2, (121, 203, 230, 120))
    return c
