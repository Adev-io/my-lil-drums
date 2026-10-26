// test_voice.h: the sample voice.
#pragma once
#include "check.h"
#include "Voice.h"

inline Sample ramp (int n, float rate)
{
    Sample s;
    s.rate = rate;
    for (int i = 0; i < n; ++i) { s.l.push_back ((float) i / (n - 1)); s.r.push_back (-(float) i / (n - 1)); }
    return s;
}

inline void testVoiceBasic()
{
    Sample s = ramp (5, 48000.f);   // 0, .25, .5, .75, 1
    Voice v;
    float l, r;
    v.process (48000.f, l, r);
    check (l == 0.f && r == 0.f, "an idle voice is silent");
    v.trigger (&s, 127);
    v.process (48000.f, l, r);
    check (near (l, 0.0) && near (r, 0.0), "first sample out is index 0");
    v.process (48000.f, l, r);
    check (near (l, 0.25) && near (r, -0.25), "one sample per tick at native rate", l);
    v.process (48000.f, l, r); v.process (48000.f, l, r);
    check (near (l, 0.75), "index 3", l);
    v.process (48000.f, l, r);
    check (! v.on && l == 0.f, "the voice ends at the last sample");

    v.trigger (&s, 127);
    v.process (96000.f, l, r); v.process (96000.f, l, r);
    check (near (l, 0.125), "a 2x engine rate reads half a sample per tick (interpolated)", l);

    v.trigger (&s, 64);
    v.process (48000.f, l, r); v.process (48000.f, l, r);
    check (near (l, 0.25 * 64 / 127.0), "velocity scales gain", l);

    v.trigger (&s, 127);
    v.process (48000.f, l, r); v.process (48000.f, l, r);
    v.trigger (&s, 127);
    v.process (48000.f, l, r);
    check (near (l, 0.0), "retrigger restarts the sample", l);

    Sample empty;
    Voice e;
    e.trigger (&empty, 100);
    check (! e.on, "an empty sample never starts");
    e.trigger (nullptr, 100);
    check (! e.on, "a null sample never starts");
}
