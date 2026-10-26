// test_comp.h: the feed-forward compressor.
#pragma once
#include "check.h"
#include "Comp.h"

// settled gain for a constant input level (linear)
inline float compGain (float thr, float ratio, float makeup, float level)
{
    Comp c;
    c.thr = thr; c.ratio = ratio; c.makeup = makeup;
    float g = 1.f;
    for (int i = 0; i < 48000; ++i) g = c.gain (level, level, 48000.f);
    return g;
}

inline void testComp()
{
    check (near (compGain (-20.f, 4.f, 0.f, 1.f), 0.1778, 0.01), "0 dB in, -20 thr, 4:1 => -15 dB", compGain (-20.f, 4.f, 0.f, 1.f));
    check (near (compGain (-20.f, 1.f, 0.f, 1.f), 1.0, 1e-3), "ratio 1:1 does nothing", compGain (-20.f, 1.f, 0.f, 1.f));
    check (near (compGain (0.f, 8.f, 0.f, 0.5f), 1.0, 1e-3), "below the threshold nothing happens", compGain (0.f, 8.f, 0.f, 0.5f));
    check (near (compGain (0.f, 1.f, 6.f, 0.1f), 1.995, 0.01), "makeup 6 dB = x1.995", compGain (0.f, 1.f, 6.f, 0.1f));

    // attack: a loud burst is not clamped on the very first sample
    Comp c;
    c.thr = -20.f; c.ratio = 20.f; c.atk = 10.f;
    const float first = c.gain (1.f, 1.f, 48000.f);
    float settled = first;
    for (int i = 0; i < 48000; ++i) settled = c.gain (1.f, 1.f, 48000.f);
    check (first > settled * 2.f, "attack lets the transient through", first);

    // release: after the burst the gain recovers toward 1
    c.rel = 50.f;
    float g = settled;
    for (int i = 0; i < 48000; ++i) g = c.gain (0.f, 0.f, 48000.f);
    check (near (g, 1.0, 0.02), "release recovers to unity", g);
}
