// test_filter.h: one-pole low-pass and high-pass.
#pragma once
#include "check.h"
#include "Filter.h"

// peak of the filter's output for a sine at hz, after it has settled
inline float filterGain (float lp, float hp, float hz)
{
    PadFilter f;
    f.lpHz = lp; f.hpHz = hp;
    const float sr = 48000.f;
    float peak = 0.f;
    for (int i = 0; i < 48000; ++i)
    {
        float l = std::sin (6.2831853f * hz * i / sr), r = l;
        f.process (l, r, sr);
        if (i > 24000) peak = std::max (peak, std::fabs (l));
    }
    return peak;
}

inline void testFilter()
{
    check (near (filterGain (20000.f, 20.f, 1000.f), 1.0, 0.06), "open filter passes 1 kHz", filterGain (20000.f, 20.f, 1000.f));
    check (filterGain (100.f, 20.f, 5000.f) < 0.05f, "100 Hz low-pass cuts 5 kHz", filterGain (100.f, 20.f, 5000.f));
    check (filterGain (20000.f, 4000.f, 100.f) < 0.05f, "4 kHz high-pass cuts 100 Hz", filterGain (20000.f, 4000.f, 100.f));
    check (filterGain (20000.f, 20.f, 100.f) > 0.9f, "20 Hz high-pass keeps 100 Hz", filterGain (20000.f, 20.f, 100.f));

    PadFilter f;
    f.lpHz = 1e9f; f.hpHz = 1e-3f;   // out-of-range cutoffs must stay finite
    float l = 1.f, r = 1.f;
    for (int i = 0; i < 1000; ++i) { l = 1.f; r = 1.f; f.process (l, r, 48000.f); }
    check (std::isfinite (l) && std::isfinite (r), "extreme cutoffs stay finite");
}
