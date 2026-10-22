"""Convert the sentences of the 2007 typing trainer to UTF-8 for Keyboard Simulator.

    python -I tools/import/kbsim_texts.py
    python -I tools/import/kbsim_texts.py --source archive/diplom-2007/program/Data/Texts/text_rus.txt \
        --dest keyboard-simulator/data/texts/ru.txt

Input: CP866, CRLF, one sentence per line. Output: UTF-8 without BOM, LF, one sentence per
line, final newline. The only change to the content: a Latin 'c' typed by mistake inside a
Russian word is replaced with the Cyrillic 'с' (it cannot be typed on the ЙЦУКЕН layout).
"""
import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE = REPO / "archive/diplom-2007/program/Data/Texts/text_rus.txt"
DEFAULT_DEST = REPO / "keyboard-simulator/data/texts/ru.txt"

# Latin letters that look like Cyrillic ones; only these may be silently fixed.
HOMOGLYPHS = str.maketrans("aceopxyACEHKMOPTXB", "асеорхуАСЕНКМОРТХВ")
SENTENCE_END = (".", "!", "?")


def convert(raw: bytes) -> tuple[list[str], int]:
    """Return (sentences, number of fixed Latin homoglyphs)."""
    text = raw.decode("cp866")
    lines = text.split("\r\n")
    if lines and lines[-1] == "":
        lines.pop()
    fixed = 0
    out = []
    for n, line in enumerate(lines, 1):
        if "\n" in line or "\r" in line:
            raise ValueError(f"line {n}: stray line break")
        new = re.sub(r"[A-Za-z]", lambda m: m.group(0).translate(HOMOGLYPHS), line)
        fixed += sum(a != b for a, b in zip(line, new))
        if re.search(r"[A-Za-z]", new):
            raise ValueError(f"line {n}: Latin letter without a Cyrillic look-alike: {new!r}")
        if not new or not new.endswith(SENTENCE_END):
            raise ValueError(f"line {n}: does not end with . ! ?: {new!r}")
        if new != new.strip() or "  " in new:
            raise ValueError(f"line {n}: unexpected whitespace: {new!r}")
        out.append(new)
    return out, fixed


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    ap.add_argument("--dest", type=Path, default=DEFAULT_DEST)
    args = ap.parse_args()

    sentences, fixed = convert(args.source.read_bytes())
    args.dest.parent.mkdir(parents=True, exist_ok=True)
    with open(args.dest, "w", encoding="utf-8", newline="\n") as f:
        for s in sentences:
            f.write(s + "\n")
    chars = sorted(set("".join(sentences)))
    sys.stdout.reconfigure(encoding="utf-8")
    print(f"{len(sentences)} sentences, {sum(map(len, sentences))} characters, "
          f"{fixed} Latin look-alike(s) fixed -> {args.dest}")
    print("alphabet:", "".join(chars))
    return 0


if __name__ == "__main__":
    sys.exit(main())
