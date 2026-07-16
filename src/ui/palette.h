// palette.h -- the liquid colour schemes for HP / MP / TP and their level logic.
// Colours are packed 0xAARRGGBB. Each palette is {body, mid, bright-accent}.
#pragma once
#include "gfx/d3d.h"

namespace aio {

// base palettes (the bar loop uses these as the per-resource default tint).
extern const u32 PAL_HP[3];   // emerald -> lime
extern const u32 PAL_MP[3];   // magical cyan
extern const u32 PAL_TP[3];   // violet -> magenta

// TP weaponskill tier from fill (0..1): 0 = <1000, 1 = 1000, 2 = 2000, 3 = 3000.
int tp_tier(float fill);

// pick the HP palette for a fill: green >75%, orange 25-75% (undead aggro), red <=25%.
void hp_palette(float fill, u32 out[3]);

// CONTINUOUS TP palette: interpolate through the 4 tier anchors as TP charges.
void tp_palette(float fill, u32 out[3]);

} // namespace aio
