// Adsr.h: a linear gate ADSR with a fast choke. No Rack dependency.
// a, d, r are seconds; s is a level 0..1. gate(true) retriggers from the current level (no click).
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>

struct Adsr
{
    enum State { IDLE, ATK, DEC, SUS, REL };
    State st = IDLE;
    float v = 0.f, a = 0.001f, d = 0.2f, s = 1.f, r = 0.05f, rate = 0.f;

    bool active() const { return st != IDLE; }

    void gate (bool on)
    {
        if (on) st = ATK;
        else if (st != IDLE) { st = REL; rate = v / std::max (r, 1e-4f); }
    }
    // ~5 ms fade whatever the release knob says (choke groups)
    void choke()
    {
        if (st == IDLE) return;
        st = REL;
        rate = v / 0.005f;
    }

    float next (float dt)
    {
        switch (st)
        {
            case ATK: v += dt / std::max (a, 1e-4f); if (v >= 1.f) { v = 1.f; st = DEC; } break;
            case DEC: v -= (1.f - s) * dt / std::max (d, 1e-4f); if (v <= s) { v = s; st = SUS; } break;
            case SUS: v = s; break;
            case REL: v -= rate * dt; if (v <= 0.f) { v = 0.f; st = IDLE; } break;
            case IDLE: break;
        }
        return v;
    }
};
