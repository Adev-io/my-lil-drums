// test_wav.h: the WAV parser on hand-built files.
#pragma once
#include "check.h"
#include "Sample.h"
#include <cstring>

inline std::vector<uint8_t> makeWav (int tag, int ch, int bits, int rate, const std::vector<uint8_t>& data, uint32_t dataLen = 0xFFFFFFFF)
{
    std::vector<uint8_t> b;
    auto s4 = [&] (const char* s) { b.insert (b.end(), s, s + 4); };
    auto p32 = [&] (uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back ((uint8_t) (v >> (8 * i))); };
    auto p16 = [&] (int v) { b.push_back ((uint8_t) v); b.push_back ((uint8_t) (v >> 8)); };
    s4 ("RIFF"); p32 ((uint32_t) (36 + data.size())); s4 ("WAVE");
    s4 ("fmt "); p32 (16); p16 (tag); p16 (ch); p32 (rate); p32 (rate * ch * bits / 8); p16 (ch * bits / 8); p16 (bits);
    s4 ("data"); p32 (dataLen == 0xFFFFFFFF ? (uint32_t) data.size() : dataLen);
    b.insert (b.end(), data.begin(), data.end());
    return b;
}

inline void testWav()
{
    Sample s;
    // 16-bit mono: 0, +0.5, -1.0
    std::vector<uint8_t> d = { 0, 0, 0x00, 0x40, 0x00, 0x80 };
    std::vector<uint8_t> w = makeWav (1, 1, 16, 44100, d);
    check (wavfile::parse (w.data(), w.size(), s), "16-bit mono parses");
    check (s.size() == 3 && near (s.l[1], 0.5) && near (s.l[2], -1.0) && s.r == s.l && s.rate == 44100.f, "16-bit mono values", s.l.size());

    // 16-bit stereo: one frame L=+0.5 R=-0.5
    d = { 0x00, 0x40, 0x00, 0xC0 };
    w = makeWav (1, 2, 16, 48000, d);
    check (wavfile::parse (w.data(), w.size(), s) && near (s.l[0], 0.5) && near (s.r[0], -0.5), "16-bit stereo splits channels");

    // 24-bit mono: 0x400000 = +0.5, 0x800000 = -1.0
    d = { 0x00, 0x00, 0x40, 0x00, 0x00, 0x80 };
    w = makeWav (1, 1, 24, 48000, d);
    check (wavfile::parse (w.data(), w.size(), s) && near (s.l[0], 0.5) && near (s.l[1], -1.0), "24-bit mono values");

    // float32 mono: 0.25, NaN (must read as 0)
    float f[2] = { 0.25f, std::numeric_limits<float>::quiet_NaN() };
    d.assign ((uint8_t*) f, (uint8_t*) f + 8);
    w = makeWav (3, 1, 32, 48000, d);
    check (wavfile::parse (w.data(), w.size(), s) && near (s.l[0], 0.25) && s.l[1] == 0.f, "float32 parses, NaN becomes 0");

    // refusals
    w = makeWav (1, 1, 8, 48000, { 1, 2, 3 });
    check (! wavfile::parse (w.data(), w.size(), s), "8-bit is refused");
    w = makeWav (1, 6, 16, 48000, { 0, 0, 0, 0 });
    check (! wavfile::parse (w.data(), w.size(), s), "6 channels is refused");
    check (! wavfile::parse (w.data(), 5, s), "a truncated header is refused");
    const uint8_t junk[16] = { 'N', 'O', 'P', 'E' };
    check (! wavfile::parse (junk, 16, s), "not RIFF is refused");

    // a data chunk that claims more than the file holds is clipped, not read past the end
    d = { 0, 0, 0x00, 0x40 };
    w = makeWav (1, 1, 16, 44100, d, 1000000);
    check (wavfile::parse (w.data(), w.size(), s) && s.size() == 2, "lying chunk size is clipped to the file", s.l.size());
}
