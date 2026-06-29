// font.h -- a GDI-baked bitmap-font ATLAS, drawn as textured quads in D3D8.
//
// On first ensure() it renders the printable ASCII glyphs (antialiased, via GDI) into
// one A8R8G8B8 texture and records each glyph's UV + advance. draw() then emits one
// textured quad per character, tinted by the vertex colour (MODULATE) -> coloured text
// fully inside our D3D8 block, so it composites ON TOP of the widget graphics and we
// control font / size / colour. Shared by the whole HUD (owned by the Hud, handed to
// widgets through Frame). The atlas belongs to its device -> dropped on zoning.
#pragma once
#include "d3d.h"

namespace aio {

class Font {
public:
    void  ensure(u32 dev);                  // build the atlas once (idempotent ; rebuilds if face changed)
    void  on_device_lost() { tex_ = 0; }    // forget the texture (rebuilt next ensure)
    void  dispose();                        // release the texture
    bool  ready() const { return tex_ != 0; }
    void  set_face(const char* face, int weight);   // change the baked GDI face/weight (rebuilds the atlas)

    void  begin(u32 dev);                   // set textured-quad state + bind atlas (before a draw batch)
    // draw `s` with its CELL top-left at (x,y); `size` = em px; `color` = ARGB. Returns advance width.
    // An `outline` with alpha>0 paints a stroke of width `ow` px (8 offset passes) behind the glyphs.
    float draw(u32 dev, float x, float y, const char* s, float size, u32 color, u32 outline = 0, float ow = 0.0f);
    // draw left-aligned at x, VERTICALLY centred on cy (uses the cap/digit ink box).
    float draw_lv(u32 dev, float x, float cy, const char* s, float size, u32 color, u32 outline = 0, float ow = 0.0f);
    // left-aligned at x, vertically centred on the string's REAL ink box (font/size-proof).
    float draw_lc(u32 dev, float x, float cy, const char* s, float size, u32 color, u32 outline = 0, float ow = 0.0f);
    // draw centred BOTH ways on (cx, cy).
    float draw_c(u32 dev, float cx, float cy, const char* s, float size, u32 color, u32 outline = 0, float ow = 0.0f);
    // draw centred on (cx,cy) using the string's REAL INK bounding box (both axes) -> always
    // visually centred regardless of font face / size / bearings. Best for gauge numbers + badge.
    float draw_cc(u32 dev, float cx, float cy, const char* s, float size, u32 color, u32 outline = 0, float ow = 0.0f);
    float measure(const char* s, float size) const;   // pixel width of `s` at `size`

private:
    void build(u32 dev);
    void glyphs(u32 dev, float x, float y, const char* s, float scale, u32 color);

    u32   tex_ = 0;
    bool  dirty_ = false;                   // face/weight changed -> rebuild atlas on next ensure
    char  face_[64] = "Segoe UI";           // GDI face name (configurable, global)
    int   weight_ = 600;                    // FW_SEMIBOLD
    int   aw_ = 0, ah_ = 0;
    float base_ = 32.0f;                    // baked em (px)
    float cap_top_ = 0.0f, cap_h_ = 0.0f;   // ink box of caps/digits (offset from cell top, height), base px
    struct G { float u0, v0, u1, v1; float w, h, adv; float il, ir, it, ib; };  // il..ib = ink bbox in base px (rel. cell top-left)
    G g_[96] = {};                          // ASCII 32..127 (zero-init : an unbaked glyph is invisible, never garbage)
};

// Cache of Font atlases keyed by (face, weight). Lets different text use different faces
// and weights (bold) -- each unique (face,weight) combo gets its own baked atlas, built
// lazily. One is the global default (set from layout.json). Owned by the Hud.
class FontManager {
public:
    void  set_default(const char* face, int weight);
    Font* get(const char* face, int weight);   // face ""/null = default ; weight 0 = default
    void  ensure_all(u32 dev);
    void  on_device_lost();
    void  dispose();
private:
    static const int MAXF = 8;
    Font  f_[MAXF];
    char  face_[MAXF][64];
    int   wt_[MAXF];
    int   n_ = 0;
    char  defFace_[64] = "Segoe UI";
    int   defWeight_ = 600;
};

} // namespace aio
