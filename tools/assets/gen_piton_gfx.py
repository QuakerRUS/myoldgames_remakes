"""Piton graphics generator (procedural pixel art, deterministic).

Outputs into piton/data/gfx/:
  tiles.png/.json   24x24 grass variants, brick wall autotile (16 masks), shade overlays
  snake.png/.json   24x24 snake pieces: heads (+tongue/eat/dead), body, turns, tails
  rabbit.png/.json  24x24 rabbit frames + animations
  ui.png/.json      HUD icons and a 9-slice panel
  logo.png          PITON title logo (pre-scaled x3, transparent)
  menu_bg.png       1280x720 menu background (320x180 art scaled x4)

Run: python -I tools/assets/gen_piton_gfx.py
"""
from __future__ import annotations

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy as np  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402

from common import FONTS, PREVIEW_DIR, ROOT, Sheet, hexrgb, save_png  # noqa: E402

OUT = ROOT / "piton" / "data" / "gfx"
C = 24  # cell size

# ----------------------------------------------------------------------------
# Palette (all art uses only these colours, plus alpha for shadows)
# ----------------------------------------------------------------------------
PAL = {
    # snake (python: amber skin, dark brown saddles)
    "ol": "#22150d",       # outline
    "s0": "#6e3c1c", "s1": "#9c5a26", "s2": "#c7883a", "s3": "#e3b45c", "s4": "#f6dd96",
    "b0": "#3a2213", "b1": "#5c3519", "b2": "#7d4c22",
    "eye": "#f2cf3a", "pupil": "#120a06", "white": "#fffbea",
    "tg0": "#b8243c", "tg1": "#ff5a6e",
    "m0": "#4a0f1c", "m1": "#922538",
    # grass
    "g0": "#1f4a1e", "g1": "#2c6a28", "g2": "#3a8530", "g3": "#4c9c3a", "g4": "#6bb84a", "g5": "#94d266",
    "gm": "#337a2b", "gl": "#45923a",
    "fw": "#fff4d8", "fy": "#f6d343", "fp": "#ee7f9a",
    # bricks
    "k_ol": "#2a1310", "k_mo": "#5c4640", "k_mo2": "#7d655c",
    "k0": "#6a2a22", "k1": "#8c3a2c", "k2": "#a84a37", "k3": "#c2614a", "k4": "#df8a68",
    "kf0": "#46201a", "kf1": "#5e2a22", "kf2": "#753629",
    # rabbit
    "r_ol": "#3d322d", "r0": "#8e817a", "r1": "#b9ada3", "r2": "#ddd4c9", "r3": "#f4efe6", "r4": "#ffffff",
    "r_pk": "#eea0aa", "r_eye": "#1a1212",
    # sky / scenery
    "sk0": "#4fa3d1", "sk1": "#79c2e0", "sk2": "#a9dde6", "sk3": "#d7eedd", "sk4": "#f5e6b4",
    "sun": "#fff6c8", "hl0": "#86bca6", "hl1": "#6aa68a", "hl2": "#4e8c6e",
    "tr0": "#2a1a12", "tr1": "#4a3022", "tr2": "#6b4a32",
    "lf0": "#163a20", "lf1": "#1f5230", "lf2": "#2f6e3e", "lf3": "#4c9a4a",
}
PRGB = {k: hexrgb(v) for k, v in PAL.items()}
SHADOW = (8, 24, 6)

LIGHT = np.array([-0.45, -0.65, 0.85])
LIGHT = LIGHT / np.linalg.norm(LIGHT)


def rgba_from_names(names: np.ndarray, alpha: np.ndarray | None = None) -> Image.Image:
    """names: 2D object array of palette keys or None."""
    h, w = names.shape
    arr = np.zeros((h, w, 4), np.uint8)
    for k, v in PRGB.items():
        m = names == k
        if m.any():
            arr[m] = v
    if alpha is not None:
        arr[..., 3] = np.where(names == None, alpha, arr[..., 3])  # noqa: E711
    return Image.fromarray(arr, "RGBA")


def grid(w, h):
    y, x = np.mgrid[0:h, 0:w].astype(float)
    return x + 0.5, y + 0.5


def shade_from_height(H):
    gy, gx = np.gradient(H)
    n = np.sqrt(gx * gx + gy * gy + 1)
    return (-gx * LIGHT[0] - gy * LIGHT[1] + LIGHT[2]) / n


def outline_of(mask):
    m = mask
    pad = np.pad(m, 1, constant_values=False)
    inner = pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:]
    return m & ~inner


def shift(mask, dx, dy):
    out = np.zeros_like(mask)
    h, w = mask.shape
    xs0, xs1 = max(0, -dx), min(w, w - dx)
    ys0, ys1 = max(0, -dy), min(h, h - dy)
    out[ys0 + dy:ys1 + dy, xs0 + dx:xs1 + dx] = mask[ys0:ys1, xs0:xs1]
    return out


# ----------------------------------------------------------------------------
# Snake rendering: fields along a polyline centre line
# ----------------------------------------------------------------------------
def path_field(X, Y, pts, svals):
    best_d = np.full(X.shape, 1e9)
    best_s = np.zeros(X.shape)
    best_lat = np.zeros(X.shape)
    for i in range(len(pts) - 1):
        (px, py), (qx, qy) = pts[i], pts[i + 1]
        dx, dy = qx - px, qy - py
        l2 = dx * dx + dy * dy
        if l2 == 0:
            continue
        t = np.clip(((X - px) * dx + (Y - py) * dy) / l2, 0, 1)
        vx, vy = X - (px + t * dx), Y - (py + t * dy)
        d = np.hypot(vx, vy)
        s = svals[i] + t * (svals[i + 1] - svals[i])
        lat = (dx * vy - dy * vx) / math.sqrt(l2)
        u = d < best_d
        best_d = np.where(u, d, best_d)
        best_s = np.where(u, s, best_s)
        best_lat = np.where(u, lat, best_lat)
    return best_d, best_s, best_lat


def arc_pts(cx, cy, r, th0, th1, s0, s1, n=24):
    pts, ss = [], []
    for i in range(n + 1):
        t = i / n
        th = th0 + (th1 - th0) * t
        pts.append((cx + r * math.cos(th), cy + r * math.sin(th)))
        ss.append(s0 + (s1 - s0) * t)
    return pts, ss


BODY_R = 8.0
PERIOD = 12.0


def body_pattern(s, lat, R):
    """0 = skin, 1 = saddle core, 2 = saddle edge (dark)."""
    q = lat / np.maximum(R, 0.5)
    u = np.mod(s, PERIOD) - PERIOD / 2
    v = np.abs(u) / 4.4 + np.abs(q) / 0.70
    pat = np.zeros(s.shape, np.int8)
    pat[v < 1.08] = 2
    pat[v < 0.78] = 1
    u2 = np.mod(s + PERIOD / 2, PERIOD) - PERIOD / 2
    side = (np.abs(u2) < 1.2) & (np.abs(q) > 0.62) & (np.abs(q) < 0.92)
    pat[side] = 2
    return pat


