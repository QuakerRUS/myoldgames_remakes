"""Application / window icons (.ico) for every program, built from the generated sprites.

Outputs:
  soldier/data/gfx/soldier.ico        Soldier game icon (from soldier/data/gfx/icon.png)
  soldier/editor/res/app.ico          Soldier icon + pencil badge
  piton/data/gfx/icon.png             256x256 Piton icon (snake head on a grass tile; runtime window icon)
  piton/game/res/app.ico              Piton icon
  piton/editor/res/app.ico            Piton icon + pencil badge
  keyboard-simulator/app/res/app.ico  from keyboard-simulator/data/gfx/icon*.png
  tools/painter/res/app.ico           painter's palette with a brush
  tools/engine-demo/res/app.ico       bold "V" on a dark tile

Every .ico holds the sizes 16, 24, 32, 48, 64 and 256. Each size is rendered from a 256 px master
(pixel-art sources are first upscaled with NEAREST) and downsampled with LANCZOS; hand-made small
PNGs (icon_16/32/48.png) are used as-is when they exist.

Depends on gen_piton_gfx.py (snake.png) and gen_soldier_gfx.py / gen_kbsim_gfx.py (icon.png).

Run: python -I tools/assets/gen_icons.py
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image, ImageDraw, ImageFilter, ImageFont  # noqa: E402

from common import FONTS, ROOT, hexrgb, rel, save_png  # noqa: E402

SIZES = (16, 24, 32, 48, 64, 256)
M = 256  # master size


# ----------------------------------------------------------------------------
# helpers
# ----------------------------------------------------------------------------
def to_master(img: Image.Image) -> Image.Image:
    img = img.convert("RGBA")
    if img.size != (M, M):
        img = img.resize((M, M), Image.NEAREST if img.width < M else Image.LANCZOS)
    return img


def downsample(master: Image.Image, s: int) -> Image.Image:
    if s == master.width:
        return master.copy()
    # premultiplied resize avoids dark fringes around transparent edges
    return master.convert("RGBa").resize((s, s), Image.LANCZOS).convert("RGBA")


def save_ico(path: Path, images: dict[int, Image.Image]):
    """Multi-size .ico. Pillow's ICO writer resizes the base image for each size, so pass every
    hand-rendered size through append_images (Pillow >= 8 picks exact-size frames from it)."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    big = images[256]
    others = [images[s] for s in SIZES if s != 256]
    big.save(path, format="ICO", sizes=[(s, s) for s in SIZES], append_images=others)
    print("  ", rel(path))


def build_sizes(master: Image.Image, badge=None, small: dict[int, Image.Image] | None = None):
    """Per-size images; badge(size) -> RGBA overlay of that size (or None to omit)."""
    out = {}
    for s in SIZES:
        if small and s in small:
            im = small[s].convert("RGBA")
        else:
            im = downsample(master, s)
        if badge:
            b = badge(s)
            if b is not None:
                im = im.copy()
                im.alpha_composite(b)
        out[s] = im
    return out


# ----------------------------------------------------------------------------
# pencil badge (drawn supersampled at each target size for crispness)
# ----------------------------------------------------------------------------
def pencil_badge(s: int):
    if s < 24:
        frac = 0.56  # at 16 px a bigger, simpler pencil stays legible
    else:
        frac = 0.50
    SS = 8
    S = s * SS
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    bw = S * frac
    x0, y0 = S - bw, S - bw
    # badge disc
    pad = bw * 0.02
    out_c = (24, 30, 44, 255)
    d.ellipse([x0 + pad, y0 + pad, S - pad, S - pad], fill=out_c)
    ring = bw * (0.075 if s >= 24 else 0.06)
    d.ellipse([x0 + pad + ring, y0 + pad + ring, S - pad - ring, S - pad - ring], fill=(250, 248, 240, 255))

    # pencil along the diagonal from top-right (eraser) to bottom-left (tip) of the disc
    cx, cy = x0 + bw / 2, y0 + bw / 2
    L = bw * 0.80
    W = bw * (0.22 if s >= 24 else 0.30)
    import math
    ang = math.radians(135)  # direction eraser -> tip (down-left)
    ux, uy = math.cos(ang), math.sin(ang)
    px_, py_ = -uy, ux  # perpendicular

    def P(t, w):  # t along axis from -L/2..L/2, w across
        return (cx + ux * t + px_ * w, cy + uy * t + py_ * w)

    def quad(t0, t1, w, col):
        d.polygon([P(t0, -w), P(t1, -w), P(t1, w), P(t0, w)], fill=col)

    a, b = -L / 2, L / 2
    tip_len = L * 0.28
    ol = W * 0.16
    olc = (24, 30, 44, 255)
    # outline silhouette
    d.polygon([P(a - ol, -W / 2 - ol), P(b - tip_len, -W / 2 - ol), P(b + ol * 1.6, 0),
               P(b - tip_len, W / 2 + ol), P(a - ol, W / 2 + ol)], fill=olc)
    er = L * 0.16
    quad(a, a + er, W / 2, (238, 120, 140, 255))          # eraser
    quad(a + er, a + er + L * 0.07, W / 2, (196, 200, 210, 255))  # ferrule
    body0 = a + er + L * 0.07
    quad(body0, b - tip_len, W / 2, (246, 196, 52, 255))  # yellow body
    quad(body0, b - tip_len, W / 6, (255, 222, 110, 255) if s >= 32 else (246, 196, 52, 255))
    d.polygon([P(b - tip_len, -W / 2), P(b, 0), P(b - tip_len, W / 2)], fill=(232, 196, 150, 255))  # wood
    g = tip_len * 0.38
    d.polygon([P(b - g, -W / 2 * g / tip_len), P(b, 0), P(b - g, W / 2 * g / tip_len)], fill=(40, 40, 48, 255))
    return im.resize((s, s), Image.LANCZOS)


