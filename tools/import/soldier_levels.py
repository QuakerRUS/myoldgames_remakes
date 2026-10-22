"""Convert the original Soldier 1.1 levels (LEVELn.DAT) to soldier/data/levels/NN.json.

The 1.1 level set is the patch's LEVELS folder plus the levels the patch does not contain
(1, 24, 33, 34), which are taken from the 1.0 LEVELS folder.

    python -I tools/import/soldier_levels.py
    python -I tools/import/soldier_levels.py --levels-1-0 archive/soldier-1.0/workdir/LEVELS \
        --levels-1-1 archive/soldier-1.1-patch/patch/LEVELS --out soldier/data/levels

Format: docs/formats/soldier-level.md.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

LEVEL_COUNT = 51

# One character per tile code 0..63 (index = code). Keep in sync with soldier/core/src/tile.cpp.
TILE_CHARS = (
    ".#Xv^<>!B_b"                             # 0..10
    "1234D5678d"                              # 11..20
    "rRaA" "gGhH" "uUiI" "yYjJ" "wWlL" "kKmM"  # 21..44: key, door, key on button, key in door
    "%?@T90{}$~*"                             # 45..55
    "=()[]-+&"                                # 56..63
)
assert len(TILE_CHARS) == 64 and len(set(TILE_CHARS)) == 64
CHAR_TILES = {c: i for i, c in enumerate(TILE_CHARS)}


def read_dat(path):
    """Return (width, height, time, number, codes) of an original LEVELn.DAT."""
    data = Path(path).read_bytes()
    if len(data) < 8:
        raise ValueError(f"{path}: too short")
    width, height, time, number = struct.unpack("<4h", data[:8])
    if not (1 <= width <= 100 and 1 <= height <= 100):
        raise ValueError(f"{path}: bad size {width}x{height}")
    codes = data[8:8 + width * height]
    if len(codes) != width * height:
        raise ValueError(f"{path}: truncated map")
    bad = sorted({c for c in codes if c >= 64})
    if bad:
        raise ValueError(f"{path}: unknown tile codes {bad}")
    return width, height, time, number, bytes(codes)


def to_json_obj(width, height, time, number, codes):
    rows = ["".join(TILE_CHARS[c] for c in codes[y * width:(y + 1) * width]) for y in range(height)]
    return {"number": number, "width": width, "height": height, "time_limit": time, "rows": rows}


def from_json_obj(obj):
    """Inverse of to_json_obj: (width, height, time, number, codes)."""
    codes = bytes(CHAR_TILES[c] for row in obj["rows"] for c in row)
    return obj["width"], obj["height"], obj["time_limit"], obj["number"], codes


def dump(obj):
    """Stable, readable JSON: one row per line."""
    head = {k: obj[k] for k in ("number", "width", "height", "time_limit")}
    lines = ["{"] + [f'  "{k}": {json.dumps(v)},' for k, v in head.items()]
    lines.append('  "rows": [')
    rows = obj["rows"]
    lines += [f"    {json.dumps(r)}{',' if i + 1 < len(rows) else ''}" for i, r in enumerate(rows)]
    lines += ["  ]", "}"]
    return "\n".join(lines) + "\n"


def level_source(number, dir10, dir11):
    p = Path(dir11) / f"LEVEL{number}.DAT"
    return p if p.exists() else Path(dir10) / f"LEVEL{number}.DAT"


def main(argv=None):
    root = Path(__file__).resolve().parents[2]
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--levels-1-0", default=root / "archive/soldier-1.0/workdir/LEVELS", type=Path)
    ap.add_argument("--levels-1-1", default=root / "archive/soldier-1.1-patch/patch/LEVELS", type=Path)
    ap.add_argument("--out", default=root / "soldier/data/levels", type=Path)
    args = ap.parse_args(argv)

    args.out.mkdir(parents=True, exist_ok=True)
    for n in range(1, LEVEL_COUNT + 1):
        src = level_source(n, args.levels_1_0, args.levels_1_1)
        w, h, t, num, codes = read_dat(src)
        if num != n:
            raise ValueError(f"{src}: header says level {num}")
        obj = to_json_obj(w, h, t, num, codes)
        assert from_json_obj(obj) == (w, h, t, num, codes)
        (args.out / f"{n:02d}.json").write_text(dump(obj), encoding="utf-8", newline="\n")
        print(f"{n:02d}.json  {w}x{h}  time {t:<3}  from {src.parent.parent.name}/{src.parent.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
