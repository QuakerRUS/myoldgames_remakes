"""Synthesizes all sound effects (WAV, 44.1 kHz, 16-bit, mono).

Run: python -I tools/assets/gen_sfx.py
Deterministic: every sound uses its own fixed numpy seed.
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from synth import (SR, adsr, bandpass, exp_env, fade, highpass, lofi_noise, lowpass,  # noqa: E402
                   lowpass_sweep, midi_hz, noise, note, pad_to, pulse, reverb, saw, secs, sine,
                   square, tri, write_wav)

ROOT = Path(__file__).resolve().parents[2]
OUT_SOLDIER = ROOT / "soldier" / "data" / "sfx"
OUT_PITON = ROOT / "piton" / "data" / "sfx"
OUT_KS = ROOT / "keyboard-simulator" / "data" / "sfx"


def rng(seed):
    return np.random.default_rng(seed)


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[: len(p)] += p
    return out


def at(x, t, total=None):
    """Delay signal x by t seconds."""
    y = np.concatenate([np.zeros(secs(t)), x])
    return pad_to(y, secs(total))[: secs(total)] if total else y


def tone(kind, f, dur, env, **kw):
    n = secs(dur)
    if kind == "sine":
        x = sine(f, n)
    elif kind == "tri":
        x = tri(f, n)
    elif kind == "saw":
        x = saw(f, n)
    elif kind == "square":
        x = square(f, n)
    else:
        x = pulse(f, n, kw.get("duty", 0.25))
    return x * env[:n] if len(env) >= n else x * pad_to(env, n)


def blip(m, dur, kind="pulse", duty=0.25, decay=None, a=0.002, vib=0.0):
    n = secs(dur)
    f = midi_hz(m) * np.ones(n)
    if vib:
        f *= 1 + vib * np.sin(2 * np.pi * 6 * np.arange(n) / SR)
    env = exp_env(n, decay, a) if decay else adsr(n, a, 0.04, 0.6, 0.04)
    return tone(kind, f, dur, env, duty=duty)


# ======================================================================== soldier
def s_step(seed=11, bright=1.0):
    r = rng(seed)
    n = secs(0.12)
    thump = sine(np.linspace(140, 60, n), n) * exp_env(n, 0.03, 0.001)
    grit = bandpass(noise(n, r), 400, 2600 * bright) * exp_env(n, 0.022, 0.001)
    scuff = highpass(noise(n, r), 3000) * exp_env(n, 0.012) * 0.25
    x = thump * 0.8 + grit * 0.9 + scuff
    return fade(reverb(x, 0.12, 0.4, 0.15, seed), 0.0005, 0.02)


def s_turn():
    r = rng(12)
    n = secs(0.14)
    sw = bandpass(noise(n, r), 900, 4000) * np.sin(np.linspace(0, np.pi, n)) ** 2
    click = sine(np.linspace(300, 180, n), n) * exp_env(n, 0.015) * 0.5
    return fade(sw * 0.6 + click, 0.001, 0.02)


def s_push_box():
    r = rng(13)
    n = secs(0.42)
    t = np.arange(n) / SR
    # wooden scrape: band-limited noise modulated by a jittery friction LFO
    lfo = 0.6 + 0.4 * np.abs(np.sin(2 * np.pi * (23 + 6 * np.sin(2 * np.pi * 3 * t)) * t))
    scrape = bandpass(noise(n, r), 180, 1400) * lfo
    env = np.minimum(1, t / 0.03) * np.clip((0.42 - t) / 0.12, 0, 1)
    body = sine(95 + 10 * np.sin(2 * np.pi * 7 * t), n) * 0.35
    knock = sine(np.linspace(180, 90, secs(0.08)), secs(0.08)) * exp_env(secs(0.08), 0.03)
    x = (scrape + body) * env + pad_to(knock, n) * 0.8
    return fade(reverb(x, 0.15, 0.6, 0.2, 13), 0.001, 0.03)


def s_box_sink():
    r = rng(14)
    n = secs(1.1)
    t = np.arange(n) / SR
    splash = bandpass(noise(n, r), 300, 5000) * exp_env(n, 0.12, 0.004)
    plunge = sine(np.linspace(220, 55, n), n) * exp_env(n, 0.2, 0.002) * 0.7
    bubbles = np.zeros(n)
    for i in range(14):
        st = 0.12 + r.uniform(0, 0.75)
        d = r.uniform(0.03, 0.07)
        bn = secs(d)
        f0 = r.uniform(500, 1300)
        b = sine(np.linspace(f0, f0 * 1.7, bn), bn) * np.sin(np.linspace(0, np.pi, bn)) * r.uniform(0.15, 0.35)
        s = secs(st)
        bubbles[s:s + bn] += b[: n - s]
    gurgle = lowpass(noise(n, r), 300) * np.clip(t / 0.2, 0, 1) * exp_env(n, 0.4) * 1.5
    x = splash + plunge + bubbles * np.exp(-t * 1.5) + gurgle
    return fade(reverb(x, 0.25, 1.0, 0.3, 14), 0.001, 0.08)


def s_button_press():
    r = rng(15)
    n = secs(0.18)
    clunk = sine(np.linspace(260, 120, n), n) * exp_env(n, 0.04)
    click = highpass(noise(n, r), 2500) * exp_env(n, 0.006) * 0.6
    ping = tone("square", midi_hz(note("A5")), 0.18, exp_env(n, 0.05, 0.004)) * 0.12
    return fade(reverb(clunk + click + at(ping, 0.02)[:n], 0.15, 0.5, 0.15, 15), 0.0005, 0.03)


def s_button_release():
    r = rng(16)
    n = secs(0.16)
    clunk = sine(np.linspace(150, 240, n), n) * exp_env(n, 0.035)
    click = highpass(noise(n, r), 2500) * exp_env(n, 0.005) * 0.5
    ping = tone("square", midi_hz(note("E5")), 0.16, exp_env(n, 0.04, 0.004)) * 0.1
    return fade(reverb(clunk * 0.8 + click + at(ping, 0.015)[:n], 0.15, 0.5, 0.15, 16), 0.0005, 0.03)


def _door(opening: bool, seed):
    r = rng(seed)
    dur = 0.75
    n = secs(dur)
    t = np.arange(n) / SR
    motor_f = np.linspace(70, 110, n) if opening else np.linspace(110, 70, n)
    motor = saw(motor_f, n) + 0.5 * saw(motor_f * 2.01, n)
    motor = lowpass(motor, 700) * 0.35
    hiss = bandpass(noise(n, r), 1500, 6000) * 0.25
    slide = (motor + hiss) * np.clip(t / 0.05, 0, 1) * np.clip((0.62 - t) / 0.1, 0, 1)
    m = secs(0.2)
    clank = (sine(np.linspace(170, 70, m), m) + 0.5 * bandpass(noise(m, r), 200, 2500)) * exp_env(m, 0.05)
    x = slide + at(clank, 0.56, dur) * 1.1
    if not opening:
        x += at(clank * 0.5, 0.0, dur)
    return fade(reverb(x, 0.25, 0.9, 0.3, seed), 0.002, 0.05)


def s_key_pickup():
    notes = ["E6", "G6", "B6", "E7"]
    out = np.zeros(secs(0.5))
    for i, nm in enumerate(notes):
        b = blip(note(nm), 0.22, "square", decay=0.07) * (0.5 if i < 3 else 0.6)
        out = mix(out, at(b, i * 0.045))
    shimmer = blip(note("B7"), 0.3, "sine", decay=0.12) * 0.25
    out = mix(out, at(shimmer, 0.18))
    return fade(reverb(out, 0.3, 0.8, 0.3, 17), 0.001, 0.05)


def s_key_drop():
    r = rng(18)
    out = np.zeros(secs(0.45))
    for i, (tt, g) in enumerate([(0.0, 1.0), (0.11, 0.5), (0.18, 0.28)]):
        n = secs(0.2)
        clink = (sine(midi_hz(note("D7")) * np.ones(n), n) * 0.6
                 + sine(midi_hz(note("D7")) * 2.76 * np.ones(n), n) * 0.4
                 + highpass(noise(n, r), 5000) * exp_env(n, 0.004) * 0.6) * exp_env(n, 0.05)
        out = mix(out, at(clink * g, tt))
    return fade(reverb(out, 0.2, 0.6, 0.2, 18), 0.0005, 0.05)


def s_door_unlock():
    r = rng(19)
    out = np.zeros(secs(0.9))
    # key turn: two mechanical clicks
    for tt, f in [(0.0, 2500), (0.12, 1800)]:
        n = secs(0.06)
        c = bandpass(noise(n, r), f, f * 2.5) * exp_env(n, 0.008)
        c += sine(np.linspace(400, 200, n), n) * exp_env(n, 0.012) * 0.4
        out = mix(out, at(c, tt))
    # bolt
    n = secs(0.2)
    bolt = (sine(np.linspace(220, 90, n), n) + 0.4 * bandpass(noise(n, r), 300, 3000)) * exp_env(n, 0.05)
    out = mix(out, at(bolt, 0.24))
    # confirmation chime (major third up)
    ch = mix(blip(note("C6"), 0.3, "square", decay=0.09) * 0.25,
             at(blip(note("G6"), 0.4, "square", decay=0.12) * 0.25, 0.08))
    out = mix(out, at(ch, 0.34))
    return fade(reverb(out, 0.25, 0.8, 0.25, 19), 0.0005, 0.06)


def s_wall_reveal():
    r = rng(20)
    dur = 1.2
    n = secs(dur)
    t = np.arange(n) / SR
    rumble = lowpass(noise(n, r), 140) * 3.0
    grind = bandpass(noise(n, r), 250, 1200) * (0.5 + 0.5 * np.sin(2 * np.pi * 9 * t) ** 2) * 0.5
    env = np.clip(t / 0.08, 0, 1) * np.exp(-t * 2.3)
    # mysterious low shimmer: tritone dyad
    sh = (sine(midi_hz(note("D3")) * (1 + 0.003 * np.sin(2 * np.pi * 5 * t)), n)
          + 0.7 * sine(midi_hz(note("G#3")), n)) * np.sin(np.pi * np.clip(t / dur, 0, 1)) * 0.3
    m = secs(0.2)
    thud = sine(np.linspace(120, 45, m), m) * exp_env(m, 0.07)
    x = (rumble + grind) * env + sh + at(thud, 0.0, dur) * 0.9
    return fade(reverb(x, 0.35, 1.4, 0.45, 20), 0.002, 0.15)


def s_guillotine_chop():
    r = rng(21)
    dur = 0.7
    out = np.zeros(secs(dur))
    # whoosh down
    n = secs(0.16)
    t = np.arange(n) / SR
    wh = bandpass(noise(n, r), 800, 7000) * (t / 0.16) ** 2
    out = mix(out, wh * 0.6)
    # metallic impact
    m = secs(0.6)
    tt = np.arange(m) / SR
    ring = sum(sine(f * np.ones(m), m) * a * np.exp(-tt / d)
               for f, a, d in [(523, 0.5, 0.25), (1310, 0.35, 0.15), (2470, 0.25, 0.08), (3910, 0.15, 0.05)])
    hit = (sine(np.linspace(160, 50, m), m) * exp_env(m, 0.06) * 1.2
           + bandpass(noise(m, r), 1000, 9000) * exp_env(m, 0.02) * 0.8 + ring * 0.6)
    out = mix(out, at(hit, 0.15))
    return fade(reverb(out[: secs(dur)], 0.25, 0.9, 0.3, 21), 0.0005, 0.06)


def s_drown():
    r = rng(22)
    dur = 1.6
    n = secs(dur)
    t = np.arange(n) / SR
    splash = bandpass(noise(n, r), 400, 6000) * exp_env(n, 0.15, 0.003) * 0.9
    out = splash
    # glug-glug: descending bubble bursts
    for i in range(7):
        st = 0.25 + i * 0.16 + r.uniform(-0.02, 0.02)
        bn = secs(0.09)
        f0 = 600 - i * 55
        b = sine(np.linspace(f0, f0 * 1.6, bn), bn) * np.sin(np.linspace(0, np.pi, bn))
        b += lowpass(noise(bn, r), 400) * np.sin(np.linspace(0, np.pi, bn)) * 1.5
        out = mix(out, at(b * (0.6 - i * 0.06), st))
    out = out[:n]
    low = sine(np.linspace(110, 50, n), n) * np.exp(-t * 2.0) * 0.4
    return fade(reverb(lowpass(out, 3500) + low, 0.3, 1.2, 0.4, 22), 0.001, 0.15)


def s_death():
    """Generic death sting: descending minor pulse arpeggio + low hit."""
    out = np.zeros(secs(1.6))
    seq = ["E5", "C5", "A4", "F4", "D4"]
    for i, nm in enumerate(seq):
        b = blip(note(nm), 0.32, "pulse", duty=0.25, decay=0.14, vib=0.004)
        out = mix(out, at(b * 0.5, i * 0.11))
    m = secs(0.9)
    tt = np.arange(m) / SR
    low = (sine(midi_hz(note("D2")) * np.ones(m), m) + 0.4 * tri(midi_hz(note("D3")) * np.ones(m), m)) * np.exp(-tt * 3)
    out = mix(out, at(low * 0.6, 0.5))
    return fade(reverb(out, 0.3, 1.2, 0.4, 23), 0.001, 0.1)


def s_give_up():
    """Self-destruct: short fuse beep then explosion."""
    r = rng(24)
    out = np.zeros(secs(1.6))
    for i in range(3):
        out = mix(out, at(blip(note("A5"), 0.06, "square", decay=0.03) * 0.3, i * 0.09))
    n = secs(1.3)
    t = np.arange(n) / SR
    boom = lowpass_sweep(noise(n, r), 4000 * np.exp(-t * 4) + 120, q=0.8) * np.exp(-t * 3.2) * 1.4
    sub = sine(np.linspace(90, 30, n), n) * np.exp(-t * 4) * 0.9
    crack = highpass(noise(n, r), 2000) * exp_env(n, 0.02) * 0.6
    debris = lofi_noise(n, r, 3000) * np.exp(-t * 5) * 0.25
    out = mix(out, at(boom + sub + crack + debris, 0.27))
    return fade(reverb(out, 0.3, 1.3, 0.4, 24), 0.001, 0.15)


def s_timer_tick():
    r = rng(25)
    n = secs(0.09)
    c = bandpass(noise(n, r), 2500, 7000) * exp_env(n, 0.006)
    b = sine(midi_hz(note("E6")) * np.ones(n), n) * exp_env(n, 0.02) * 0.5
    return fade(c + b, 0.0003, 0.01)


def s_level_win():
    out = np.zeros(secs(1.8))
    # rising fanfare: G C E G | C (major) with harmony
    seq = [("G4", 0.0, 0.12), ("C5", 0.1, 0.12), ("E5", 0.2, 0.12), ("G5", 0.3, 0.12), ("C6", 0.42, 0.7)]
    for nm, st, d in seq:
        b = blip(note(nm), d + 0.15, "pulse", duty=0.25, decay=d * 0.9 + 0.05, vib=0.003 if d > 0.3 else 0)
        out = mix(out, at(b * 0.45, st))
    ch = ["C5", "E5", "G5"]
    for nm in ch:
        b = blip(note(nm), 0.9, "pulse", duty=0.5, decay=0.35) * 0.18
        out = mix(out, at(b, 0.42))
    bass = blip(note("C3"), 0.9, "tri", decay=0.4) * 0.6
    out = mix(out, at(bass, 0.42))
    sp = blip(note("G7"), 0.4, "sine", decay=0.15) * 0.15
    out = mix(out, at(sp, 0.5), at(sp, 0.62))
    return fade(reverb(out, 0.3, 1.2, 0.4, 26), 0.001, 0.15)


def s_menu_move(m="A5"):
    n = secs(0.07)
    x = blip(note(m), 0.07, "pulse", duty=0.25, decay=0.025)
    x += blip(note(m) + 12, 0.07, "square", decay=0.012) * 0.3
    return fade(x, 0.0005, 0.01)


def s_menu_select():
    out = mix(blip(note("E5"), 0.12, "pulse", duty=0.25, decay=0.05) * 0.6,
              at(blip(note("A5"), 0.25, "pulse", duty=0.25, decay=0.1) * 0.6, 0.06),
              at(blip(note("E6"), 0.25, "square", decay=0.08) * 0.2, 0.06))
    return fade(reverb(out, 0.2, 0.6, 0.25, 27), 0.0005, 0.04)


def s_error():
    n = secs(0.28)
    t = np.arange(n) / SR
    f = midi_hz(note("A2")) * np.ones(n)
    x = (pulse(f, n, 0.5) * 0.6 + pulse(f * 1.06, n, 0.25) * 0.4)
    gate = (np.sin(2 * np.pi * 14 * t) > -0.3).astype(float)
    gate = lowpass(gate, 200)
    x = lowpass(x, 2200) * gate * adsr(n, 0.004, 0.05, 0.8, 0.05)
    return fade(x, 0.001, 0.02)


# ======================================================================== piton
def p_eat():
    r = rng(40)
    out = mix(blip(note("C6"), 0.08, "pulse", duty=0.125, decay=0.03) * 0.5,
              at(blip(note("G6"), 0.12, "pulse", duty=0.125, decay=0.05) * 0.5, 0.045))
    n = secs(0.06)
    chomp = bandpass(noise(n, r), 600, 3000) * exp_env(n, 0.012) * 0.6
    out = mix(chomp, out)
    return fade(reverb(out, 0.15, 0.4, 0.15, 40), 0.0005, 0.02)


def p_turn():
    n = secs(0.05)
    x = blip(note("E5"), 0.05, "pulse", duty=0.5, decay=0.015) * 0.6
    x += sine(np.linspace(900, 600, n), n) * exp_env(n, 0.01) * 0.3
    return fade(x, 0.0005, 0.008)


def p_die():
    r = rng(42)
    n = secs(1.0)
    t = np.arange(n) / SR
    f = 600 * np.exp(-t * 2.4) + 60
    zap = pulse(f * (1 + 0.04 * np.sin(2 * np.pi * 30 * t)), n, 0.25) * np.exp(-t * 2.5) * 0.6
    m = secs(0.15)
    hit = (lowpass(noise(m, r), 1500) * 1.5 + sine(np.linspace(150, 50, m), m)) * exp_env(m, 0.04)
    out = mix(hit * 0.7, zap)
    return fade(reverb(out, 0.25, 0.9, 0.3, 42), 0.0005, 0.1)


def p_level_complete():
    out = np.zeros(secs(1.6))
    seq = ["C5", "E5", "G5", "C6", "E6", "G6"]
    for i, nm in enumerate(seq):
        out = mix(out, at(blip(note(nm), 0.18, "pulse", duty=0.125, decay=0.07) * 0.45, i * 0.065))
    for nm in ["C6", "E6", "G6", "C7"]:
        out = mix(out, at(blip(note(nm), 0.8, "pulse", duty=0.25, decay=0.3) * 0.15, 0.42))
    out = mix(out, at(blip(note("C3"), 0.8, "tri", decay=0.3) * 0.6, 0.42))
    return fade(reverb(out, 0.3, 1.0, 0.35, 43), 0.001, 0.12)


def p_menu_move():
    return s_menu_move("C6")


def p_menu_select():
    out = mix(blip(note("G5"), 0.1, "pulse", duty=0.125, decay=0.04) * 0.6,
              at(blip(note("C6"), 0.22, "pulse", duty=0.125, decay=0.09) * 0.6, 0.06))
    return fade(reverb(out, 0.2, 0.5, 0.2, 44), 0.0005, 0.03)


# ======================================================================== keyboard simulator
def k_key_correct():
    r = rng(60)
    n = secs(0.06)
    click = bandpass(noise(n, r), 1800, 6500) * exp_env(n, 0.004)
    body = sine(np.linspace(1400, 900, n), n) * exp_env(n, 0.008) * 0.25
    thock = sine(np.linspace(320, 220, n), n) * exp_env(n, 0.012) * 0.35
    return fade(click + body + thock, 0.0002, 0.01)


def k_key_wrong():
    n = secs(0.2)
    t = np.arange(n) / SR
    f = midi_hz(note("D3"))
    x = (saw(f * np.ones(n), n) + saw(f * 1.012 * np.ones(n), n)) * 0.5
    x = lowpass(x, 900) * adsr(n, 0.01, 0.05, 0.7, 0.08) * (0.8 + 0.2 * np.sin(2 * np.pi * 25 * t))
    return fade(x, 0.002, 0.03)


def k_sentence_complete():
    out = mix(blip(note("E6"), 0.4, "sine", decay=0.18) * 0.5,
              at(blip(note("B6"), 0.6, "sine", decay=0.25) * 0.45, 0.08))
    out = mix(out, blip(note("E7"), 0.3, "sine", decay=0.08) * 0.08, at(blip(note("B5"), 0.6, "tri", decay=0.25) * 0.2, 0.08))
    return fade(reverb(out, 0.3, 1.0, 0.35, 62), 0.001, 0.08)


def k_button_click():
    r = rng(63)
    n = secs(0.05)
    a = bandpass(noise(n, r), 2000, 8000) * exp_env(n, 0.003)
    b = sine(np.linspace(700, 500, n), n) * exp_env(n, 0.01) * 0.4
    return fade(a + b, 0.0002, 0.008)


# ======================================================================== main
SOUNDS = [
    # (dir, name, fn, gain)  -- gain trims frequent/subtle sounds below -1 dBFS peak
    (OUT_SOLDIER, "step", lambda: s_step(11), 0.55),
    (OUT_SOLDIER, "step_2", lambda: s_step(111, 1.25), 0.55),
    (OUT_SOLDIER, "turn", s_turn, 0.5),
    (OUT_SOLDIER, "push_box", s_push_box, 0.8),
    (OUT_SOLDIER, "box_sink", s_box_sink, 0.9),
    (OUT_SOLDIER, "button_press", s_button_press, 0.8),
    (OUT_SOLDIER, "button_release", s_button_release, 0.7),
    (OUT_SOLDIER, "door_open", lambda: _door(True, 17), 0.85),
    (OUT_SOLDIER, "door_close", lambda: _door(False, 18), 0.85),
    (OUT_SOLDIER, "key_pickup", s_key_pickup, 0.8),
    (OUT_SOLDIER, "key_drop", s_key_drop, 0.7),
    (OUT_SOLDIER, "door_unlock", s_door_unlock, 0.85),
    (OUT_SOLDIER, "wall_reveal", s_wall_reveal, 0.9),
    (OUT_SOLDIER, "guillotine_chop", s_guillotine_chop, 1.0),
    (OUT_SOLDIER, "drown", s_drown, 0.9),
    (OUT_SOLDIER, "death", s_death, 0.9),
    (OUT_SOLDIER, "give_up", s_give_up, 1.0),
    (OUT_SOLDIER, "timer_tick", s_timer_tick, 0.5),
    (OUT_SOLDIER, "level_win", s_level_win, 0.9),
    (OUT_SOLDIER, "menu_move", s_menu_move, 0.5),
    (OUT_SOLDIER, "menu_select", s_menu_select, 0.7),
    (OUT_SOLDIER, "error", s_error, 0.6),
    (OUT_PITON, "eat", p_eat, 0.75),
    (OUT_PITON, "turn", p_turn, 0.4),
    (OUT_PITON, "die", p_die, 0.9),
    (OUT_PITON, "level_complete", p_level_complete, 0.9),
    (OUT_PITON, "menu_move", p_menu_move, 0.5),
    (OUT_PITON, "menu_select", p_menu_select, 0.7),
    (OUT_KS, "key_correct", k_key_correct, 0.45),
    (OUT_KS, "key_wrong", k_key_wrong, 0.5),
    (OUT_KS, "sentence_complete", k_sentence_complete, 0.6),
    (OUT_KS, "button_click", k_button_click, 0.5),
]


def main():
    for d, name, fn, gain in SOUNDS:
        d.mkdir(parents=True, exist_ok=True)
        x = np.asarray(fn(), dtype=float)
        x = x - np.mean(x)
        assert np.all(np.isfinite(x)), name
        x = fade(x, 0.0005, 0.005)
        write_wav(d / f"{name}.wav", x, gain)
        print(f"{d.relative_to(ROOT).as_posix()}/{name}.wav  {len(x) / SR:.2f}s")


if __name__ == "__main__":
    main()