class Layer:
    """Accumulates a heightfield + pattern for snake-like shapes on a canvas."""

    def __init__(self, w, h):
        self.w, self.h = w, h
        self.X, self.Y = grid(w, h)
        self.H = np.zeros((h, w))
        self.pat = np.zeros((h, w), np.int8)
        self.extra = np.full((h, w), None, dtype=object)  # forced colours (eyes...)

    def add_tube(self, pts, svals, rfun=None, pattern=True, hscale=0.85):
        d, s, lat = path_field(self.X, self.Y, pts, svals)
        R = rfun(s) if rfun else np.full(s.shape, BODY_R)
        Hn = np.sqrt(np.maximum(0, R * R - d * d)) * hscale
        Hn[d > R] = 0
        upd = Hn > self.H
        self.H = np.where(upd, Hn, self.H)
        if pattern:
            p = body_pattern(s, lat, R)
            self.pat = np.where(upd, p, self.pat)
        else:
            self.pat = np.where(upd, 0, self.pat)

    def colours(self, with_shadow=True):
        mask = self.H > 0
        sh = shade_from_height(self.H)
        names = np.full(self.H.shape, None, dtype=object)
        # skin
        skin = np.select([sh < 0.50, sh < 0.66, sh < 0.80, sh < 0.93], ["s0", "s1", "s2", "s3"], "s4")
        core = np.select([sh < 0.62, sh < 0.85], ["b0", "b1"], "b2")
        names[mask] = skin[mask]
        m1 = mask & (self.pat == 1)
        names[m1] = core[m1]
        m2 = mask & (self.pat == 2)
        names[m2] = "b0"
        m3 = mask & (self.pat == 3)
        names[m3] = np.where(sh < 0.6, "s2", np.where(sh < 0.85, "s3", "s4"))[m3]
        ex = self.extra != None  # noqa: E711
        names[ex] = self.extra[ex]
        ol = outline_of(mask)
        names[ol] = "ol"
        alpha = np.zeros(self.H.shape, np.uint8)
        if with_shadow:
            shd = shift(mask, 1, 2) & ~mask
            alpha[shd] = 80
        img = rgba_from_names(names)
        a = np.array(img)
        sm = (alpha > 0)
        a[sm, 0:3] = SHADOW
        a[sm, 3] = alpha[sm]
        return Image.fromarray(a, "RGBA")


# Canvas for game pieces: 48x48 with the cell at [12, 36)
CAN = 48
O = 12
DIRV = {"r": (1, 0), "l": (-1, 0), "d": (0, 1), "u": (0, -1)}
EDGE_MID = {"n": (24, 12), "s": (24, 36), "w": (12, 24), "e": (36, 24)}
OUTV = {"n": (0, -1), "s": (0, 1), "w": (-1, 0), "e": (1, 0)}


def crop(img):
    return img.crop((O, O, O + C, O + C))


def piece_straight(horizontal):
    L = Layer(CAN, CAN)
    if horizontal:
        L.add_tube([(-24, 24), (72, 24)], [-36, 60])
    else:
        L.add_tube([(24, -24), (24, 72)], [-36, 60])
    return crop(L.colours())


def piece_turn(a, b):
    """Turn joining edges a and b (n/e/s/w)."""
    L = Layer(CAN, CAN)
    ax, ay = EDGE_MID[a]
    bx, by = EDGE_MID[b]
    cx = ax if a in "ew" else bx
    cy = ay if a in "ns" else by
    # corner is the cell corner shared by both edges
    cx = 36 if "e" in (a + b) else 12
    cy = 12 if "n" in (a + b) else 36
    th0 = math.atan2(ay - cy, ax - cx)
    th1 = math.atan2(by - cy, bx - cx)
    if abs(th1 - th0) > math.pi:
        th1 += 2 * math.pi if th1 < th0 else -2 * math.pi
    pts, ss = arc_pts(cx, cy, 12, th0, th1, 0, 24)
    va, vb = OUTV[a], OUTV[b]
    pre = [(ax + va[0] * 36, ay + va[1] * 36)]
    post = [(bx + vb[0] * 36, by + vb[1] * 36)]
    L.add_tube(pre + pts + post, [-36] + ss + [60])
    return crop(L.colours())


def tail_radius(s):
    a = np.clip((s - 1.0) / 20.0, 0, 1)
    return np.maximum(1.0, BODY_R * (1 - a) ** 0.85)


def piece_tail(tip):
    """Tail whose tip points towards `tip` (u/d/l/r); body attaches on the opposite edge."""
    fx, fy = DIRV[tip]
    L = Layer(CAN, CAN)
    bx, by = 24 - fx * 12, 24 - fy * 12  # back edge midpoint
    pts = [(bx - fx * 36, by - fy * 36), (bx, by), (bx + fx * 21, by + fy * 21)]
    L.add_tube(pts, [-36, 0, 21], rfun=tail_radius)
    return crop(L.colours())


