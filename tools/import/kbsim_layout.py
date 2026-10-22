"""Build the ЙЦУКЕН layout description of Keyboard Simulator from the 2007 originals.

    python -I tools/import/kbsim_layout.py
    python -I tools/import/kbsim_layout.py --data archive/diplom-2007/program/Data \
        --dest keyboard-simulator/data/layout/ru.json

The physical layout (rows, key codes, widths, the characters of the Russian Windows layout)
is written down below. The original program is used to derive and cross-check it:

* diplom.dat part 1 (97 key rectangles in bg.bmp) — every key the original knows must exist
  here, in the same row and in the same left-to-right order;
* diplom.dat part 2 (the QWERTY character getch() returns for each glyph) — must match the
  physical key (the known bug of glyph 76, '0' recorded as ':', is reported and ignored);
* bg.bmp — the colour of every key gives its finger: the hue names the finger pair
  (green: left pinky / right middle, yellow: ring fingers, lavender: left middle / right
  pinky, pink: left index, cyan: right index, grey: thumbs), and the side of the boundary
  between the pink and the cyan keys of the row gives the hand.

The format of the output is described in docs/formats/kbsim-layout.md.
"""
import argparse
import colorsys
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_DATA = REPO / "archive/diplom-2007/program/Data"
DEFAULT_DEST = REPO / "keyboard-simulator/data/layout/ru.json"

# Glyph order of the original (fonts.bmp and diplom.dat): А–Я with Ё at 6, а–я with ё at 39,
# space, 1–9, 0, then 18 punctuation characters; 95/96 are the Shift keys.
UPPER = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
GLYPHS = UPPER + UPPER.lower() + " 1234567890" + '-=!"№;%:?*()_+\\/.,'
assert len(GLYPHS) == 95
GLYPH_LSHIFT, GLYPH_RSHIFT = 95, 96

# (code, unshifted char or None, shifted char or None, width in key units, label or None,
#  QWERTY char of the key or None). Codes follow the W3C KeyboardEvent.code names.
ROWS = [
    [("Backquote", "ё", "Ё", 1, None, "`"), ("Digit1", "1", "!", 1, None, "1"),
     ("Digit2", "2", '"', 1, None, "2"), ("Digit3", "3", "№", 1, None, "3"),
     ("Digit4", "4", ";", 1, None, "4"), ("Digit5", "5", "%", 1, None, "5"),
     ("Digit6", "6", ":", 1, None, "6"), ("Digit7", "7", "?", 1, None, "7"),
     ("Digit8", "8", "*", 1, None, "8"), ("Digit9", "9", "(", 1, None, "9"),
     ("Digit0", "0", ")", 1, None, "0"), ("Minus", "-", "_", 1, None, "-"),
     ("Equal", "=", "+", 1, None, "="), ("Backspace", None, None, 2, "Backspace", None)],
    [("Tab", None, None, 1.5, "Tab", None), ("KeyQ", "й", "Й", 1, None, "q"),
     ("KeyW", "ц", "Ц", 1, None, "w"), ("KeyE", "у", "У", 1, None, "e"),
     ("KeyR", "к", "К", 1, None, "r"), ("KeyT", "е", "Е", 1, None, "t"),
     ("KeyY", "н", "Н", 1, None, "y"), ("KeyU", "г", "Г", 1, None, "u"),
     ("KeyI", "ш", "Ш", 1, None, "i"), ("KeyO", "щ", "Щ", 1, None, "o"),
     ("KeyP", "з", "З", 1, None, "p"), ("BracketLeft", "х", "Х", 1, None, "["),
     ("BracketRight", "ъ", "Ъ", 1, None, "]"), ("Backslash", "\\", "/", 1.5, None, "\\")],
    [("CapsLock", None, None, 1.75, "Caps Lock", None), ("KeyA", "ф", "Ф", 1, None, "a"),
     ("KeyS", "ы", "Ы", 1, None, "s"), ("KeyD", "в", "В", 1, None, "d"),
     ("KeyF", "а", "А", 1, None, "f"), ("KeyG", "п", "П", 1, None, "g"),
     ("KeyH", "р", "Р", 1, None, "h"), ("KeyJ", "о", "О", 1, None, "j"),
     ("KeyK", "л", "Л", 1, None, "k"), ("KeyL", "д", "Д", 1, None, "l"),
     ("Semicolon", "ж", "Ж", 1, None, ";"), ("Quote", "э", "Э", 1, None, "'"),
     ("Enter", None, None, 2.25, "Enter", None)],
    [("ShiftLeft", None, None, 2.25, "Shift", None), ("KeyZ", "я", "Я", 1, None, "z"),
     ("KeyX", "ч", "Ч", 1, None, "x"), ("KeyC", "с", "С", 1, None, "c"),
     ("KeyV", "м", "М", 1, None, "v"), ("KeyB", "и", "И", 1, None, "b"),
     ("KeyN", "т", "Т", 1, None, "n"), ("KeyM", "ь", "Ь", 1, None, "m"),
     ("Comma", "б", "Б", 1, None, ","), ("Period", "ю", "Ю", 1, None, "."),
     ("Slash", ".", ",", 1, None, "/"), ("ShiftRight", None, None, 2.75, "Shift", None)],
    [("ControlLeft", None, None, 1.25, "Ctrl", None), ("MetaLeft", None, None, 1.25, "Win", None),
     ("AltLeft", None, None, 1.25, "Alt", None), ("Space", " ", None, 6.25, "", None),
     ("AltRight", None, None, 1.25, "Alt", None), ("MetaRight", None, None, 1.25, "Win", None),
     ("ContextMenu", None, None, 1.25, "Menu", None),
     ("ControlRight", None, None, 1.25, "Ctrl", None)],
]

