// Drums.cpp: the panel widget and the model.
// Author: Ari Klopfer / A_Development · MIT
#include "Drums.hpp"
#include "TalkingFace.hpp"
#include "Widgets.hpp"
#include "layout.hpp"
#include "Beats.h"
#include <osdialog.h>

struct DrumsWidget : ModuleWidget
{
    DrumsWidget (Drums* module)
    {
        setModule (module);
        setPanel (createPanel (asset::plugin (pluginInstance, "res/Drums.svg")));
        namespace L = layout::drums;
        {
            TalkingFace* f = createWidget<TalkingFace> (mm2px (Vec (L::FACE[0] - L::FACE[2], L::FACE[1] - L::FACE[2])));
            f->box.size = mm2px (Vec (2 * L::FACE[2], 2 * L::FACE[2]));
            if (module) f->tap = &module->face;
            addChild (f);
        }
        addChild (createWidget<ScrewBlack> (Vec (RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewBlack> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewBlack> (Vec (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild (createWidget<ScrewBlack> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        auto region = [&] (Widget* w, const float* r) {
            w->box.pos = mm2px (Vec (r[0], r[1]));
            w->box.size = mm2px (Vec (r[2], r[3]));
            addChild (w);
        };
        Roll* roll = new Roll;       roll->m = module;  region (roll, L::ROLL);
        Tabs* tabs = new Tabs;       tabs->m = module;  region (tabs, L::TABS);
        VelLane* vel = new VelLane;  vel->m = module;   region (vel, L::VEL);
        PadGrid* grid = new PadGrid; grid->m = module;  region (grid, L::PADGRID);

        auto at = [] (const float* mm) { return mm2px (Vec (mm[0], mm[1])); };
        addParam (createParamCentered<Rogan1PSWhite> (at (L::BPM_PARAM), module, Drums::BPM_PARAM));
        addParam (createParamCentered<Rogan1PSWhite> (at (L::SWING_PARAM), module, Drums::SWING_PARAM));
        addParam (createParamCentered<Rogan1PSWhite> (at (L::LENGTH_PARAM), module, Drums::LENGTH_PARAM));
        addParam (createParamCentered<Rogan1PSWhite> (at (L::PATTERN_PARAM), module, Drums::PATTERN_PARAM));
        addParam (createParamCentered<Rogan1PSWhite> (at (L::LEVEL_PARAM), module, Drums::LEVEL_PARAM));
        addParam (createLightParamCentered<VCVLightLatch<MediumSimpleLight<LimeLight>>> (at (L::RUN_PARAM), module, Drums::RUN_PARAM, Drums::RUN_LIGHT));
        addInput (createInputCentered<PJ301MPort> (at (L::CLOCK_INPUT), module, Drums::CLOCK_INPUT));
        addInput (createInputCentered<PJ301MPort> (at (L::RESET_INPUT), module, Drums::RESET_INPUT));
        addInput (createInputCentered<PJ301MPort> (at (L::RUN_INPUT), module, Drums::RUN_INPUT));
        addOutput (createOutputCentered<PJ301MPort> (at (L::OUT_L_OUTPUT), module, Drums::OUT_L_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (at (L::OUT_R_OUTPUT), module, Drums::OUT_R_OUTPUT));
        for (int p = 0; p < PADS; ++p)
            addOutput (createOutputCentered<PJ301MPort> (at (L::PADOUT[p]), module, Drums::PAD_OUTPUT + p));

        static const float* const edit[E_N] = { L::VOL_PARAM, L::PAN_PARAM, L::PITCH_PARAM, L::ATK_PARAM, L::DEC_PARAM, L::SUS_PARAM, L::REL_PARAM,
            L::MODE_PARAM, L::LP_PARAM, L::HP_PARAM, L::THR_PARAM, L::RATIO_PARAM, L::CATK_PARAM, L::CREL_PARAM, L::MAKEUP_PARAM, L::CHOKE_PARAM };
        for (int i = 0; i < E_N; ++i)
        {
            if (i == E_MODE)
                addParam (createLightParamCentered<VCVLightLatch<MediumSimpleLight<LimeLight>>> (at (edit[i]), module, Drums::EDIT_PARAM + i, Drums::MODE_LIGHT));
            else
                addParam (createParamCentered<Rogan1PSWhite> (at (edit[i]), module, Drums::EDIT_PARAM + i));
        }
    }

    void appendContextMenu (Menu* menu) override
    {
        Drums* m = dynamic_cast<Drums*> (module);
        if (! m) return;
        menu->addChild (new MenuSeparator);
        menu->addChild (createMenuItem ("Load WAV into the selected pad...", "", [=] {
            osdialog_filters* filters = osdialog_filters_parse ("WAV:wav");
            char* path = osdialog_file (OSDIALOG_OPEN, nullptr, nullptr, filters);
            osdialog_filters_free (filters);
            if (path) { m->loadSample (m->sel.load(), path); std::free (path); }
        }));
        menu->addChild (createSubmenuItem ("Kits", "", [=] (Menu* sub) {
            sub->addChild (createMenuItem ("Save kit as...", "", [=] {
                system::createDirectories (m->kitDir());
                osdialog_filters* filters = osdialog_filters_parse ("Kit:json");
                char* path = osdialog_file (OSDIALOG_SAVE, m->kitDir().c_str(), "kit.json", filters);
                osdialog_filters_free (filters);
                if (path)
                {
                    std::string p = path;
                    std::free (path);
                    if (system::getExtension (p) != ".json") p += ".json";
                    m->saveKit (p);
                }
            }));
            sub->addChild (new MenuSeparator);
            for (const std::string& f : system::getEntries (m->kitDir()))
                if (system::getExtension (f) == ".json")
                    sub->addChild (createMenuItem (system::getStem (f), "", [=] { m->loadKit (f); }));
        }));
        menu->addChild (createMenuItem ("Clear the selected pad's sample", "", [=] { m->loadSample (m->sel.load(), ""); }));
        menu->addChild (createSubmenuItem ("Factory beat into this pattern", "", [=] (Menu* sub) {
            for (int i = 0; i < NUM_BEATS; ++i)
                sub->addChild (createMenuItem (BEATS[i].name, "", [=] {
                    applyBeat (m->seq.pat[m->seq.cur], BEATS[i]);
                    m->seq.setLength (16);
                    m->params[Drums::LENGTH_PARAM].setValue (16.f);
                }));
        }));
        menu->addChild (createMenuItem ("Clear the current pattern", "", [=] { m->seq.pat[m->seq.cur].clear(); }));
    }
};

Model* modelDrums = createModel<Drums, DrumsWidget> ("Drums");
