"""Shared helpers for the asset generators.

Sprite sheet JSON format (engine format):
    {"image": "file.png",
     "frames": {"name": {"x": .., "y": .., "w": .., "h": .. [, "pivot_x": .., "pivot_y": ..]}},
     "animations": {"name": {"frames": ["f1", "f2"], "ms": 100, "loop": true}}}
Per-frame durations are written as {"frames": [{"frame": "f1", "ms": 100}, ...]} (engine/README.md,
engine/src/sprite_sheet_data.cpp). A pivot is the point (relative to the frame's top-left) that is
placed on the draw position; frames taller than a cell use it so they can be drawn at the cell origin.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ASSETS = Path(__file__).resolve().parent
FONTS = ASSETS / "fonts"
PREVIEW_DIR = Path(os.environ.get(
    "ASSET_PREVIEW_DIR",
    r"C:/Users/esdf3/AppData/Local/Temp/claude/E--Work-Remakes/806d9525-f68c-46b7-8b46-01a36f1389e9/scratchpad/previews"))


def hexrgb(h: str, a: int = 255):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


class Sheet:
    """Collects named frames (PIL RGBA images) and packs them into one PNG + JSON.

    Frames are packed in insertion order into rows; frames of the same size flow
    left-to-right, a row break happens when the width would exceed max_width.
    A 1 px transparent gap separates frames (prevents bleeding with filtering).
    """

    def __init__(self, max_width: int = 1024, gap: int = 1):
        self.frames: dict[str, Image.Image] = {}
        self.pivots: dict[str, tuple[int, int]] = {}
        self.animations: dict[str, dict] = {}
        self.max_width = max_width
        self.gap = gap

    def add(self, name: str, img: Image.Image, pivot: tuple[int, int] | None = None):
        if name in self.frames:
            raise KeyError(f"duplicate frame {name}")
        self.frames[name] = img.convert("RGBA")
        if pivot and pivot != (0, 0):
            self.pivots[name] = pivot
        return name

    def anim(self, name: str, frames: list[str], ms, loop: bool = True):
        for f in frames:
            if f not in self.frames:
                raise KeyError(f"animation {name}: unknown frame {f}")
        if isinstance(ms, list) and len(ms) != len(frames):
            raise ValueError(f"animation {name}: ms/frames length mismatch")
        self.animations[name] = {"frames": list(frames), "ms": ms, "loop": loop}

    def save(self, png_path: Path):
        png_path = Path(png_path)
        png_path.parent.mkdir(parents=True, exist_ok=True)
        g = self.gap
        x = y = 0
        row_h = 0
        rects = {}
        width = 0
        for name, im in self.frames.items():
            w, h = im.size
            if x > 0 and x + w > self.max_width:
                x = 0
                y += row_h + g
                row_h = 0
            rects[name] = (x, y, w, h)
            x += w + g
            row_h = max(row_h, h)
            width = max(width, x - g)
        height = y + row_h
        sheet = Image.new("RGBA", (max(width, 1), max(height, 1)), (0, 0, 0, 0))
        for name, im in self.frames.items():
            rx, ry, _, _ = rects[name]
            sheet.paste(im, (rx, ry))
        save_png(sheet, png_path)
        data = {
            "image": png_path.name,
            "frames": {n: self._frame_json(n, r) for n, r in rects.items()},
            "animations": {n: self._anim_json(a) for n, a in self.animations.items()},
        }
        png_path.with_suffix(".json").write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8")
        return data


    def _frame_json(self, n, r):
        d = {"x": r[0], "y": r[1], "w": r[2], "h": r[3]}
        if n in self.pivots:
            d["pivot_x"], d["pivot_y"] = self.pivots[n]
        return d

    @staticmethod
    def _anim_json(a):
        ms = a["ms"]
        if isinstance(ms, list):
            if len(set(ms)) == 1:
                return {"frames": a["frames"], "ms": int(ms[0]), "loop": a["loop"]}
            return {"frames": [{"frame": f, "ms": int(m)} for f, m in zip(a["frames"], ms)], "loop": a["loop"]}
        return {"frames": a["frames"], "ms": int(ms), "loop": a["loop"]}


def save_png(img: Image.Image, path: Path):
    """Deterministic PNG (no timestamps/metadata)."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="PNG", optimize=True)


def rel(p: Path) -> str:
    return Path(p).resolve().relative_to(ROOT).as_posix()
