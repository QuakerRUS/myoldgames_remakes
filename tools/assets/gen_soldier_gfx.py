"""Generate all Soldier graphics into soldier/data/gfx/.

    python -I tools/assets/gen_soldier_gfx.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import ROOT, Sheet  # noqa: E402
import soldier_tiles as st  # noqa: E402

OUT = ROOT / "soldier" / "data" / "gfx"
COLOURS = ["red", "green", "blue", "yellow", "white", "black"]
DOOR_STAGES = 5


def build_tiles() -> Sheet:
    s = Sheet(max_width=32 * 16 + 15)
    for v in range(4):
        s.add(f"floor_{v}", st.floor(v).img())
    for m in range(16):
        s.add(f"wall_{m}", st.wall(m).img())
    for m in range(16):
        s.add(f"darkwall_{m}", st.wall(m, st.DARK_PAL, seed=9).img())
    for k in ("n", "w", "nw"):
        s.add(f"shadow_{k}", st.shadow(k).img())
    for f in range(8):
        s.add(f"darkwall_shimmer_{f}", st.dark_shimmer(f).img())
    s.anim("darkwall_shimmer", [f"darkwall_shimmer_{f}" for f in range(8)], [120] * 7 + [2400], True)
    for f in range(7):
        s.add(f"reveal_fx_{f}", st.reveal_fx(f).img())
    s.anim("reveal_fx", [f"reveal_fx_{f}" for f in range(7)], 60, False)
    for f in range(8):
        s.add(f"exit_{f}", st.exit_tile(f).img())
    s.anim("exit", [f"exit_{f}" for f in range(8)], 110, True)
    for i, n in enumerate(("button_up", "button_half", "button_down")):
        s.add(n, st.button(i).img())
    s.anim("button_press", ["button_up", "button_half", "button_down"], 60, False)
    s.anim("button_release", ["button_down", "button_half", "button_up"], 60, False)
    for col in [None] + COLOURS:
        base = "door" if col is None else f"door_{col}"
        for i in range(DOOR_STAGES):
            s.add(f"{base}_{i}", st.door(i, col, DOOR_STAGES).img())
        fr = [f"{base}_{i}" for i in range(DOOR_STAGES)]
        s.anim(f"{base}_open", fr, 70, False)
        s.anim(f"{base}_close", fr[::-1], 70, False)
    for col in COLOURS:
        s.add(f"key_{col}", st.key(col).img())
    s.add("guillotine_base", st.guillotine_base().img())
    for i in range(4):
        s.add(f"guillotine_top_{i}", st.guillotine_top(i).img(), pivot=(0, 16))
    for i in range(4):
        s.add(f"guillotine_{i}", st.guillotine(i).img(), pivot=(0, 16))
    s.anim("guillotine_close", [f"guillotine_{i}" for i in range(4)], [40, 40, 40, 100], False)
    s.anim("guillotine_open", [f"guillotine_{i}" for i in (3, 2, 1, 0)], [80, 80, 80, 80], False)
    s.anim("guillotine_top_close", [f"guillotine_top_{i}" for i in range(4)], [40, 40, 40, 100], False)
    s.anim("guillotine_top_open", [f"guillotine_top_{i}" for i in (3, 2, 1, 0)], 80, False)
    for f in range(8):
        s.add(f"water_{f}", st.water(f).img())
    s.anim("water", [f"water_{f}" for f in range(8)], 140, True)
    for side in "nesw":
        s.add(f"water_edge_{side}", st.water_edge(side).img())
    s.add("box", st.box().img())
    for f in range(6):
        s.add(f"box_sink_{f}", st.box_sink(f).img())
    s.anim("box_sink", [f"box_sink_{f}" for f in range(6)], 70, False)
    for f in range(8):
        s.add(f"sunk_box_{f}", st.sunk_box(f).img())
    s.anim("sunk_box", [f"sunk_box_{f}" for f in range(8)], 140, True)
    for f in range(4):
        s.add(f"dust_{f}", st.dust_puff(f).img())
    s.anim("dust", [f"dust_{f}" for f in range(4)], 70, False)
    return s


def main():
    import soldier_char as sc
    import soldier_ui as su
    build_tiles().save(OUT / "tiles.png")
    sc.build_player().save(OUT / "player.png")
    su.build_all(OUT)
    print("soldier gfx ->", OUT)


if __name__ == "__main__":
    main()