# ----------------------------------------------------------------------------
# Soldier
# ----------------------------------------------------------------------------
def soldier_master() -> Image.Image:
    return to_master(Image.open(ROOT / "soldier" / "data" / "gfx" / "icon.png"))


# ----------------------------------------------------------------------------
# Piton: snake head on a grass tile (pixel art on a 32x32 grid, upscaled x8)
# ----------------------------------------------------------------------------
PIT = {k: hexrgb(v) for k, v in {
    "g0": "#1f4a1e", "g1": "#2c6a28", "g2": "#3a8530", "g3": "#4c9c3a", "g4": "#6bb84a", "g5": "#94d266",
}.items()}


BAYER4 = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))


def piton_icon() -> Image.Image:
    gfx = ROOT / "piton" / "data" / "gfx"
    sheet = Image.open(gfx / "snake.png").convert("RGBA")
    frames = json.loads((gfx / "snake.json").read_text(encoding="utf-8"))["frames"]

    def frame(n):
        f = frames[n]
        return sheet.crop((f["x"], f["y"], f["x"] + f["w"], f["y"] + f["h"]))

    G = 32
    tile = Image.new("RGBA", (G, G), (0, 0, 0, 0))
    mask = Image.new("L", (G, G), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, G - 1, G - 1], 6, fill=255)
    # vertical gradient g4 (top) -> g2 (bottom), in bands
    bands = [PIT["g4"], PIT["g3"], PIT["g3"], PIT["g2"]]
    fill = Image.new("RGBA", (G, G))
    dd = ImageDraw.Draw(fill)
    for y in range(G):
        t = y / (G - 1)
        # dithered gradient between bands
        pos = t * (len(bands) - 1)
        i = min(int(pos), len(bands) - 2)
        fr = pos - i
        for x in range(G):
            thr = (BAYER4[y % 4][x % 4] + 0.5) / 16
            dd.point((x, y), fill=bands[i + 1] if fr > thr else bands[i])
    # sprinkle a few lighter grass blades (deterministic)
    for x, y in ((5, 6), (24, 4), (8, 27), (27, 25), (15, 3), (3, 18), (28, 15)):
        dd.point((x, y), fill=PIT["g5"])
        dd.point((x, y + 1), fill=PIT["g4"])
    tile.paste(fill, (0, 0), mask)
    # border: dark outer ring + light inner highlight on top/left
    edge = Image.new("L", (G, G), 0)
    ImageDraw.Draw(edge).rounded_rectangle([0, 0, G - 1, G - 1], 6, outline=255, width=1)
    tile.paste(Image.new("RGBA", (G, G), PIT["g0"]), (0, 0), edge)
    hi = Image.new("L", (G, G), 0)
    ImageDraw.Draw(hi).rounded_rectangle([1, 1, G - 2, G - 2], 5, outline=255, width=1)
    lo = hi.copy()
    hi.paste(0, (0, G // 2, G, G))
    lo.paste(0, (0, 0, G, G // 2))
    tile.paste(Image.new("RGBA", (G, G), PIT["g5"]), (0, 0), hi)
    tile.paste(Image.new("RGBA", (G, G), PIT["g1"]), (0, 0), lo)

    # snake: body segment entering from the left, head roughly centred
    snake = Image.new("RGBA", (G, G), (0, 0, 0, 0))
    hx, hy = 7, 3   # head frame visible bbox is x 0..20, y 2..23 -> centred-ish, neck at x=7
    snake.alpha_composite(frame("body_h"), (hx - 24, hy))
    snake.alpha_composite(frame("head_r"), (hx, hy))
    inner = Image.new("L", (G, G), 0)
    ImageDraw.Draw(inner).rounded_rectangle([1, 1, G - 2, G - 2], 5, fill=255)
    sm = Image.new("L", (G, G), 0)
    sm.paste(snake.getchannel("A"), (0, 0), inner)
    snake.putalpha(sm)
    tile.alpha_composite(snake)
    return tile.resize((M, M), Image.NEAREST)


# ----------------------------------------------------------------------------
# Painter: wooden palette + brush
# ----------------------------------------------------------------------------
def painter_icon() -> Image.Image:
    SS = 4
    S = M * SS
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # tile: soft blue-grey with vertical gradient and dark rim
    tile = Image.new("RGBA", (S, S))
    td = ImageDraw.Draw(tile)
    top, bot = (92, 132, 188), (52, 78, 128)
    for y in range(S):
        t = y / (S - 1)
        td.line([(0, y), (S, y)], fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bot)) + (255,))
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).rounded_rectangle([S * .03, S * .03, S * .97, S * .97], S * .18, fill=255)
    im.paste(tile, (0, 0), m)
    rim = Image.new("L", (S, S), 0)
    ImageDraw.Draw(rim).rounded_rectangle([S * .03, S * .03, S * .97, S * .97], S * .18, outline=255, width=int(S * .018))
    im.paste(Image.new("RGBA", (S, S), (28, 40, 70, 255)), (0, 0), rim)

    # palette shape: big ellipse minus a notch, with thumb hole
    pal = Image.new("L", (S, S), 0)
    pd = ImageDraw.Draw(pal)
    pd.ellipse([S * .10, S * .20, S * .86, S * .84], fill=255)
    pd.ellipse([S * .58, S * .56, S * .84, S * .82], fill=0)       # notch (bottom-right bite)
    pd.ellipse([S * .30, S * .55, S * .43, S * .68], fill=0)       # thumb hole
    # shadow
    sh = pal.filter(ImageFilter.GaussianBlur(S * .02))
    shadow = Image.new("RGBA", (S, S), (10, 18, 40, 0))
    shadow.putalpha(sh.point(lambda v: v * 0.55))
    shm = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shm.paste(shadow, (int(S * .015), int(S * .03)))
    tmp = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    tmp.paste(shm, (0, 0), m)
    im.alpha_composite(tmp)
    # outline + wood
    outl = pal.filter(ImageFilter.MaxFilter(int(S * .022) | 1))
    im.paste(Image.new("RGBA", (S, S), (62, 34, 16, 255)), (0, 0), outl)
    wood = Image.new("RGBA", (S, S), (214, 160, 98, 255))
    wd = ImageDraw.Draw(wood)
    for i in range(9):  # gentle grain arcs
        y = S * (.26 + i * .065)
        wd.arc([S * .02, y - S * .25, S * .98, y + S * .25], 200, 340, fill=(196, 140, 80, 255), width=int(S * .008))
    wd.ellipse([S * .16, S * .22, S * .60, S * .44], fill=(230, 184, 124, 255))  # highlight
    wood = wood.filter(ImageFilter.GaussianBlur(S * .006))
    im.paste(wood, (0, 0), pal)
    # paint blobs
    blobs = [
        ((.22, .40), .085, (226, 48, 52)),
        ((.33, .28), .080, (246, 196, 40)),
        ((.48, .25), .080, (60, 170, 72)),
        ((.63, .30), .080, (52, 120, 220)),
        ((.72, .44), .072, (150, 72, 196)),
        ((.20, .58), .062, (250, 250, 248)),
    ]
    for (bx, by), r, col in blobs:
        x, y, rr = S * bx, S * by, S * r
        d.ellipse([x - rr * 1.08, y - rr * 1.0, x + rr * 1.08, y + rr * 1.12], fill=(62, 34, 16, 255))
        d.ellipse([x - rr, y - rr * .92, x + rr, y + rr * 1.02], fill=col + (255,))
        dk = tuple(int(c * .78) for c in col)
        d.chord([x - rr, y - rr * .92, x + rr, y + rr * 1.02], 20, 160, fill=dk + (255,))
        d.ellipse([x - rr * .55, y - rr * .62, x - rr * .05, y - rr * .2], fill=(255, 255, 255, 170))

    # brush: diagonal from bottom-right (handle end) to upper-middle (tip)
    import math
    ang = math.radians(-128)
    ux, uy = math.cos(ang), math.sin(ang)
    px_, py_ = -uy, ux
    ox, oy = S * .84, S * .86  # handle end

    def P(t, w):
        return (ox + ux * t * S + px_ * w * S, oy + uy * t * S + py_ * w * S)

    def seg(t0, t1, w0, w1, col):
        d.polygon([P(t0, -w0), P(t1, -w1), P(t1, w1), P(t0, w0)], fill=col)

    olc = (30, 22, 20, 255)
    o = .014
    # outline pass
    seg(-o, .44, .040 + o, .030 + o, olc)
    seg(.44, .54, .036 + o, .036 + o, olc)
    d.polygon([P(.54, -.040 - o), P(.62, -.044 - o), P(.70 + o * 1.5, 0), P(.62, .044 + o), P(.54, .040 + o)], fill=olc)
    d.ellipse([P(-o, 0)[0] - S * (.040 + o), P(-o, 0)[1] - S * (.040 + o),
               P(-o, 0)[0] + S * (.040 + o), P(-o, 0)[1] + S * (.040 + o)], fill=olc)
    # handle
    d.ellipse([P(0, 0)[0] - S * .040, P(0, 0)[1] - S * .040, P(0, 0)[0] + S * .040, P(0, 0)[1] + S * .040], fill=(170, 34, 40, 255))
    seg(0, .44, .040, .030, (200, 44, 50, 255))
    seg(0, .44, -.005, -.020, (236, 96, 96, 255))
    # ferrule
    seg(.44, .54, .036, .036, (200, 204, 212, 255))
    seg(.44, .54, -.010, -.026, (240, 242, 246, 255))
    # bristles with paint tip
    d.polygon([P(.54, -.040), P(.60, -.043), P(.60, .043), P(.54, .040)], fill=(120, 80, 48, 255))
    d.polygon([P(.60, -.043), P(.62, -.044), P(.70, 0), P(.62, .044), P(.60, .043)], fill=(52, 120, 220, 255))
    return im.resize((M, M), Image.LANCZOS)


# ----------------------------------------------------------------------------
# Engine demo: bold "V" on a dark tile
# ----------------------------------------------------------------------------
def engine_demo_icon() -> Image.Image:
    SS = 4
    S = M * SS
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    tile = Image.new("RGBA", (S, S))
    td = ImageDraw.Draw(tile)
    top, bot = (54, 60, 74), (24, 27, 36)
    for y in range(S):
        t = y / (S - 1)
        td.line([(0, y), (S, y)], fill=tuple(round(a + (b - a) * t) for a, b in zip(top, bot)) + (255,))
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).rounded_rectangle([S * .03, S * .03, S * .97, S * .97], S * .18, fill=255)
    im.paste(tile, (0, 0), m)
    rim = Image.new("L", (S, S), 0)
    ImageDraw.Draw(rim).rounded_rectangle([S * .03, S * .03, S * .97, S * .97], S * .18, outline=255, width=int(S * .016))
    im.paste(Image.new("RGBA", (S, S), (96, 104, 124, 255)), (0, 0), rim)
    d = ImageDraw.Draw(im)
    f = ImageFont.truetype(str(FONTS / "ptsans" / "PT_Sans-Web-Bold.ttf"), int(S * .80))
    bb = d.textbbox((0, 0), "V", font=f)
    x = S * .5 - (bb[0] + bb[2]) / 2
    y = S * .5 - (bb[1] + bb[3]) / 2
    d.text((x, y + S * .02), "V", font=f, fill=(8, 10, 16, 200))
    d.text((x, y), "V", font=f, fill=(255, 176, 48, 255))
    return im.resize((M, M), Image.LANCZOS)


