"""Soldier palette (one fixed palette for every Soldier sprite)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pixel import Palette  # noqa: E402

COLOURS = {
    # outlines / darks
    "ink": "#0f0e17", "ink2": "#1d1b29",
    # neutral greys (slightly cool) - concrete, steel
    "g0": "#2a2b39", "g1": "#3b3d4e", "g2": "#4f5263", "g3": "#666a7a",
    "g4": "#7f8391", "g5": "#999ca8", "g6": "#b3b6bf", "g7": "#cdd0d5", "g8": "#e7e8ea",
    "white": "#fbfaf5",
    # brick (wall)
    "br0": "#2e1216", "br1": "#4e1c1e", "br2": "#702a26", "br3": "#923d31", "br4": "#b25840", "br5": "#d17f5c",
    "mortar0": "#3a3036", "mortar1": "#57494c",
    # dark wall (mysterious indigo)
    "dk0": "#0b0d22", "dk1": "#141a3c", "dk2": "#1f2a5c", "dk3": "#2c3d7d", "dk4": "#3f569e", "dkglow": "#7ad8ff",
    "dkglow2": "#c8f4ff",
    # wood (crate)
    "wd0": "#2f1c12", "wd1": "#4c2f1b", "wd2": "#6d4626", "wd3": "#8f6233", "wd4": "#b2833f", "wd5": "#d3a85a",
    # olive (soldier)
    "ol0": "#1b2412", "ol1": "#2d3b1c", "ol2": "#435628", "ol3": "#5c7334", "ol4": "#7b9444", "ol5": "#a2b85e",
    # skin
    "sk0": "#5a3021", "sk1": "#93573c", "sk2": "#cf8e66", "sk3": "#efbf93",
    # exit / glow green
    "gr0": "#0f3a26", "gr1": "#16683c", "gr2": "#22a356", "gr3": "#4fdc7a", "gr4": "#a9ffb6",
    # water
    "wa0": "#0a1f3d", "wa1": "#0f3563", "wa2": "#16528a", "wa3": "#1f75ad", "wa4": "#3a9ccc", "wa5": "#79cbe6",
    "wa6": "#c6f1f7",
    # key / door colours
    "red0": "#5c0f1b", "red1": "#9a1e2a", "red2": "#d4383a", "red3": "#ff7a5c",
    "grn0": "#0c3d1f", "grn1": "#1c7a35", "grn2": "#33b34a", "grn3": "#8fe873",
    "blu0": "#101f5a", "blu1": "#1f3fa0", "blu2": "#3a6be0", "blu3": "#8ab4ff",
    "yel0": "#5f4205", "yel1": "#a87a12", "yel2": "#f0be2e", "yel3": "#fff08a",
    # fire / blood
    "fire0": "#fff6c8", "fire1": "#ffd25e", "fire2": "#ff9a2e", "fire3": "#e2542a",
    "blood0": "#5e0d14", "blood1": "#8f1622", "blood2": "#c02a2e",
}

# translucent helpers (not palette colours, only used for shadows/glows)
SHADOW = (8, 6, 16, 110)
SHADOW_SOFT = (8, 6, 16, 60)

PAL = Palette(COLOURS)
