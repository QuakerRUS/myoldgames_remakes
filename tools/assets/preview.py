"""Render contact sheets of every sprite sheet (soldier/piton/keyboard-simulator data/gfx/*.json)
and the branding images into the preview directory (see common.PREVIEW_DIR).

    python -I tools/assets/preview.py [scale]
"""
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FONTS, PREVIEW_DIR, ROOT  # noqa: E402

BG = (52, 54, 64)
CHECK = (60, 62, 72)


def checker(w, h, s=8):
    im = Image.new("RGB", (w, h), BG)
    d = ImageDraw.Draw(im)
    for y in range(0, h, s):
        for x in range(0, w, s):
            if (x // s + y // s) % 2:
                d.rectangle([x, y, x + s - 1, y + s - 1], fill=CHECK)
    return im


def contact(json_path: Path, scale: int, out: Path):
    data = json.loads(json_path.read_text(encoding="utf-8"))
    sheet = Image.open(json_path.parent / data["image"]).convert("RGBA")
    font = ImageFont.truetype(str(FONTS / "vt323/VT323-Regular.ttf"), 16)
    frames = data["frames"]
    items = list(frames.items())
    max_w = 1800
    cells = []
    for name, r in items:
        im = sheet.crop((r["x"], r["y"], r["x"] + r["w"], r["y"] + r["h"]))
        k = scale
        while im.width * k > max_w and k > 1:
            k -= 1
        if im.width * k > max_w:
            im = im.resize((max_w, int(im.height * max_w / im.width)), Image.NEAREST)
            k = 1
        im = im.resize((im.width * k, im.height * k), Image.NEAREST)
        cells.append((name, im))
    pad, label = 6, 16
    x = y = 0
    row_h = 0
    pos = []
    for name, im in cells:
        cw = max(im.width, min(len(name) * 8, 220)) + pad
        ch = im.height + label + pad
        if x and x + cw > max_w:
            x = 0
            y += row_h
            row_h = 0
        pos.append((x, y))
        x += cw
        row_h = max(row_h, ch)
    anims = data.get("animations", {})
    def fl(v):
        return ", ".join(f if isinstance(f, str) else f"{f['frame']}/{f['ms']}" for f in v["frames"])
    lines = [f"{a}: {fl(v)}  ms={v.get('ms', '-')} loop={v['loop']}" for a, v in anims.items()]
    text_h = 16 * len(lines) + 8
    W = max_w + pad
    H = y + row_h + text_h + pad
    out_im = checker(W, H)
    d = ImageDraw.Draw(out_im)
    for (name, im), (px, py) in zip(cells, pos):
        out_im.paste(im, (px + pad, py + pad + label), im)
        d.text((px + pad, py + pad - 2), name, fill=(230, 230, 220), font=font)
    ty = y + row_h + pad
    for ln in lines:
        d.text((pad, ty), ln[:230], fill=(170, 200, 255), font=font)
        ty += 16
    out.parent.mkdir(parents=True, exist_ok=True)
    out_im.save(out)
    return out


class Atlas:
    def __init__(self, json_path: Path):
        self.data = json.loads(json_path.read_text(encoding="utf-8"))
        self.img = Image.open(json_path.parent / self.data["image"]).convert("RGBA")

    def __contains__(self, name):
        return name in self.data["frames"]

    def get(self, name):
        r = self.data["frames"][name]
        return self.img.crop((r["x"], r["y"], r["x"] + r["w"], r["y"] + r["h"]))


SOLDIER_MOCK = [
    "WWWWWWWWWWWWWWWW",
    "W....W.....DDDDW",
    "W.B..W.r.K.D...W",
    "W.b..R.....D.f.W",
    "W....W~~~~.DDDDW",
    "WW.WWW~s~~..G..W",
    "W...b.~~~~..Q..W",
    "W.B.X..P.....E.W",
    "WWWWWWWWWWWWWWWW",
]


def soldier_mock(scale=2):
    gfx = ROOT / "soldier" / "data" / "gfx"
    if not (gfx / "tiles.json").exists():
        return None
    t = Atlas(gfx / "tiles.json")
    p = Atlas(gfx / "player.json") if (gfx / "player.json").exists() else None
    m = SOLDIER_MOCK
    H, W = len(m), len(m[0])
    im = Image.new("RGBA", (W * 32, H * 32), (0, 0, 0, 255))

    def at(x, y):
        return m[y][x] if 0 <= y < H and 0 <= x < W else "W"

    def is_wall(ch, kind):
        return ch in ("W",) if kind == "W" else ch in ("D", "f")

    for y in range(H):
        for x in range(W):
            ch = at(x, y)
            px, py = x * 32, y * 32
            if ch in "WDf":
                kind = "W" if ch == "W" else "D"
                mask = sum(b for b, (dx, dy) in ((1, (0, -1)), (2, (1, 0)), (4, (0, 1)), (8, (-1, 0)))
                           if is_wall(at(x + dx, y + dy), kind))
                name = f"wall_{mask}" if kind == "W" else f"darkwall_{mask}"
                im.alpha_composite(t.get(name), (px, py))
                continue
            base = {"~": "water_0", "s": "sunk_box_0", "E": "exit_0", "b": "button_up", "X": "door_0",
                    "R": "door_red_0", "G": "guillotine_base", "Q": "guillotine_base"}.get(ch, f"floor_{(x * 7 + y * 13) % 4 if (x * 7 + y * 13) % 5 == 0 else 0}")
            if ch == "B" and at(x, y + 1) == "b":
                base = "floor_0"
            im.alpha_composite(t.get(base), (px, py))
            if ch not in "GQ":
                n, w_ = at(x, y - 1) in "WDf", at(x - 1, y) in "WDf"
                if n:
                    im.alpha_composite(t.get("shadow_n"), (px, py))
                if w_:
                    im.alpha_composite(t.get("shadow_w"), (px, py))
                if not n and not w_ and at(x - 1, y - 1) in "WDf":
                    im.alpha_composite(t.get("shadow_nw"), (px, py))
            if ch in "~s":
                for side, (dx, dy) in zip("nesw", ((0, -1), (1, 0), (0, 1), (-1, 0))):
                    if at(x + dx, y + dy) not in "~s":
                        im.alpha_composite(t.get(f"water_edge_{side}"), (px, py))
            if ch == "B":
                im.alpha_composite(t.get("box"), (px, py))
            if ch == "K":
                im.alpha_composite(t.get("key_yellow"), (px, py))
            if ch == "r":
                im.alpha_composite(t.get("key_red"), (px, py))
            if ch == "P" and p:
                im.alpha_composite(p.get("idle_d_0") if "idle_d_0" in p else p.get(list(p.data["frames"])[0]), (px, py))
    for y in range(H):
        for x in range(W):
            if at(x, y) in "GQ":
                if at(x, y) == "Q" and p:
                    im.alpha_composite(p.get("idle_d_0"), (x * 32, y * 32))
                top = t.get("guillotine_top_0" if at(x, y) == "Q" else "guillotine_top_2")
                im.alpha_composite(top, (x * 32, y * 32 - 16))
    if p:
        names = list(p.data["frames"])
        im.alpha_composite(p.get("walk_r_1") if "walk_r_1" in p else p.get(names[0]), (9 * 32, 2 * 32))
        im.alpha_composite(p.get("idle_u_0") if "idle_u_0" in p else p.get(names[0]), (2 * 32, 4 * 32))
        if "push_l_0" in p:
            im.alpha_composite(p.get("push_l_0"), (3 * 32, 7 * 32))
    im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    o = PREVIEW_DIR / "soldier_mock.png"
    im.save(o)
    return o


def main():
    scale = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    outs = []
    mo = soldier_mock()
    if mo:
        outs.append(mo)
    for game in ("soldier", "piton", "keyboard-simulator"):
        for j in sorted((ROOT / game / "data" / "gfx").glob("*.json")):
            outs.append(contact(j, scale, PREVIEW_DIR / f"{game}_{j.stem}.png"))
        for p in sorted((ROOT / game / "data" / "gfx").glob("*.png")):
            if not p.with_suffix(".json").exists():
                im = Image.open(p).convert("RGBA")
                bg = checker(im.width, im.height, 16)
                bg.paste(im, (0, 0), im)
                o = PREVIEW_DIR / f"{game}_{p.stem}.png"
                bg.save(o)
                outs.append(o)
    for p in sorted((ROOT / "shared" / "branding").glob("*.png")):
        im = Image.open(p).convert("RGBA")
        for name, col in (("dark", (12, 12, 18)), ("light", (235, 235, 235))):
            bg = Image.new("RGB", im.size, col)
            bg.paste(im, (0, 0), im)
            o = PREVIEW_DIR / f"branding_{p.stem}_{name}.png"
            bg.save(o)
            outs.append(o)
    for o in outs:
        print(o)


if __name__ == "__main__":
    main()
