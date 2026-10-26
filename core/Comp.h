// Comp.h: feed-forward peak compressor for a pad. No Rack dependency.
// gain() returns the linear gain to multiply both channels by (reduction and makeup in one).
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>
#include <cmath>

struct Comp
{
    float thr = 0.f, ratio = 1.f, atk = 5.f, rel = 80.f, makeup = 0.f;   // dB, :1, ms, ms, dB
    float env = 0.f;

    float gain (float inL, float inR, float sr)
    {
        const float x = std::max (std::fabs (inL), std::fabs (inR));
        const float ms = x > env ? atk : rel;
        env += (1.f - std::exp (-1.f / (std::max (ms, 0.01f) * 0.001f * sr))) * (x - env);
        const float over = 20.f * std::log10 (std::max (env, 1e-6f)) - thr;
        const float red = (over > 0.f && ratio > 1.f) ? over * (1.f - 1.f / ratio) : 0.f;
        return std::pow (10.f, (makeup - red) / 20.f);
    }
};
