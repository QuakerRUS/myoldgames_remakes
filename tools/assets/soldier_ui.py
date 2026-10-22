"""Soldier UI: inventory slot, cursor, 9-slice frames, icons, side panel, menu background, logo."""
from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FONTS, Sheet, save_png  # noqa: E402
from pixel import Canvas, hash2, dither  # noqa: E402
from soldier_pal import PAL, COLOURS  # noqa: E402
import soldier_tiles as st  # noqa: E402
import soldier_char as sc  # noqa: E402


def cv(w, h, fill=None):
    return Canvas(PAL, w, h, fill)


# ------------------------------------------------------------------ pieces
def steel(c: Canvas, x, y, w, h, seed=1, base="g2"):
    """Brushed steel fill."""
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            r = hash2(xx // 3, yy, seed)
            col = base
            if r < 0.12:
                col = "g1"
            elif r > 0.9:
                col = "g3"
            c.px(xx, yy, col)


def frame9(size=48, border=16) -> Canvas:
    """Raised riveted metal frame for 9-slice scaling; centre is a dark translucent panel."""
    c = cv(size, size)
    steel(c, 0, 0, size, size, 3, "g2")
    inner = border - 4
    c.rect(inner, inner, size - 2 * inner, size - 2 * inner, (15, 14, 23, 225))
    # bevels
    c.hline(1, size - 2, 1, "g4"); c.vline(1, 1, size - 2, "g4")
    c.hline(1, size - 2, size - 2, "g0"); c.vline(size - 2, 1, size - 2, "g0")
    c.frame(0, 0, size, size, "ink")
    c.frame(inner - 1, inner - 1, size - 2 * inner + 2, size - 2 * inner + 2, "ink")
    c.hline(inner - 1, size - inner, size - inner, "g4")
    c.vline(size - inner, inner - 1, size - inner, "g4")
    for rx, ry in ((5, 5), (size - 7, 5), (5, size - 7), (size - 7, size - 7)):
        c.rect(rx, ry, 2, 2, "g5")
        c.px(rx, ry, "g7")
        c.px(rx + 1, ry + 1, "g1")
    # hazard tick marks on the corners
    for i in range(3):
        c.px(9 + i, 2, "yel2"); c.px(size - 12 + i, size - 3, "yel2")
    return c


def inset9(size=24, border=8) -> Canvas:
    """Recessed dark display (LCD-like) for numbers; 9-slice."""
    c = cv(size, size, "ink2")
    c.rect(2, 2, size - 4, size - 4, (16, 30, 22, 255))
    c.hline(0, size - 1, 0, "g0"); c.vline(0, 0, size - 1, "g0")
    c.hline(0, size - 1, size - 1, "g4"); c.vline(size - 1, 0, size - 1, "g4")
    c.hline(1, size - 2, 1, "ink"); c.vline(1, 1, size - 2, "ink")
    return c


def button9(state: str, size=24) -> Canvas:
    """Menu button 9-slice (border 8): normal / hot."""
    c = cv(size, size)
    base = "g2" if state == "normal" else "ol2"
    hi = "g4" if state == "normal" else "ol4"
    lo = "g0" if state == "normal" else "ol0"
    c.rect(0, 0, size, size, base)
    c.hline(1, size - 2, 1, hi); c.vline(1, 1, size - 2, hi)
    c.hline(1, size - 2, size - 2, lo); c.vline(size - 2, 1, size - 2, lo)
    c.frame(0, 0, size, size, "ink")
    if state == "hot":
        c.px(2, 2, "ol5"); c.px(size - 3, 2, "ol5")
    return c


def inv_slot(s=40) -> Canvas:
    c = cv(s, s, "g1")
    c.rect(2, 2, s - 4, s - 4, "g0")
    for y in range(3, s - 3):
        for x in range(3, s - 3):
            if (x + y) % 6 == 0:
                c.px(x, y, (59, 61, 78, 255))
    c.hline(0, s - 1, 0, "ink"); c.vline(0, 0, s - 1, "ink")
    c.hline(1, s - 2, 1, "ink2"); c.vline(1, 1, s - 2, "ink2")
    c.hline(0, s - 1, s - 1, "g3"); c.vline(s - 1, 0, s - 1, "g3")
    c.hline(1, s - 2, s - 2, "g2"); c.vline(s - 2, 1, s - 2, "g2")
    return c


def cursor(frame: int, s=44) -> Canvas:
    """Corner-bracket selection cursor, pulsing outwards (4 frames)."""
    c = cv(s, s)
    o = [0, 1, 1, 0][frame]
    col = ["yel2", "yel3", "yel3", "yel2"][frame]
    L = 9
    for (x, y, dx, dy) in ((o, o, 1, 1), (s - 1 - o, o, -1, 1), (o, s - 1 - o, 1, -1), (s - 1 - o, s - 1 - o, -1, -1)):
        for i in range(L):
            for t in range(2):
                c.px(x + dx * i, y + dy * t, col)
                c.px(x + dx * t, y + dy * i, col)
    c.outline("ink")
    return c


def menu_arrow(frame: int) -> Canvas:
    c = cv(16, 16)
    o = [0, 1, 2, 1][frame]
    art = ["oo......",
           "oYoo....",
           "oYYYoo..",
           "oYYYYYoo",
           "oYYYYYoo",
           "oYYYoo..",
           "oYoo....",
           "oo......"]
    c.stamp(art, 3 + o, 4, {"o": "ink", "Y": "yel2"})
    c.px(4 + o, 7, "yel3"); c.px(5 + o, 7, "yel3")
    return c


ICON_CLOCK = [
    "....oooooo....",
    "..ooWWWWWWoo..",
    ".oWWWWRWWWWWo.",
    ".oWWWWRWWWWWo.",
    "oWWWWWRWWWWWWo",
    "oWWWWWRWWWWWWo",
    "oWWWWWRRRRWWWo",
    "oWWWWWWWWWWWWo",
    "oWWWWWWWWWWWWo",
    ".oWWWWWWWWWWo.",
    ".oGWWWWWWWWGo.",
    "..ooGGGGGGoo..",
    "....oooooo....",
]
ICON_FLAG = [
    "oo...........",
    "oSoooooooo...",
    "oSRRRRRRRRo..",
    "oSRRRYRRRRRo.",
    "oSRRYYYRRRo..",
    "oSRRRYRRRRRo.",
    "oSRRRRRRRRo..",
    "oSoooooooo...",
    "oSo..........",
    "oSo..........",
    "oSo..........",
    "oSo..........",
    "ooo..........",
]


def icon(art, cmap, size=16) -> Canvas:
    c = cv(size, size)
    c.stamp(art, (size - len(art[0])) // 2, (size - len(art)) // 2, cmap)
    return c


def build_ui() -> Sheet:
    s = Sheet(max_width=256)
    s.add("frame9", frame9().img())
    s.add("inset9", inset9().img())
    s.add("button9", button9("normal").img())
    s.add("button9_hot", button9("hot").img())
    s.add("inv_slot", inv_slot().img())
    for f in range(4):
        s.add(f"cursor_{f}", cursor(f).img())
    s.anim("cursor", [f"cursor_{f}" for f in range(4)], [160, 90, 160, 90], True)
    for f in range(4):
        s.add(f"menu_arrow_{f}", menu_arrow(f).img())
    s.anim("menu_arrow", [f"menu_arrow_{f}" for f in range(4)], 120, True)
    s.add("icon_timer", icon(ICON_CLOCK, {"o": "ink", "W": "g7", "R": "red1", "G": "g4"}).img())
    s.add("icon_level", icon(ICON_FLAG, {"o": "ink", "S": "g5", "R": "red2", "Y": "yel2"}).img())
    return s


# ------------------------------------------------------------------ side panel background
def panel_bg(w=288, h=720) -> Image.Image:
    c = cv(w, h)
    steel(c, 0, 0, w, h, 9, "g1")
    # vertical seams + rivets every 96 px
    for y in range(0, h, 96):
        c.hline(0, w - 1, y, "ink2")
        c.hline(0, w - 1, y + 1, "g2")
        for x in range(8, w, 40):
            c.px(x, y + 6, "g4"); c.px(x + 1, y + 7, "ink2")
    c.vline(0, 0, h - 1, "ink")
    c.vline(1, 0, h - 1, "g3")
    c.vline(2, 0, h - 1, "g2")
    # hazard stripe on the left edge
    for y in range(h):
        for x in range(3, 9):
            c.px(x, y, "yel1" if ((x + y) // 6) % 2 else "ink2")
    c.vline(9, 0, h - 1, "ink")
    return c.img()


# ------------------------------------------------------------------ logo
def logo(scale=3) -> Image.Image:
    font = ImageFont.truetype(str(FONTS / "blackopsone/BlackOpsOne-Regular.ttf"), 84)
    text = "SOLDIER"
    bbox = font.getbbox(text)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    W, H = tw + 40, th + 40
    m = Image.new("L", (W, H), 0)
    ImageDraw.Draw(m).text((20 - bbox[0], 14 - bbox[1]), text, font=font, fill=255)
    mask = (np.array(m) > 110)
    c = cv(W, H)
    ys, xs = np.nonzero(mask)
    top, bot = ys.min(), ys.max()
    # extrusion (3D depth down-right)
    depth = 6
    for d in range(depth, 0, -1):
        for y, x in zip(ys, xs):
            c.px(x + d // 2, y + d, "ol0" if d > 2 else "ol1")
    # face: olive gradient + camo blotches, two-tone like the original (light top, green bottom)
    camo = np.zeros((H, W))
    rng = np.random.default_rng(7)
    for _ in range(60):
        cx, cy = rng.uniform(0, W), rng.uniform(0, H)
        rx, ry = rng.uniform(5, 14), rng.uniform(3, 7)
        yy, xx = np.ogrid[0:H, 0:W]
        blob = ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 < 1
        camo[blob] = rng.integers(1, 3)
    for y, x in zip(ys, xs):
        t = (y - top) / max(1, bot - top)
        if t < 0.5:
            col = "ol5" if t < 0.18 else "ol4"
        else:
            col = "ol3" if t < 0.8 else "ol2"
        k = camo[y, x]
        if k == 1:
            col = {"ol5": "ol4", "ol4": "ol3", "ol3": "ol2", "ol2": "ol1"}[col]
        elif k == 2 and t > 0.3:
            col = "wd2" if t > 0.6 else "wd3"
        c.px(x, y, col)
    # top-edge highlight and bottom-edge shade on the face
    for y, x in zip(ys, xs):
        if not mask[y - 1, x]:
            c.px(x, y, "g8")
        elif not mask[y + 1, x]:
            c.px(x, y, "ol1")
    # outline (2 px)
    c.outline("ink", diagonal=True)
    c.outline("ink")
    # scratches / wear
    for i in range(40):
        x, y = int(hash2(i, 0, 5) * W), int(hash2(i, 1, 5) * H)
        if mask[y % H, x % W]:
            c.px(x, y, "g6")
            c.px(x + 1, y, "g4")
    img = c.img()
    bb = img.getbbox()
    img = img.crop((bb[0] - 2, bb[1] - 2, bb[2] + 2, bb[3] + 2))
    return img.resize((img.width * scale, img.height * scale), Image.NEAREST)


# ------------------------------------------------------------------ menu background
MENU_MAP = [
    "WWWWWWWWWWWWWWW",
    "WDDDDD....~~~.W",
    "W....D.B..~s~.W",
    "W.b..X....~~~GW",
    "W......K......W",
    "WB...WWWW..b..W",
    "W..R.W..P.B.E.W",
    "W....W........W",
    "WWWWWWWWWWWWWWW",
]


def render_map(m, ox=0, oy=0, W=640, H=360) -> Image.Image:
    im = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    h, w = len(m), len(m[0])

    def at(x, y):
        return m[y][x] if 0 <= y < h and 0 <= x < w else "W"
    t = {}

    def g(name, fn):
        if name not in t:
            t[name] = fn().img()
        return t[name]
    for y in range(h):
        for x in range(w):
            ch = at(x, y)
            px, py = x * 32 + ox, y * 32 + oy
            if ch in "WD":
                mask = sum(b for b, (dx, dy) in ((1, (0, -1)), (2, (1, 0)), (4, (0, 1)), (8, (-1, 0)))
                           if at(x + dx, y + dy) == ch)
                pal = st.WALL_PAL if ch == "W" else st.DARK_PAL
                im.alpha_composite(g(f"{ch}{mask}", lambda: st.wall(mask, pal, 3 if ch == "W" else 9)), (px, py))
                continue
            v = (x * 7 + y * 13) % 4 if (x * 5 + y * 3) % 7 == 0 else 0
            base = {"~": ("water", lambda: st.water(0)), "s": ("sunk", lambda: st.sunk_box(0)),
                    "E": ("exit", lambda: st.exit_tile(2)), "b": ("btn", lambda: st.button(0)),
                    "X": ("door", lambda: st.door(0)), "R": ("rdoor", lambda: st.door(0, "red")),
                    "G": ("gbase", st.guillotine_base)}.get(ch, (f"f{v}", lambda: st.floor(v)))
            im.alpha_composite(g(*base), (px, py))
            if ch not in "~sG":
                if at(x, y - 1) in "WD":
                    im.alpha_composite(g("sn", lambda: st.shadow("n")), (px, py))
                if at(x - 1, y) in "WD":
                    im.alpha_composite(g("sw", lambda: st.shadow("w")), (px, py))
            if ch in "~s":
                for side, (dx, dy) in zip("nesw", ((0, -1), (1, 0), (0, 1), (-1, 0))):
                    if at(x + dx, y + dy) not in "~s":
                        im.alpha_composite(g("we" + side, lambda: st.water_edge(side)), (px, py))
            if ch == "B":
                im.alpha_composite(g("box", st.box), (px, py))
            if ch == "K":
                im.alpha_composite(g("key", lambda: st.key("yellow")), (px, py))
            if ch == "P":
                im.alpha_composite(sc.facing("d").img(), (px, py))
    for y in range(h):
        for x in range(w):
            if at(x, y) == "G":
                im.alpha_composite(st.guillotine_top(0).img(), (x * 32 + ox, y * 32 + oy - 16))
    return im


def menu_bg() -> Image.Image:
    W, H = 427, 240
    im = render_map(MENU_MAP, -26, -30, W, H)
    a = np.array(im).astype(np.float32)
    yy, xx = np.mgrid[0:H, 0:W]
    # darken + cool tint, vignette, spotlight centred lower-middle
    d = np.sqrt(((xx - W / 2) / (W * 0.62)) ** 2 + ((yy - H * 0.58) / (H * 0.75)) ** 2)
    light = np.clip(1.05 - d, 0.0, 1.0) * 0.55 + 0.12
    # top band darker for the logo
    light *= np.clip(0.55 + yy / H * 0.6, 0, 1)
    a[:, :, 0] = a[:, :, 0] * light * 0.92
    a[:, :, 1] = a[:, :, 1] * light * 0.97
    a[:, :, 2] = a[:, :, 2] * light * 1.08
    # posterise the lighting with an ordered dither to keep a pixel-art look
    a = np.clip(a, 0, 255)
    bayer = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0 - 0.5
    q = 12.0
    a[:, :, :3] = np.floor(a[:, :, :3] / q + 0.5 + bayer[yy % 4, xx % 4][:, :, None] * 0.6) * q
    a = np.clip(a, 0, 255).astype(np.uint8)
    a[:, :, 3] = 255
    # scanlines (subtle)
    a[1::2, :, :3] = (a[1::2, :, :3].astype(np.int32) * 92 // 100).astype(np.uint8)
    img = Image.fromarray(a, "RGBA")
    # dust motes
    dr = ImageDraw.Draw(img)
    for i in range(40):
        x, y = int(hash2(i, 0, 17) * W), int(hash2(i, 1, 17) * H)
        b = 120 + int(hash2(i, 2, 17) * 100)
        dr.point((x, y), fill=(b, b, int(b * 0.9), 255))
    return img.resize((W * 3, H * 3), Image.NEAREST).crop((0, 0, 1280, 720))


def build_all(out: Path):
    build_ui().save(out / "ui.png")
    save_png(panel_bg(), out / "panel_bg.png")
    save_png(logo(), out / "logo.png")
    save_png(menu_bg(), out / "menu_bg.png")
    save_png(app_icon(), out / "icon.png")


def app_icon() -> Image.Image:
    """256x256 window icon: the soldier's head and shoulders on a round olive badge, x8 pixels."""
    c = cv(32, 32)
    c.ellipse(15.5, 15.5, 15, 15, "ink")
    c.ellipse(15.5, 15.5, 14, 14, "g1")
    c.ellipse(15.5, 15.5, 12, 12, "g2")
    c.ring(15.5, 15.5, 13, 14, "yel1")
    head = sc.facing("d").shifted(0, 7)
    head.a[29:] = 0
    m = np.zeros((32, 32), bool)
    yy, xx = np.mgrid[0:32, 0:32]
    m = (xx - 15.5) ** 2 + (yy - 15.5) ** 2 <= 13.5 ** 2
    head.a[~m] = 0
    c.paste(head)
    return c.scaled(8)

