"""V Software Entertainment branding: emblem, wordmark, lockups, "presents".

Smooth (anti-aliased) artwork rendered with 4x supersampling. Outputs to shared/branding/.
    python -I tools/assets/gen_branding.py
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FONTS, ROOT, save_png  # noqa: E402

OUT = ROOT / "shared" / "branding"
SS = 4  # supersampling

CYAN = (64, 196, 255)
DEEP = (28, 46, 140)
STEEL_HI = (246, 249, 255)
STEEL_LO = (150, 165, 190)


def font(weight: int, size: int):
    f = ImageFont.truetype(str(FONTS / "orbitron/Orbitron.ttf"), size)
    f.set_variation_by_axes([weight])
    return f


def lin_grad(w, h, stops, vertical=True):
    """stops: list of (pos 0..1, (r,g,b)). Returns RGB numpy (h,w,3)."""
    t = np.linspace(0, 1, h if vertical else w)
    out = np.zeros((len(t), 3))
    for ch in range(3):
        out[:, ch] = np.interp(t, [s[0] for s in stops], [s[1][ch] for s in stops])
    if vertical:
        return np.repeat(out[:, None, :], w, axis=1)
    return np.repeat(out[None, :, :], h, axis=0)


def fill_mask(mask: Image.Image, rgb: np.ndarray) -> Image.Image:
    im = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    im.putalpha(mask)
    return im


def shadowed(layer: Image.Image, offset=(0, 10), blur=14, alpha=150, glow=None) -> Image.Image:
    w, h = layer.size
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    a = layer.getchannel("A")
    if glow:
        g = Image.new("RGBA", (w, h), glow + (0,))
        g.putalpha(a.filter(ImageFilter.GaussianBlur(blur * 1.6)).point(lambda v: int(v * 0.55)))
        out.alpha_composite(g)
    sh = Image.new("RGBA", (w, h), (0, 0, 8, 0))
    sa = Image.new("L", (w, h), 0)
    sa.paste(a, offset)
    sh.putalpha(sa.filter(ImageFilter.GaussianBlur(blur)).point(lambda v: int(v * alpha / 255)))
    out.alpha_composite(sh)
    out.alpha_composite(layer)
    return out


# ------------------------------------------------------------------ emblem
def emblem(size=512) -> Image.Image:
    S = size * SS
    pad = S * 0.08
    W = S
    # V outline polygon (angular, with a notch at the top centre)
    L, R, T, B = pad, W - pad, pad * 1.4, S - pad * 1.1
    cx = W / 2
    arm = (R - L) * 0.24
    outer = [(L, T), (L + arm, T), (cx, B - (B - T) * 0.36), (R - arm, T), (R, T), (cx + arm * 0.18, B),
             (cx - arm * 0.18, B)]
    m = Image.new("L", (W, S), 0)
    ImageDraw.Draw(m).polygon(outer, fill=255)
    # facets: left arm lighter, right arm darker (light from top-left)
    left = Image.new("L", (W, S), 0)
    ImageDraw.Draw(left).polygon([(0, 0), (cx, 0), (cx, S), (0, S)], fill=255)
    left = ImageChops.multiply(left, m)
    right = ImageChops.subtract(m, left)
    g_left = lin_grad(W, S, [(0, (120, 220, 255)), (0.55, CYAN), (1, (30, 90, 200))])
    g_right = lin_grad(W, S, [(0, (60, 120, 230)), (0.6, DEEP), (1, (14, 22, 80))])
    body = fill_mask(left, g_left)
    body.alpha_composite(fill_mask(right, g_right))
    # inner bevel: shrink mask and lighten edges
    edge = m.filter(ImageFilter.MinFilter(int(S * 0.025) | 1))
    rim = ImageChops.subtract(m, edge)
    rim_col = Image.new("RGBA", (W, S), (220, 245, 255, 0))
    rim_col.putalpha(ImageChops.multiply(rim, left).point(lambda v: int(v * 0.85)))
    body.alpha_composite(rim_col)
    rim_dark = Image.new("RGBA", (W, S), (5, 10, 40, 0))
    rim_dark.putalpha(ImageChops.multiply(rim, right).point(lambda v: int(v * 0.6)))
    body.alpha_composite(rim_dark)
    # specular streak across the top-left arm
    spec = Image.new("L", (W, S), 0)
    ImageDraw.Draw(spec).polygon([(L, T), (L + arm * 0.55, T), (cx - arm * 0.6, B - (B - T) * 0.25),
                                  (cx - arm * 0.85, B - (B - T) * 0.32)], fill=110)
    spec = ImageChops.multiply(spec.filter(ImageFilter.GaussianBlur(S * 0.01)), edge)
    sp = Image.new("RGBA", (W, S), (255, 255, 255, 0))
    sp.putalpha(spec)
    body.alpha_composite(sp)
    # thin outer stroke
    stroke = ImageChops.subtract(m.filter(ImageFilter.MaxFilter(int(S * 0.012) | 1)), m)
    st = Image.new("RGBA", (W, S), (10, 14, 40, 0))
    st.putalpha(stroke)
    body.alpha_composite(st)
    # orbit ring behind (swoosh)
    ring = Image.new("L", (W, S), 0)
    d = ImageDraw.Draw(ring)
    d.ellipse([S * 0.03, S * 0.30, S * 0.97, S * 0.80], outline=255, width=int(S * 0.022))
    cut = Image.new("L", (W, S), 0)
    ImageDraw.Draw(cut).rectangle([0, 0, W, S * 0.55], fill=255)
    ring_back = ImageChops.multiply(ring, cut)  # upper half of the ring goes behind the V
    ring_front = ImageChops.subtract(ring, cut)
    ring_front = ImageChops.subtract(ring_front, m.filter(ImageFilter.MaxFilter(int(S * 0.03) | 1)).point(
        lambda v: 0))  # keep front part as is
    rg = lin_grad(W, S, [(0, (255, 255, 255)), (0.5, (170, 230, 255)), (1, (90, 170, 255))], vertical=False)
    out = Image.new("RGBA", (W, S), (0, 0, 0, 0))
    back = fill_mask(ring_back, rg * 0.7)
    out.alpha_composite(back)
    out.alpha_composite(body)
    front = fill_mask(ImageChops.subtract(ring_front, m.point(lambda v: 0)), rg)
    # front part of the ring passes over the V
    out.alpha_composite(front)
    out = out.resize((size, size), Image.LANCZOS)
    return out


# ------------------------------------------------------------------ text
def text_layer(text, weight, size, spacing, grad_stops, stroke=0, stroke_col=(10, 14, 40)) -> Image.Image:
    f = font(weight, size * SS)
    widths = [f.getbbox(ch)[2] - f.getbbox(ch)[0] if ch != " " else size * SS * 0.4 for ch in text]
    total = sum(widths) + spacing * SS * (len(text) - 1)
    asc, desc = f.getmetrics()
    H = asc + desc + 8 * SS
    W = int(total + 16 * SS)
    m = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(m)
    x = 8 * SS
    for ch, w in zip(text, widths):
        if ch != " ":
            bx = f.getbbox(ch)[0]
            d.text((x - bx, 4 * SS), ch, font=f, fill=255)
        x += w + spacing * SS
    bb = m.getbbox()
    m = m.crop((bb[0] - 6 * SS, bb[1] - 6 * SS, bb[2] + 6 * SS, bb[3] + 6 * SS))
    W, H = m.size
    g = lin_grad(W, H, grad_stops)
    layer = fill_mask(m, g)
    if stroke:
        sm = m.filter(ImageFilter.MaxFilter(stroke * SS * 2 + 1))
        base = Image.new("RGBA", (W, H), stroke_col + (0,))
        base.putalpha(sm)
        base.alpha_composite(layer)
        layer = base
    return layer.resize((W // SS, H // SS), Image.LANCZOS)


def software() -> Image.Image:
    return text_layer("SOFTWARE", 900, 120, 10,
                      [(0, STEEL_HI), (0.48, (214, 224, 240)), (0.52, STEEL_LO), (1, (205, 215, 232))], stroke=2)


def entertainment() -> Image.Image:
    return text_layer("ENTERTAINMENT", 700, 46, 20, [(0, (150, 230, 255)), (1, CYAN)], stroke=1)


def presents() -> Image.Image:
    return text_layer("presents", 500, 56, 14, [(0, (230, 236, 245)), (1, (170, 190, 215))])


def rule(width, height=4) -> Image.Image:
    g = lin_grad(width, height, [(0, (0, 0, 0)), (0.5, CYAN), (1, (0, 0, 0))], vertical=False)
    a = np.interp(np.linspace(0, 1, width), [0, 0.15, 0.85, 1], [0, 255, 255, 0])
    im = Image.fromarray(g.astype(np.uint8), "RGB").convert("RGBA")
    im.putalpha(Image.fromarray(np.repeat(a[None, :], height, 0).astype(np.uint8), "L"))
    return im


def lockup_horizontal() -> Image.Image:
    em = emblem(300)
    sw = software()
    en = entertainment()
    W = 1024
    gap = 8
    text_w = max(sw.width, en.width)
    scale = (W - em.width - gap - 40) / text_w
    if scale < 1:
        sw = sw.resize((int(sw.width * scale), int(sw.height * scale)), Image.LANCZOS)
        en = en.resize((int(en.width * scale), int(en.height * scale)), Image.LANCZOS)
    H = em.height + 40
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    img.alpha_composite(em, (10, 20))
    tx = 10 + em.width + gap
    ty = H // 2 - (sw.height + en.height + 14) // 2
    img.alpha_composite(sw, (tx, ty))
    r = rule(sw.width, 3)
    img.alpha_composite(r, (tx, ty + sw.height + 4))
    img.alpha_composite(en, (tx + (sw.width - en.width) // 2, ty + sw.height + 12))
    img = shadowed(img, offset=(0, 8), blur=10, alpha=140, glow=(40, 150, 255))
    bb = img.getbbox()
    return img.crop((max(0, bb[0] - 12), max(0, bb[1] - 12), min(W, bb[2] + 12), min(H, bb[3] + 12)))


def lockup_stacked() -> Image.Image:
    em = emblem(420)
    sw = software()
    en = entertainment()
    W = 1024
    s = min(1.0, (W - 80) / sw.width)
    sw = sw.resize((int(sw.width * s), int(sw.height * s)), Image.LANCZOS)
    H = em.height + sw.height + en.height + 80
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    img.alpha_composite(em, ((W - em.width) // 2, 10))
    y = em.height + 10
    img.alpha_composite(sw, ((W - sw.width) // 2, y))
    y += sw.height + 6
    r = rule(int(sw.width * 0.9), 3)
    img.alpha_composite(r, ((W - r.width) // 2, y))
    y += 12
    img.alpha_composite(en, ((W - en.width) // 2, y))
    img = shadowed(img, offset=(0, 8), blur=10, alpha=140, glow=(40, 150, 255))
    bb = img.getbbox()
    return img.crop((0, max(0, bb[1] - 12), W, min(H, bb[3] + 12)))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    save_png(lockup_horizontal(), OUT / "vse_logo.png")
    save_png(lockup_stacked(), OUT / "vse_logo_stacked.png")
    save_png(shadowed(emblem(512), offset=(0, 10), blur=12, alpha=130), OUT / "vse_emblem.png")
    save_png(shadowed(software(), offset=(0, 6), blur=6, alpha=130), OUT / "vse_word_software.png")
    save_png(shadowed(entertainment(), offset=(0, 4), blur=4, alpha=130), OUT / "vse_word_entertainment.png")
    save_png(rule(900, 4), OUT / "vse_rule.png")
    save_png(shadowed(presents(), offset=(0, 4), blur=5, alpha=120), OUT / "presents.png")
    print("branding ->", OUT)


if __name__ == "__main__":
    main()
