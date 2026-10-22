"""Keyboard Simulator graphics: application icon (a keycap with the Cyrillic letter "Ф").

    python -I tools/assets/gen_kbsim_gfx.py
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FONTS, ROOT, save_png  # noqa: E402

OUT = ROOT / "keyboard-simulator" / "data" / "gfx"


def icon(size=256) -> Image.Image:
    S = size * 4
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    sh = Image.new("L", (S, S), 0)
    ImageDraw.Draw(sh).rounded_rectangle([S * .10, S * .14, S * .92, S * .95], S * .14, fill=150)
    shadow = Image.new("RGBA", (S, S), (10, 20, 40, 0))
    shadow.putalpha(sh.filter(ImageFilter.GaussianBlur(S * .03)))
    im.alpha_composite(shadow)
    d = ImageDraw.Draw(im)
    # key body (side) and top face; finger colour of "Ф" (left little finger) = soft green
    d.rounded_rectangle([S * .08, S * .08, S * .92, S * .90], S * .14, fill=(92, 160, 104, 255))
    d.rounded_rectangle([S * .14, S * .10, S * .86, S * .78], S * .11, fill=(196, 236, 198, 255))
    d.rounded_rectangle([S * .17, S * .13, S * .83, S * .74], S * .09, fill=(170, 222, 176, 255))
    f = ImageFont.truetype(str(FONTS / "ptsans/PT_Sans-Web-Bold.ttf"), int(S * .48))
    bb = d.textbbox((0, 0), "Ф", font=f)
    x = S * .5 - (bb[0] + bb[2]) / 2
    y = S * .435 - (bb[1] + bb[3]) / 2
    d.text((x, y + S * .012), "Ф", font=f, fill=(255, 255, 255, 160))
    d.text((x, y), "Ф", font=f, fill=(32, 64, 44, 255))
    # home-row bump
    d.rounded_rectangle([S * .42, S * .66, S * .58, S * .685], S * .01, fill=(120, 180, 128, 255))
    return im.resize((size, size), Image.LANCZOS)


def main():
    big = icon(256)
    save_png(big, OUT / "icon.png")
    for s in (16, 32, 48):
        save_png(big.resize((s, s), Image.LANCZOS), OUT / f"icon_{s}.png")
    print("kbsim gfx ->", OUT)


if __name__ == "__main__":
    main()
