// box_style.h -- the shared "box appearance" bundle every module (except Party/Alliance, the master) uses so a
// box gets the SAME chrome options as Target/Player : frame on/off, transparency, theme (Same-as-Party or own
// procedural/FFXI family + hue + luminosity). One draw helper + one config-row helper -> no per-module dup.
#pragma once
#include "gfx/d3d.h"
#include "model/ui_config.h"    // BoxStyle (defined in model so ui_config can hold one per module)

namespace aio {
struct WindowSkin;

// Draw a themed box at [x,y,w,h]. Resolves Same-as-Party internally. `partySkin` = the shared FFXI skin (f.skin,
// reused for FFXI-family themes). `base` = an extra fade (1 = none). `S` = the box's UI scale (fallback corner).
void draw_themed_box(u32 dev, const WindowSkin* partySkin, float x, float y, float w, float h,
                     const BoxStyle& bs, float base, float S);

} // namespace aio
