// Beats.h: factory beats as text. 16 characters per row: 'x' hit (100), 'o' soft hit (60), '.' nothing.
// Pad map: 0 kick, 1 snare, 2 closed hat, 3 open hat, 4 clap, 5 rim, 6 tom, 7 crash. No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include "Pattern.h"

struct BeatRow { int pad; const char* steps; };
struct Beat { const char* name; BeatRow rows[4]; };   // pad < 0 ends a short list

constexpr int NUM_BEATS = 3;
static const Beat BEATS[NUM_BEATS] = {
    { "Boom bap", { { 0, "x.....x..x......" }, { 1, "....x.......x..." }, { 2, "x.x.x.x.x.x.x.x." }, { -1, "" } } },
    { "House",    { { 0, "x...x...x...x..." }, { 4, "....x.......x..." }, { 3, "..x...x...x...x." }, { 2, "o.o.o.o.o.o.o.o." } } },
    { "Trap",     { { 0, "x..x......x....." }, { 4, "........x......." }, { 2, "xoxoxoxoxoxoxxxo" }, { 3, "..............x." } } },
};

inline void applyBeat (Pattern& p, const Beat& b)
{
    p.clear();
    p.length = 16;
    for (const BeatRow& r : b.rows)
    {
        if (r.pad < 0) break;
        for (int s = 0; s < 16 && r.steps[s]; ++s) p.vel[r.pad][s] = r.steps[s] == 'x' ? 100 : r.steps[s] == 'o' ? 60 : 0;
    }
}
