// test_clock.h: the step clock, internal and external.
#pragma once
#include "check.h"
#include "StepClock.h"

inline void testClock()
{
    const double dt = 1.0 / 48000;
    StepClock c;
    c.bpm = 120; c.length = 16;
    int fires = 0;
    for (int i = 0; i < 95520; ++i) if (c.tick (dt)) ++fires;   // 1.99 s: steps at 0, 0.125 ... 1.875
    check (fires == 16, "120 bpm = 16 sixteenth steps in 1.99 s", fires);

    StepClock s;
    s.bpm = 120; s.swing = 0.5; s.length = 16;
    double t = 0, t1 = -1, t2 = -1;
    for (int i = 0; i < 48000 && t2 < 0; ++i, t += dt)
        if (s.tick (dt)) { if (s.step == 1) t1 = t; if (s.step == 2) t2 = t; }
    check (near (t1, 0.1875, 1e-3), "swing 0.5: step 1 lands 1.5 steps in", t1);
    check (near (t2, 0.25, 1e-3), "swing 0.5: step 2 is back on the grid", t2);

    StepClock w;
    w.length = 4;
    for (int i = 0; i < 9; ++i) w.pulse();
    check (w.step == 0, "9 pulses at length 4 wrap to step 0", w.step);
    w.reset();
    w.pulse();
    check (w.step == 0, "after reset the next pulse is step 0", w.step);

    StepClock r;
    r.bpm = 120; r.reset();
    check (r.tick (dt) && r.step == 0, "after reset the internal clock fires step 0 at once");
}
