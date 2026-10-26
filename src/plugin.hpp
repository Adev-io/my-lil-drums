// plugin.hpp: the plugin's model and the house light colours.
// Author: Ari Klopfer / A_Development · MIT
#pragma once
#include <rack.hpp>
using namespace rack;
extern Plugin* pluginInstance;
extern Model* modelDrums;

// House light colours, the same as the panel (tools/panel.py), My lil Looper and My lil Acid Bass.
struct LimeLight : GrayModuleLightWidget { LimeLight() { addBaseColor (nvgRGB (0xc8, 0xff, 0x3a)); } };
