"""Soldier character (3/4 view, 32x32 frames): idle, walk, push, turn, deaths."""
from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import Sheet  # noqa: E402
from pixel import Canvas, hash2  # noqa: E402
from soldier_pal import PAL  # noqa: E402
import soldier_tiles as st  # noqa: E402

T = 32
CM = {
    "o": "ink",
    "H": "ol2", "h": "ol3", "j": "ol4", "G": "ol1", "g": "ol0",
    "S": "sk2", "L": "sk3", "s": "sk1", "e": "ink2",
    "U": "ol3", "V": "ol4", "u": "ol2", "D": "ol1",
    "B": "wd1", "Y": "yel2",
    "P": "ol2", "p": "ol1",
    "K": "wd2", "k": "wd1",
    "A": "wd3", "a": "wd4", "Z": "wd2",
    "R": "red2",
}

# ---------------------------------------------------------------- front (facing down)
HEAD_D = [
    ".....oooooo.....",
    "...oojjhhhHoo...",
    "..ojjhhhHHHHGo..",
    "..ohhHHHHHHHGo..",
    ".oHHHHHHHHHHGGo.",
    ".ogGGGGGGGGGGgo.",
    "..osssssssssso..",
    "..oSeSSSSSSeSo..",
    "..oSSSSLLSSSSo..",
    "...osSSSSSSso...",
]
BODY_D = [
    "..oVUossssoUuo..",
    ".oVVuUVUUUUuUuo.",
    ".oVVuUDUUDUuUuo.",
    ".oVVuUDUUDUuUuo.",
    ".oVUuUUUUUUuUuo.",
    ".oSSuUUUUUUuSso.",
    ".osSoBBBYYBBosSo",
]
LEGS_D = {
    "idle": [
        "..oooPPPpPPpooo.",
        "....oPPPpPPpo...",
        "....oPPpoPPpo...",
        "....oPPpoPPpo...",
        "....oPPpoPPpo...",
        "....oKKkoKKko...",
        "...oKKKkoKKKko..",
        "...ooooooooooo..",
    ],
    "a": [
        "..oooPPPpPPpooo.",
        "....oPPPpPPpo...",
        "....oPPpoPPpo...",
        "....oKKkoPPpo...",
        "...oKKKkoPPpo...",
        "...ooooooKKko...",
        "........oKKKko..",
        "........oooooo..",
    ],
}

# ---------------------------------------------------------------- back (facing up)
HEAD_U = [
    ".....oooooo.....",
    "...oojjhhhHoo...",
    "..ojjhhhHHHHGo..",
    "..ohhHHHHHHHGo..",
    ".oHhHHHHHHHHGGo.",
    ".oHHHHHHHHHHGGo.",
    ".ogGGGGGGGGGGgo.",
    "..oGGGGGGGGGGo..",
    "...osSSSSSSso...",
    "...oosssssoo....",
]
BODY_U = [
    "..oVUoooooooUuo.",
    ".oVVuoaAAAAAoUuo",
    ".oVVuoaAAAAZoUuo",
    ".oVVuoaAAAAZoUuo",
    ".oVUuoAAAAZZoUuo",
    ".oSSuoZZZZZZoSso",
    ".osSoBBBBBBBosSo",
]

