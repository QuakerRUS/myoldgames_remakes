"""Copy the OFL fonts (downloaded once from github.com/google/fonts, stored in
tools/assets/fonts/) into each program's data/fonts directory together with the licences."""
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from common import FONTS, ROOT  # noqa: E402

SETS = {
    "soldier/data/fonts": [("pressstart2p", "PressStart2P-Regular.ttf"), ("vt323", "VT323-Regular.ttf")],
    "piton/data/fonts": [("pressstart2p", "PressStart2P-Regular.ttf"), ("vt323", "VT323-Regular.ttf")],
    "keyboard-simulator/data/fonts": [("ptsans", "PT_Sans-Web-Regular.ttf"), ("ptsans", "PT_Sans-Web-Bold.ttf"),
                                      ("ptmono", "PTM55FT.ttf")],
}
RENAME = {"PT_Sans-Web-Regular.ttf": "PTSans-Regular.ttf", "PT_Sans-Web-Bold.ttf": "PTSans-Bold.ttf",
          "PTM55FT.ttf": "PTMono-Regular.ttf"}


def main():
    for dst, fonts in SETS.items():
        d = ROOT / dst
        d.mkdir(parents=True, exist_ok=True)
        licences = []
        seen = set()
        for family, fname in fonts:
            out = RENAME.get(fname, fname)
            shutil.copyfile(FONTS / family / fname, d / out)
            if family not in seen:
                seen.add(family)
                text = (FONTS / family / "OFL.txt").read_text(encoding="utf-8").lstrip("﻿")
                licences.append(f"===== {family} ({', '.join(RENAME.get(f, f) for fa, f in fonts if fa == family)}) =====\n\n{text.strip()}\n")
        (d / "OFL.txt").write_text("\n\n".join(licences), encoding="utf-8", newline="\n")
        print("fonts ->", dst)


if __name__ == "__main__":
    main()
