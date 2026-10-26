// Drums.hpp: the My lil Drums module. Phase 1: clock, pattern, 16 sample pads, stereo mix.
// Author: Ari Klopfer / A_Development · MIT
#pragma once
#include "plugin.hpp"
#include "FaceTap.h"
#include "Sample.h"
#include "Sequencer.h"
#include "Voice.h"
#include <atomic>

// The pad editor's knobs, in panel order. Declared now, wired in Phase 2.
enum Edit { E_VOL, E_PAN, E_PITCH, E_ATK, E_DEC, E_SUS, E_REL, E_MODE, E_LP, E_HP, E_THR, E_RATIO, E_CATK, E_CREL, E_MAKEUP, E_CHOKE, E_N };

struct PadSlot
{
    Sample* smp = nullptr;                    // the audio thread owns this one
    std::atomic<Sample*> incoming { nullptr }; // the UI thread hands a new sample over here
    std::string path;                         // UI thread only; saved in the patch
};

struct Drums : Module
{
    enum ParamId { BPM_PARAM, SWING_PARAM, LENGTH_PARAM, PATTERN_PARAM, RUN_PARAM, LEVEL_PARAM, EDIT_PARAM, PARAMS_LEN = EDIT_PARAM + E_N };
    enum InputId { CLOCK_INPUT, RESET_INPUT, RUN_INPUT, INPUTS_LEN };
    enum OutputId { OUT_L_OUTPUT, OUT_R_OUTPUT, PAD_OUTPUT, OUTPUTS_LEN = PAD_OUTPUT + PADS };
    enum LightId { RUN_LIGHT, MODE_LIGHT, LIGHTS_LEN };

    Sequencer seq;
    Voice voice[PADS];
    PadSlot slot[PADS];
    std::atomic<bool> dirty { false };        // a slot has a new incoming sample
    std::atomic<int> sel { 0 }, page { 0 };   // selected pad, roll page (UI sets, audio reads)
    std::atomic<bool> audition[PADS];
    float flash[PADS] = {};                   // pad light, 1 -> 0 after a hit
    FaceTap face;
    dsp::SchmittTrigger clockTrig, resetTrig;
    bool wasRun = false;
    PadCfg cfg[PADS];            // the saved truth for every pad
    int lastSel = -1;            // -1 forces the knobs to load from cfg on the first tick
    double stepSec = 0.125, sinceFire = 0;   // length of the last step, for gate mode
    bool haveFire = false;

    Drums()
    {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam (BPM_PARAM, 30.f, 300.f, 120.f, "Tempo", " BPM");
        configParam (SWING_PARAM, 0.f, 0.5f, 0.f, "Swing", "%", 0.f, 200.f);
        configParam (LENGTH_PARAM, 1.f, 64.f, 16.f, "Steps in this pattern")->snapEnabled = true;
        configParam (PATTERN_PARAM, 0.f, 15.f, 0.f, "Pattern", "", 0.f, 1.f, 1.f)->snapEnabled = true;
        configSwitch (RUN_PARAM, 0.f, 1.f, 1.f, "Run", { "Stopped", "Running" });
        configParam (LEVEL_PARAM, 0.f, 2.f, 1.f, "Mix level", "%", 0.f, 100.f);
        configInput (CLOCK_INPUT, "Clock (one pulse per step)");
        configInput (RESET_INPUT, "Reset");
        configInput (RUN_INPUT, "Run gate");
        configOutput (OUT_L_OUTPUT, "Mix left");
        configOutput (OUT_R_OUTPUT, "Mix right");
        for (int p = 0; p < PADS; ++p)
        {
            audition[p] = false;
            configOutput (PAD_OUTPUT + p, "Pad " + std::to_string (p + 1));
        }
        const int E = EDIT_PARAM;
        configParam (E + E_VOL, 0.f, 2.f, 1.f, "Pad volume", "%", 0.f, 100.f);
        configParam (E + E_PAN, -1.f, 1.f, 0.f, "Pad pan");
        configParam (E + E_PITCH, -12.f, 12.f, 0.f, "Pad pitch", " st");
        configParam (E + E_ATK, 0.0001f, 1.f, 0.001f, "Attack", " ms", 0.f, 1000.f);
        configParam (E + E_DEC, 0.001f, 2.f, 0.2f, "Decay", " ms", 0.f, 1000.f);
        configParam (E + E_SUS, 0.f, 1.f, 1.f, "Sustain", "%", 0.f, 100.f);
        configParam (E + E_REL, 0.001f, 2.f, 0.05f, "Release", " ms", 0.f, 1000.f);
        configSwitch (E + E_MODE, 0.f, 1.f, 0.f, "Pad mode", { "One-shot", "Gate (one step)" });
        configParam (E + E_LP, std::log2 (20.f), std::log2 (20000.f), std::log2 (20000.f), "Low-pass", " Hz", 2.f);
        configParam (E + E_HP, std::log2 (20.f), std::log2 (20000.f), std::log2 (20.f), "High-pass", " Hz", 2.f);
        configParam (E + E_THR, -60.f, 0.f, 0.f, "Compressor threshold", " dB");
        configParam (E + E_RATIO, 1.f, 20.f, 1.f, "Compressor ratio", ":1");
        configParam (E + E_CATK, 0.1f, 100.f, 5.f, "Compressor attack", " ms");
        configParam (E + E_CREL, 10.f, 1000.f, 80.f, "Compressor release", " ms");
        configParam (E + E_MAKEUP, 0.f, 24.f, 0.f, "Compressor makeup", " dB");
        configParam (E + E_CHOKE, 0.f, 8.f, 0.f, "Choke group (0 = none)")->snapEnabled = true;
        loadStarterKit();
    }

