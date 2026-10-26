#!/usr/bin/env python3
"""gen_kit.py: synthesises the bundled starter kit into res/kit/*.wav (16-bit mono, 44.1 kHz). Fixed seed, stdlib only.
Run from anywhere: py -3.11 tools/gen_kit.py
Author: Ari Klopfer / A_Development. MIT
"""
import math
import os
import random
import struct
import wave

SR = 44100
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rng = random.Random(505)


def render(seconds, fn):
    return [fn(i / SR) for i in range(int(seconds * SR))]


def sweep(f0, f1, k, t):
    """sine whose pitch falls from f0 to f1 at rate k; phase is the integral of the frequency"""
    return math.sin(2 * math.pi * (f1 * t + (f0 - f1) * (1 - math.exp(-k * t)) / k))


def noise():
    return rng.uniform(-1, 1)


def hp(xs):
    """first difference: a crude high-pass that is enough for hats"""
    return [b - a for a, b in zip([0.0] + xs, xs)]


def kick():
    return render(0.45, lambda t: sweep(160, 48, 28, t) * math.exp(-t * 8))


def snare():
    return render(0.28, lambda t: 0.6 * noise() * math.exp(-t * 18) + 0.6 * math.sin(2 * math.pi * 190 * t) * math.exp(-t * 28))


def chat():
    return hp(render(0.09, lambda t: noise() * math.exp(-t * 55)))


def ohat():
    return hp(render(0.45, lambda t: noise() * math.exp(-t * 9)))


def clap():
    def f(t):
        burst = sum(math.exp(-((t - b) * 120)) for b in (0.0, 0.011, 0.023) if t >= b)
        return noise() * (0.7 * burst + 0.5 * math.exp(-t * 16))
    return render(0.32, f)


def rim():
    return render(0.06, lambda t: (0.7 * math.sin(2 * math.pi * 1700 * t) + 0.5 * noise()) * math.exp(-t * 80))


def tom():
    return render(0.45, lambda t: sweep(220, 110, 12, t) * math.exp(-t * 9))


def crash():
    return hp(render(1.4, lambda t: noise() * math.exp(-t * 3)))


def write(name, xs):
    peak = max(abs(x) for x in xs) or 1
    path = os.path.join(ROOT, 'res', 'kit', name + '.wav')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b''.join(struct.pack('<h', int(x / peak * 0.9 * 32767)) for x in xs))
    print('wrote', path, len(xs), 'frames')


if __name__ == '__main__':
    for name, fn in (('kick', kick), ('snare', snare), ('chat', chat), ('ohat', ohat), ('clap', clap), ('rim', rim), ('tom', tom), ('crash', crash)):
        write(name, fn())