# ---- head ------------------------------------------------------------------
def head_layer(L: Layer, origin, f, k=1.0, mode="closed", tongue=0, neck=36):
    """Draw a snake head into layer L.

    origin: back-edge centre point; f: forward unit vector; k: scale (1 = 24px cell).
    mode: closed | open | eat | dead ; tongue: 0 none, 1 half, 2 full.
    """
    fx, fy = f
    lx, ly = -fy, fx
    X, Y = L.X, L.Y
    a = ((X - origin[0]) * fx + (Y - origin[1]) * fy) / k
    b = ((X - origin[0]) * lx + (Y - origin[1]) * ly) / k
    # neck tube behind
    L.add_tube([(origin[0] - fx * neck * k, origin[1] - fy * neck * k), (origin[0], origin[1])],
               [-neck, 0], rfun=lambda s: np.full(s.shape, BODY_R * k))
    # head profile half-width
    # neck pinch -> wide jaw -> wedge taper -> rounded snout
    t = np.clip((a - 2.0) / 6.0, 0, 1)
    w_neck = np.where(a < 2, 8.0 - 0.6 * np.clip(a, 0, 2), 6.8 + 3.7 * (t * t * (3 - 2 * t)))
    w_wedge = 10.5 - 4.5 * np.clip((a - 8.0) / 9.0, 0, 1) ** 1.4
    w_snout = 6.0 * np.sqrt(np.maximum(0, 1 - (np.clip(a - 17.0, 0, 9)) ** 2 / 3.5 ** 2))
    w = np.where(a < 8, w_neck, np.where(a < 17, w_wedge, w_snout))
    inside = (a >= -0.5) & (np.abs(b) <= w) & (a <= 20.5)
    Hh = np.where(inside, np.sqrt(np.maximum(0, 1 - np.abs(b / np.maximum(w, 0.1)) ** (2 + 1.5 * np.clip(a / 6.0, 0, 1)))) * w * k * (0.85 - 0.3 * np.clip(a / 6.0, 0, 1)), 0)
    # flatten top slightly so the eyes sit on a plateau
    upd = Hh > L.H
    L.H = np.where(upd, Hh, L.H)
    # head markings
    pat = np.zeros(a.shape, np.int8)
    crown = np.abs(a - 5.5) / 4.0 + np.abs(b) / 3.4
    pat[crown < 1.0] = 2
    pat[crown < 0.65] = 1
    spear = (np.abs(b) < 0.9) & (a > 10.0) & (a < 14.0)
    pat[spear] = 2
    # dark eye stripe: from the neck corner through the eye to the snout side
    ab = np.abs(b)
    s1 = (a > 3.0) & (a < 12.0) & (np.abs(ab - (7.8 - (a - 3.0) * 0.244)) < 0.75)
    s2 = (a >= 12.0) & (a < 18.5) & (np.abs(ab - (5.6 - (a - 12.0) * 0.25)) < 0.75)
    pat[(s1 | s2) & (ab < w - 0.6)] = 2
    # light lower jaw scales along the edge, with a dark jaw line inside
    jaw = (a > 9.0) & (a < 19.5) & (ab > w - 2.4) & (ab < w - 0.5) & ~(s1 | s2)
    pat[jaw] = 3
    L.pat = np.where(upd, pat, L.pat)
    head_mask = inside
    ex = L.extra
    # mouth
    if mode in ("open", "eat", "dead"):
        start, slope = {"open": (15.0, 0.55), "eat": (11.5, 0.95), "dead": (16.0, 0.45)}[mode]
        mouth = head_mask & (a > start) & (np.abs(b) < (a - start) * slope)
        inner = mouth & (np.abs(b) < (a - start) * slope - 1.0)
        ex[mouth] = "ol"
        ex[inner] = "m0"
        lip = inner & (np.abs(b) < (a - start) * slope - 2.0) & (a > start + 3)
        ex[lip] = "m1"
        # opening cuts the silhouette in front
        cut = (a > 17.5) & (np.abs(b) < (a - start) * slope - 1.0)
        L.H = np.where(cut & (L.H > 0), 0, L.H)
        ex[cut] = None
    # eyes
    eb = np.abs(b) - 5.6
    if mode == "dead":
        ea = a - 11.5
        cross = ((np.abs(np.abs(ea) - np.abs(eb)) < 0.6) & (np.abs(ea) < 1.6) & (np.abs(eb) < 1.6))
        ex[cross & head_mask] = "ol"
    else:
        rim = (np.abs(a - 11.5) < 2.1) & (np.abs(eb) < 1.6) & head_mask
        ex[rim] = "b0"
        eye = (np.abs(a - 11.5) < 1.1) & (np.abs(eb) < 1.0)
        ex[eye] = "eye"
        pupil = (np.abs(a - 11.5) < 0.5) & (np.abs(eb) < 1.0)
        ex[pupil] = "pupil"
    # nostrils
    nos = (np.abs(a - 18.5) < 0.5) & (np.abs(np.abs(b) - 2.5) < 0.5) & head_mask
    if mode in ("closed",):
        ex[nos] = "b0"
    # tongue
    if tongue or mode == "dead":
        tl = 2.5 if (tongue == 1) else 4.5
        if mode == "dead":
            tl = 4.5
        a0 = 19.5
        stem = (a > a0) & (a < a0 + tl) & (np.abs(b) < 1.0)
        fork = np.zeros_like(stem)
        if tl >= 4.0:
            fork = (a > a0 + tl - 2.0) & (a < a0 + tl + 0.5) & (np.abs(np.abs(b) - 1.5) < 0.5)
            stem = stem & (a < a0 + tl - 2.0)
        tg = stem | fork
        L.tongue = tg
        ex[tg] = "tg0"
        ex[tg & (b < 0) & ~fork] = "tg1"
    return head_mask


def finish_with_tongue(L: Layer, img: Image.Image):
    """Tongue pixels may lie outside the body silhouette: paint them on top."""
    tg = getattr(L, "tongue", None)
    if tg is None:
        return img
    a = np.array(img)
    outside = tg & (L.H <= 0)
    for key in ("tg0", "tg1"):
        m = outside & (L.extra == key)
        a[m] = PRGB[key]
    return Image.fromarray(a, "RGBA")


def piece_head(d, mode="closed", tongue=0):
    fx, fy = DIRV[d]
    L = Layer(CAN, CAN)
    origin = (24 - fx * 12, 24 - fy * 12)
    head_layer(L, origin, (fx, fy), 1.0, mode, tongue)
    img = L.colours()
    img = finish_with_tongue(L, img)
    return crop(img)


# ----------------------------------------------------------------------------
# Grass and bricks
# ----------------------------------------------------------------------------
def grass_tile(seed, flowers=0):
    rng = np.random.default_rng(1000 + seed)
    names = np.full((C, C), "g2", dtype=object)
    # soft mottling: low-frequency value noise (periodic, so neighbours agree at edges)
    xs, ys = grid(C, C)
    ph = rng.uniform(0, 2 * math.pi, 4)
    n = (np.sin(2 * math.pi * xs / C + ph[0]) * np.sin(2 * math.pi * ys / C + ph[1])
         + 0.5 * np.sin(4 * math.pi * (xs + ys) / C + ph[2]))
    rnd = rng.random((C, C))
    names[(rnd < 0.035 + 0.04 * np.clip(n, 0, 1))] = "gl"
    names[(rnd > 0.965 - 0.04 * np.clip(-n, 0, 1))] = "gm"
    # clustered low-contrast tufts of 2-3 px blades (kept inside the tile)
    for _ in range(int(rng.integers(2, 4))):
        cx, cy = int(rng.integers(3, C - 4)), int(rng.integers(4, C - 2))
        for _b in range(int(rng.integers(2, 4))):
            x = cx + int(rng.integers(-1, 3))
            y = cy + int(rng.integers(-1, 2))
            h = int(rng.integers(2, 4))
            lean = int(rng.choice([-1, 0, 0, 1]))
            for i in range(h):
                xx = x + (lean if i == h - 1 else 0)
                names[y - i, xx] = "g3" if i == h - 1 else "gl"
            names[y + 1, x] = "gm"
    for _ in range(flowers):
        x, y = int(rng.integers(3, C - 3)), int(rng.integers(3, C - 3))
        col = ["fw", "fy", "fp"][int(rng.integers(0, 3))]
        names[y - 1, x] = col
        names[y + 1, x] = col
        names[y, x - 1] = col
        names[y, x + 1] = col
        names[y, x] = "fy" if col != "fy" else "fw"
        names[y + 2, x] = "g1"
    return rgba_from_names(names)


