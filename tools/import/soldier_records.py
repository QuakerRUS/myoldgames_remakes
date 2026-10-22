"""Convert the original Soldier demo records of player LISIY to the new recording format.

    python -I tools/import/soldier_records.py
    python -I tools/import/soldier_records.py --workdir archive/soldier-1.0/workdir \
        --levels soldier/data/levels --out soldier/data

Reads <workdir>/RECORDS/REC0<level>.DAT (slot 0 = LISIY; slot 1 = TARAS is ignored) and
<workdir>/GAMESAV0.DAT, writes <out>/records/LISIY/NN.json and <out>/players.json.

Every record is replayed by an exact transliteration of the 1.1 replay loop of GAME1.BAS
(class OriginalReplay below, including its bugs) to find the tick at which each key was
executed and how the game ended; the outcome is stored as "result" so the C++ tests can check
that the remake ends the same way at the same tick. Formats: docs/formats/soldier-recording.md.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import soldier_levels  # noqa: E402

PLAYER = "LISIY"
SLOT = 0
GAMESAV_LEVEL_RECORD = 66  # 2-byte record number holding the level reached

# INKEY$ value (first 2 bytes of the 4-byte field, trailing space stripped) -> command name.
KEYS = {
    b"\x00P": "down", b"\x00H": "up", b"\x00K": "left", b"\x00M": "right",
    b",": "ccw", b"<": "ccw", b".": "cw", b">": "cw",
    b" ": "use", b"\r": "use", b"\t": "next", b"\x1b": "giveup",
}
DIRS = {"down": (1, 0, 1), "up": (2, 0, -1), "left": (3, -1, 0), "right": (4, 1, 0)}


def read_record(path):
    """Return (seed, gs, [(numsec, f, key bytes)]) of an original REC*.DAT."""
    data = Path(path).read_bytes()
    if len(data) < 8 or (len(data) - 8) % 12:
        raise ValueError(f"{path}: bad size {len(data)}")
    seed, gs = struct.unpack("<2i", data[:8])
    events = []
    for off in range(8, len(data), 12):
        numsec, f = struct.unpack("<2i", data[off:off + 8])
        key = data[off + 8:off + 10]
        if key[1:2] == b" ":
            key = key[:1]
        events.append((numsec, f, key))
    return seed, gs, events


def read_level_reached(path):
    data = Path(path).read_bytes()
    name = data[:8].decode("ascii").rstrip()
    off = (GAMESAV_LEVEL_RECORD - 1) * 2
    return name, struct.unpack("<h", data[off:off + 2])[0]


class Finished(Exception):
    def __init__(self, status, code=None):
        super().__init__(status)
        self.status, self.code = status, code


class OriginalReplay:
    """GAME1.BAS 1.1 rules on the original 101x101 array ml(0..100, 0..100) (outside = 0)."""

    def __init__(self, level, fix_push_right=False):
        w, h, t, _num, codes = level
        self.hor, self.ver = w, h
        self.ml = [[0] * 101 for _ in range(101)]
        self.onkey = 0
        for j in range(h):
            for i in range(w):
                c = codes[j * w + i]
                self.ml[i + 1][j + 1] = c
                if 2 < c < 7 or 10 < c < 15 or 15 < c < 20 or 48 < c < 53:
                    self.absx, self.absy = i + 1, j + 1
                if c == 9:
                    self.onkey += 1
        self.time = t if t != 0 else -1
        self.inv = [0] * 26
        self.numinv, self.invcur = 0, 1
        self.nnabsx = self.nnabsy = 0
        self.numpartsec = 1
        self.fix_push_right = fix_push_right
        self.warnings = []

    # -- array access: the original crashed outside 0..100 --
    def g(self, x, y):
        if not (0 <= x <= 100 and 0 <= y <= 100):
            raise Finished("crash")
        if not (1 <= x <= self.hor and 1 <= y <= self.ver):
            self.warnings.append(f"reads outside the level at {x},{y}")
        return self.ml[x][y]

    def s(self, x, y, v):
        self.g(x, y)
        self.ml[x][y] = v

    def me(self):
        return self.ml[self.absx][self.absy]

    def death(self, a, b, c):
        m = self.me()
        return a if m < 49 else b if 48 < m < 53 else c if 56 < m < 61 else None

    def key(self, k):  # 1300 with game = 2
        if k == b"\x1b":
            raise Finished("dead", self.death(7, 53, 61))
        if k in (b",", b"<"):
            self.turn(False)
        if k in (b".", b">"):
            self.turn(True)
        if k in (b" ", b"\r") and self.numinv > 0:
            self.use()
        if len(k) == 1:
            return
        name = KEYS.get(k)
        if name in DIRS:
            self.n, self.dx, self.dy = DIRS[name]
            self.nabsx, self.nabsy = self.absx + self.dx, self.absy + self.dy
            self.move()

    def turn(self, cw):  # 1800 / 1850
        m = self.me()
        for base in (3, 11, 16, 49, 57):
            if base <= m < base + 4:
                o = m - base
                o = (2, 3, 1, 0)[o] if cw else (3, 2, 0, 1)[o]
                self.ml[self.absx][self.absy] = base + o

    def use(self):  # 1960
        m = self.me()
        for base in (3, 11, 16):
            if base <= m < base + 4:
                dx, dy = ((0, 1), (0, -1), (-1, 0), (1, 0))[m - base]
                self.nnabsx, self.nnabsy = self.absx + dx, self.absy + dy
                break
        else:
            self.warnings.append("use from a guillotine/sunk box (stale target)")
        if self.inv[self.invcur] in (21, 25, 29, 33, 37, 41):
            self.usekey(self.inv[self.invcur])

    def usekey(self, k):  # 20000..20500
        x, y = self.nnabsx, self.nnabsy
        if self.g(x, y) == 0:
            self.s(x, y, k); self.remove()
        if self.g(x, y) == 9:
            self.s(x, y, k + 2); self.onkey -= 1; self.remove()
        if self.g(x, y) == 15 and self.onkey == 0:
            self.s(x, y, k + 3); self.remove()
        if self.g(x, y) == k + 1:
            self.s(x, y, 0); self.remove()

    def remove(self):  # 12300
        if self.invcur != self.numinv:
            for f in range(self.invcur + 1, self.numinv + 1):
                self.inv[f - 1] = self.inv[f]
        self.inv[self.numinv] = 0
        self.numinv -= 1
        self.invcur = 1

    def leave(self):  # 12100
        m, x, y = self.me(), self.absx, self.absy
        if 2 < m < 7: self.ml[x][y] = 0
        if 10 < m < 15: self.ml[x][y] = 9; self.onkey += 1
        if 15 < m < 20: self.ml[x][y] = 15
        if 48 < m < 53: self.ml[x][y] = 48
        if 56 < m < 61: self.ml[x][y] = 56
        self.absx, self.absy = self.nabsx, self.nabsy

    def walk(self, base):
        self.leave()
        self.s(self.absx, self.absy, base + self.n - 1)
        self.moved = True

    def bump(self):  # 2100 (does not turn on a sunk box)
        m = self.me()
        for base in (3, 11, 16, 49):
            if base <= m < base + 4:
                self.ml[self.absx][self.absy] = base + self.n - 1

    def pushable(self, water):  # checks of 2300 / 2500 / 2700 / 5600
        t2 = self.g(self.absx + 2 * self.dx, self.absy + 2 * self.dy)
        blocked = t2 not in ({0, 9, 15, 54, 56} if water else {0, 9, 15})
        if water and self.n == 4:
            buggy = t2 not in (0, 9, 15, 54) and self.g(self.absx - 2, self.absy) != 56
            if buggy != blocked:
                self.warnings.append("depends on the push-right bug")
            if not self.fix_push_right:
                blocked = buggy
        return not blocked and not (t2 == 15 and self.onkey > 0)

    def push(self):  # 12200
        x, y = self.absx + 2 * self.dx, self.absy + 2 * self.dy
        t = self.g(x, y)
        new = {0: 8, 9: 10, 15: 20, 54: 56, 56: 63}.get(t)
        if t == 9:
            self.onkey -= 1
        if new is not None:
            self.s(x, y, new)

    def move(self):  # 1500
        t = self.g(self.nabsx, self.nabsy)
        if t in (0, 46):
            self.walk(3)
        elif t == 1:
            self.bump()
        elif t == 2:
            self.leave()
            raise Finished("won")
        elif t in (8, 63, 10, 20):
            if self.pushable(t in (8, 63)):
                self.push()
                self.walk({8: 3, 63: 57, 10: 11, 20: 16}[t])
            else:
                self.bump()
        elif t == 9:
            self.leave(); self.onkey -= 1
            self.s(self.absx, self.absy, 10 + self.n); self.moved = True
        elif t == 15:
            self.bump() if self.onkey > 0 else self.walk(16)
        elif 21 <= t <= 44:
            kind, key = (t - 21) % 4, 21 + 4 * ((t - 21) // 4)
            if kind == 1:
                found = [f for f in range(1, self.numinv + 1) if self.inv[f] == key]
                if not found:
                    return self.bump()
                self.invcur = found[0]
                self.nnabsx, self.nnabsy = self.nabsx, self.nabsy
                self.usekey(key)
                self.walk(3)
            elif self.numinv == 25:
                self.bump()
            else:
                self.numinv += 1
                self.inv[self.numinv] = key
                self.walk({0: 3, 2: 11, 3: 16}[kind])
        elif t == 45:
            self.s(self.nabsx, self.nabsy, 1)
            self.bump()
        elif t == 48:
            self.bump() if self.numpartsec == 4 else self.walk(49)
        elif t == 54:
            self.leave()
            self.s(self.absx, self.absy, 55)
            raise Finished("dead", 55)
        elif t == 56:
            self.walk(57)

    def begin_part(self, numsec):
        """Boundary checks of the main loop when part `numsec` (0-based, >= 1) begins."""
        self.numpartsec = numsec % 4 + 1
        if self.numpartsec == 4 and 48 < self.me() < 53:
            raise Finished("dead", 53)
        if self.numpartsec == 1:
            if self.time == 0:
                raise Finished("dead", self.death(47, 53, 62))
            if self.time != -1:
                self.time -= 1


DEATH_CAUSE = {7: "gave_up", 61: "gave_up", 47: "time_out", 62: "time_out", 53: "guillotine", 55: "drowned"}


def replay(level, gs, events, fix_push_right=False, extra_seconds=1000):
    """Replay as the original (one key per loop iteration, at f/gs >= recorded f/gs).

    Returns (commands [(tick, name)], result dict, warnings, cooldown_violations).
    """
    emu = OriginalReplay(level, fix_push_right)
    numsec, last_pos, commands, last_move, violations = 0, -1, [], None, 0
    try:
        for numsec_rec, f, key in events:
            pos = max(numsec_rec * gs + (f - 1), last_pos + 1)
            while numsec < pos // gs:
                numsec += 1
                emu.begin_part(numsec)
            last_pos = pos
            name = KEYS.get(key)
            if name is None:
                emu.warnings.append(f"ignored key {key!r}")
                continue
            commands.append((numsec, name))
            emu.moved = False
            try:
                emu.key(key)
            finally:
                if name in DIRS and (emu.moved or emu.me() in (2, 55)):
                    if last_move is not None and numsec < last_move + 2:
                        violations += 1
                    last_move = numsec
        for _ in range(4 * extra_seconds):
            numsec += 1
            emu.begin_part(numsec)
        result = {"status": "playing", "tick": numsec}
    except Finished as fin:
        result = {"status": fin.status, "tick": numsec}
        if fin.status == "dead":
            result["cause"] = DEATH_CAUSE[fin.code]
    return commands, result, sorted(set(emu.warnings)), violations


def dump(rec):
    head = {k: rec[k] for k in ("format", "version", "level", "player", "source", "original", "result")}
    lines = ["{"] + [f'  "{k}": {json.dumps(v)},' for k, v in head.items()]
    cmds = [json.dumps(c) for c in rec["commands"]]
    lines.append('  "commands": [')
    for i in range(0, len(cmds), 8):
        chunk = ", ".join(cmds[i:i + 8])
        lines.append(f"    {chunk}{',' if i + 8 < len(cmds) else ''}")
    lines += ["  ]", "}"]
    return "\n".join(lines) + "\n"


def main(argv=None):
    root = Path(__file__).resolve().parents[2]
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--workdir", default=root / "archive/soldier-1.0/workdir", type=Path,
                    help="folder with RECORDS/ and GAMESAV0.DAT")
    ap.add_argument("--levels", default=root / "soldier/data/levels", type=Path,
                    help="imported level JSON files (run soldier_levels.py first)")
    ap.add_argument("--out", default=root / "soldier/data", type=Path)
    args = ap.parse_args(argv)

    out_dir = args.out / "records" / PLAYER
    out_dir.mkdir(parents=True, exist_ok=True)
    problems = 0
    for n in range(1, soldier_levels.LEVEL_COUNT + 1):
        src = args.workdir / "RECORDS" / f"REC{SLOT}{n}.DAT"
        if not src.exists():
            print(f"{n:02d}: no record")
            continue
        level = soldier_levels.from_json_obj(json.loads((args.levels / f"{n:02d}.json").read_text("utf-8")))
        seed, gs, events = read_record(src)
        commands, result, warnings, violations = replay(level, gs, events)
        _, fixed_result, _, _ = replay(level, gs, events, fix_push_right=True)
        if fixed_result != result:
            warnings.append(f"with the push-right fix: {fixed_result}")
        if violations:
            warnings.append(f"{violations} moves inside the 2-tick cooldown")
        rec = {
            "format": "soldier-recording", "version": 1, "level": n, "player": PLAYER,
            "source": src.name, "original": {"seed": seed, "game_speed": gs},
            "result": result, "commands": commands,
        }
        (out_dir / f"{n:02d}.json").write_text(dump(rec), encoding="utf-8", newline="\n")
        flag = "" if result["status"] == "won" and not warnings else "  <-- " + "; ".join(warnings or [str(result)])
        problems += bool(flag)
        print(f"{n:02d}.json  {len(commands):4d} commands  {result['status']} at tick {result['tick']}{flag}")

    name, reached = read_level_reached(args.workdir / f"GAMESAV{SLOT}.DAT")
    if name != PLAYER:
        raise ValueError(f"GAMESAV{SLOT}.DAT belongs to {name!r}")
    players = {"players": [{"name": PLAYER, "level_reached": reached, "builtin": True}]}
    (args.out / "players.json").write_text(json.dumps(players, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"players.json  {PLAYER} level_reached {reached}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
