"""Regenerate every asset (fonts, branding, graphics, sounds, music) deterministically.

    python -I tools/assets/build_all.py

The generators need Pillow, numpy, scipy and soundfile. If the running interpreter lacks them,
the script re-runs itself with the repository venv (.venv), creating it on demand:
    python -m venv .venv && .venv/Scripts/python -m pip install pillow numpy scipy soundfile
"""
from __future__ import annotations

import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
REQ = ["pillow", "numpy", "scipy", "soundfile"]
ORDER = ["gen_fonts.py", "gen_branding.py", "gen_soldier_gfx.py", "gen_piton_gfx.py", "gen_kbsim_gfx.py", "gen_icons.py", "gen_sfx.py", "gen_music.py"]


def venv_python() -> Path:
    if os.name == "nt":
        return ROOT / ".venv" / "Scripts" / "python.exe"
    return ROOT / ".venv" / "bin" / "python"


def have_deps() -> bool:
    try:
        import numpy  # noqa: F401
        import PIL  # noqa: F401
        import scipy  # noqa: F401
        import soundfile  # noqa: F401
        return True
    except ImportError:
        return False


def main():
    if not have_deps():
        py = venv_python()
        if not py.exists():
            print("creating .venv ...")
            subprocess.check_call([sys.executable, "-m", "venv", str(ROOT / ".venv")])
            subprocess.check_call([str(py), "-m", "pip", "install", "-q", *REQ])
        if Path(sys.executable).resolve() == py.resolve():
            sys.exit("dependencies missing in .venv: pip install " + " ".join(REQ))
        sys.exit(subprocess.call([str(py), "-I", str(Path(__file__).resolve()), *sys.argv[1:]]))

    scripts = [HERE / n for n in ORDER if (HERE / n).exists()]
    scripts += sorted(p for p in HERE.glob("gen_*.py") if p.name not in ORDER)
    for s in scripts:
        t = time.time()
        print(f"== {s.name}", flush=True)
        r = subprocess.call([sys.executable, "-I", str(s)], cwd=ROOT)
        if r:
            sys.exit(f"{s.name} failed with code {r}")
        print(f"   {time.time() - t:.1f} s", flush=True)
    if "--preview" in sys.argv:
        subprocess.call([sys.executable, "-I", str(HERE / "preview.py")], cwd=ROOT)
    print("all assets built")


if __name__ == "__main__":
    main()