# QWERTY getch() characters of the shifted symbols on the digit row and the two other keys.
QWERTY_SHIFTED = dict(zip("1234567890-=\\/", "!@#$%^&*()_+|?"))

# Keys the original has no rectangle for: a point inside the key on bg.bmp to sample the colour.
SAMPLE_POINTS = {"Backspace": (570, 186), "Tab": (58, 227), "CapsLock": (63, 265),
                 "Enter": (565, 265)}
# Keys that are not on the original picture at all: standard touch-typing fingers.
STANDARD_FINGERS = {"ControlLeft": "left_pinky", "MetaLeft": "left_pinky",
                    "AltLeft": "thumbs", "AltRight": "thumbs",
                    "MetaRight": "right_pinky", "ContextMenu": "right_pinky",
                    "ControlRight": "right_pinky"}

FINGERS = [  # id, hand, legend digit on the original picture, colour of its keys on bg.bmp
    ("left_pinky", "left", 5, "green"), ("left_ring", "left", 4, "yellow"),
    ("left_middle", "left", 3, "lavender"), ("left_index", "left", 2, "pink"),
    ("thumbs", "both", 1, "grey"), ("right_index", "right", 2, "cyan"),
    ("right_middle", "right", 3, "green"), ("right_ring", "right", 4, "yellow"),
    ("right_pinky", "right", 5, "lavender"),
]
HOME_ROW = {"KeyA", "KeyS", "KeyD", "KeyF", "KeyJ", "KeyK", "KeyL", "Semicolon"}
BUMPS = {"KeyF", "KeyJ"}


def read_bmp8(path):
    """Return (width, height, palette [(r,g,b)], pixel(x, y)) of an uncompressed 8-bit BMP."""
    b = path.read_bytes()
    off = struct.unpack_from("<I", b, 10)[0]
    w, h = struct.unpack_from("<ii", b, 18)
    bpp = struct.unpack_from("<H", b, 28)[0]
    if bpp != 8:
        raise ValueError(f"{path}: {bpp} bits per pixel, 8 expected")
    hdr = struct.unpack_from("<I", b, 14)[0]
    pal = [(b[14 + hdr + 4 * i + 2], b[14 + hdr + 4 * i + 1], b[14 + hdr + 4 * i]) for i in range(256)]
    stride = (w + 3) & ~3

    def pixel(x, y):
        return pal[b[off + (h - 1 - y) * stride + x]]

    return w, h, pal, pixel


