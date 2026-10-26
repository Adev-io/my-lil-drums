// Pattern.h: one drum pattern, 16 pads x 64 steps of velocity (0 = no hit), plus hex rows for the patch file.
// No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>

constexpr int PADS = 16, MAXSTEPS = 64, PATTERNS = 16;

struct Pattern
{
    uint8_t vel[PADS][MAXSTEPS] = {};
    int length = 16;

    void set (int pad, int step, int v) { vel[pad][step] = (uint8_t) std::max (0, std::min (127, v)); }
    void toggle (int pad, int step) { vel[pad][step] = vel[pad][step] ? 0 : 100; }
    void clear() { for (auto& row : vel) for (auto& s : row) s = 0; }
};

// Two lowercase hex digits per step.
inline std::string rowToHex (const uint8_t* row)
{
    static const char* d = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < MAXSTEPS; ++i) { s += d[row[i] >> 4]; s += d[row[i] & 15]; }
    return s;
}

// Writes row only when the whole string is valid.
inline bool rowFromHex (const std::string& s, uint8_t* row)
{
    if (s.size() != 2 * MAXSTEPS) return false;
    auto h = [] (char c) -> int { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
    uint8_t tmp[MAXSTEPS];
    for (int i = 0; i < MAXSTEPS; ++i)
    {
        const int a = h (s[2 * i]), b = h (s[2 * i + 1]);
        if (a < 0 || b < 0) return false;
        tmp[i] = (uint8_t) std::min (a * 16 + b, 127);
    }
    std::copy (tmp, tmp + MAXSTEPS, row);
    return true;
}
