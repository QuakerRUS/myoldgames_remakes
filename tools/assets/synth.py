"""Small numpy synthesizer used by gen_sfx.py and gen_music.py.

Everything here is original code; all sounds are synthesized from scratch
(oscillators, filtered noise, envelopes, a generated reverb impulse response).
"""
from __future__ import annotations

import re

import numpy as np
from scipy import signal

SR = 44100


# ---------------------------------------------------------------- helpers
def secs(n: float) -> int:
    return int(round(n * SR))


def t_axis(dur: float) -> np.ndarray:
    return np.arange(secs(dur)) / SR


def midi_hz(m):
    return 440.0 * 2.0 ** ((np.asarray(m, dtype=float) - 69.0) / 12.0)


_NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def note(name: str) -> int:
    """'C4' -> 60, 'F#5' -> 78, 'Bb3' -> 58."""
    m = re.fullmatch(r"([A-G])([#b]?)(-?\d)", name)
    if not m:
        raise ValueError(name)
    v = _NOTE[m.group(1)] + (1 if m.group(2) == "#" else -1 if m.group(2) == "b" else 0)
    return v + 12 * (int(m.group(3)) + 1)


def as_freq(f, n):
    f = np.asarray(f, dtype=float)
    if f.ndim == 0:
        return np.full(n, float(f))
    return f[:n] if len(f) >= n else np.pad(f, (0, n - len(f)), mode="edge")


# ---------------------------------------------------------------- oscillators
def phase(freq, n, ph0=0.0):
    f = as_freq(freq, n)
    return (ph0 + np.cumsum(f) / SR) % 1.0, f / SR


def _blep(t, dt):
    out = np.zeros_like(t)
    dt = np.maximum(dt, 1e-9)
    a = t < dt
    x = t[a] / dt[a]
    out[a] = x + x - x * x - 1.0
    b = t > 1.0 - dt
    x = (t[b] - 1.0) / dt[b]
    out[b] = x * x + x + x + 1.0
    return out


def saw(freq, n, ph0=0.0):
    t, dt = phase(freq, n, ph0)
    return 2.0 * t - 1.0 - _blep(t, dt)


def pulse(freq, n, duty=0.5, ph0=0.0):
    t, dt = phase(freq, n, ph0)
    duty = as_freq(duty, n)
    t2 = (t + 1.0 - duty) % 1.0
    s1 = 2.0 * t - 1.0 - _blep(t, dt)
    s2 = 2.0 * t2 - 1.0 - _blep(t2, dt)
    return (s1 - s2) * 0.5 + (2 * duty - 1) * 0.0


def square(freq, n, ph0=0.0):
    return pulse(freq, n, 0.5, ph0)


def tri(freq, n, ph0=0.0):
    t, _ = phase(freq, n, ph0)
    return 1.0 - 4.0 * np.abs(t - 0.5)


def nes_tri(freq, n, ph0=0.0):
    """4-bit stepped triangle (NES flavour), slightly smoothed."""
    x = tri(freq, n, ph0)
    x = np.round(x * 7.5) / 7.5
    return lowpass(x, 9000)


def sine(freq, n, ph0=0.0):
    t, _ = phase(freq, n, ph0)
    return np.sin(2 * np.pi * t)


def noise(n, rng):
    return rng.uniform(-1.0, 1.0, n)


