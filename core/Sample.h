// Sample.h: a stereo sample and a WAV parser (16/24-bit PCM, 32-bit float; mono or stereo). No Rack dependency.
// Mono is stored as two equal channels. Little-endian hosts only (x86, ARM).
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

struct Sample
{
    std::vector<float> l, r;
    float rate = 48000.f;
    int size() const { return (int) l.size(); }
};

namespace wavfile
{
constexpr size_t MAX_FRAMES = 48000 * 30;   // one pad holds at most 30 s; longer files are cut

inline uint32_t u32 (const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t) p[3] << 24; }
inline uint16_t u16 (const uint8_t* p) { return (uint16_t) (p[0] | p[1] << 8); }

inline bool parse (const uint8_t* d, size_t n, Sample& out)
{
    if (n < 12 || std::memcmp (d, "RIFF", 4) || std::memcmp (d + 8, "WAVE", 4)) return false;
    int tag = 0, ch = 0, bits = 0;
    uint32_t rate = 0;
    for (size_t i = 12; i + 8 <= n;)
    {
        const uint32_t len = u32 (d + i + 4);
        const uint8_t* c = d + i + 8;
        const size_t avail = n - (i + 8), body = len < avail ? len : avail;   // a lying size is clipped to the file
        if (! std::memcmp (d + i, "fmt ", 4) && body >= 16)
        {
            tag = u16 (c); ch = u16 (c + 2); rate = u32 (c + 4); bits = u16 (c + 14);
            if (tag == 0xFFFE && body >= 26) tag = u16 (c + 24);   // WAVE_FORMAT_EXTENSIBLE: real tag is the sub-format's first word
        }
        else if (! std::memcmp (d + i, "data", 4))
        {
            const bool pcm = tag == 1 && (bits == 16 || bits == 24), flt = tag == 3 && bits == 32;
            if (! (pcm || flt) || (ch != 1 && ch != 2) || rate < 8000 || rate > 384000) return false;
            const size_t bps = (size_t) bits / 8, frames = std::min<size_t> (body / (bps * ch), MAX_FRAMES);
            if (! frames) return false;
            out.l.assign (frames, 0.f);
            out.r.assign (frames, 0.f);
            out.rate = (float) rate;
            for (size_t f = 0; f < frames; ++f)
            {
                float v[2] = { 0.f, 0.f };
                for (int k = 0; k < ch; ++k)
                {
                    const uint8_t* s = c + (f * ch + k) * bps;
                    if (bits == 16) v[k] = (int16_t) u16 (s) / 32768.f;
                    else if (bits == 24) v[k] = ((int32_t) (s[0] << 8 | s[1] << 16 | (uint32_t) s[2] << 24) >> 8) / 8388608.f;
                    else { float x; std::memcpy (&x, s, 4); v[k] = std::isfinite (x) ? x : 0.f; }
                }
                out.l[f] = v[0];
                out.r[f] = ch == 2 ? v[1] : v[0];
            }
            return true;
        }
        i += 8 + (size_t) len + (len & 1);   // chunks are word-aligned
    }
    return false;
}
}
