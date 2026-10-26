// Voice.h: one drum voice. Sample player -> amp ADSR -> low/high-pass -> compressor -> volume/pan.
// PadCfg is the per-pad sound setting. No Rack dependency.
// Author: Ari Klopfer / A_Development. MIT
#pragma once
#include "Adsr.h"
#include "Comp.h"
#include "Filter.h"
#include "Pattern.h"
#include "Sample.h"

struct PadCfg
{
    float vol = 1.f, pan = 0.f, pitch = 0.f;                     // gain, -1..1, semitones
    float atk = 0.001f, dec = 0.2f, sus = 1.f, rel = 0.05f;      // seconds, level
    float lp = 20000.f, hp = 20.f;                               // Hz
    float thr = 0.f, ratio = 1.f, catk = 5.f, crel = 80.f, makeup = 0.f;   // dB, :1, ms, ms, dB
    bool gateMode = false;                                       // false: one-shot, true: release after one step
    int choke = 0;                                               // 0 = none; pads sharing a number cut each other
};

struct Voice
{
    const Sample* smp = nullptr;   // owned by the module's pad slot; never freed while a voice plays it
    double pos = 0, gateLeft = 0;
    float vel = 0.f;
    bool on = false, released = false;
    Adsr env;
    PadFilter filt;
    Comp comp;

    void trigger (const Sample* s, int v, const PadCfg& c, double stepSec)
    {
        if (! s || s->size() < 2) return;
        smp = s; pos = 0; vel = v / 127.f; on = true; released = false;
        gateLeft = stepSec;
        (void) c;
        env.gate (true);
    }
    void choke() { env.choke(); }

    void process (const PadCfg& c, float sr, float& l, float& r)
    {
        l = r = 0.f;
        if (! on) return;
        env.a = c.atk; env.d = c.dec; env.s = c.sus; env.r = c.rel;
        const double rate = smp->rate / sr * std::pow (2.0, c.pitch / 12.0);   // sample frames per output frame
        const int i = (int) pos;
        if (i >= smp->size() - 1) { on = false; return; }

        // One-shot: start releasing so the tail ends with the sample. Gate: release after one step.
        if (! released)
        {
            const bool end = c.gateMode ? (gateLeft -= 1.0 / sr) <= 0.0 : (smp->size() - 1 - pos) / (rate * sr) <= c.rel + 2.0 / sr;   // 2-frame margin so the fade finishes before the sample runs out
            if (end) { env.gate (false); released = true; }
        }
        const float f = (float) (pos - i);
        l = smp->l[i] + f * (smp->l[i + 1] - smp->l[i]);
        r = smp->r[i] + f * (smp->r[i + 1] - smp->r[i]);
        pos += rate;

        const float e = env.next (1.f / sr) * vel;
        if (! env.active()) { on = false; l = r = 0.f; return; }
        l *= e; r *= e;

        filt.lpHz = c.lp; filt.hpHz = c.hp;
        filt.process (l, r, sr);
        comp.thr = c.thr; comp.ratio = c.ratio; comp.atk = c.catk; comp.rel = c.crel; comp.makeup = c.makeup;
        const float g = comp.gain (l, r, sr) * c.vol;
        l *= g * (c.pan > 0.f ? 1.f - c.pan : 1.f);
        r *= g * (c.pan < 0.f ? 1.f + c.pan : 1.f);
    }
};

// A pad fired: cut every other voice in its choke group (open hat by closed hat).
inline void chokeGroup (Voice* v, const PadCfg* c, int pad)
{
    if (! c[pad].choke) return;
    for (int q = 0; q < PADS; ++q)
        if (q != pad && c[q].choke == c[pad].choke) v[q].choke();
}
