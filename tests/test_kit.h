// test_kit.h: the bundled starter kit must parse with our own parser.
#pragma once
#include "check.h"
#include "Sample.h"
#include <cstdio>

inline void testKit()
{
    const char* names[8] = { "kick", "snare", "chat", "ohat", "clap", "rim", "tom", "crash" };
    for (int i = 0; i < 8; ++i)
    {
        std::string path = std::string ("../res/kit/") + names[i] + ".wav";
        std::vector<uint8_t> buf;
        if (FILE* f = std::fopen (path.c_str(), "rb"))
        {
            uint8_t tmp[4096]; size_t k;
            while ((k = std::fread (tmp, 1, sizeof tmp, f)) > 0) buf.insert (buf.end(), tmp, tmp + k);
            std::fclose (f);
        }
        Sample s;
        const bool ok = wavfile::parse (buf.data(), buf.size(), s);
        float peak = 0;
        for (float x : s.l) peak = std::max (peak, std::fabs (x));
        check (ok && s.size() > 1000 && peak > 0.3f && peak <= 1.f, names[i], peak);
    }
}
