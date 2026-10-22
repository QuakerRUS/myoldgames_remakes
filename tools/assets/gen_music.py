"""Procedurally composed music (OGG Vorbis, 44.1 kHz stereo).

Run: python -I tools/assets/gen_music.py

Every piece is written out note-by-note below (melodies, chord progressions,
bass and drum patterns) and rendered with the small synthesizer in synth.py.
Looped pieces are rendered with a tail which is folded back onto the start, so
the loop point is seamless (reverb/delay tails wrap around) and the file length is
an exact number of bars. Deterministic: fixed seeds everywhere.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402
from scipy import signal  # noqa: E402

from synth import (SR, adsr, bandpass, exp_env, highpass, lofi_noise, lowpass, lowpass_sweep,  # noqa: E402
                   make_ir, midi_hz, nes_tri, note, pulse, saw, secs, sine, soft_clip)

ROOT = Path(__file__).resolve().parents[2]
PREVIEW = Path(r"C:/Users/esdf3/AppData/Local/Temp/claude/E--Work-Remakes/806d9525-f68c-46b7-8b46-01a36f1389e9/scratchpad/previews/audio")

# =============================================================== instruments
# Each instrument: fn(midi, dur_seconds, vel, rng) -> mono array (gate + release)


def _vib(n, f, depth=0.004, rate=5.5, delay=0.18):
    t = np.arange(n) / SR
    ramp = np.clip((t - delay) / 0.25, 0, 1)
    return f * (1 + depth * ramp * np.sin(2 * np.pi * rate * t))


def i_lead(m, dur, vel, rng, duty=0.25, rel=0.09):
    n = secs(dur + rel)
    f = _vib(n, midi_hz(m))
    x = pulse(f, n, duty) * adsr(n, 0.004, 0.12, 0.62, rel, gate=dur)
    return lowpass(x, 7000) * vel


def i_lead_thin(m, dur, vel, rng):
    return i_lead(m, dur, vel, rng, duty=0.125)


def i_lead_sq(m, dur, vel, rng):
    return i_lead(m, dur, vel, rng, duty=0.5) * 0.8


def i_brass(m, dur, vel, rng):
    rel = 0.12
    n = secs(dur + rel)
    f = _vib(n, midi_hz(m), 0.003)
    x = saw(f * 1.003, n) + saw(f * 0.997, n) + 0.4 * pulse(f * 0.5, n, 0.5)
    t = np.arange(n) / SR
    cut = 500 + 2600 * np.minimum(t / 0.06, 1) * np.exp(-t * 2.5) + 900
    x = lowpass_sweep(x, cut, q=0.8, block=256)
    return x * adsr(n, 0.02, 0.15, 0.75, rel, gate=dur) * vel * 0.35


def i_pad(m, dur, vel, rng):
    rel = 0.6
    n = secs(dur + rel)
    f = midi_hz(m)
    t = np.arange(n) / SR
    lfo = 1 + 0.002 * np.sin(2 * np.pi * 0.3 * t + rng.uniform(0, 6))
    x = sum(saw(f * d * lfo, n, rng.uniform()) for d in (0.996, 1.0, 1.0045))
    x = lowpass(x, 1500, order=2)
    return x * adsr(n, 0.35, 0.3, 0.8, rel, gate=dur) * vel * 0.12


def i_pad_soft(m, dur, vel, rng):
    rel = 0.8
    n = secs(dur + rel)
    f = midi_hz(m)
    x = sum(pulse(f * d, n, 0.3, rng.uniform()) for d in (0.997, 1.003))
    x = lowpass(x, 1100)
    return x * adsr(n, 0.5, 0.3, 0.85, rel, gate=dur) * vel * 0.14


def i_bass_tri(m, dur, vel, rng):
    rel = 0.03
    n = secs(dur + rel)
    x = nes_tri(midi_hz(m), n)
    return x * adsr(n, 0.003, 0.05, 0.9, rel, gate=max(dur - 0.01, 0.01)) * vel


def i_bass_synth(m, dur, vel, rng):
    rel = 0.05
    n = secs(dur + rel)
    f = midi_hz(m)
    x = saw(f, n) * 0.7 + pulse(f * 0.5, n, 0.5) * 0.5
    t = np.arange(n) / SR
    x = lowpass_sweep(x, 250 + 1400 * np.exp(-t * 14), q=1.1, block=128)
    return x * adsr(n, 0.003, 0.08, 0.8, rel, gate=dur) * vel * 0.8


def i_pluck(m, dur, vel, rng):
    n = secs(min(dur, 0.25) + 0.12)
    x = pulse(midi_hz(m), n, 0.125) * exp_env(n, 0.07, 0.002)
    return lowpass(x, 5000) * vel


def i_glass(m, dur, vel, rng):
    rel = 0.4
    n = secs(dur + rel)
    t = np.arange(n) / SR
    f = _vib(n, midi_hz(m), 0.003, 4.8, 0.3)
    ph = np.cumsum(f) / SR
    idx = 1.6 * np.exp(-t * 3) + 0.35
    x = np.sin(2 * np.pi * ph + idx * np.sin(2 * np.pi * 2 * ph))
    x += 0.25 * np.sin(2 * np.pi * 2 * ph)
    return x * adsr(n, 0.01, 0.4, 0.55, rel, gate=dur) * vel * 0.8


def i_timp(m, dur, vel, rng):
    n = secs(1.2)
    t = np.arange(n) / SR
    f = midi_hz(m) * (1 + 0.15 * np.exp(-t * 30))
    x = sine(f, n) * np.exp(-t * 2.5) + 0.4 * sine(f * 1.5, n) * np.exp(-t * 4)
    x += lowpass(rng.uniform(-1, 1, n), 800) * np.exp(-t * 40) * 0.6
    return x * vel


# ---- drums (midi ignored)
def d_kick(m, dur, vel, rng):
    n = secs(0.35)
    t = np.arange(n) / SR
    f = 45 + 110 * np.exp(-t * 28)
    x = sine(f, n) * np.exp(-t * 9)
    x += lowpass(rng.uniform(-1, 1, n), 3000) * np.exp(-t * 250) * 0.5
    return soft_clip(x * 1.4, 1.5) * vel


def d_snare(m, dur, vel, rng):
    n = secs(0.25)
    t = np.arange(n) / SR
    tone = sine(185 * (1 + 0.3 * np.exp(-t * 40)), n) * np.exp(-t * 25) * 0.6
    nz = bandpass(rng.uniform(-1, 1, n), 1200, 9000) * np.exp(-t * 16)
    return (tone + nz * 0.9) * vel * 0.8


def d_snare_chip(m, dur, vel, rng):
    n = secs(0.18)
    t = np.arange(n) / SR
    nz = highpass(lofi_noise(n, rng, 11000), 900) * np.exp(-t * 22)
    tone = pulse(220 * np.exp(-t * 6), n, 0.5) * np.exp(-t * 35) * 0.3
    return (nz + tone) * vel * 0.7


def d_hat(m, dur, vel, rng):
    n = secs(0.06)
    t = np.arange(n) / SR
    return highpass(rng.uniform(-1, 1, n), 7000) * np.exp(-t * 70) * vel * 0.5


def d_ohat(m, dur, vel, rng):
    n = secs(0.3)
    t = np.arange(n) / SR
    return highpass(rng.uniform(-1, 1, n), 6500) * np.exp(-t * 9) * vel * 0.4


def d_rim(m, dur, vel, rng):
    n = secs(0.08)
    t = np.arange(n) / SR
    x = sine(1700, n) * np.exp(-t * 90) + bandpass(rng.uniform(-1, 1, n), 2000, 6000) * np.exp(-t * 120) * 0.6
    return x * vel * 0.5


def d_crash(m, dur, vel, rng):
    n = secs(1.8)
    t = np.arange(n) / SR
    x = highpass(rng.uniform(-1, 1, n), 4000) * np.exp(-t * 2.2)
    x += bandpass(rng.uniform(-1, 1, n), 2500, 6000) * np.exp(-t * 4) * 0.5
    return x * vel * 0.35


def d_tom(m, dur, vel, rng):
    n = secs(0.35)
    t = np.arange(n) / SR
    f = midi_hz(m) * (1 + 0.4 * np.exp(-t * 20))
    return (sine(f, n) * np.exp(-t * 8) + lowpass(rng.uniform(-1, 1, n), 2000) * np.exp(-t * 50) * 0.3) * vel


def d_shaker(m, dur, vel, rng):
    n = secs(0.07)
    t = np.arange(n) / SR
    env = np.minimum(t / 0.01, 1) * np.exp(-t * 60)
    return bandpass(rng.uniform(-1, 1, n), 5000, 12000) * env * vel * 0.4


# instrument table: fn, gain, pan, delay send, reverb send
INST = {
    "lead":      (i_lead, 0.32, 0.05, 0.25, 0.25),
    "lead_thin": (i_lead_thin, 0.22, -0.35, 0.3, 0.25),
    "lead_sq":   (i_lead_sq, 0.3, 0.1, 0.25, 0.25),
    "harm":      (i_lead_thin, 0.2, 0.4, 0.15, 0.3),
    "brass":     (i_brass, 0.55, -0.15, 0.05, 0.3),
    "pad":       (i_pad, 1.0, 0.0, 0.0, 0.45),
    "pad_soft":  (i_pad_soft, 1.0, 0.0, 0.1, 0.5),
    "bass":      (i_bass_tri, 0.5, 0.0, 0.0, 0.05),
    "bass_syn":  (i_bass_synth, 0.42, 0.0, 0.0, 0.05),
    "pluck":     (i_pluck, 0.16, -0.45, 0.35, 0.2),
    "pluck_r":   (i_pluck, 0.13, 0.5, 0.35, 0.2),
    "glass":     (i_glass, 0.3, 0.15, 0.4, 0.4),
    "timp":      (i_timp, 0.45, -0.1, 0.0, 0.4),
    "kick":      (d_kick, 0.75, 0.0, 0.0, 0.05),
    "snare":     (d_snare, 0.42, 0.05, 0.0, 0.25),
    "snare_c":   (d_snare_chip, 0.45, 0.05, 0.0, 0.2),
    "hat":       (d_hat, 0.22, 0.3, 0.0, 0.08),
    "ohat":      (d_ohat, 0.2, 0.3, 0.0, 0.15),
    "rim":       (d_rim, 0.35, -0.25, 0.15, 0.25),
    "crash":     (d_crash, 0.45, -0.3, 0.0, 0.3),
    "tom":       (d_tom, 0.5, 0.2, 0.0, 0.25),
    "shaker":    (d_shaker, 0.25, -0.4, 0.0, 0.1),
}

# =============================================================== theory helpers
CHORDS = {"": (0, 4, 7), "m": (0, 3, 7), "7": (0, 4, 7, 10), "maj7": (0, 4, 7, 11),
          "m7": (0, 3, 7, 10), "sus4": (0, 5, 7), "sus2": (0, 2, 7), "dim": (0, 3, 6)}


def chord(name):
    """'Dm' -> (root pitch class, intervals)."""
    root = name[0]
    rest = name[1:]
    acc = 0
    if rest[:1] in ("#", "b"):
        acc = 1 if rest[0] == "#" else -1
        rest = rest[1:]
    pc = (note(root + "4") - 60 + acc) % 12
    return pc, CHORDS[rest]


def chord_pcs(name):
    pc, iv = chord(name)
    return [(pc + i) % 12 for i in iv]


def voicings(name, lo, hi):
    pcs = chord_pcs(name)[:3] if len(chord_pcs(name)) == 3 else chord_pcs(name)
    out = []
    for base in range(lo, hi):
        if base % 12 not in pcs:
            continue
        v = [base]
        for pc in pcs[pcs.index(base % 12) + 1:] + pcs[:pcs.index(base % 12)]:
            nxt = v[-1] + 1
            while nxt % 12 != pc:
                nxt += 1
            v.append(nxt)
        if v[-1] <= hi:
            out.append(v)
    return out


def voice_lead(prog, lo=52, hi=74):
    prev = None
    res = []
    for c in prog:
        cands = voicings(c, lo, hi)
        if prev is None:
            best = min(cands, key=lambda v: abs(np.mean(v) - (lo + hi) / 2))
        else:
            best = min(cands, key=lambda v: sum(min(abs(a - b) for b in prev) for a in v) + 0.1 * abs(np.mean(v) - np.mean(prev)))
        res.append(best)
        prev = best
    return res


def parse(text):
    """'D5:1.5 A4:.5 r:1' -> [(midi|None, beats)]"""
    out = []
    for tok in text.split():
        n, d = tok.split(":")
        out.append((None if n == "r" else note(n), float(eval(d, {}, {}))))
    return out


# =============================================================== song container
class Song:
    def __init__(self, name, bpm, bars, seed, beats_per_bar=4, loop=True, tail=6.0):
        self.name, self.bpm, self.bars, self.bpb = name, bpm, bars, beats_per_bar
        self.loop, self.tail = loop, tail
        self.rng = np.random.default_rng(seed)
        self.seed = seed
        self.events = []  # (inst, beat, dur_beats, midi, vel)

    @property
    def spb(self):
        return 60.0 / self.bpm

    def add(self, inst, beat, dur, m, vel=1.0):
        self.events.append((inst, beat, dur, m, vel))

    def melody(self, inst, bar, text, vel=1.0, transpose=0, legato=0.92):
        b = bar * self.bpb
        notes = []
        for m, d in parse(text):
            if m is not None:
                self.add(inst, b, d * legato, m + transpose, vel)
                notes.append((b, d, m + transpose))
            b += d
        return notes

    def harmony_below(self, inst, notes, chord_at, vel=0.8, min_gap=3):
        """Doubles a melody with the nearest chord tone at least min_gap semitones below."""
        for b, d, m in notes:
            pcs = chord_pcs(chord_at(b))
            h = m - min_gap
            while h % 12 not in pcs:
                h -= 1
            self.add(inst, b, d * 0.9, h, vel)

    def drums(self, bar, pats, vel=1.0):
        """pats: {inst: 'x...o...' } 16 steps per bar (or 12 for 3/4); x=1 o=.65 -=.35"""
        for inst, p in pats.items():
            step = self.bpb / len(p)
            for i, ch in enumerate(p):
                v = {"x": 1.0, "o": 0.65, "-": 0.35}.get(ch)
                if v:
                    self.add(inst, bar * self.bpb + i * step, step, 60, v * vel)

    # -------------------------------------------------------------- render
    def render(self):
        loop_n = secs(self.bars * self.bpb * self.spb)
        loop_n = int(round(self.bars * self.bpb * self.spb * SR))
        total = loop_n + secs(self.tail)
        dry = np.zeros((2, total))
        dsend = np.zeros(total)
        rsend = np.zeros(total)
        cache = {}
        for inst, beat, dur, m, vel in sorted(self.events, key=lambda e: (e[1], e[0])):
            fn, gain, pan, ds, rs = INST[inst]
            key = (inst, round(dur, 4), m)
            # drums vary per hit (fresh noise), tonal notes may be cached
            if inst in ("lead", "lead_thin", "lead_sq", "harm", "pad", "pad_soft", "bass", "bass_syn",
                        "pluck", "pluck_r", "glass", "brass") and key in cache:
                x = cache[key]
            else:
                x = fn(m, dur * self.spb, 1.0, self.rng)
                cache[key] = x
            x = x * gain * vel
            s = int(round(beat * self.spb * SR))
            e = min(total, s + len(x))
            x = x[: e - s]
            pl = np.cos((pan + 1) * np.pi / 4)
            pr = np.sin((pan + 1) * np.pi / 4)
            dry[0, s:e] += x * pl * 1.414
            dry[1, s:e] += x * pr * 1.414
            dsend[s:e] += x * ds
            rsend[s:e] += x * rs
        # ping-pong delay, dotted eighth
        dt = int(round(0.75 * self.spb * SR))
        dl = lowpass(highpass(dsend, 300), 4500)
        wet = np.zeros((2, total))
        g = 0.5
        for k in range(1, 7):
            off = dt * k
            if off >= total:
                break
            wet[(k + 1) % 2, off:] += dl[: total - off] * g
            g *= 0.55
        # reverb
        irl, irr = make_ir(2.6, 0.9, seed=self.seed + 7, damp=5500, predelay=0.02)
        rv = np.stack([signal.fftconvolve(rsend, irl)[:total], signal.fftconvolve(rsend, irr)[:total]])
        mix = dry + wet * 0.8 + rv * 0.9
        mix = np.stack([highpass(c, 28) for c in mix])
        if self.loop:
            out = mix[:, :loop_n].copy()
            tail = mix[:, loop_n:]
            k = 0
            while k < tail.shape[1]:
                seg = tail[:, k:k + loop_n]
                out[:, :seg.shape[1]] += seg
                k += loop_n
        else:
            out = mix
            # trim silence at the end, then fade
            env = np.max(np.abs(out), axis=0)
            last = np.nonzero(env > 1e-4)[0]
            end = (last[-1] + 1) if len(last) else out.shape[1]
            out = out[:, :end]
            f = min(secs(0.3), out.shape[1])
            out[:, -f:] *= np.linspace(1, 0, f) ** 2
        # master: level, soft limiter (memoryless, so the loop stays seamless)
        rms = np.sqrt(np.mean(out ** 2))
        out = out * (0.16 / max(rms, 1e-9))
        out = np.tanh(out * 1.1) / 1.1
        pk = np.max(np.abs(out))
        if pk > 0.95:
            out *= 0.95 / pk
        return out.T


# =============================================================== compositions
def soldier_menu():
    """Military / heroic march in D minor, 112 bpm. Intro(4) A(8) A'(8) B(8) A''(8) = 36 bars."""
    s = Song("soldier_menu", 112, 36, seed=101)
    intro = ["Dm", "Dm", "Bb", "A"]
    pa = ["Dm", "Bb", "F", "C", "Dm", "Bb", "C", "Dm"]
    pb = ["Gm", "Dm", "Bb", "A", "Gm", "Dm", "Bb", "A"]
    prog = intro + pa + pa + pb + pa
    at = lambda beat: prog[min(int(beat // 4), len(prog) - 1)]  # noqa: E731

    mel_a = ("D5:1.5 A4:.5 D5:1 F5:1 "
             "F5:1.5 E5:.5 D5:1 Bb4:1 "
             "C5:1.5 A4:.5 C5:1 F5:1 "
             "E5:2 D5:1 C5:1 "
             "D5:1.5 A4:.5 D5:1 F5:1 "
             "G5:1.5 F5:.5 D5:1 Bb4:1 "
             "C5:1 E5:1 G5:1 E5:1 "
             "D5:3 r:1")
    mel_b = ("Bb4:1 D5:1 G5:1.5 F5:.5 "
             "F5:1 E5:.5 D5:.5 A4:2 "
             "Bb4:1 D5:1 F5:1.5 E5:.5 "
             "E5:2 C#5:1 A4:1 "
             "G5:1.5 F5:.5 D5:1 G5:1 "
             "A5:1.5 G5:.5 F5:1 D5:1 "
             "F5:1 G5:1 A5:1 Bb5:1 "
             "A5:2 G5:.5 F5:.5 E5:1")

    # pads, voice-led
    for i, v in enumerate(voice_lead(prog, 50, 70)):
        for m in v:
            s.add("pad", i * 4, 4, m, 0.9 if i >= 4 else 0.7)

    # bass: gallop
    for i, c in enumerate(prog):
        pc, iv = chord(c)
        r = 36 + pc if pc >= 2 else 48 + pc
        r = r if r <= 45 else r - 12
        f5 = r + 7
        if i < 4:
            s.add("bass", i * 4, 3.8, r, 0.9)
            continue
        for h in (0, 2):
            b = i * 4 + h
            s.add("bass", b, 0.45, r, 1.0)
            s.add("bass", b + 0.5, 0.22, r, 0.7)
            s.add("bass", b + 0.75, 0.22, r, 0.7)
            s.add("bass", b + 1, 0.45, f5 if h == 0 else r + 12, 0.85)
            s.add("bass", b + 1.5, 0.45, r, 0.8)

    # intro: timpani + growing snare roll
    for i in range(4):
        s.add("timp", i * 4, 1, 38 if i < 3 else 45, 1.0)
        s.add("timp", i * 4 + 3.5, 0.5, 38 if i < 3 else 45, 0.5)
    for k in range(32):
        s.add("snare", 8 + k * 0.25, 0.25, 60, 0.15 + 0.6 * k / 31)
    s.add("crash", 16, 1, 60, 0.9)

    march = {"kick": "x.......x.......",
             "snare": "....x..-....x.o-",
             "hat": "x.o.x.o.x.o.x.o."}
    march2 = {"kick": "x.....o.x.......",
              "snare": "-...x.--.-..x.oo",
              "hat": "x-o-x-o-x-o-x-o-"}
    for bar in range(4, 36):
        p = march if bar < 12 else march2
        s.drums(bar, p, 0.9 if bar < 12 else 1.0)
        if bar in (12, 20, 24, 28):
            s.add("crash", bar * 4, 1, 60, 0.8)
    # tom fills at section ends
    for bar in (11, 19, 27):
        for k, m in enumerate([50, 50, 47, 47, 43, 43, 40, 40]):
            s.add("tom", bar * 4 + 2 + k * 0.25, 0.25, m, 0.6 + 0.05 * k)

    # A
    n = s.melody("lead", 4, mel_a, 1.0)
    s.melody("brass", 4, mel_a, 0.6, transpose=-12)
    # A' with harmony + arps
    n = s.melody("lead", 12, mel_a, 1.0)
    s.harmony_below("harm", n, at, 0.9)
    s.melody("brass", 12, mel_a, 0.6, transpose=-12)
    # B
    n = s.melody("brass", 20, mel_b, 1.0)
    s.melody("lead", 20, mel_b, 0.8)
    s.harmony_below("harm", n, at, 0.8)
    # A''
    n = s.melody("lead", 28, mel_a, 1.0)
    s.melody("lead_thin", 28, mel_a, 0.6, transpose=12)
    s.melody("brass", 28, mel_a, 0.7, transpose=-12)
    s.harmony_below("harm", n, at, 0.9)

    # 16th arpeggios in A', B, A''
    vl = voice_lead(prog, 57, 79)
    pat = [0, 1, 2, 1, 0, 2, 1, 2, 0, 1, 2, 1, 2, 1, 0, 1]
    for i in range(12, 36):
        v = vl[i]
        for k, idx in enumerate(pat):
            s.add("pluck" if k % 2 == 0 else "pluck_r", i * 4 + k * 0.25, 0.25, v[idx] + 12, 0.7 if k % 4 == 0 else 0.45)
    return s


def soldier_game(bpm=96, name="soldier_game"):
    """Tense puzzle music in E minor (Phrygian colour from F major). 36 bars."""
    s = Song(name, bpm, 36, seed=202)
    pa = ["Em", "C", "Em", "F", "Em", "C", "Am", "B"]
    pc_ = ["Am", "F", "C", "B", "Am", "F", "G", "B"]
    pd = ["Em", "F", "Em", "B"]
    prog = pa + pa + pc_ + pd + pa

    mel_b = ("r:1 E5:1 G5:1 B5:1 "
             "B5:3 G5:1 "
             "F#5:2 E5:1 B4:1 "
             "C5:3 A4:1 "
             "r:1 E5:1 G5:1 B5:1 "
             "C6:2 B5:1 G5:1 "
             "A5:2 E5:1 C5:1 "
             "D#5:2 F#5:1 B4:1")
    mel_c = ("E5:1.5 D5:.5 C5:1 A4:1 "
             "F5:2 E5:1 C5:1 "
             "G5:1.5 F5:.5 E5:1 C5:1 "
             "D#5:2 B4:1 F#5:1 "
             "A5:1.5 G5:.5 E5:1 C5:1 "
             "F5:1.5 A5:.5 C6:1 A5:1 "
             "B5:1.5 A5:.5 G5:1 D5:1 "
             "F#5:2 D#5:1 B4:1")

    # ostinato (16ths): root, fifth, octave, fifth, third, fifth ...
    for i, c in enumerate(prog):
        if 24 <= i < 28:
            continue
        pc, iv = chord(c)
        r = 48 + pc - (12 if pc >= 8 else 0)  # A2..G3
        tones = [r, r + iv[2], r + 12, r + iv[2], r + iv[1] + 12, r + iv[2], r + 12, r + iv[2]]
        for k in range(16):
            s.add("pluck" if k % 2 == 0 else "pluck_r", i * 4 + k * 0.25, 0.25, tones[k % 8] + 12,
                  (0.75 if k % 4 == 0 else 0.45) * (0.8 if i < 8 else 1.0))

    # bass: pulsing 8ths with octave jumps
    for i, c in enumerate(prog):
        pc, _ = chord(c)
        r = 36 + pc - (12 if pc >= 8 else 0)  # A1..G2
        if 24 <= i < 28:
            s.add("bass", i * 4, 3.9, r, 1.0)
            continue
        for k in range(8):
            s.add("bass", i * 4 + k * 0.5, 0.4, r + (12 if k in (3, 7) else 0), 1.0 if k % 2 == 0 else 0.75)

    # pads from B onward
    vl = voice_lead(prog, 52, 72)
    for i in range(8, 36):
        for m in vl[i]:
            s.add("pad_soft", i * 4, 4, m, 0.8 if 24 <= i < 28 else 0.6)

    # drums
    a = {"kick": "x.........x.....", "hat": "-.-.o.-.-.-.o.-.", "rim": "........x......."}
    full = {"kick": "x.....x...x.....", "snare": "....x.......x...", "hat": "-.o.-.o.-.o.-.o.",
            "shaker": "..-...-...-...-."}
    tick = {"rim": "x...x...x...x...", "hat": "..-...-...-...-."}
    for bar in range(36):
        if bar < 16:
            s.drums(bar, a, 0.85)
        elif bar < 24:
            s.drums(bar, full, 0.85)
        elif bar < 28:
            s.drums(bar, tick, 0.8)
        else:
            s.drums(bar, full, 1.0)
    for bar in (16, 28):
        s.add("crash", bar * 4, 1, 60, 0.6)
    for k, m in enumerate([47, 47, 43, 43, 40, 40, 36, 36]):
        s.add("tom", 27 * 4 + 2 + k * 0.25, 0.25, m, 0.5 + 0.05 * k)

    s.melody("glass", 8, mel_b, 1.0, legato=0.95)
    s.melody("lead", 16, mel_c, 0.8)
    s.melody("glass", 16, mel_c, 0.5, transpose=12)
    # breakdown: lonely glass notes
    s.melody("glass", 24, "B4:4 C5:4 B4:4 D#5:4", 0.7)
    n = s.melody("glass", 28, mel_b, 1.0, legato=0.95)
    s.melody("lead_thin", 28, mel_b, 0.6, transpose=-12)
    at = lambda beat: prog[min(int(beat // 4), len(prog) - 1)]  # noqa: E731
    s.harmony_below("harm", n, at, 0.6)
    return s


def soldier_victory():
    s = Song("soldier_victory", 132, 3, seed=303, loop=False, tail=3.0)
    trip = 1 / 3
    mel = f"A4:{trip} A4:{trip} A4:{trip} D5:.75 A4:.25 D5:.5 F#5:.5 A5:1 F#5:1 B5:.5 A5:.5 G5:.5 E5:.5 D6:3"
    n = s.melody("lead", 0, mel, 1.0, legato=0.9)
    s.melody("brass", 0, mel, 0.8, transpose=-12)
    chords = [(1, 4, "D"), (5, 1, "G"), (6, 1, "A7"), (7, 3, "D")]
    for b, d, c in chords:
        pc, iv = chord(c)
        for k in iv[:3]:
            s.add("pad", b, d, 57 + (pc + k - 9) % 12, 1.0)
            s.add("brass", b, d * 0.9, 50 + (pc + k - 2) % 12, 0.35)
        s.add("bass", b, d * 0.9, 38 + (pc - 2) % 12, 1.0)
    at = lambda beat: next((c for b, d, c in chords if b <= beat < b + d), "D")  # noqa: E731
    s.harmony_below("harm", [x for x in n if x[0] >= 1], at, 0.8)
    for k in range(12):
        s.add("snare", k / 12, 1 / 12, 60, 0.2 + 0.05 * k)
    s.add("timp", 1, 1, 38, 1.0)
    s.add("timp", 7, 1, 38, 1.0)
    s.add("crash", 1, 1, 60, 0.8)
    s.add("crash", 7, 1, 60, 1.0)
    s.add("kick", 1, 1, 60, 1.0)
    s.add("kick", 7, 1, 60, 1.0)
    for k, b in enumerate([5, 5.5, 6, 6.5]):
        s.add("snare", b, 0.5, 60, 0.6 + 0.1 * k)
    return s


def piton_menu():
    """Playful, C major, 120 bpm. A A' B A'' = 32 bars."""
    s = Song("piton_menu", 120, 32, seed=404)
    pa = ["C", "Am", "F", "G", "C", "Am", "F", "G"]
    pb = ["F", "G", "Em", "Am", "F", "G", "C", "C"]
    prog = pa + pa + pb + pa
    mel_a = ("E5:.5 G5:.5 C6:.5 G5:.5 E5:.5 r:.5 C5:1 "
             "A4:.5 C5:.5 E5:.5 A5:.5 G5:1 E5:1 "
             "F5:.5 E5:.5 D5:.5 C5:.5 A4:1 C5:1 "
             "D5:.5 E5:.5 F5:.5 D5:.5 B4:1 G4:1 "
             "E5:.5 G5:.5 C6:.5 G5:.5 E5:.5 r:.5 C5:1 "
             "A4:.5 C5:.5 E5:.5 A5:.5 C6:1 A5:1 "
             "A5:.5 G5:.5 F5:.5 A5:.5 G5:1 E5:1 "
             "D5:.5 E5:.5 D5:.5 B4:.5 C5:2")
    mel_b = ("C5:1 F5:.5 A5:.5 G5:1 F5:1 "
             "D5:1 G5:.5 B5:.5 A5:1 G5:1 "
             "E5:.5 G5:.5 B5:.5 G5:.5 E5:1 B4:1 "
             "C5:.5 E5:.5 A5:.5 E5:.5 C5:1 A4:1 "
             "F5:1.5 G5:.5 A5:1 F5:1 "
             "G5:1.5 A5:.5 B5:1 D6:1 "
             "C6:.5 G5:.5 E5:.5 G5:.5 C6:1 r:1 "
             "E5:.5 D5:.5 C5:.5 D5:.5 E5:.5 G5:.5 r:1")
    at = lambda beat: prog[min(int(beat // 4), len(prog) - 1)]  # noqa: E731

    for i, c in enumerate(prog):
        pc, iv = chord(c)
        r = 36 + pc
        if r > 43:
            r -= 12
        pat = [(0, r), (0.5, r + 12), (1, r + 7), (1.5, r + 12), (2, r), (2.5, r + 12), (3, r + 7), (3.5, r + iv[1] + 12)]
        for b, m in pat:
            s.add("bass", i * 4 + b, 0.35, m, 1.0 if b % 1 == 0 else 0.7)
    vl = voice_lead(prog, 55, 74)
    for i, v in enumerate(vl):
        # off-beat chord stabs (ska-ish bounce)
        for b in (0.5, 1.5, 2.5, 3.5):
            for m in v:
                s.add("pluck_r", i * 4 + b, 0.2, m, 0.55)
        if i >= 16:
            for m in v:
                s.add("pad_soft", i * 4, 4, m, 0.6)
    beat = {"kick": "x.......x.......", "snare_c": "....x.......x...", "hat": "x.o.x.o.x.o.x.o."}
    beat2 = {"kick": "x.....x.x.......", "snare_c": "....x.......x..o", "hat": "x-o-x-o-x-o-x-o-",
             "shaker": "..x...x...x...x."}
    for bar in range(32):
        s.drums(bar, beat if bar < 8 else beat2, 0.9)
    for bar in (8, 16, 24):
        s.add("crash", bar * 4, 1, 60, 0.6)

    s.melody("lead", 0, mel_a, 1.0, legato=0.75)
    n = s.melody("lead", 8, mel_a, 1.0, legato=0.75)
    s.harmony_below("harm", n, at, 0.8)
    # A': sparkling arp
    for i in range(8, 16):
        v = vl[i]
        for k, idx in enumerate([0, 1, 2, 1] * 4):
            s.add("pluck", i * 4 + k * 0.25, 0.2, v[idx] + 12, 0.5)
    s.melody("lead_sq", 16, mel_b, 1.0, legato=0.85)
    s.melody("glass", 16, mel_b, 0.4, transpose=12)
    n = s.melody("lead", 24, mel_a, 1.0, legato=0.75)
    s.melody("lead_thin", 24, mel_a, 0.6, transpose=12, legato=0.6)
    s.harmony_below("harm", n, at, 0.8)
    return s


def piton_game():
    """Upbeat, G major, 140 bpm. Intro(4) A A' B C A'' B' = 52 bars."""
    s = Song("piton_game", 140, 52, seed=505)
    intro = ["G", "G", "C", "D"]
    pa = ["G", "D", "Em", "C", "G", "D", "C", "D"]
    pb = ["Em", "C", "G", "D", "Em", "C", "Am", "D"]
    pc_ = ["C", "D", "Bm", "Em", "C", "D", "G", "G"]
    prog = intro + pa + pa + pb + pc_ + pa + pb
    at = lambda beat: prog[min(int(beat // 4), len(prog) - 1)]  # noqa: E731
    mel_a = ("B4:.5 D5:.5 G5:.5 D5:.5 B5:1 A5:.5 G5:.5 "
             "F#5:1 A5:.5 F#5:.5 D5:1 A4:1 "
             "G5:.5 F#5:.5 E5:.5 B4:.5 E5:1 G5:1 "
             "E5:.5 D5:.5 C5:.5 E5:.5 G5:1.5 r:.5 "
             "B4:.5 D5:.5 G5:.5 D5:.5 B5:1 A5:.5 G5:.5 "
             "F#5:.5 G5:.5 A5:.5 B5:.5 A5:1 F#5:1 "
             "E5:.5 G5:.5 C6:.5 B5:.5 A5:1 G5:1 "
             "F#5:1 E5:.5 F#5:.5 A5:1 D5:1")
    mel_b = ("E5:1.5 F#5:.5 G5:1 B5:1 "
             "C6:1.5 B5:.5 A5:1 G5:1 "
             "B5:.5 A5:.5 G5:.5 D5:.5 G5:1 B5:1 "
             "A5:2 F#5:1 D5:1 "
             "E5:.5 G5:.5 B5:.5 E6:.5 D6:1 B5:1 "
             "C6:.5 B5:.5 A5:.5 G5:.5 E5:1 G5:1 "
             "A5:.5 C6:.5 B5:.5 A5:.5 G5:1 E5:1 "
             "F#5:.5 G5:.5 A5:.5 F#5:.5 D5:2")

    for i, c in enumerate(prog):
        pc, iv = chord(c)
        r = 36 + pc
        if r > 42:
            r -= 12
        if 28 <= i < 36:  # bridge: half-time long notes
            s.add("bass_syn", i * 4, 1.8, r, 1.0)
            s.add("bass_syn", i * 4 + 2, 1.8, r + 7 if i % 2 else r, 0.9)
            continue
        for k in range(8):
            m = r + (12 if k % 2 else 0)
            if k == 7:
                m = r + 7
            s.add("bass_syn", i * 4 + k * 0.5, 0.42, m, 1.0 if k % 2 == 0 else 0.8)
    vl = voice_lead(prog, 55, 74)
    for i, v in enumerate(vl):
        if i < 4:
            continue
        for m in v:
            s.add("pad", i * 4, 4, m, 0.7 if 28 <= i < 36 else 0.5)
    drive = {"kick": "x...x...x...x...", "snare_c": "....x.......x...", "hat": "-.x.-.x.-.x.-.x.",
             "shaker": "-x-x-x-x-x-x-x-x"}
    drive2 = {"kick": "x...x...x...x.x.", "snare_c": "....x..-....x.-.", "ohat": "..x...x...x...x.",
              "hat": "x-x-x-x-x-x-x-x-"}
    half = {"kick": "x.........x.....", "snare_c": "........x.......", "hat": "x.x.x.x.x.x.x.x."}
    for bar in range(52):
        if bar < 2:
            s.drums(bar, {"kick": "x...x...x...x...", "hat": "-.x.-.x.-.x.-.x."}, 0.9)
        elif bar < 4:
            s.drums(bar, drive, 0.9)
        elif 28 <= bar < 36:
            s.drums(bar, half, 0.9)
        elif bar >= 36 or 20 <= bar < 28:
            s.drums(bar, drive2, 1.0)
        else:
            s.drums(bar, drive, 1.0)
    for bar in (4, 12, 20, 28, 36, 44):
        s.add("crash", bar * 4, 1, 60, 0.7)
    for bar in (3, 27, 35):
        for k, m in enumerate([55, 55, 52, 52, 48, 48, 45, 45]):
            s.add("tom", bar * 4 + 2 + k * 0.25, 0.25, m, 0.55 + 0.05 * k)

    s.melody("lead", 4, mel_a, 1.0, legato=0.8)
    n = s.melody("lead", 12, mel_a, 1.0, legato=0.8)
    s.harmony_below("harm", n, at, 0.8)
    for i in range(12, 20):
        v = vl[i]
        for k, idx in enumerate([0, 1, 2, 1, 2, 1, 0, 1] * 2):
            s.add("pluck", i * 4 + k * 0.25, 0.2, v[idx] + 12, 0.45)
    n = s.melody("lead_sq", 20, mel_b, 1.0, legato=0.85)
    s.harmony_below("harm", n, at, 0.7)
    # bridge: arpeggio carries the tune
    for i in range(28, 36):
        v = vl[i]
        for k, idx in enumerate([0, 1, 2, 1, 0, 2, 1, 2, 0, 1, 2, 1, 2, 1, 0, 1]):
            s.add("pluck" if k % 2 == 0 else "pluck_r", i * 4 + k * 0.25, 0.25, v[idx] + 12, 0.7 if k % 4 == 0 else 0.45)
    s.melody("glass", 28, "E5:2 G5:2 F#5:2 A5:2 F#5:4 E5:4 C5:2 E5:2 D5:2 F#5:2 G5:6 r:2", 0.9, legato=0.95)
    n = s.melody("lead", 36, mel_a, 1.0, legato=0.8)
    s.melody("lead_thin", 36, mel_a, 0.6, transpose=12, legato=0.6)
    s.harmony_below("harm", n, at, 0.8)
    n = s.melody("lead_sq", 44, mel_b, 1.0, legato=0.85)
    s.melody("glass", 44, mel_b, 0.4, transpose=12)
    s.harmony_below("harm", n, at, 0.7)
    return s


def piton_victory():
    s = Song("piton_victory", 150, 2, seed=606, loop=False, tail=2.5)
    mel = "C5:.5 E5:.5 G5:.5 C6:1.5 A5:.5 B5:.5 C6:.5 D6:.5 E6:3"
    n = s.melody("lead", 0, mel, 1.0, legato=0.85)
    s.melody("lead_thin", 0, mel, 0.5, transpose=-12, legato=0.7)
    chords = [(0, 3, "C"), (3, 1, "F"), (4, 1, "G"), (5, 3, "C")]
    for b, d, c in chords:
        pc, iv = chord(c)
        s.add("bass", b, d * 0.9, 36 + pc if pc < 8 else 24 + pc, 1.0)
        for k in iv:
            s.add("pluck_r", b, 0.3, 60 + (pc + k) % 12, 0.6)
    at = lambda beat: next((c for b, d, c in chords if b <= beat < b + d), "C")  # noqa: E731
    s.harmony_below("harm", n, at, 0.8)
    s.drums(0, {"kick": "x.......x.x.....", "snare_c": "....x.....x.xxxx", "hat": "x.x.x.x.x.x.x.x."})
    s.add("crash", 5, 1, 60, 1.0)
    s.add("kick", 5, 1, 60, 1.0)
    return s


# =============================================================== output
def write_ogg(path, x):
    """Writes in blocks: libsndfile's Vorbis encoder overflows the stack on huge single writes."""
    import soundfile as sf
    path.parent.mkdir(parents=True, exist_ok=True)
    x = np.ascontiguousarray(x, dtype=np.float32)
    kw = dict(samplerate=SR, channels=2, format="OGG", subtype="VORBIS")
    try:
        f = sf.SoundFile(str(path), "w", compression_level=0.55, **kw)
    except TypeError:
        f = sf.SoundFile(str(path), "w", **kw)
    with f:
        for i in range(0, len(x), 8192):
            f.write(x[i:i + 8192])
    _fix_ogg_serial(path)


def _ogg_crc(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b << 24
        for _ in range(8):
            crc = ((crc << 1) ^ 0x04C11DB7) if crc & 0x80000000 else (crc << 1)
            crc &= 0xFFFFFFFF
    return crc


def _fix_ogg_serial(path, serial=0x56534F46):
    """libsndfile picks a random Ogg stream serial; pin it so the output is byte-identical."""
    data = bytearray(path.read_bytes())
    pos = 0
    while pos < len(data):
        assert data[pos:pos + 4] == b"OggS", "bad ogg page"
        nseg = data[pos + 26]
        size = 27 + nseg + sum(data[pos + 27:pos + 27 + nseg])
        data[pos + 14:pos + 18] = serial.to_bytes(4, "little")
        data[pos + 22:pos + 26] = bytes(4)
        data[pos + 22:pos + 26] = _ogg_crc(bytes(data[pos:pos + size])).to_bytes(4, "little")
        pos += size
    path.write_bytes(bytes(data))


def preview_png(path, x, title):
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        return
    PREVIEW.mkdir(parents=True, exist_ok=True)
    mono = x.mean(axis=1)
    f, t, Z = signal.stft(mono, SR, nperseg=2048, noverlap=1024)
    Z = 20 * np.log10(np.abs(Z) + 1e-7)
    Z = Z[: np.searchsorted(f, 8000)]
    Z = np.clip((Z + 90) / 70, 0, 1)
    W, H = 1200, 300
    img = np.array(Image.fromarray((Z[::-1] * 255).astype(np.uint8)).resize((W, H)))
    rgb = np.stack([img, (img * 0.7).astype(np.uint8), (255 - img) // 3], axis=-1)
    wave = Image.new("RGB", (W, 120), (20, 20, 30))
    d = ImageDraw.Draw(wave)
    step = len(mono) / W
    for i in range(W):
        seg = x[int(i * step): int((i + 1) * step)]
        if len(seg):
            d.line([(i, 60 - seg[:, 0].max() * 58), (i, 60 - seg[:, 0].min() * 58)], fill=(120, 200, 255))
    out = Image.new("RGB", (W, H + 140), (0, 0, 0))
    out.paste(Image.fromarray(rgb), (0, 0))
    out.paste(wave, (0, H + 20))
    ImageDraw.Draw(out).text((5, H + 3), title, fill=(255, 255, 255))
    out.save(PREVIEW / (path.stem + "_" + path.parent.parent.parent.name + ".png"))


PIECES = [
    ("soldier/data/music/menu.ogg", soldier_menu),
    ("soldier/data/music/game.ogg", lambda: soldier_game(96, "soldier_game")),
    ("soldier/data/music/game_fast.ogg", lambda: soldier_game(128, "soldier_game_fast")),
    ("soldier/data/music/victory.ogg", soldier_victory),
    ("piton/data/music/menu.ogg", piton_menu),
    ("piton/data/music/game.ogg", piton_game),
    ("piton/data/music/victory.ogg", piton_victory),
]


def main(only=None):
    for rel, fn in PIECES:
        if only and only not in rel:
            continue
        s = fn()
        x = s.render()
        assert np.all(np.isfinite(x)), rel
        p = ROOT / rel
        write_ogg(p, x)
        dur = len(x) / SR
        info = f"{rel}: {dur:.2f}s bpm={s.bpm} bars={s.bars} loop={s.loop} peak={np.abs(x).max():.3f}"
        if s.loop:
            seam = np.abs(x[0] - x[-1]).max()
            info += f" seam_jump={seam:.4f}"
        print(info)
        preview_png(p, x, info)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else None)