# ---------------------------------------------------------------- side (facing left)
HEAD_L = [
    "......ooooo.....",
    "....oojhhhHoo...",
    "...ojhhhHHHHGo..",
    "...ohhHHHHHHGo..",
    "..oHHHHHHHHHGGo.",
    ".ogGGGGGGGGGGgo.",
    "..oLsssGGGGGGo..",
    "..oSeSSGGGGGo...",
    "..oSSSSSsGGo....",
    "...osSSSsoo.....",
]
BODY_L = [
    "....oUUUUoAAo...",
    "...oVUUUUoaAAo..",
    "...oVUVVUoaAAo..",
    "...oVUVVUoAAAo..",
    "...oUUVVUoAAZo..",
    "...oUUSSUoZZo...",
    "...oBBBBBBo.....",
]
LEGS_L = {
    "idle": [
        "....oPPPPo......",
        "....oPPPPo......",
        "....oPPpPo......",
        "....oPPpPo......",
        "....oPPpPo......",
        "...oKKKKKo......",
        "..oKKKKKko......",
        "..ooooooo.......",
    ],
    "a": [
        "....oPPPPo......",
        "...oPPPoPPo.....",
        "..oPPPo.oPPo....",
        "..oPPo...oPPo...",
        ".oPPo.....oPpo..",
        "oKKKo....oKKko..",
        "oKKKKo...oKKKko.",
        "oooooo...ooooo..",
    ],
    "b": [
        "....oPPPPo......",
        "....oPPPPo......",
        "...oPPpPPo......",
        "...oPPooPPo.....",
        "..oPPo..oPPo....",
        ".oKKKo..oKKko...",
        ".oKKKKo.oKKKko..",
        ".oooooo.ooooo...",
    ],
}
ARMS_L_PUSH = [  # arms stretched forward (left) at chest height, overlay on body rows 2..4
    "oSSVVVVVo",
    "oSsUUUUUo",
    ".oooooo..",
]


def _canvas():
    return Canvas(PAL, T, T)


def _shadow(c: Canvas, w=7.0, cy=28.5):
    sh = _canvas()
    sh.ellipse(15.5, cy - 0.5, w, 2.2, (8, 6, 16, 95))
    out = _canvas()
    out.paste(sh)
    out.paste(c)
    return out


def _flip_rows(rows):
    return [r[::-1] for r in rows]


def front(legs="idle", mirror_legs=False, bob=0, arm=0, push=False, back=False):
    """Compose the front or back view. arm: -1/0/1 swing (hand rows shift)."""
    c = _canvas()
    x0, y0 = 8, 3 + bob
    head = HEAD_U if back else HEAD_D
    body = [list(r) for r in (BODY_U if back else BODY_D)]
    lg = LEGS_D[legs]
    if mirror_legs:
        lg = _flip_rows(lg)
    c.stamp(lg, x0, y0 + 17 - bob, CM)
    c.stamp(["".join(r) for r in body], x0, y0 + 10, CM)
    c.stamp(head, x0, y0, CM)
    if arm:
        # swing: one hand up one px, the other down one px
        for side, d in ((0, arm), (1, -arm)):
            xs = (x0 + 1, x0 + 2, x0 + 3) if side == 0 else (x0 + 12, x0 + 13, x0 + 14)
            ys = y0 + 15
            if d > 0:  # hand lower: extend sleeve
                for x in xs:
                    c.px(x, ys + 2, c.get(x, ys + 1)[:].tolist() and PAL["ink"])
                for x in xs[1:] if side == 0 else xs[:-1]:
                    c.px(x, ys + 1, "sk1")
                    c.px(x, ys, "ol3")
            else:  # hand higher: skin moves up, bottom becomes outline
                for x in (xs[1:] if side == 0 else xs[:-1]):
                    c.px(x, ys - 1, "sk2")
                    c.px(x, ys + 1, "ink")
    if push:
        if not back:
            # both hands forward in front of the belt
            for x in (x0 + 1, x0 + 2, x0 + 3, x0 + 12, x0 + 13, x0 + 14):
                for y in (y0 + 15, y0 + 16):
                    c.px(x, y, "ol2")
                c.px(x, y0 + 17, None) if c.get(x, y0 + 17)[3] and x in (x0 + 1, x0 + 14) else None
            c.stamp(["oSSo..oSSo", "oSso..osSo", ".oo....oo."], x0 + 3, y0 + 15, CM)
            for x in (x0 + 1, x0 + 14):
                c.px(x, y0 + 15, "ink"); c.px(x, y0 + 16, "ink")
        else:
            # hands raised to the shoulders (pushing ahead, seen from behind)
            for x in (x0 + 2, x0 + 3, x0 + 12, x0 + 13):
                c.px(x, y0 + 15, "ol2"); c.px(x, y0 + 16, "ink")
            c.stamp(["oSSo", "oSso"], x0 + 0, y0 + 8, CM)
            c.stamp(["oSSo", "osSo"], x0 + 12, y0 + 8, CM)
    return _shadow(c)


