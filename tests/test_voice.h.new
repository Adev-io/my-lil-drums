// test_voice.h: the sample voice and its chain.
#pragma once
#include "check.h"
#include "Voice.h"

inline Sample constant (float v, int n, float rate)
{
    Sample s;
    s.rate = rate;
    s.l.assign (n, v);
    s.r.assign (n, v);
    return s;
}

// engine rate 48 kHz throughout
inline int voiceLength (Voice& v, const PadCfg& c)
{
    float l, r;
    int n = 0;
    while (v.on && n < 480000) { v.process (c, 48000.f, l, r); ++n; }
    return n;
}

inline void testVoice()
{
    const float sr = 48000.f;
    float l, r;
    PadCfg c;
    c.atk = 0.0001f; c.rel = 0.01f;

    Sample ramp5;   // 0, .25, .5, .75, 1
    ramp5.rate = sr;
    for (int i = 0; i < 5; ++i) { ramp5.l.push_back (i / 4.f); ramp5.r.push_back (-i / 4.f); }
    PadCfg flat;   // instant attack, full sustain: the envelope is invisible
    flat.atk = 0.f; flat.rel = 0.f;
    Voice v;
    v.process (flat, sr, l, r);
    check (l == 0.f && r == 0.f, "an idle voice is silent");
    v.trigger (&ramp5, 127, flat, 0.125);
    v.process (flat, sr, l, r);
    v.process (flat, sr, l, r);
    check (near (l, 0.25 * v.env.v, 0.02) && r < 0.f, "native rate: one sample per tick, right channel kept", l);
    check (voiceLength (v, flat) < 5, "the voice ends at the last sample");

    Sample body = constant (1.f, 4800, sr);   // 100 ms of full-scale DC: easy to read levels off
    Voice a;
    a.trigger (&body, 127, c, 0.125);
    float mid = 0.f;
    for (int i = 0; i < 2400; ++i) a.process (c, sr, l, r);
    mid = l;
    check (near (mid, 1.0, 0.02), "one-shot sustains at full level", mid);
    int rest = 0;
    float tail = 1.f;
    while (a.on) { a.process (c, sr, l, r); tail = l; ++rest; }
    check (rest < 2400 && rest > 300, "one-shot fades over the release before the sample ends", rest);

    PadCfg up = c;
    up.pitch = 12.f;
    Voice p1, p2;
    p1.trigger (&body, 127, c, 0.125);
    p2.trigger (&body, 127, up, 0.125);
    const int n1 = voiceLength (p1, c), n2 = voiceLength (p2, up);
    check (std::abs (n1 / 2 - n2) < 30, "+12 semitones plays twice as fast", n2);

    PadCfg gate = c;
    gate.gateMode = true;
    Voice g;
    g.trigger (&body, 127, gate, 0.01);   // gate = 10 ms
    const int ng = voiceLength (g, gate);
    check (ng > 400 && ng < 1200, "gate mode releases after the step length (10 ms + 10 ms release)", ng);

    PadCfg pan = c;
    pan.pan = -1.f; pan.vol = 0.5f;
    Voice q;
    q.trigger (&body, 127, pan, 0.125);
    for (int i = 0; i < 2000; ++i) q.process (pan, sr, l, r);
    check (near (l, 0.5, 0.02) && near (r, 0.0, 1e-4), "pan -1 and vol 0.5: left 0.5, right 0", l);

    PadCfg quiet = c;
    Voice vq;
    vq.trigger (&body, 64, quiet, 0.125);
    for (int i = 0; i < 2000; ++i) vq.process (quiet, sr, l, r);
    check (near (l, 64 / 127.0, 0.02), "velocity scales the level", l);

    Voice rt;
    rt.trigger (&body, 127, c, 0.125);
    for (int i = 0; i < 1000; ++i) rt.process (c, sr, l, r);
    rt.trigger (&body, 127, c, 0.125);
    check (rt.pos == 0.0 && rt.on, "retrigger restarts the sample");

    Sample empty;
    Voice e;
    e.trigger (&empty, 100, c, 0.125);
    check (! e.on, "an empty sample never starts");
    e.trigger (nullptr, 100, c, 0.125);
    check (! e.on, "a null sample never starts");

    PadCfg lowpass = c;
    lowpass.lp = 50.f;
    Voice f;
    f.trigger (&body, 127, lowpass, 0.125);
    f.process (lowpass, sr, l, r);
    check (l < 0.1f, "the filter acts on the voice (50 Hz LP slows the first sample)", l);

    PadCfg squash = c;
    squash.thr = -20.f; squash.ratio = 20.f;
    Voice cv;
    cv.trigger (&body, 127, squash, 0.125);
    for (int i = 0; i < 4000; ++i) cv.process (squash, sr, l, r);
    check (l < 0.3f, "the compressor acts on the voice", l);
}

inline void testChoke()
{
    Sample body = constant (1.f, 48000, 48000.f);
    PadCfg cfg[PADS];
    cfg[2].choke = 1; cfg[3].choke = 1; cfg[4].choke = 2; cfg[5].choke = 0;
    Voice v[PADS];
    float l, r;
    for (int p = 2; p <= 5; ++p) v[p].trigger (&body, 127, cfg[p], 0.125);
    for (int i = 0; i < 100; ++i) for (int p = 2; p <= 5; ++p) v[p].process (cfg[p], 48000.f, l, r);
    chokeGroup (v, cfg, 3);   // pad 4 (index 3) fires: pad index 2 shares group 1
    for (int i = 0; i < 480; ++i) for (int p = 2; p <= 5; ++p) v[p].process (cfg[p], 48000.f, l, r);
    check (! v[2].on, "choke: same group is silenced");
    check (v[3].on, "choke: the firing pad itself keeps playing");
    check (v[4].on && v[5].on, "choke: other groups and group 0 are untouched");
    chokeGroup (v, cfg, 5);
    for (int i = 0; i < 480; ++i) v[4].process (cfg[4], 48000.f, l, r);
    check (v[4].on, "choke group 0 chokes nobody");
}
