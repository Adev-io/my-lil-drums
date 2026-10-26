// StepClock.h: sixteenth-note step clock. tick(dt) runs it from BPM; pulse() steps it from an external clock.
// Both return true when a new step starts; step is the index just started. Swing delays odd steps.
// No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once

struct StepClock
{
    int length = 16, step = -1;
    double bpm = 120, swing = 0, t = 0;   // swing 0..0.5: even steps last (1+swing) steps, odd ones (1-swing)
    bool first = true;

    void reset() { step = -1; t = 0; first = true; }

    // ponytail: swing only applies to the internal clock; an external clock keeps its own timing
    bool tick (double dt)
    {
        if (first) { first = false; step = 0; t = 0; return true; }
        t += dt;
        const double d = 15.0 / bpm, dur = (step & 1) ? d * (1 - swing) : d * (1 + swing);
        if (t < dur) return false;
        t -= dur;
        step = (step + 1) % length;
        return true;
    }

    bool pulse()
    {
        first = false;
        step = (step + 1) % length;
        t = 0;
        return true;
    }
};