def side(legs="idle", bob=0, arm=0, push=False):
    c = _canvas()
    x0, y0 = 8, 3 + bob
    lean = -1 if push else 0
    c.stamp(LEGS_L[legs], x0, y0 + 17 - bob, CM)
    c.stamp(BODY_L, x0 + lean, y0 + 10, CM)
    c.stamp(HEAD_L, x0 + lean * 2, y0, CM)
    if arm and not push:
        # arm swing: hand moves forward/back one px
        hx = x0 + 6 + lean + (-1 if arm > 0 else 1)
        c.px(x0 + 6, y0 + 15, "ol3"); c.px(x0 + 7, y0 + 15, "ol3")
        c.px(hx, y0 + 15, "sk2"); c.px(hx + 1, y0 + 15, "sk1")
        c.px(hx, y0 + 16, "ink"); c.px(hx + 1, y0 + 16, "ink")
    if push:
        c.stamp(ARMS_L_PUSH, x0 - 1 + lean, y0 + 12, CM)
        c.px(x0 - 2 + lean, y0 + 12, "ink"); c.px(x0 - 2 + lean, y0 + 13, "ink")
    return _shadow(c)


def facing(d: str, **kw):
    if d == "d":
        return front(**kw)
    if d == "u":
        return front(back=True, **kw)
    legs = kw.pop("legs", "idle")
    mirror = kw.pop("mirror_legs", False)
    if legs == "a" and mirror:
        legs = "b"
    img = side(legs=legs, **kw)
    return img if d == "l" else img.flipped()


def walk_frames(d):
    # contact, step A, contact, step B
    return [
        facing(d, legs="idle", bob=0, arm=0),
        facing(d, legs="a", bob=-1, arm=1),
        facing(d, legs="idle", bob=0, arm=0),
        facing(d, legs="a", mirror_legs=True, bob=-1, arm=-1),
    ]


def push_frames(d):
    return [
        facing(d, legs="a", bob=0, push=True),
        facing(d, legs="a", mirror_legs=True, bob=0, push=True),
    ]


def idle_frames(d):
    a = facing(d)
    # breathing: the upper body dips 1 px (legs stay)
    b = a.copy()
    top = _canvas()
    top.a[:22] = a.a[:22]
    b2 = _canvas()
    b2.a[22:] = a.a[22:]
    b2.paste(top, 0, 1)
    return [a, b2]


# ---------------------------------------------------------------- deaths
def _fire_ball(c: Canvas, cx, cy, r, phase):
    for y in range(int(cy - r - 2), int(cy + r + 3)):
        for x in range(int(cx - r - 2), int(cx + r + 3)):
            n = hash2(x, y, 51 + phase) * 0.35
            d = math.hypot(x - cx, (y - cy) * 1.1) / max(r, 0.1) + n - 0.15
            if d < 0.35:
                c.px(x, y, "fire0")
            elif d < 0.6:
                c.px(x, y, "fire1")
            elif d < 0.85:
                c.px(x, y, "fire2")
            elif d < 1.05:
                c.px(x, y, "fire3")


def _smoke(c: Canvas, cx, cy, r, alpha, seed):
    for i in range(7):
        a = hash2(i, seed, 61) * 2 * math.pi
        dd = hash2(i, seed, 62) * r * 0.6
        c.ellipse(cx + math.cos(a) * dd, cy + math.sin(a) * dd * 0.7, r * 0.5, r * 0.45,
                  (60, 58, 72, alpha))
        c.ellipse(cx + math.cos(a) * dd - 1, cy + math.sin(a) * dd * 0.7 - 1, r * 0.3, r * 0.25,
                  (110, 108, 122, alpha))


def _scorch(c: Canvas):
    for y in range(T):
        for x in range(T):
            d = math.hypot(x - 15.5, (y - 21) * 1.6)
            if d < 10 - hash2(x, y, 71) * 3:
                c.px(x, y, (15, 14, 23, 150 if d < 6 else 90))
    # helmet left behind
    c.stamp([".oooo.", "ojhHGo", "oHHGgo", ".oooo."], 13, 17, CM)


