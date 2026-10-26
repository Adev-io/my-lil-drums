// Voice.h: one drum voice, a sample player with linear interpolation. No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include "Sample.h"

struct Voice
{
    const Sample* smp = nullptr;   // owned by the module's pad slot; never freed while a voice plays it
    double pos = 0;
    float gain = 0;
    bool on = false;

    void trigger (const Sample* s, int vel)
    {
        if (! s || s->size() < 2) return;
        smp = s; pos = 0; gain = vel / 127.f; on = true;
    }

    void process (float sr, float& l, float& r)
    {
        l = r = 0.f;
        if (! on) return;
        const int i = (int) pos;
        if (i >= smp->size() - 1) { on = false; return; }
        const float f = (float) (pos - i);
        l = gain * (smp->l[i] + f * (smp->l[i + 1] - smp->l[i]));
        r = gain * (smp->r[i] + f * (smp->r[i + 1] - smp->r[i]));
        pos += smp->rate / sr;
    }
};