def colour_name(pixel, x1, y1, x2, y2):
    """Classify the mean colour of a rectangle by hue (the key colours are pastel)."""
    n, s = 0, [0, 0, 0]
    for y in range(y1, y2 + 1):
        for x in range(x1, x2 + 1):
            p = pixel(x, y)
            s = [a + c for a, c in zip(s, p)]
            n += 1
    r, g, b = (c / n / 255 for c in s)
    h, _, sat = colorsys.rgb_to_hls(r, g, b)
    if sat < 0.08:
        return "grey"
    hue = h * 360
    for name, centre in (("yellow", 60), ("green", 120), ("cyan", 180), ("lavender", 240),
                         ("pink", 300)):
        if abs(hue - centre) <= 20:
            return name
    raise ValueError(f"unexpected key colour, hue {hue:.0f}")


def build(data_dir: Path):
    dat = (data_dir / "diplom.dat").read_bytes()
    rects = [struct.unpack_from("<4H", dat, 8 * i) for i in range(97)]
    qwerty = dat[97 * 8: 97 * 8 + 95]
    _, _, _, pixel = read_bmp8(data_dir / "Images/bg.bmp")
    notes = []

    # 1. Match every glyph to a key and pick the key rectangles.
    key_rect = {}
    char_key = {}
    for r, row in enumerate(ROWS):
        for code, normal, shifted, *_ in row:
            for ch in (normal, shifted):
                if ch is not None:
                    char_key[ch] = (r, code)
    for g, ch in enumerate(GLYPHS):
        if ch not in char_key:
            raise ValueError(f"glyph {g} {ch!r} has no key")
        r, code = char_key[ch]
        rect = rects[g]
        if code in key_rect and key_rect[code] != rect:
            raise ValueError(f"key {code}: glyphs disagree on its rectangle")
        key_rect[code] = rect
    key_rect["ShiftLeft"] = rects[GLYPH_LSHIFT]
    key_rect["ShiftRight"] = rects[GLYPH_RSHIFT]

    # 2. The rectangles must follow the rows and their order.
    prev_y = -1
    for r, row in enumerate(ROWS):
        known = [code for code, *_ in row if code in key_rect]
        if not known:
            continue
        xs = [key_rect[c][0] for c in known]
        if xs != sorted(xs) or max(key_rect[c][1] for c in known) - min(key_rect[c][1] for c in known) > 4:
            raise ValueError(f"row {r}: rectangles do not form a left-to-right row")
        y = min(key_rect[c][1] for c in known)
        if y <= prev_y:
            raise ValueError(f"row {r}: rows out of order")
        prev_y = y

    # 3. The QWERTY characters of part 2 must describe the same physical keys.
    for g, ch in enumerate(GLYPHS):
        r, code = char_key[ch]
        row = {k[0]: k for k in ROWS[r]}
        _, normal, shifted, _, _, q = row[code]
        if code == "Space":
            want = " "
        elif ch == normal:
            want = q
        elif q.isalpha() or q in "[];',.`":
            want = {"[": "{", "]": "}", ";": ":", "'": '"', ",": "<", ".": ">", "`": "~"}.get(q, q.upper())
        else:
            want = QWERTY_SHIFTED[q]
        got = chr(qwerty[g])
        if got != want:
            if g == 76 and got == ":":
                notes.append("diplom.dat glyph 76 ('0') records ':' instead of '0' (known bug)")
                continue
            raise ValueError(f"glyph {g} {ch!r}: diplom.dat says {got!r}, key {code} gives {want!r}")

    # 4. Fingers from the colours.
    colours = {}
    for code, rect in key_rect.items():
        x1, y1, x2, y2 = rect
        colours[code] = colour_name(pixel, x1, y1, x2, y2)
    for code, (x, y) in SAMPLE_POINTS.items():
        colours[code] = colour_name(pixel, x - 10, y - 10, x + 10, y + 10)
    by_colour = {}
    for fid, hand, _, colour in FINGERS:
        by_colour.setdefault(colour, {})[hand] = fid

    rows_out = []
    for r, row in enumerate(ROWS):
        # Hand boundary of the row: between the pink (left index) and cyan (right index) keys.
        centres = {}
        for code, *_ in row:
            if code in key_rect:
                x1, _, x2, _ = key_rect[code]
                centres[code] = (x1 + x2) / 2
        pink = [centres[c] for c in centres if colours[c] == "pink"]
        cyan = [centres[c] for c in centres if colours[c] == "cyan"]
        boundary = (max(pink) + min(cyan)) / 2 if pink and cyan else None
        keys = []
        for i, (code, normal, shifted, width, label, _) in enumerate(row):
            if code in STANDARD_FINGERS:
                finger = STANDARD_FINGERS[code]
            else:
                colour = colours[code]
                options = by_colour[colour]
                if len(options) == 1:
                    finger = next(iter(options.values()))
                else:
                    if code in centres and boundary is not None:
                        hand = "left" if centres[code] < boundary else "right"
                    else:  # modifier keys at the row ends
                        hand = "left" if i < len(row) / 2 else "right"
                    finger = options[hand]
            key = {"code": code}
            if normal is not None:
                key["normal"] = normal
            if shifted is not None:
                key["shift"] = shifted
            if label is not None:
                key["label"] = label
            elif shifted is not None and shifted == normal.upper() and shifted != normal:
                key["label"] = shifted  # letter keys show the capital letter
            if width != 1:
                key["width"] = width
            key["finger"] = finger
            if code in HOME_ROW:
                key["home"] = True
            if code in BUMPS:
                key["bump"] = True
            keys.append(key)
        rows_out.append(keys)

    # 5. Shift of the opposite hand for every shifted character.
    hand_of = {fid: hand for fid, hand, *_ in FINGERS}
    for keys in rows_out:
        for key in keys:
            if "shift" in key:
                hand = hand_of[key["finger"]]
                if hand == "both":
                    raise ValueError(f"{key['code']}: shifted character on a thumb key")
                key["shift_with"] = "ShiftRight" if hand == "left" else "ShiftLeft"

    layout = {
        "format": "kbsim-layout",
        "version": 1,
        "name": "ЙЦУКЕН",
        "language": "ru",
        "fingers": [{"id": f, "hand": h, "legend": n, "original_colour": c} for f, h, n, c in FINGERS],
        "rows": rows_out,
    }
    return layout, notes