def death_giveup():
    frames = []
    base = facing("d")
    # 0: flash
    f = base.copy()
    for y in range(T):
        for x in range(T):
            if f.a[y, x, 3] and not (f.a[y, x, :3] < 40).all():
                f.a[y, x, :3] = 255 - (255 - f.a[y, x, :3]) // 3
    frames.append(f)
    for i, r in enumerate((5, 9, 12)):
        f = _canvas()
        if i == 0:
            f.paste(base)
        _fire_ball(f, 15.5, 16, r, i)
        frames.append(f)
    for i, (r, a) in enumerate(((12, 220), (13, 160), (14, 100))):
        f = _canvas()
        _scorch(f)
        _smoke(f, 15.5, 14 - i * 2, r, a, i)
        if i == 0:
            _fire_ball(f, 15.5, 18, 6, 9)
        frames.append(f)
    f = _canvas()
    _scorch(f)
    frames.append(f)
    return frames


LYING = [  # soldier lying on his back, head left, boots right
    "..oooo....oooooooooo........",
    ".ojjhHo..oVVVVVVVVVVoooooooo",
    "ohhHHGGooVUUUUUUUUUUBPPPPPPo",
    "oLSSSsGoSSUUDUUDUUUUYPPPPPPKo",
    "oSeSeSGoSSUUDUUDUUUUBppppppKo",
    "osSSSsGo.ouuuuuuuuuuopppppKKo",
    ".oosso...oooooooooooooooooooo",
]


def death_timeout():
    frames = []
    frames.append(facing("d"))
    # knees buckle: squash (drop upper body 2px, legs shorter)
    f = _canvas()
    src = facing("d")
    f.a[:] = 0
    f.paste(src, 0, 2)
    f.a[29:] = src.a[29:]
    frames.append(f)
    # kneel
    k = _canvas()
    k.stamp(HEAD_D, 8, 8, CM)
    k.stamp(BODY_D, 8, 18, CM)
    k.stamp(["..oooPPPpPPpooo.", "..oKKKkooKKKkoo.", "..ooooooooooooo."], 8, 25, CM)
    frames.append(_shadow(k))
    # tipping over (rotated)
    from PIL import Image
    im = k.img().rotate(50, resample=Image.NEAREST, center=(16, 27))
    tip = _canvas()
    tip.a = np.array(im)
    frames.append(_shadow(tip))
    # lying
    ly = _canvas()
    ly.stamp(LYING, 2, 18, CM)
    frames.append(_shadow(ly, w=13, cy=25.5))
    # final: lying, slightly desaturated
    fin = _shadow(ly, w=13, cy=25.5)
    a = fin.a.astype(np.int32)
    g = (a[:, :, 0] + a[:, :, 1] + a[:, :, 2]) // 3
    for ch in range(3):
        a[:, :, ch] = (a[:, :, ch] * 2 + g) // 3
    fin.a = a.astype(np.uint8)
    frames.append(fin)
    return frames


def _blood(c: Canvas, cx, cy, r, seed, alpha=255):
    for y in range(int(cy - r - 2), int(cy + r + 3)):
        for x in range(int(cx - r - 2), int(cx + r + 3)):
            d = math.hypot(x - cx, (y - cy) * 1.4) / r + hash2(x, y, seed) * 0.5
            if d < 0.7:
                c.px(x, y, ("blood1"[:], ) and (143, 22, 34, alpha))
            elif d < 1.0:
                c.px(x, y, (94, 13, 20, alpha))


def death_guillotine():
    """32x48 frames drawn at (cell_x, cell_y - 16) over guillotine_base (they include posts and blade)."""
    frames = []
    body = facing("d")
    for stage in (0, 1, 2):
        f = Canvas(PAL, T, st.GH)
        f.paste(body, 0, 16)
        f.paste(st.guillotine_top(stage))
        frames.append(f)
    for i in range(4):
        f = Canvas(PAL, T, st.GH)
        _blood(f, 15.5, 16 + 25, 6 + i * 1.5, 81)
        for k in range(8 + i * 3):
            a = hash2(k, 1, 83) * 2 * math.pi
            d = 6 + hash2(k, 2, 83) * (5 + i * 3)
            f.px(int(15.5 + math.cos(a) * d), int(16 + 24 + math.sin(a) * d * 0.6), "blood2" if k % 2 else "blood1")
        if i >= 1:
            f.stamp([".oooo.", "ojhHGo", "oHHGgo", ".oooo."], 21 + min(i, 2), 16 + 25, CM)
        f.paste(st.guillotine_top(3))
        frames.append(f)
    return frames


