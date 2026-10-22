"""Build archive/ from the original zips, byte-for-byte, plus UTF-8 copies of the sources.

    python -I tools/import/build_archive.py --source E:/Archive/Victor/Work --dest archive
    python -I tools/import/build_archive.py --source E:/Archive/Victor/Work --dest archive --verify
"""
import argparse, hashlib, sys, zipfile
from pathlib import Path

# (zip relative to --source, member prefix inside the zip, destination under --dest, excluded member prefixes, excluded suffixes)
RULES = [
    ("QBasic/Soldier/soldier.zip", "", "soldier-1.0/release", [], []),
    ("QBasic/QBasic.zip", "SOLDIER/", "soldier-1.0/workdir", ["SOLDIER/SOLD_BAS/"], []),
    ("QBasic/QBasic.zip", "SOLDIER/SOLD_BAS/", "soldier-1.0/src", [], []),
    ("QBasic/Soldier/patch1_1.zip", "", "soldier-1.1-patch/patch", [], []),
    ("QBasic/Soldier/source1_1.zip", "", "soldier-1.1-patch/src", [], []),
    ("QBasic/QBasic.zip", "SOLDIER.NEW/", "soldier-2.0", [], []),
    ("QBasic/QBasic.zip", "PITON/", "piton", [], []),
    ("Diplom.zip", "Program/", "diplom-2007/program", [], []),
    ("Diplom.zip", "ForDiplom/", "diplom-2007/for-diplom", [], [".doc"]),
]
SOURCE_ENCODINGS = {".bas": "cp866", ".cpp": "cp1251"}
# Hand-written content inside --dest: never listed in the manifest, kept by --force.
HANDWRITTEN_DIRS = ("dosbox",)


def handwritten(dest, f):
    """True for READMEs and the DOSBox configs, which are not originals."""
    return f.suffix == ".md" or f.relative_to(dest).parts[0] in HANDWRITTEN_DIRS


def entries(source):
    """Yield (zip path, ZipInfo, destination relative path) for every file and directory to archive."""
    for zip_rel, prefix, dest, excluded, bad_suffixes in RULES:
        zpath = source / zip_rel
        with zipfile.ZipFile(zpath) as zf:
            for info in zf.infolist():
                name = info.filename
                if not name.startswith(prefix) or any(name.startswith(e) for e in excluded):
                    continue
                rel = name[len(prefix):]
                if not rel or any(rel.lower().endswith(s) for s in bad_suffixes):
                    continue
                yield zpath, info, Path(dest) / rel


def build(source, dest):
    for zpath, info, rel in entries(source):
        target = dest / rel
        if info.is_dir():
            target.mkdir(parents=True, exist_ok=True)
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(zpath) as zf:
            target.write_bytes(zf.read(info))
    for d in sorted(p for p in dest.rglob("*") if p.is_dir()):
        if not any(d.iterdir()):
            (d / ".gitkeep").write_bytes(b"")
    for f in sorted(p for p in dest.rglob("*") if p.is_file()):
        enc = SOURCE_ENCODINGS.get(f.suffix.lower())
        if enc and "src-utf8" not in f.parts:
            out = f.parent / "src-utf8" / f.name
            out.parent.mkdir(exist_ok=True)
            out.write_bytes(f.read_bytes().decode(enc).encode("utf-8"))
    lines = []
    for f in sorted(p for p in dest.rglob("*") if p.is_file()):
        rel = f.relative_to(dest).as_posix()
        if "src-utf8" in f.parts or f.name in (".gitkeep", "MANIFEST.sha256") or handwritten(dest, f):
            continue
        lines.append("%s  %s" % (hashlib.sha256(f.read_bytes()).hexdigest(), rel))
    (dest / "MANIFEST.sha256").write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")
    return len(lines)


def verify(source, dest):
    bad = 0
    for zpath, info, rel in entries(source):
        if info.is_dir():
            continue
        target = dest / rel
        with zipfile.ZipFile(zpath) as zf:
            want = hashlib.sha256(zf.read(info)).hexdigest()
        have = hashlib.sha256(target.read_bytes()).hexdigest() if target.is_file() else None
        if have != want:
            print("MISMATCH", rel.as_posix())
            bad += 1
    print("verified, mismatches:", bad)
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--dest", type=Path, required=True)
    ap.add_argument("--verify", action="store_true")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    if a.verify:
        return 1 if verify(a.source, a.dest) else 0
    generated = [p for p in a.dest.rglob("*") if p.is_file() and not handwritten(a.dest, p)] if a.dest.exists() else []
    if generated:
        if not a.force:
            sys.exit("dest is not empty; use --force")
        for p in generated:
            p.unlink()
        for d in sorted((p for p in a.dest.rglob("*") if p.is_dir()), key=lambda p: len(p.parts), reverse=True):
            if not any(d.iterdir()):
                d.rmdir()
    a.dest.mkdir(parents=True, exist_ok=True)
    print("archived files:", build(a.source, a.dest))
    return 0


if __name__ == "__main__":
    sys.exit(main())
