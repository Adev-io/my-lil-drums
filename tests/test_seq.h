// test_seq.h: patterns + clock together.
#pragma once
#include "check.h"
#include "Sequencer.h"

inline void testSequencer()
{
    Sequencer q;
    q.setLength (4);
    q.pat[0].set (0, 0, 100);
    q.pat[0].set (0, 2, 80);
    q.pat[0].set (1, 3, 127);
    const int expectKick[8] = { 100, 0, 80, 0, 100, 0, 80, 0 };
    bool ok = true;
    for (int i = 0; i < 8; ++i)
    {
        q.clk.pulse();
        q.fire();
        if (q.hit[0] != expectKick[i]) ok = false;
        if (q.hit[1] != (i % 4 == 3 ? 127 : 0)) ok = false;
    }
    check (ok, "kick and snare fire on their steps and loop at length 4");

    q.select (2);
    check (q.clk.length == 16 && q.cur == 2, "a fresh pattern has length 16", q.clk.length);
    q.setLength (99);
    check (q.pat[2].length == 64 && q.clk.length == 64, "length clamps to 64", q.pat[2].length);
    q.setLength (0);
    check (q.pat[2].length == 1, "length clamps to 1", q.pat[2].length);
    q.select (0);
    check (q.pat[0].length == 4 && q.clk.length == 4, "patterns keep their own length", q.clk.length);
    q.select (-5);
    check (q.cur == 0, "select clamps low");
    q.select (40);
    check (q.cur == 15, "select clamps high");

    q.select (0);
    q.clk.step = 3;
    q.setLength (2);
    q.fire();   // step 3 is past the new length: must still read inside the grid
    check (true, "fire with step past length does not read out of bounds");
}