def dump(layout) -> str:
    """JSON with one key per line, so diffs stay readable."""
    head = {k: v for k, v in layout.items() if k not in ("fingers", "rows")}
    lines = ["{"]
    for k, v in head.items():
        lines.append(f"  {json.dumps(k)}: {json.dumps(v, ensure_ascii=False)},")
    lines.append('  "fingers": [')
    lines.append(",\n".join("    " + json.dumps(f, ensure_ascii=False) for f in layout["fingers"]))
    lines.append("  ],")
    lines.append('  "rows": [')
    rows = []
    for row in layout["rows"]:
        rows.append("    [\n" + ",\n".join("      " + json.dumps(k, ensure_ascii=False) for k in row) + "\n    ]")
    lines.append(",\n".join(rows))
    lines.append("  ]")
    lines.append("}")
    text = "\n".join(lines) + "\n"
    assert json.loads(text) == layout
    return text


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--data", type=Path, default=DEFAULT_DATA, help="the Data folder of the original")
    ap.add_argument("--dest", type=Path, default=DEFAULT_DEST)
    args = ap.parse_args()
    layout, notes = build(args.data)
    args.dest.parent.mkdir(parents=True, exist_ok=True)
    with open(args.dest, "w", encoding="utf-8", newline="\n") as f:
        f.write(dump(layout))
    sys.stdout.reconfigure(encoding="utf-8")
    for n in notes:
        print("note:", n)
    nkeys = sum(len(r) for r in layout["rows"])
    nchars = sum(("normal" in k) + ("shift" in k) for r in layout["rows"] for k in r)
    print(f"{len(layout['rows'])} rows, {nkeys} keys, {nchars} characters -> {args.dest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