# ----------------------------------------------------------------------------
def main():
    print("icons:")
    sol = soldier_master()
    save_ico(ROOT / "soldier" / "data" / "gfx" / "soldier.ico", build_sizes(sol))
    save_ico(ROOT / "soldier" / "editor" / "res" / "app.ico", build_sizes(sol, pencil_badge))

    pit = piton_icon()
    save_png(pit, ROOT / "piton" / "data" / "gfx" / "icon.png")
    print("  ", rel(ROOT / "piton" / "data" / "gfx" / "icon.png"))
    save_ico(ROOT / "piton" / "game" / "res" / "app.ico", build_sizes(pit))
    save_ico(ROOT / "piton" / "editor" / "res" / "app.ico", build_sizes(pit, pencil_badge))

    kg = ROOT / "keyboard-simulator" / "data" / "gfx"
    small = {s: Image.open(kg / f"icon_{s}.png") for s in SIZES if (kg / f"icon_{s}.png").exists()}
    save_ico(ROOT / "keyboard-simulator" / "app" / "res" / "app.ico",
             build_sizes(to_master(Image.open(kg / "icon.png")), small=small))

    save_ico(ROOT / "tools" / "painter" / "res" / "app.ico", build_sizes(painter_icon()))
    save_ico(ROOT / "tools" / "engine-demo" / "res" / "app.ico", build_sizes(engine_demo_icon()))


if __name__ == "__main__":
    main()