def brick_tile(mask):
    """Raised brick block. mask bits: 1=N, 2=E, 4=S, 8=W neighbour is also wall."""
    N, E, S, W = bool(mask & 1), bool(mask & 2), bool(mask & 4), bool(mask & 8)
    names = np.full((C, C), None, dtype=object)
    front = 0 if S else 7  # front face height
    top_h = C - front
    # top face bricks: 12x6, rows offset by 6; world aligned
    for y in range(C):
        for x in range(C):
            row = y // 6
            off = 6 if row % 2 else 0
            bx = (x + off) % 12
            by = y % 6
            if y >= top_h:
                continue
            if by == 5 or bx == 11:
                names[y, x] = "k_mo"
            else:
                bid = (row * 7 + ((x + off) // 12) * 3) % 5
                base = ["k1", "k2", "k2", "k1", "k3"][bid]
                if by == 0 or bx == 0:
                    base = {"k1": "k2", "k2": "k3", "k3": "k4"}[base]
                elif by == 4 or bx == 10:
                    base = {"k1": "k0", "k2": "k1", "k3": "k2"}[base]
                names[y, x] = base
    # chips / texture (deterministic per position, world aligned within the tile)
    for (x, y) in [(3, 2), (16, 9), (8, 14), (20, 20), (13, 3)]:
        if y < top_h and names[y, x] not in ("k_mo",):
            names[y, x] = "k0"
    # front face
    if front:
        for y in range(top_h, C):
            fy = y - top_h
            for x in range(C):
                if fy == 0:
                    names[y, x] = "k_ol"
                elif fy == front - 1:
                    names[y, x] = "k_ol"
                else:
                    row = (fy - 1) // 3
                    off = 6 if row % 2 else 0
                    if (fy - 1) % 3 == 2 or (x + off) % 12 == 11:
                        names[y, x] = "kf0"
                    else:
                        names[y, x] = "kf2" if (fy - 1) % 3 == 0 else "kf1"
    # edges of the top face
    if not N:
        names[0, :] = "k_ol"
        for x in range(C):
            if names[1, x] != "k_mo":
                names[1, x] = "k4"
    if not W:
        names[:, 0] = "k_ol"
        for y in range(1, C):
            if names[y, 1] not in ("k_mo", "k_ol") and y < top_h:
                names[y, 1] = "k4"
    if not E:
        names[:, C - 1] = "k_ol"
        for y in range(1, top_h):
            if names[y, C - 2] != "k_mo":
                names[y, C - 2] = "k0"
        for y in range(top_h, C):
            names[y, C - 2] = "kf0" if names[y, C - 2] != "k_ol" else "k_ol"
    return rgba_from_names(names)


def shade_overlay(kind):
    a = np.zeros((C, C, 4), np.uint8)
    a[..., 0:3] = SHADOW
    al = a[..., 3]
    if kind == "w":
        al[:, 0:3] = 90
        al[:, 3:5] = 45
    elif kind == "n":
        al[0:3, :] = 90
        al[3:4, :] = 45
    elif kind == "nw":
        for y in range(5):
            for x in range(5):
                if x + y < 4:
                    al[y, x] = 90
                elif x + y < 5:
                    al[y, x] = 45
    return Image.fromarray(a, "RGBA")


# ----------------------------------------------------------------------------
# Rabbit
# ----------------------------------------------------------------------------
def ellipse_h(X, Y, cx, cy, rx, ry, ang=0.0, hs=1.0):
    ca, sa = math.cos(ang), math.sin(ang)
    dx, dy = X - cx, Y - cy
    u = (dx * ca + dy * sa) / rx
    v = (-dx * sa + dy * ca) / ry
    r2 = u * u + v * v
    return np.where(r2 < 1, np.sqrt(np.maximum(0, 1 - r2)) * min(rx, ry) * hs, 0)


def rabbit_frame(pose="sit", nose=0, ear=0.0, lift=0):
    W = 24
    X, Y = grid(W, W)
    oy = -lift
    H = np.zeros((W, W))
    part = np.zeros((W, W), np.int8)  # 0 fur, 1 inner ear, 2 tail

    def add(h, p=0):
        nonlocal H, part
        u = h > H
        H = np.where(u, h, H)
        part = np.where(u, p, part)

    # far ear (behind)
    add(ellipse_h(X, Y, 12.8, 5.6 + oy, 1.5, 4.4, math.radians(18) + ear * 0.5, 0.6))
    # body + haunch
    if pose == "hop":
        add(ellipse_h(X, Y, 13.5, 15.0 + oy, 7.5, 4.6, math.radians(-8)))
        add(ellipse_h(X, Y, 17.0, 15.5 + oy, 4.6, 4.2))
    else:
        add(ellipse_h(X, Y, 14.0, 16.0 + oy, 7.0, 5.2))
        add(ellipse_h(X, Y, 16.5, 16.2 + oy, 5.0, 5.0))
    # head
    add(ellipse_h(X, Y, 8.0, 11.5 + oy + (1 if nose == 2 else 0), 4.6, 3.9))
    # near ear
    add(ellipse_h(X, Y, 10.6, 5.2 + oy, 1.7, 4.6, math.radians(10) + ear, 0.8))
    inner = ellipse_h(X, Y, 10.6, 5.2 + oy, 0.75, 3.2, math.radians(10) + ear, 0.8) > 0
    # tail
    add(ellipse_h(X, Y, 21.4, 14.8 + oy, 2.1, 2.1, 0, 1.2), 2)
    # feet
    if pose == "hop":
        add(ellipse_h(X, Y, 20.0, 19.5 + oy, 3.6, 1.3, math.radians(25)))
        add(ellipse_h(X, Y, 8.5, 19.0 + oy, 2.2, 1.1, math.radians(-30)))
    else:
        add(ellipse_h(X, Y, 15.0, 21.0 + oy, 4.2, 1.4))
        add(ellipse_h(X, Y, 8.6, 20.0 + oy, 1.9, 1.2))
    mask = H > 0
    sh = shade_from_height(H * 0.9)
    names = np.full((W, W), None, dtype=object)
    fur = np.select([sh < 0.55, sh < 0.72, sh < 0.86, sh < 0.96], ["r0", "r1", "r2", "r3"], "r4")
    names[mask] = fur[mask]
    names[mask & (part == 2)] = np.where(sh < 0.7, "r2", "r4")[mask & (part == 2)]
    names[inner & mask] = "r_pk"
    ol = outline_of(mask)
    names[ol] = "r_ol"
    # eye, nose, mouth (in pixels)
    hy = int(round(11 + oy + (1 if nose == 2 else 0)))
    names[hy - 1, 6] = "r_eye"
    names[hy, 6] = "r_eye"
    names[hy - 1, 7] = "r_eye"
    names[hy, 7] = "r_eye"
    names[hy - 1, 6] = "r4"
    nx = 3 if nose != 1 else 4
    names[hy + 1, nx + 1] = "r_pk"
    names[hy + 1, nx] = "r_pk" if nose == 1 else names[hy + 1, nx]
    names[hy + 2, nx + 2] = "r0"
    # ground shadow
    a = np.array(rgba_from_names(names))
    sx, sy = grid(W, W)
    rx = 9.5 if lift == 0 else 7.0
    sm = (((sx - 13.5) / rx) ** 2 + ((sy - 21.7) / 1.8) ** 2 < 1) & ~mask
    a[sm, 0:3] = SHADOW
    a[sm, 3] = 80
    return Image.fromarray(a, "RGBA")


def rabbit_icon():
    img = rabbit_frame("sit")
    return img


# ----------------------------------------------------------------------------
# Logo
# ----------------------------------------------------------------------------
def text_mask(text, font_path, size, spacing=0):
    font = ImageFont.truetype(str(font_path), size)
    widths = [font.getbbox(ch)[2] - font.getbbox(ch)[0] for ch in text]
    tw = sum(font.getlength(ch) for ch in text) + spacing * (len(text) - 1) + 20
    img = Image.new("L", (int(tw) + 20, size * 2), 0)
    d = ImageDraw.Draw(img)
    x = 10
    for ch in text:
        d.text((x, 10), ch, font=font, fill=255)
        x += font.getlength(ch) + spacing
    a = np.array(img) > 110
    ys, xs = np.nonzero(a)
    a = a[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    del widths
    return a


def make_logo():
    letters = text_mask("PITON", FONTS / "bungee" / "Bungee-Regular.ttf", 64, spacing=3)
    lh, lw = letters.shape
    W, Hh = lw + 40, lh + 46
    ox, oy = 20, 8
    mask = np.zeros((Hh, W), bool)
    mask[oy:oy + lh, ox:ox + lw] = letters
    X, Y = grid(W, Hh)
    # letter fill: python skin with diamond saddles
    dist_in = np.zeros(mask.shape)
    m = mask.copy()
    k = 0
    while m.any() and k < 12:
        dist_in += m
        m = m & ~outline_of(m)
        k += 1
    # bevel shading from distance-to-edge height map
    Hl = np.minimum(dist_in, 4.0) * 1.3
    sh = shade_from_height(Hl)
    # vertical gradient bias
    gy = (Y - oy) / lh
    sh2 = sh - 0.18 * gy
    names = np.full(mask.shape, None, dtype=object)
    skin = np.select([sh2 < 0.42, sh2 < 0.58, sh2 < 0.74, sh2 < 0.86], ["s0", "s1", "s2", "s3"], "s4")
    names[mask] = skin[mask]
    # diamond lattice saddles
    row = np.floor((Y - oy) / 12.0)
    u = np.mod(X - (row % 2) * 8, 16) - 8
    v = np.mod(Y - oy, 12) - 6
    dia = np.abs(u) / 5.5 + np.abs(v) / 4.2
    core = mask & (dia < 0.75) & (dist_in > 3)
    edge = mask & (dia < 1.0) & (dist_in > 2) & ~core
    names[core] = np.where(sh2 < 0.7, "b1", "b2")[core]
    names[edge] = "b0"
    names[outline_of(mask)] = "ol"

    # snake winding below / through the letters
    L = Layer(W, Hh)
    pts, ss = [], []
    x0, x1 = 6, W - 52
    base_y = oy + lh + 12
    n = 160
    for i in range(n + 1):
        t = i / n
        x = x0 + (x1 - x0) * t
        y = base_y - 2 + 7 * math.sin(t * 2 * math.pi * 2.25 + 0.3)
        pts.append((x, y))
    s = 0.0
    ss = [0.0]
    for i in range(1, len(pts)):
        s += math.dist(pts[i], pts[i - 1])
        ss.append(s)
    total = ss[-1]
    kk = 0.8

    def rf(sv):
        a = np.clip(sv / 40.0, 0, 1)
        return np.maximum(1.0, BODY_R * kk * (0.15 + 0.85 * a ** 0.7))

    L.add_tube(pts, ss, rfun=rf, hscale=0.85)
    # head rising at the end, facing up-right
    endp = pts[-1]
    ang = math.radians(-35)
    f = (math.cos(ang), math.sin(ang))
    # extend neck from end point to the head origin
    hx, hy = endp[0] + f[0] * 10, endp[1] + f[1] * 10
    L.add_tube([endp, (hx, hy)], [total, total + 10], rfun=lambda sv: np.full(sv.shape, BODY_R * kk))
    head_layer(L, (hx, hy), f, kk, "closed", 2, neck=0)
    snake = L.colours(with_shadow=False)
    snake = finish_with_tongue(L, snake)
    sa = np.array(snake)
    smask = sa[..., 3] > 0

    # compose: letters, snake in front of the lower part of the letters
    comp = np.array(rgba_from_names(names))
    comp[smask] = sa[smask]
    allm = comp[..., 3] > 0
    # thick dark outline + drop shadow
    pad = 3
    big = np.zeros((Hh + pad * 2 + 4, W + pad * 2), np.uint8)
    big = np.zeros((Hh + 2 * pad + 4, W + 2 * pad, 4), np.uint8)
    bm = np.zeros(big.shape[:2], bool)
    bm[pad:pad + Hh, pad:pad + W] = allm
    dil = bm.copy()
    for _ in range(2):
        d2 = dil.copy()
        d2[1:] |= dil[:-1]
        d2[:-1] |= dil[1:]
        d2[:, 1:] |= dil[:, :-1]
        d2[:, :-1] |= dil[:, 1:]
        dil = d2
    shadow = shift(dil, 0, 4)
    big[shadow] = (10, 30, 12, 150)
    big[dil] = hexrgb("#10240f")
    region = big[pad:pad + Hh, pad:pad + W]
    region[allm] = comp[allm]
    # sparkle highlight
    img = Image.fromarray(big, "RGBA")
    bbox = img.getbbox()
    img = img.crop(bbox)
    scale = 3
    return img.resize((img.width * scale, img.height * scale), Image.NEAREST)


# ----------------------------------------------------------------------------
# Menu background
# ----------------------------------------------------------------------------
BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def bump_light(X, Y, cx, cy, rr):
    """2 = lit cap (top-left), 1 = mid, 0 = shaded rim (bottom-right)."""
    hi = np.hypot(X - (cx - 0.35 * rr), Y - (cy - 0.35 * rr)) < 0.55 * rr
    lo = np.hypot(X - (cx - 0.25 * rr), Y - (cy - 0.25 * rr)) > 0.98 * rr
    return np.where(hi, 2, np.where(lo, 0, 1))


def menu_background():
    W, H = 320, 180
    rng = np.random.default_rng(77)
    X, Y = grid(W, H)
    names = np.full((H, W), None, dtype=object)
    # sky gradient with ordered dithering
    sky = ["sk0", "sk1", "sk2", "sk3", "sk4"]
    t = np.clip((Y - 5) / 120.0, 0, 1) * (len(sky) - 1)
    bay = BAYER4[(Y.astype(int)) % 4, (X.astype(int)) % 4]
    idx = np.clip(np.floor(t + bay - 0.5 + 0.5).astype(int), 0, len(sky) - 1)
    idx = np.clip(np.floor(t + bay).astype(int), 0, len(sky) - 1)
    for i, k in enumerate(sky):
        names[idx == i] = k
    # sun with glow
    sx, sy = 236, 70
    r = np.hypot(X - sx, Y - sy)
    names[(r < 26) & (r + bay * 4 < 26) & (idx >= 2)] = "sk4"
    names[r < 13] = "sun"
    # clouds
    for (cx, cy, s) in [(60, 34, 1.0), (150, 22, 0.8), (285, 30, 0.7), (200, 48, 0.55)]:
        cm = np.zeros((H, W), bool)
        cs = np.zeros((H, W), bool)
        for j in range(7):
            ox = (j - 3) * 9 * s + rng.uniform(-3, 3)
            oyy = -abs(j - 3) * 2.2 * s + rng.uniform(-1, 1)
            rr = (9 - abs(j - 3) * 1.4) * s + 2
            c = np.hypot(X - (cx + ox), (Y - (cy + oyy)) * 1.25) < rr
            cm |= c
        cm &= Y < cy + 5 * s
        cs = cm & ~shift(cm, 0, -2)
        names[cm] = "white"
        names[cs] = "sk3"
    # far hills
    def ridge(base, amp, freqs, seed):
        rr = np.random.default_rng(seed)
        xs = np.arange(W)
        yv = np.full(W, float(base))
        for fq in freqs:
            yv += amp / fq * np.sin(xs / W * 2 * math.pi * fq + rr.uniform(0, 6.28))
        return yv
    for base, amp, fr, col, seed in [(108, 10, (1, 2, 5), "hl0", 1), (122, 9, (1, 3, 7), "hl1", 2),
                                      (134, 7, (2, 5, 11), "hl2", 3)]:
        yv = ridge(base, amp, fr, seed)
        m = Y >= yv[X.astype(int)]
        names[m] = col
    # jungle canopy (bumps) mid layer
    canopy = np.zeros((H, W), bool)
    hl = np.zeros((H, W), float)
    for i in range(34):
        cx = i * 10 + rng.uniform(-4, 4)
        cy = 142 + rng.uniform(-6, 4)
        rr = rng.uniform(9, 15)
        d = np.hypot(X - cx, Y - cy)
        c = d < rr
        canopy |= c
        hl = np.where(c, bump_light(X, Y, cx, cy, rr), hl)
    names[canopy] = "lf2"
    names[canopy & (hl == 2)] = "lf3"
    names[canopy & (hl == 0)] = "lf1"
    # foreground meadow
    gy = 160 + 2 * np.sin(X / 23.0)
    meadow = Y >= gy
    names[meadow] = "g2"
    names[meadow & (Y < gy + 2)] = "g3"
    names[meadow & (Y > 172) & (bay > 0.5)] = "g1"
    for i in range(140):
        x = int(rng.integers(0, W))
        y = int(rng.integers(162, 180))
        h = int(rng.integers(2, 6))
        for j in range(h):
            if 0 <= y - j < H:
                names[y - j, x] = "g4" if j < h - 1 else "g5"
    for i in range(22):
        x = int(rng.integers(40, W - 40))
        y = int(rng.integers(165, 177))
        col = ["fw", "fy", "fp"][i % 3]
        for (dx, dy) in [(0, -1), (0, 1), (-1, 0), (1, 0)]:
            names[y + dy, x + dx] = col
        names[y, x] = "fy" if col != "fy" else "fw"
    # framing trees: left and right trunks with heavy leaf masses at the top corners
    for side in (0, 1):
        tx = 18 if side == 0 else W - 22
        trunk = (np.abs(X - tx - 3 * np.sin(Y / 30.0)) < 7 - Y / 90.0) & (Y > 20)
        names[trunk] = "tr1"
        names[trunk & (X < tx - 2 + 3 * np.sin(Y / 30.0))] = "tr2"
        names[trunk & (X > tx + 3 + 3 * np.sin(Y / 30.0))] = "tr0"
        leaf = np.zeros((H, W), bool)
        lh = np.full((H, W), -9.0)
        for i in range(26):
            cx = (rng.uniform(-20, 95) if side == 0 else W - rng.uniform(-20, 95))
            cy = rng.uniform(-10, 40) + (abs(cx - (0 if side == 0 else W)) / 6)
            rr = rng.uniform(10, 18)
            d = np.hypot(X - cx, Y - cy)
            c = d < rr
            leaf |= c
            lh = np.where(c, bump_light(X, Y, cx, cy, rr), lh)
        names[leaf] = "lf1"
        names[leaf & (lh >= 1)] = "lf2"
        names[leaf & (lh == 2)] = "lf3"
        names[leaf & (lh == 0)] = "lf0"
        # vines
        for i in range(5):
            vx = (rng.integers(30, 90) if side == 0 else W - rng.integers(30, 90))
            vlen = int(rng.integers(30, 70))
            for y in range(20, 20 + vlen):
                x = int(vx + 2 * math.sin(y / 7.0 + i))
                if 0 <= x < W and y < H:
                    names[y, x] = "lf1"
                    if y % 6 == 0 and x + 1 < W:
                        names[y, x + 1] = "lf3"
                        names[y, x - 1] = "lf2"
    img = rgba_from_names(names)
    # rabbit on the meadow
    rab = rabbit_frame("sit")
    img.alpha_composite(rab, (250, 151))
    # coiled python on the left branch region: a few body loops near the trunk base
    L = Layer(W, H)
    pts = []
    ss = []
    s = 0.0
    prev = None
    for i in range(140):
        t = i / 139
        p = (40 + 70 * t, 170 + 3.5 * math.sin(t * 2 * math.pi * 1.5))
        if prev:
            s += math.dist(p, prev)
        pts.append(p)
        ss.append(s)
        prev = p
    kk = 0.55
    L.add_tube(pts, ss, rfun=lambda sv: np.maximum(1.0, BODY_R * kk * np.clip(sv / 20, 0.2, 1)))
    endp = pts[-1]
    f = (1.0, 0.0)
    head_layer(L, endp, f, kk, "closed", 2, neck=0)
    sn = finish_with_tongue(L, L.colours())
    img.alpha_composite(sn)
    # subtle vignette darkening behind the menu area is left to the menu renderer
    img = img.resize((W * 4, H * 4), Image.NEAREST)
    return img


# ----------------------------------------------------------------------------
# UI
# ----------------------------------------------------------------------------
def ui_panel():
    """24x24 9-slice wooden/leafy frame, 8px borders, translucent dark centre."""
    S = 24
    a = np.zeros((S, S, 4), np.uint8)
    names = np.full((S, S), None, dtype=object)
    for y in range(S):
        for x in range(S):
            e = min(x, y, S - 1 - x, S - 1 - y)
            if e == 0:
                names[y, x] = "tr0"
            elif e == 1:
                names[y, x] = "tr2" if (x < y) or y == 1 or x == 1 else "tr1"
                if y == S - 2 or x == S - 2:
                    names[y, x] = "tr1"
            elif e == 2:
                names[y, x] = "tr1"
            elif e == 3:
                names[y, x] = "tr0"
    img = np.array(rgba_from_names(names))
    inner = np.zeros((S, S), bool)
    inner[4:S - 4, 4:S - 4] = True
    img[inner] = (12, 26, 14, 200)
    # corner leaves
    for (cx, cy) in [(1, 1), (S - 2, 1), (1, S - 2), (S - 2, S - 2)]:
        for dx, dy, col in [(0, 0, "lf3"), (1, 0, "lf2"), (0, 1, "lf2"), (-1, 0, "lf2"), (0, -1, "lf2")]:
            x, y = cx + dx, cy + dy
            if 0 <= x < S and 0 <= y < S:
                img[y, x] = PRGB[col]
    return Image.fromarray(img, "RGBA")


def icon_star():
    """16x16 golden star icon (level/score)."""
    S = 16
    big = Image.new("L", (S * 8, S * 8), 0)
    pts = []
    for i in range(10):
        r = (7.6 if i % 2 == 0 else 3.3) * 8
        ang = -math.pi / 2 + i * math.pi / 5
        pts.append((S * 4 + r * math.cos(ang), S * 4 + 4 + r * math.sin(ang)))
    ImageDraw.Draw(big).polygon(pts, fill=255)
    star = np.array(big.resize((S, S), Image.BOX)) > 110
    dist = np.zeros((S, S))
    m = star.copy()
    while m.any():
        dist += m
        m = m & ~outline_of(m)
    sh = shade_from_height(np.minimum(dist, 3) * 1.2)
    names = np.full((S, S), None, dtype=object)
    c = np.select([sh < 0.55, sh < 0.75, sh < 0.9], ["s1", "s2", "s3"], "s4")
    names[star] = c[star]
    names[outline_of(star)] = "ol"
    return rgba_from_names(names)


# ----------------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------------
TURNS = {"turn_ne": ("n", "e"), "turn_nw": ("n", "w"), "turn_se": ("s", "e"), "turn_sw": ("s", "w")}


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    out = {}
    # tiles
    t = Sheet(max_width=24 * 8 + 7)
    for i in range(8):
        t.add(f"grass_{i}", grass_tile(i, flowers=(2 if i == 6 else 1 if i == 7 else 0)))
    for m in range(16):
        t.add(f"wall_{m}", brick_tile(m))
    t.add("shade_w", shade_overlay("w"))
    t.add("shade_n", shade_overlay("n"))
    t.add("shade_nw", shade_overlay("nw"))
    out["tiles"] = t.save(OUT / "tiles.png")

    # snake
    s = Sheet(max_width=24 * 8 + 7)
    for d in "udlr":
        s.add(f"head_{d}", piece_head(d))
        s.add(f"head_{d}_tongue1", piece_head(d, tongue=1))
        s.add(f"head_{d}_tongue2", piece_head(d, tongue=2))
        s.add(f"head_{d}_open", piece_head(d, "open"))
        s.add(f"head_{d}_eat", piece_head(d, "eat"))
        s.add(f"head_{d}_dead", piece_head(d, "dead"))
    s.add("body_h", piece_straight(True))
    s.add("body_v", piece_straight(False))
    for name, (a, b) in TURNS.items():
        s.add(name, piece_turn(a, b))
    for d in "udlr":
        s.add(f"tail_{d}", piece_tail(d))
    for d in "udlr":
        s.anim(f"head_{d}", [f"head_{d}", f"head_{d}_tongue1", f"head_{d}_tongue2", f"head_{d}_tongue1",
                             f"head_{d}_tongue2", f"head_{d}_tongue1"], [1400, 50, 90, 50, 90, 50], True)
        s.anim(f"head_eat_{d}", [f"head_{d}_open", f"head_{d}_eat", f"head_{d}_eat", f"head_{d}_open", f"head_{d}"],
               [50, 70, 60, 50, 100], False)
        s.anim(f"head_dead_{d}", [f"head_{d}_dead"], 1000, False)
    out["snake"] = s.save(OUT / "snake.png")

    # rabbit
    r = Sheet(max_width=24 * 8 + 7)
    r.add("rabbit_sit", rabbit_frame("sit"))
    r.add("rabbit_nose", rabbit_frame("sit", nose=1))
    r.add("rabbit_look", rabbit_frame("sit", nose=2))
    r.add("rabbit_ears", rabbit_frame("sit", ear=math.radians(-28)))
    r.add("rabbit_hop1", rabbit_frame("hop", lift=2))
    r.add("rabbit_hop2", rabbit_frame("hop", lift=4))
    r.anim("rabbit_idle", ["rabbit_sit", "rabbit_nose", "rabbit_sit", "rabbit_nose", "rabbit_sit", "rabbit_ears",
                           "rabbit_sit", "rabbit_look", "rabbit_sit", "rabbit_hop1", "rabbit_hop2", "rabbit_hop1"],
           [700, 90, 90, 90, 600, 260, 500, 400, 300, 70, 110, 70], True)
    r.anim("rabbit_hop", ["rabbit_hop1", "rabbit_hop2", "rabbit_hop1", "rabbit_sit"], [70, 110, 70, 120], False)
    out["rabbit"] = r.save(OUT / "rabbit.png")

    # ui
    u = Sheet(max_width=128)
    u.add("panel", ui_panel())
    icon = rabbit_frame("sit").crop((2, 1, 24, 23))
    u.add("icon_rabbit", icon)
    u.add("icon_star", icon_star())
    out["ui"] = u.save(OUT / "ui.png")

    logo = make_logo()
    save_png(logo, OUT / "logo.png")
    bg = menu_background()
    save_png(bg, OUT / "menu_bg.png")
    return out, logo, bg


# ----------------------------------------------------------------------------
# Previews (not committed; written to the scratch preview folder)
# ----------------------------------------------------------------------------
def contact(png, scale=3):
    import json
    data = json.loads(png.with_suffix(".json").read_text())
    sheet = Image.open(png)
    names = list(data["frames"])
    cols = 8
    cw, ch = 24 * scale + 8, 24 * scale + 22
    rows = (len(names) + cols - 1) // cols
    img = Image.new("RGBA", (cols * cw, rows * ch), (40, 44, 40, 255))
    d = ImageDraw.Draw(img)
    for i, n in enumerate(names):
        f = data["frames"][n]
        fr = sheet.crop((f["x"], f["y"], f["x"] + f["w"], f["y"] + f["h"]))
        bg = Image.new("RGBA", fr.size, PRGB["g2"])
        bg.alpha_composite(fr)
        x, y = (i % cols) * cw + 4, (i // cols) * ch + 16
        img.paste(bg.resize((fr.width * scale, fr.height * scale), Image.NEAREST), (x, y))
        d.text((x, y - 13), n, fill=(255, 255, 255))
    return img


LEVEL = [
    "......................................",
    "......................................",
    "...######...................######....",
    "...#..............................#...",
    "...#..............................#...",
    "......................................",
    "..........##########..................",
    "..........#...........................",
    "..........#.............#.............",
    "........................#.............",
    "...................######.............",
    "......................................",
    "......#...............................",
    "......#............########...........",
    "......#...............................",
    "......####............................",
    "......................................",
    "...#..............................#...",
    "...#..............................#...",
    "...######....................######...",
    "......................................",
    "......................................",
    "......................................",
]


def mock_screen(tiles_png, snake_png, rabbit_png):
    import json

    def load(p):
        return Image.open(p), json.loads(p.with_suffix(".json").read_text())

    ts, tj = load(tiles_png)
    ss, sj = load(snake_png)
    rs, rj = load(rabbit_png)

    def fr(sh, js, n):
        f = js["frames"][n]
        return sh.crop((f["x"], f["y"], f["x"] + f["w"], f["y"] + f["h"]))

    W, H = 38, 23
    ox, oy = (1280 - W * C) // 2, 720 - H * C - 40
    img = Image.new("RGBA", (1280, 720), hexrgb("#10240f"))
    wall = lambda x, y: 0 <= x < W and 0 <= y < H and LEVEL[y][x] == "#"
    for y in range(H):
        for x in range(W):
            h = (x * 73856093 ^ y * 19349663) & 0xFFFF
            g = h % 8 if h % 5 == 0 else (h // 7) % 6
            img.alpha_composite(fr(ts, tj, f"grass_{g}"), (ox + x * C, oy + y * C))
            if not wall(x, y):
                if wall(x - 1, y):
                    img.alpha_composite(fr(ts, tj, "shade_w"), (ox + x * C, oy + y * C))
                if wall(x, y - 1):
                    img.alpha_composite(fr(ts, tj, "shade_n"), (ox + x * C, oy + y * C))
                if wall(x - 1, y - 1) and not wall(x - 1, y) and not wall(x, y - 1):
                    img.alpha_composite(fr(ts, tj, "shade_nw"), (ox + x * C, oy + y * C))
    for y in range(H):
        for x in range(W):
            if wall(x, y):
                m = (wall(x, y - 1) * 1) | (wall(x + 1, y) * 2) | (wall(x, y + 1) * 4) | (wall(x - 1, y) * 8)
                img.alpha_composite(fr(ts, tj, f"wall_{m}"), (ox + x * C, oy + y * C))
    # snake path tail->head
    path = [(14, 4), (15, 4), (16, 4), (17, 4), (17, 5), (18, 5), (19, 5), (20, 5), (21, 5), (21, 4), (22, 4),
            (23, 4), (24, 4), (24, 5), (24, 6), (24, 7), (25, 7), (26, 7), (27, 7)]
    dname = {(1, 0): "e", (-1, 0): "w", (0, 1): "s", (0, -1): "n"}
    hd = {"e": "r", "w": "l", "s": "d", "n": "u"}
    for i, (x, y) in enumerate(path):
        if i == 0:
            nx, ny = path[1]
            toward = dname[(nx - x, ny - y)]
            opp = {"e": "w", "w": "e", "n": "s", "s": "n"}[toward]
            n = f"tail_{hd[opp]}"
        elif i == len(path) - 1:
            px, py = path[i - 1]
            n = f"head_{hd[dname[(x - px, y - py)]]}_tongue2"
        else:
            a = dname[(path[i - 1][0] - x, path[i - 1][1] - y)]
            b = dname[(path[i + 1][0] - x, path[i + 1][1] - y)]
            e = set(a + b)
            if e == {"e", "w"}:
                n = "body_h"
            elif e == {"n", "s"}:
                n = "body_v"
            else:
                v = "n" if "n" in e else "s"
                hz = "e" if "e" in e else "w"
                n = f"turn_{v}{hz}"
        img.alpha_composite(fr(ss, sj, n), (ox + x * C, oy + y * C))
    # second snake, dead head demo, eating head
    img.alpha_composite(fr(rs, rj, "rabbit_sit"), (ox + 30 * C, oy + 7 * C))
    img.alpha_composite(fr(rs, rj, "rabbit_hop2"), (ox + 12 * C, oy + 17 * C))
    for i, x in enumerate(range(10, 14)):
        img.alpha_composite(fr(ss, sj, "tail_l" if i == 0 else "body_h"), (ox + x * C, oy + 21 * C))
    img.alpha_composite(fr(ss, sj, "head_r_eat"), (ox + 14 * C, oy + 21 * C))
    img.alpha_composite(fr(ss, sj, "tail_u"), (ox + 33 * C, oy + 9 * C))
    img.alpha_composite(fr(ss, sj, "body_v"), (ox + 33 * C, oy + 10 * C))
    img.alpha_composite(fr(ss, sj, "head_d_dead"), (ox + 33 * C, oy + 11 * C))
    # flip so dead head points up: just a demo of the frame
    font = ImageFont.truetype(str(FONTS / "pressstart2p" / "PressStart2P-Regular.ttf"), 16)
    d = ImageDraw.Draw(img)
    d.text((ox, 40), "LEVEL 3   RABBITS 12/50   SCORE 01250", font=font, fill=(255, 240, 200))
    return img


def main():
    out, logo, bg = build()
    if "--preview" not in sys.argv:
        print("piton gfx: ok")
        return
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    for n in ("tiles", "snake", "rabbit", "ui"):
        contact(OUT / f"{n}.png").save(PREVIEW_DIR / f"piton_{n}.png")
    shot = mock_screen(OUT / "tiles.png", OUT / "snake.png", OUT / "rabbit.png")
    shot.save(PREVIEW_DIR / "piton_mock.png")
    menu = bg.copy()
    menu.alpha_composite(logo, ((1280 - logo.width) // 2, 70))
    menu.save(PREVIEW_DIR / "piton_menu.png")
    print("piton gfx: ok")


if __name__ == "__main__":
    main()
