// window.h -- FFXI native window skin (9-slice), composed from the game's menu textures.
//
// The game builds EVERY window from 4 DXT1 textures per theme : a tiled background, a
// horizontal edge band (top/bottom), a vertical edge band (left/right) and a corner sheet
// (its 4 quadrants = the 4 corners). They are grayscale and TINTED by the chosen window
// colour. We convert them to straight-alpha BGRA .raw (scripts/gen_window_skin.sh) and
// re-compose them here as a 9-slice so our HUD boxes match the game's window chrome.
#pragma once
#include "gfx/d3d.h"   // u32

namespace aio {

struct WindowSkin {
    u32 corner = 0, hframe = 0, vframe = 0, bg = 0;   // the 4 skin textures (BGRA)
    u32 borderColor = 0xFFFFFFFF;                     // per-theme border colour, derived from the bg at load
    bool ready() const { return corner && hframe && vframe && bg; }
    bool load(u32 dev, const char* themeName);   // assets/window/<themeName>/{corner,hframe,vframe,bg}.raw
    void on_device_lost();              // forget handles (zoning) -> reload on next load()
    void dispose();                     // release textures (unload)
};

// the built-in theme list (folder names under assets/window), for //aio menu N.
int         window_theme_count();
const char* window_theme_name(int idx);      // 0-based ; null if out of range

// 9-slice the skin into [x,y,w,h] : tiled native bg + 4 edges + 4 corners, MODULATEd by `tint`
// (ARGB ; 0xFFFFFFFF = texture as-is). `scale` is ignored (textures drawn 1:1, tiled). The caller
// should pixel-snap x/y/w/h. `openBottom` : omit the bottom edge + bottom corners (the side edges
// run to the bottom) -> the box reads as joined to whatever sits flush below it.
// `drawBorder` false : paint ONLY the tiled background (skip the frame edges + corners) -> a borderless
// box that keeps its skin background (the per-box "Borders off" toggle).
void draw_window(u32 dev, const WindowSkin& s, float x, float y, float w, float h, u32 tint, float scale, bool openBottom = false, bool drawBorder = true);

} // namespace aio