    ~Drums()
    {
        for (int p = 0; p < PADS; ++p) { delete slot[p].smp; delete slot[p].incoming.load(); }
    }

    // ---- samples (UI thread) ----
    // An empty Sample means "silent pad"; the audio thread only ever sees whole samples.
    bool loadSample (int p, const std::string& path)
    {
        Sample* s = new Sample;
        bool ok = false;
        if (! path.empty())
        {
            try { std::vector<uint8_t> d = system::readFile (path); ok = wavfile::parse (d.data(), d.size(), *s); }
            catch (const std::exception&) {}
        }
        if (! ok) *s = Sample();
        slot[p].path = ok ? path : "";
        delete slot[p].incoming.exchange (s);   // a previous unadopted sample is dropped
        dirty = true;
        return ok;
    }
    void loadStarterKit()
    {
        static const char* names[8] = { "kick", "snare", "chat", "ohat", "clap", "rim", "tom", "crash" };
        for (int p = 0; p < 8; ++p) loadSample (p, asset::plugin (pluginInstance, std::string ("res/kit/") + names[p] + ".wav"));
    }
    std::string padName (int p) const { return slot[p].path.empty() ? "-" : system::getStem (slot[p].path); }

    // ---- audio thread ----
    void adopt()
    {
        for (int p = 0; p < PADS; ++p)
        {
            Sample* s = slot[p].incoming.exchange (nullptr);
            if (! s) continue;
            voice[p].on = false;   // never free a sample a voice is reading
            delete slot[p].smp;    // ponytail: frees on the audio thread; fine for a user-triggered load
            slot[p].smp = s;
        }
    }

    void loadEdit (int p)
    {
        const PadCfg& c = cfg[p];
        const int E = EDIT_PARAM;
        const float v[E_N] = { c.vol, c.pan, c.pitch, c.atk, c.dec, c.sus, c.rel, c.gateMode ? 1.f : 0.f,
                               std::log2 (c.lp), std::log2 (c.hp), c.thr, c.ratio, c.catk, c.crel, c.makeup, (float) c.choke };
        for (int i = 0; i < E_N; ++i) params[E + i].setValue (v[i]);
    }
    void storeEdit (int p)
    {
        PadCfg& c = cfg[p];
        const int E = EDIT_PARAM;
        auto g = [&] (int i) { return params[E + i].getValue(); };
        c.vol = g (E_VOL); c.pan = g (E_PAN); c.pitch = g (E_PITCH);
        c.atk = g (E_ATK); c.dec = g (E_DEC); c.sus = g (E_SUS); c.rel = g (E_REL);
        c.gateMode = g (E_MODE) > 0.5f;
        c.lp = std::exp2 (g (E_LP)); c.hp = std::exp2 (g (E_HP));
        c.thr = g (E_THR); c.ratio = g (E_RATIO); c.catk = g (E_CATK); c.crel = g (E_CREL); c.makeup = g (E_MAKEUP);
        c.choke = (int) std::round (g (E_CHOKE));
    }

