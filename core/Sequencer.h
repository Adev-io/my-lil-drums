// Sequencer.h: 16 patterns, the current one, the clock, and the hits of the step just fired. No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include "Pattern.h"
#include "StepClock.h"

struct Sequencer
{
    Pattern pat[PATTERNS];
    int cur = 0;
    StepClock clk;
    uint8_t hit[PADS] = {};   // velocity per pad for the step fire() last read; 0 = none

    Sequencer() { clk.length = pat[0].length; }

    void select (int i)
    {
        cur = std::max (0, std::min (PATTERNS - 1, i));
        clk.length = pat[cur].length;
    }
    void setLength (int n) { pat[cur].length = clk.length = std::max (1, std::min (MAXSTEPS, n)); }

    // call right after the clock fired a step; clk.step is always < MAXSTEPS
    void fire() { for (int p = 0; p < PADS; ++p) hit[p] = pat[cur].vel[p][clk.step]; }
};
