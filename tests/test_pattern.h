// test_pattern.h: the pattern grid and its hex rows.
#pragma once
#include "check.h"
#include "Pattern.h"
#include <string>

inline void testPattern()
{
    Pattern p;
    p.set (3, 5, 100);
    check (p.vel[3][5] == 100, "set stores velocity", p.vel[3][5]);
    p.set (3, 5, 999);
    check (p.vel[3][5] == 127, "velocity clamps at 127", p.vel[3][5]);
    p.toggle (3, 5);
    check (p.vel[3][5] == 0, "toggle on a hit clears it");
    p.toggle (3, 5);
    check (p.vel[3][5] == 100, "toggle on an empty cell writes 100");
    p.clear();
    check (p.vel[3][5] == 0, "clear empties the grid");

    p.set (15, 63, 127);
    p.set (0, 0, 1);
    const std::string h = rowToHex (p.vel[15]);
    check (h.size() == 128 && h.substr (126) == "7f", "row hex is 2 chars per step", (double) h.size());
    uint8_t back[MAXSTEPS];
    check (rowFromHex (h, back) && back[63] == 127, "hex row round-trips");
    check (! rowFromHex ("zz", back), "short hex row is refused");
    check (! rowFromHex (std::string (128, 'x'), back), "non-hex row is refused");
    check (rowFromHex (std::string (128, 'f'), back) && back[0] == 127, "hex above 127 clamps");
}