    void hitPad (int p, int vel)
    {
        chokeGroup (voice, cfg, p);
        voice[p].trigger (slot[p].smp, vel, cfg[p], stepSec);
        flash[p] = 1.f;
    }

    void process (const ProcessArgs& a) override
    {
        if (dirty.exchange (false)) adopt();
        const int s = sel.load();
        if (s != lastSel) { loadEdit (s); lastSel = s; }
        else storeEdit (s);
        lights[MODE_LIGHT].setBrightness (cfg[s].gateMode ? 1.f : 0.f);
        sinceFire += a.sampleTime;

        const int pi = (int) std::round (params[PATTERN_PARAM].getValue());
        if (pi != seq.cur) { seq.select (pi); params[LENGTH_PARAM].setValue ((float) seq.pat[pi].length); }
        else seq.setLength ((int) std::round (params[LENGTH_PARAM].getValue()));
        seq.clk.bpm = params[BPM_PARAM].getValue();
        seq.clk.swing = params[SWING_PARAM].getValue();

        const bool run = params[RUN_PARAM].getValue() > 0.5f && (! inputs[RUN_INPUT].isConnected() || inputs[RUN_INPUT].getVoltage() > 1.f);
        if (run && ! wasRun) seq.clk.reset();
        wasRun = run;
        lights[RUN_LIGHT].setBrightness (run ? 1.f : 0.f);
        if (resetTrig.process (inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f)) seq.clk.reset();

        bool fired = false;
        if (inputs[CLOCK_INPUT].isConnected())
        {
            if (clockTrig.process (inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f) && run) fired = seq.clk.pulse();
        }
        else if (run) fired = seq.clk.tick (a.sampleTime);
        if (fired)
        {
            if (haveFire) stepSec = sinceFire;   // measured, so it follows an external clock and swing
            haveFire = true;
            sinceFire = 0;
            seq.fire();
            for (int p = 0; p < PADS; ++p) if (seq.hit[p]) hitPad (p, seq.hit[p]);
        }
        for (int p = 0; p < PADS; ++p) if (audition[p].exchange (false)) hitPad (p, 100);

        float l = 0.f, r = 0.f;
        for (int p = 0; p < PADS; ++p)
        {
            float vl, vr;
            voice[p].process (cfg[p], a.sampleRate, vl, vr);
            l += vl; r += vr;
            flash[p] = std::max (0.f, flash[p] - 6.f * a.sampleTime);
        }
        const float g = 5.f * params[LEVEL_PARAM].getValue();   // +-1 sample = +-5 V
        outputs[OUT_L_OUTPUT].setVoltage (l * g);
        outputs[OUT_R_OUTPUT].setVoltage (r * g);
        face.push (clamp ((l + r) * 0.5f, -1.f, 1.f), a.sampleRate);
    }

    // ---- patch file ----
    // Sample paths under the plugin folder are saved as "@plugin/..." so the patch survives a plugin move.
    std::string packPath (const std::string& p) const
    {
        const std::string root = asset::plugin (pluginInstance, "");
        return p.compare (0, root.size(), root) == 0 ? "@plugin/" + p.substr (root.size()) : p;
    }
    std::string unpackPath (const std::string& p) const
    {
        return p.compare (0, 8, "@plugin/") == 0 ? asset::plugin (pluginInstance, p.substr (8)) : p;
    }