def lofi_noise(n, rng, rate=8000.0):
    """Sample-and-hold noise (NES-ish noise channel)."""
    step = max(1, int(SR / rate))
    vals = rng.uniform(-1, 1, n // step + 2)
    return np.repeat(vals, step)[:n]


# ---------------------------------------------------------------- envelopes
def adsr(n, a=0.005, d=0.05, s=0.7, r=0.05, gate=None):
    """Linear-ish ADSR; gate (seconds) = note-on length, release follows."""
    if gate is None:
        gate = n / SR - r
    na, nd, ng = secs(a), secs(d), secs(gate)
    env = np.zeros(n)
    i = np.arange(n)
    with np.errstate(divide="ignore", invalid="ignore"):
        env = np.where(i < na, i / max(na, 1), 0.0)
        dec = (i >= na) & (i < na + nd)
        env = np.where(dec, 1.0 - (1.0 - s) * (i - na) / max(nd, 1), env)
        env = np.where((i >= na + nd) & (i < ng), s, env)
    # value at gate
    gv = env[min(ng, n - 1)] if ng < n else 0.0
    if ng < na + nd:
        gv = env[max(ng - 1, 0)]
    rel = i >= ng
    nr = max(secs(r), 1)
    env = np.where(rel, gv * np.clip(1.0 - (i - ng) / nr, 0, 1), env)
    return env


def exp_env(n, decay, attack=0.002):
    t = np.arange(n) / SR
    e = np.exp(-t / max(decay, 1e-4))
    na = secs(attack)
    if na > 0:
        e[:na] *= np.linspace(0, 1, na)
    return e


def fade(x, fin=0.002, fout=0.01):
    x = x.copy()
    a, b = secs(fin), secs(fout)
    if a:
        x[:a] *= np.linspace(0, 1, a)
    if b:
        x[-b:] *= np.linspace(1, 0, b)
    return x


# ---------------------------------------------------------------- filters
def _sos(kind, f, sr=SR, order=2):
    f = np.clip(f, 10, sr * 0.45)
    return signal.butter(order, f, btype=kind, fs=sr, output="sos")


def lowpass(x, f, order=2):
    return signal.sosfilt(_sos("lowpass", f, order=order), x)


def highpass(x, f, order=2):
    return signal.sosfilt(_sos("highpass", f, order=order), x)


def bandpass(x, lo, hi, order=2):
    lo = max(10, lo)
    hi = min(SR * 0.45, hi)
    return signal.sosfilt(signal.butter(order, [lo, hi], btype="bandpass", fs=SR, output="sos"), x)


def lowpass_sweep(x, cut, q=0.9, block=128):
    """Time-varying resonant lowpass (RBJ biquad, coefficients per block)."""
    n = len(x)
    cut = as_freq(cut, n)
    y = np.zeros(n)
    zi = np.zeros(2)
    for s in range(0, n, block):
        e = min(n, s + block)
        f0 = float(np.clip(cut[s], 20, SR * 0.45))
        w0 = 2 * np.pi * f0 / SR
        alpha = np.sin(w0) / (2 * q)
        cw = np.cos(w0)
        b = np.array([(1 - cw) / 2, 1 - cw, (1 - cw) / 2])
        a = np.array([1 + alpha, -2 * cw, 1 - alpha])
        y[s:e], zi = signal.lfilter(b / a[0], a / a[0], x[s:e], zi=zi)
    return y


# ---------------------------------------------------------------- effects
def make_ir(dur=1.6, decay=0.5, seed=1, damp=6000.0, predelay=0.012, stereo=True):
    rng = np.random.default_rng(seed)
    n = secs(dur)
    outs = []
    for ch in range(2 if stereo else 1):
        x = rng.standard_normal(n) * np.exp(-np.arange(n) / SR / decay * 3.0)
        x = lowpass(x, damp)
        # gentle high damping that grows with time: blend with stronger lowpass
        xl = lowpass(x, damp * 0.35)
        w = np.linspace(0, 1, n) ** 0.7
        x = x * (1 - w) + xl * w
        x = np.concatenate([np.zeros(secs(predelay) + ch * 37), x])[:n]
        x /= np.sqrt(np.sum(x * x)) + 1e-12
        outs.append(x)
    return outs


def reverb(x, wet=0.2, dur=1.4, decay=0.45, seed=1, damp=6000.0):
    """Mono in -> mono out (uses first IR channel)."""
    ir = make_ir(dur, decay, seed, damp, stereo=False)[0]
    x = np.concatenate([x, np.zeros(secs(decay * 1.2))])  # room for the tail
    y = signal.fftconvolve(x, ir)[: len(x)]
    return x * (1 - wet * 0.5) + y * wet


def pad_to(x, n):
    return np.pad(x, (0, max(0, n - len(x))))[: max(n, len(x))]


def normalize(x, peak=0.891):
    m = np.max(np.abs(x))
    return x if m < 1e-9 else x * (peak / m)


def soft_clip(x, drive=1.0):
    return np.tanh(x * drive) / np.tanh(drive)


def write_wav(path, x, gain=1.0):
    import soundfile as sf
    x = normalize(np.asarray(x, dtype=float)) * gain
    x = np.clip(x, -1.0, 1.0)
    sf.write(str(path), x.astype(np.float32), SR, subtype="PCM_16")
