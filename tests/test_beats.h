// test_beats.h: factory beats.
#pragma once
#include "check.h"
#include "Beats.h"

inline void testBeats()
{
    Pattern p;
    p.set (9, 9, 50);   // must be wiped
    applyBeat (p, BEATS[0]);
    check (p.length == 16, "a beat is 16 steps", p.length);
    check (p.vel[0][0] == 100 && p.vel[0][1] == 0, "boom-bap kick on step 1");
    check (p.vel[9][9] == 0, "applyBeat clears the old pattern");
    check (p.vel[1][4] == 100 && p.vel[1][12] == 100, "boom-bap snare on 2 and 4");
    int hits = 0;
    for (int i = 0; i < NUM_BEATS; ++i)
    {
        Pattern q;
        applyBeat (q, BEATS[i]);
        for (int pad = 0; pad < PADS; ++pad) for (int s = 0; s < MAXSTEPS; ++s) hits += q.vel[pad][s] ? 1 : 0;
        check (q.vel[0][0] > 0, BEATS[i].name);
    }
    check (hits > 30, "every beat has hits", hits);
    Pattern soft;
    applyBeat (soft, BEATS[2]);
    bool anySoft = false;
    for (int s = 0; s < 16; ++s) if (soft.vel[2][s] == 60) anySoft = true;
    check (anySoft, "trap hats include soft 'o' hits at velocity 60");
}