    json_t* padsToJson() const
    {
        json_t* pads = json_array();
        for (int p = 0; p < PADS; ++p)
        {
            const PadCfg& c = cfg[p];
            json_t* o = json_object();
            const std::pair<const char*, float> kv[] = { { "vol", c.vol }, { "pan", c.pan }, { "pitch", c.pitch }, { "atk", c.atk }, { "dec", c.dec },
                { "sus", c.sus }, { "rel", c.rel }, { "lp", c.lp }, { "hp", c.hp }, { "thr", c.thr }, { "ratio", c.ratio }, { "catk", c.catk },
                { "crel", c.crel }, { "makeup", c.makeup }, { "gate", c.gateMode ? 1.f : 0.f }, { "choke", (float) c.choke } };
            for (const auto& e : kv) json_object_set_new (o, e.first, json_real (e.second));
            json_array_append_new (pads, o);
        }
        return pads;
    }
    void padsFromJson (json_t* pads)
    {
        for (int p = 0; p < PADS && json_is_array (pads) && p < (int) json_array_size (pads); ++p)
        {
            json_t* o = json_array_get (pads, p);
            PadCfg& c = cfg[p];
            auto num = [&] (const char* k, float& dst) { if (json_t* v = json_object_get (o, k)) if (json_is_number (v)) dst = (float) json_number_value (v); };
            float gate = c.gateMode ? 1.f : 0.f, choke = (float) c.choke;
            num ("vol", c.vol); num ("pan", c.pan); num ("pitch", c.pitch); num ("atk", c.atk); num ("dec", c.dec); num ("sus", c.sus);
            num ("rel", c.rel); num ("lp", c.lp); num ("hp", c.hp); num ("thr", c.thr); num ("ratio", c.ratio); num ("catk", c.catk);
            num ("crel", c.crel); num ("makeup", c.makeup); num ("gate", gate); num ("choke", choke);
            c.gateMode = gate > 0.5f;
            c.choke = (int) clamp (choke, 0.f, 8.f);
        }
        lastSel = -1;
    }
    json_t* samplesToJson() const
    {
        json_t* smp = json_array();
        for (int p = 0; p < PADS; ++p) json_array_append_new (smp, json_string (packPath (slot[p].path).c_str()));
        return smp;
    }
    void samplesFromJson (json_t* smp)
    {
        for (int p = 0; p < PADS && json_is_array (smp) && p < (int) json_array_size (smp); ++p)
            if (json_t* s = json_array_get (smp, p))
                if (json_is_string (s)) loadSample (p, unpackPath (json_string_value (s)));
    }
    std::string kitDir() const { return asset::user ("MyLilDrums/kits"); }
    bool saveKit (const std::string& path)
    {
        json_t* j = json_object();
        json_object_set_new (j, "samples", samplesToJson());
        json_object_set_new (j, "pads", padsToJson());
        FILE* f = std::fopen (path.c_str(), "w");   // ponytail: ANSI path; Rack's own dialogs hand back ANSI-safe paths in practice
        const bool ok = f && json_dumpf (j, f, JSON_INDENT (1)) == 0;
        if (f) std::fclose (f);
        json_decref (j);
        return ok;
    }
    bool loadKit (const std::string& path)
    {
        json_error_t err;
        json_t* j = json_load_file (path.c_str(), 0, &err);
        if (! j) return false;
        samplesFromJson (json_object_get (j, "samples"));
        padsFromJson (json_object_get (j, "pads"));
        json_decref (j);
        return true;
    }

    json_t* dataToJson() override
    {
        json_t* j = json_object();
        json_t* pats = json_array();
        for (int i = 0; i < PATTERNS; ++i)
        {
            json_t* o = json_object();
            json_object_set_new (o, "length", json_integer (seq.pat[i].length));
            json_t* rows = json_array();
            for (int p = 0; p < PADS; ++p) json_array_append_new (rows, json_string (rowToHex (seq.pat[i].vel[p]).c_str()));
            json_object_set_new (o, "rows", rows);
            json_array_append_new (pats, o);
        }
        json_object_set_new (j, "patterns", pats);
        json_object_set_new (j, "samples", samplesToJson());
        json_object_set_new (j, "pads", padsToJson());
        return j;
    }

    void dataFromJson (json_t* j) override
    {
        json_t* pats = json_object_get (j, "patterns");
        for (int i = 0; i < PATTERNS && json_is_array (pats) && i < (int) json_array_size (pats); ++i)
        {
            json_t* o = json_array_get (pats, i);
            if (json_t* len = json_object_get (o, "length"))
                if (json_is_integer (len)) seq.pat[i].length = (int) clamp ((int) json_integer_value (len), 1, MAXSTEPS);
            json_t* rows = json_object_get (o, "rows");
            for (int p = 0; p < PADS && json_is_array (rows) && p < (int) json_array_size (rows); ++p)
                if (json_t* row = json_array_get (rows, p))
                    if (json_is_string (row)) rowFromHex (json_string_value (row), seq.pat[i].vel[p]);
        }
        seq.clk.length = seq.pat[seq.cur].length;
        samplesFromJson (json_object_get (j, "samples"));
        padsFromJson (json_object_get (j, "pads"));   // also forces the knobs to reload from cfg
    }
};
