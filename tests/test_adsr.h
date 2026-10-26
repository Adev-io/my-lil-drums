// test_adsr.h: the gate ADSR.
#pragma once
#include "check.h"
#include "Adsr.h"

inline void testAdsr()
{
    const float dt = 0.001f;
    Adsr e;
    e.a = 0.01f; e.d = 0.01f; e.s = 0.5f; e.r = 0.01f;
    check (! e.active() && e.next (dt) == 0.f, "idle envelope is 0 and inactive");
    e.gate (true);
    float v = 0;
    for (int i = 0; i < 10; ++i) v = e.next (dt);
    check (near (v, 1.f, 0.02), "attack reaches 1 in a ms", v);
    for (int i = 0; i < 12; ++i) v = e.next (dt);
    check (near (v, 0.5f, 0.02), "decay settles on sustain", v);
    for (int i = 0; i < 50; ++i) v = e.next (dt);
    check (near (v, 0.5f, 1e-4), "sustain holds while the gate is high", v);
    e.gate (false);
    for (int i = 0; i < 5; ++i) v = e.next (dt);
    check (v > 0.f && v < 0.5f, "release falls", v);
    for (int i = 0; i < 10; ++i) v = e.next (dt);
    check (v == 0.f && ! e.active(), "release ends at 0 and the envelope goes idle", v);

    Adsr t;
    t.a = 0.01f; t.r = 0.01f;
    t.gate (true);
    for (int i = 0; i < 5; ++i) v = t.next (dt);
    const float mid = v;
    t.gate (true);   // retrigger mid-attack: continue from here, no jump to 0
    v = t.next (dt);
    check (v >= mid, "retrigger does not drop the level", v);

    Adsr c;
    c.a = 0.0001f; c.s = 1.f; c.r = 1.f;
    c.gate (true);
    for (int i = 0; i < 20; ++i) c.next (dt);
    c.choke();
    for (int i = 0; i < 7; ++i) v = c.next (dt);
    check (v == 0.f && ! c.active(), "choke silences within ~5 ms even with a 1 s release", v);

    Adsr z;
    z.a = 0.f; z.d = 0.f; z.r = 0.f; z.s = 1.f;
    z.gate (true);
    v = z.next (dt);
    check (v > 0.f && v <= 1.f, "zero-length stages do not divide by zero", v);
}
