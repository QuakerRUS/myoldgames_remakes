"""Tiny pixel-art toolkit: palette-indexed canvases, ASCII stamping, outlines, noise.

A Canvas holds palette colour *names* resolved to RGBA in a numpy array (H, W, 4).
Everything is deterministic.
"""
from __future__ import annotations

import math

import numpy as np
from PIL import Image


def hx(h: str, a: int = 255):
    h = h.lstrip("#")
    return np.array([int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a], dtype=np.uint8)


class Palette:
    def __init__(self, colours: dict[str, str]):
        self.names = dict(colours)
        self.rgba = {k: hx(v) for k, v in colours.items()}

    def __getitem__(self, k):
        if isinstance(k, np.ndarray):
            return k
        if isinstance(k, tuple):
            return np.array(k if len(k) == 4 else (*k, 255), dtype=np.uint8)
        return self.rgba[k]

    def with_alpha(self, k, a):
        c = self[k].copy()
        c[3] = a
        return c


class Canvas:
    def __init__(self, pal: Palette, w: int, h: int, fill=None):
        self.pal = pal
        self.w, self.h = w, h
        self.a = np.zeros((h, w, 4), dtype=np.uint8)
        if fill is not None:
            self.a[:, :] = pal[fill]

    # ---- basic ----
    def copy(self):
        c = Canvas(self.pal, self.w, self.h)
        c.a = self.a.copy()
        return c

    def px(self, x, y, col):
        if 0 <= x < self.w and 0 <= y < self.h:
            if col is None:
                self.a[y, x] = 0
            else:
                c = self.pal[col]
                if c[3] == 255:
                    self.a[y, x] = c
                else:
                    self._blend(x, y, c)

    def _blend(self, x, y, c):
        dst = self.a[y, x].astype(np.float32)
        al = c[3] / 255.0
        if dst[3] == 0:
            self.a[y, x] = c
            return
        rgb = dst[:3] * (1 - al) + c[:3].astype(np.float32) * al
        a = min(255, dst[3] + c[3] * (1 - dst[3] / 255))
        self.a[y, x] = np.array([*np.round(rgb), a], dtype=np.uint8)

    def get(self, x, y):
        return self.a[y % self.h, x % self.w]

    def rect(self, x, y, w, h, col):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.px(xx, yy, col)

    def frame(self, x, y, w, h, col):
        self.hline(x, x + w - 1, y, col)
        self.hline(x, x + w - 1, y + h - 1, col)
        self.vline(x, y, y + h - 1, col)
        self.vline(x + w - 1, y, y + h - 1, col)

    def hline(self, x0, x1, y, col):
        for x in range(min(x0, x1), max(x0, x1) + 1):
            self.px(x, y, col)

    def vline(self, x, y0, y1, col):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            self.px(x, y, col)

    def line(self, x0, y0, x1, y1, col):
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx + dy
        while True:
            self.px(x0, y0, col)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    def ellipse(self, cx, cy, rx, ry, col):
        """Filled ellipse, centre may be fractional (e.g. 15.5)."""
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                if ((x + 0.5 - cx - 0.5) / max(rx, 0.01)) ** 2 + ((y + 0.5 - cy - 0.5) / max(ry, 0.01)) ** 2 <= 1.0:
                    self.px(x, y, col)

    def ring(self, cx, cy, r0, r1, col, aspect=1.0):
        for y in range(int(cy - r1 * aspect - 2), int(cy + r1 * aspect + 3)):
            for x in range(int(cx - r1 - 2), int(cx + r1 + 3)):
                d = math.hypot(x - cx, (y - cy) / aspect)
                if r0 <= d < r1:
                    self.px(x, y, col)

    def stamp(self, art: str | list[str], x, y, cmap: dict, flip=False):
        rows = art.strip("\n").split("\n") if isinstance(art, str) else art
        for j, row in enumerate(rows):
            if flip:
                row = row[::-1]
            for i, ch in enumerate(row):
                if ch in (".", " "):
                    continue
                col = cmap[ch]
                self.px(x + i, y + j, col)

    def paste(self, other: "Canvas", x=0, y=0, alpha_mul=1.0):
        for j in range(other.h):
            yy = y + j
            if not (0 <= yy < self.h):
                continue
            for i in range(other.w):
                xx = x + i
                if not (0 <= xx < self.w):
                    continue
                c = other.a[j, i]
                if c[3] == 0:
                    continue
                if alpha_mul != 1.0:
                    c = c.copy()
                    c[3] = int(c[3] * alpha_mul)
                if c[3] == 255:
                    self.a[yy, xx] = c
                else:
                    self._blend(xx, yy, c)

    def mask(self):
        return self.a[:, :, 3] > 0

    def outline(self, col, diagonal=False, only_outside=True):
        """Draw a 1px outline around opaque pixels (outside the shape)."""
        m = self.mask()
        out = np.zeros_like(m)
        shifts = [(0, 1), (0, -1), (1, 0), (-1, 0)]
        if diagonal:
            shifts += [(1, 1), (1, -1), (-1, 1), (-1, -1)]
        for dy, dx in shifts:
            out |= np.roll(np.roll(m, dy, 0), dx, 1) & self._noroll_mask(dy, dx)
        out &= ~m
        c = self.pal[col]
        self.a[out] = c

    def _noroll_mask(self, dy, dx):
        k = np.ones((self.h, self.w), dtype=bool)
        if dy > 0:
            k[:dy, :] = False
        if dy < 0:
            k[dy:, :] = False
        if dx > 0:
            k[:, :dx] = False
        if dx < 0:
            k[:, dx:] = False
        return k

    def replace(self, src, dst):
        s = self.pal[src]
        m = np.all(self.a == s, axis=2)
        self.a[m] = self.pal[dst] if dst is not None else 0

    def shifted(self, dx, dy):
        c = Canvas(self.pal, self.w, self.h)
        c.paste(self, dx, dy)
        return c

    def flipped(self):
        c = Canvas(self.pal, self.w, self.h)
        c.a = self.a[:, ::-1].copy()
        return c

    def rotated(self, k):
        """Rotate 90 deg * k counter-clockwise (square canvases)."""
        c = Canvas(self.pal, self.w, self.h)
        c.a = np.rot90(self.a, k).copy()
        return c

    def img(self) -> Image.Image:
        return Image.fromarray(self.a, "RGBA")

    def scaled(self, k: int) -> Image.Image:
        return self.img().resize((self.w * k, self.h * k), Image.NEAREST)


def hash2(x, y, seed=0):
    """Deterministic integer hash -> [0,1)."""
    h = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    h ^= h >> 16
    return (h & 0xFFFFFF) / float(0x1000000)


BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def dither(v: float, x: int, y: int) -> bool:
    """True if pixel (x,y) should be 'on' for coverage v in [0,1] (ordered dither)."""
    return v > BAYER4[y % 4, x % 4]


def periodic_noise(w, h, freqs, seed, t=0.0):
    """Sum of periodic sine waves -> seamless tileable field in [-1,1]-ish.
    freqs: list of (kx, ky, amp, speed) with integer kx, ky."""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
    f = np.zeros((h, w))
    for kx, ky, amp, sp in freqs:
        ph = rng.uniform(0, 2 * np.pi)
        f += amp * np.sin(2 * np.pi * (kx * xx / w + ky * yy / h) + ph + 2 * np.pi * sp * t)
    return f
