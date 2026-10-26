// Filter.h: stereo one-pole low-pass then high-pass for a pad. No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>
#include <cmath>

struct OnePole
{
    float z = 0.f;
    float process (float x, float a) { z += a * (x - z); return z; }
};

inline float onePoleCoef (float fc, float sr)
{
    fc = std::max (1.f, std::min (fc, 0.45f * sr));
    return 1.f - std::exp (-6.2831853f * fc / sr);
}

struct PadFilter
{
    OnePole lpL, lpR, hpL, hpR;
    float lpHz = 20000.f, hpHz = 20.f;

    void process (float& l, float& r, float sr)
    {
        // Knobs at their end stops are bypassed: a 20 Hz high-pass would otherwise bleed DC (and quiet sample bodies) away.
        if (lpHz < 20000.f)
        {
            const float al = onePoleCoef (lpHz, sr);
            l = lpL.process (l, al);
            r = lpR.process (r, al);
        }
        if (hpHz > 20.f)
        {
            const float ah = onePoleCoef (hpHz, sr);
            l -= hpL.process (l, ah);   // high-pass = input minus its own low-pass
            r -= hpR.process (r, ah);
        }
    }
};
