// hud_internal.h -- shared HUD-renderer helpers used by hud.cpp and its per-module split TUs
// (hud_skillchains.cpp / hud_treasure.cpp / hud_hatelist.cpp / hud_pointwatch.cpp / hud_grimoire.cpp /
// hud_zonetracker.cpp / hud_timers.cpp). Not public. The DEFINITIONS live in hud.cpp.
#pragma once
#include "ui/widget.h"     // Frame
#include "ui/edit_box.h"   // EditBox
#include "gfx/d3d.h"       // u32

namespace aio {

// Shared edit-mode drag for a module box : drag + snap-grid, write the new fractional origin back to
// (cfgX,cfgY), persist on drop. centerX = the stored X is the box CENTRE (else its left edge). px/py updated.
// Used by ALL seven module renderers.
void box_edit(const Frame& f, EditBox& eb, int editId, float& px, float& py, float boxW, float boxH,
              float scale, float& cfgX, float& cfgY, bool centerX);

// draw an atlas sub-cell [u0..u1]x[v0..v1] at (x,y,w,h) -- Sheol weapon strip (v 0..1) or the 2D buff atlas.
// Used by draw_zonetracker (weapon-type strip) and draw_timers (buff status atlas). Defaults on this DECLARATION only.
void draw_icon_cell(u32 dev, u32 tex, float x, float y, float w, float h, float u0, float u1, float v0 = 0.0f, float v1 = 1.0f);

}