def death_drown():
    """Frames drawn on a water tile."""
    frames = []
    full = facing("d")
    for k, cut in enumerate((30, 24, 18, 13)):
        f = _canvas()
        for y in range(min(cut, T)):
            f.a[y + (32 - cut) // 3] = full.a[y] if y + (32 - cut) // 3 < T else 0
        # arms up when half under
        if k in (2, 3):
            f.stamp(["oSo", "oSo", "oVo"], 7, 9 + (k - 2) * 3, CM)
            f.stamp(["oSo", "oSo", "oVo"], 22, 9 + (k - 2) * 3, CM)
        # splash ring at the waterline
        wl = min(T - 1, cut + (32 - cut) // 3)
        f.ring(15.5, wl, 6 + k, 8 + k, (198, 241, 247, 200), aspect=0.4)
        for i in range(6):
            a = hash2(i, k, 91) * math.pi
            f.px(int(15.5 + math.cos(a) * (9 + k)), int(wl - math.sin(a) * (3 + k)), "wa6")
        frames.append(f)
    # bubbles + ripples
    for k in range(4):
        f = _canvas()
        r = 4 + k * 3
        f.ring(15.5, 18, r - 1, r, (198, 241, 247, max(40, 200 - k * 50)), aspect=0.6)
        if k < 3:
            f.ring(15.5, 18, max(0, r - 5), r - 4, (121, 203, 230, 150), aspect=0.6)
        for i in range(3 - min(k, 2)):
            bx = 13 + int(hash2(i, k, 93) * 6)
            by = 18 - k * 2 - i * 3
            f.ring(bx, by, 0.6, 1.6, "wa6")
        frames.append(f)
    return frames


# ---------------------------------------------------------------- sheet
def build_player() -> Sheet:
    s = Sheet(max_width=32 * 16 + 15)
    for d in "durl":
        for i, f in enumerate(idle_frames(d)):
            s.add(f"idle_{d}_{i}", f.img())
        s.anim(f"idle_{d}", [f"idle_{d}_0", f"idle_{d}_1"], [900, 500], True)
        for i, f in enumerate(walk_frames(d)):
            s.add(f"walk_{d}_{i}", f.img())
        s.anim(f"walk_{d}", [f"walk_{d}_{i}" for i in range(4)], 125, True)
        for i, f in enumerate(push_frames(d)):
            s.add(f"push_{d}_{i}", f.img())
        s.anim(f"push_{d}", [f"push_{d}_0", f"push_{d}_1"], 200, True)
        s.add(f"turn_{d}", facing(d, legs="a", bob=-1).img())
        s.anim(f"turn_{d}", [f"turn_{d}", f"idle_{d}_0"], [70, 70], False)
    for name, fn, ms in (("giveup", death_giveup, [60, 60, 70, 80, 110, 130, 150, 400]),
                         ("timeout", death_timeout, [200, 160, 220, 120, 400, 600]),
                         ("guillotine", death_guillotine, [40, 40, 40, 90, 110, 140, 600]),
                         ("drown", death_drown, [90, 110, 140, 160, 160, 180, 200, 400])):
        fr = fn()
        names = []
        for i, f in enumerate(fr):
            n = f"die_{name}_{i}"
            s.add(n, f.img(), pivot=(0, f.h - 32))
            names.append(n)
        if isinstance(ms, list) and len(ms) != len(names):
            ms = ms[:len(names)] + [ms[-1]] * (len(names) - len(ms))
        s.anim(f"die_{name}", names, ms, False)
    # aliases by the original death codes
    for code, name in ((7, "giveup"), (47, "timeout"), (53, "guillotine"), (55, "drown"), (61, "giveup"),
                       (62, "timeout")):
        a = s.animations[f"die_{name}"]
        s.anim(f"death_{code}", a["frames"], a["ms"], False)
    return s
