// draw.cpp -- see draw.h.
#include "draw.h"
#include "color.h"   // lerp_argb (one implementation, shared)
#include <math.h>

namespace aio {

void tquad(u32 dev, float x, float y, float w, float h,
           float u0, float u1, float v0, float v1, u32 cL, u32 cR)
{
    x -= 0.5f; y -= 0.5f;                            // D3D half-texel rule: align texels to pixels
    Vtx q[4] = {
        { x,     y,     0, 1, cL, u0, v0 },
        { x + w, y,     0, 1, cR, u1, v0 },
        { x,     y + h, 0, 1, cL, u0, v1 },
        { x + w, y + h, 0, 1, cR, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// full-texture quad of size w x h, ROTATED `ang` radians about its centre (cx,cy), tinted `col`. For
// directional markers (rotate an up/right-pointing icon by the entity heading + tint via MODULATE).
void tquad_rot(u32 dev, float cx, float cy, float w, float h, float ang, u32 col)
{
    const float ca = cosf(ang), sa = sinf(ang), hx = w * 0.5f, hy = h * 0.5f;
    #define RX(lx,ly) (cx + (lx)*ca - (ly)*sa - 0.5f)
    #define RY(lx,ly) (cy + (lx)*sa + (ly)*ca - 0.5f)
    Vtx q[4] = {
        { RX(-hx,-hy), RY(-hx,-hy), 0, 1, col, 0.0f, 0.0f },
        { RX( hx,-hy), RY( hx,-hy), 0, 1, col, 1.0f, 0.0f },
        { RX(-hx, hy), RY(-hx, hy), 0, 1, col, 0.0f, 1.0f },
        { RX( hx, hy), RY( hx, hy), 0, 1, col, 1.0f, 1.0f },
    };
    #undef RX
    #undef RY
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// filled TEXTURED circle (triangle fan), centred at (cx,cy) radius r, tinted `col` (MODULATE). Each vertex's
// UV is the linear map (u0 + (vx-cx)*du, v0 + (vy-cy)*dv) -> the caller supplies the screen->UV mapping so a
// map quad can be drawn as a GENUINE circle (round minimap) with NO stencil buffer. Centre UV = (u0,v0).
void tdisc(u32 dev, float cx, float cy, float r, float u0, float v0, float du, float dv, u32 col)
{
    const int MAXN = 160;
    int N = (int)(r * 1.3f); if (N < 48) N = 48; if (N > MAXN) N = MAXN;   // segments scale with radius -> a big radar lens stays perfectly round (no facets at the brass edge)
    const float fth = 1.3f;                             // feathered outer rim width -> anti-aliased circular edge
    Vtx fan[MAXN + 2];
    fan[0] = { cx - 0.5f, cy - 0.5f, 0, 1, col, u0, v0 };
    for (int i = 0; i <= N; ++i) {
        const float a = (float)i / (float)N * 6.28318530f;
        const float vx = cx + r * cosf(a), vy = cy + r * sinf(a);
        fan[i + 1] = { vx - 0.5f, vy - 0.5f, 0, 1, col, u0 + (vx - cx) * du, v0 + (vy - cy) * dv };
    }
    dDrawUP(dev, D3DPT_TRIANGLEFAN, N, fan, sizeof(Vtx));
    // feathered rim : a ring from r (full alpha) to r+fth (alpha 0) so the circular edge fades cleanly (AA)
    // into whatever sits beneath (the minimap's border ring) instead of a hard polygonal edge.
    const u32 col0 = col & 0x00FFFFFFu;
    Vtx ring[(MAXN + 1) * 2];
    for (int i = 0; i <= N; ++i) {
        const float a = (float)i / (float)N * 6.28318530f, ca = cosf(a), sa = sinf(a);
        const float ix = cx + r * ca, iy = cy + r * sa, ox = cx + (r + fth) * ca, oy = cy + (r + fth) * sa;
        ring[i * 2 + 0] = { ix - 0.5f, iy - 0.5f, 0, 1, col,  u0 + (ix - cx) * du, v0 + (iy - cy) * dv };
        ring[i * 2 + 1] = { ox - 0.5f, oy - 0.5f, 0, 1, col0, u0 + (ox - cx) * du, v0 + (oy - cy) * dv };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, N * 2, ring, sizeof(Vtx));
}

void tquad_v(u32 dev, float x, float y, float w, float h,
             float u0, float u1, float v0, float v1, u32 cTop, u32 cBot)
{
    x -= 0.5f; y -= 0.5f;
    Vtx q[4] = {
        { x,     y,     0, 1, cTop, u0, v0 },
        { x + w, y,     0, 1, cTop, u1, v0 },
        { x,     y + h, 0, 1, cBot, u0, v1 },
        { x + w, y + h, 0, 1, cBot, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

void tquad4(u32 dev, float x, float y, float w, float h,
            float u0, float u1, float v0, float v1,
            u32 cTL, u32 cTR, u32 cBL, u32 cBR)
{
    x -= 0.5f; y -= 0.5f;
    Vtx q[4] = {
        { x,     y,     0, 1, cTL, u0, v0 },
        { x + w, y,     0, 1, cTR, u1, v0 },
        { x,     y + h, 0, 1, cBL, u0, v1 },
        { x + w, y + h, 0, 1, cBR, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

void glow_quad(u32 dev, float x, float y, float w, float h, u32 color)
{
    x -= 0.5f; y -= 0.5f;
    Vtx q[4] = {
        { x,     y,     0, 1, color, 0.0f, 0.0f },
        { x + w, y,     0, 1, color, 1.0f, 0.0f },
        { x,     y + h, 0, 1, color, 0.0f, 1.0f },
        { x + w, y + h, 0, 1, color, 1.0f, 1.0f },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

void cap_quad(u32 dev, float x, float y, float w, float h, bool flip)
{
    x -= 0.5f; y -= 0.5f;
    float u0 = flip ? 1.0f : 0.0f, u1 = flip ? 0.0f : 1.0f;
    Vtx q[4] = {
        { x,     y,     0, 1, 0xFFFFFFFF, u0, 0.0f },
        { x + w, y,     0, 1, 0xFFFFFFFF, u1, 0.0f },
        { x,     y + h, 0, 1, 0xFFFFFFFF, u0, 1.0f },
        { x + w, y + h, 0, 1, 0xFFFFFFFF, u1, 1.0f },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

void grad_quad(u32 dev, float x, float y, float w, float h, u32 cTL, u32 cTR, u32 cBL, u32 cBR)
{
    x -= 0.5f; y -= 0.5f;                            // D3D half-pixel rule (avoids dropped bottom/right row)
    VtxC q[4] = {
        { x,     y,     0, 1, cTL },
        { x + w, y,     0, 1, cTR },
        { x,     y + h, 0, 1, cBL },
        { x + w, y + h, 0, 1, cBR },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(VtxC));
}

void fill_tri(u32 dev, float x0, float y0, float x1, float y1, float x2, float y2, u32 col)
{
    VtxC v[3] = {
        { x0 - 0.5f, y0 - 0.5f, 0, 1, col },
        { x1 - 0.5f, y1 - 0.5f, 0, 1, col },
        { x2 - 0.5f, y2 - 0.5f, 0, 1, col },
    };
    dDrawUP(dev, D3DPT_TRIANGLEFAN, 1, v, sizeof(VtxC));   // 3 verts = 1 triangle
}

// an ANTI-ALIASED filled CONVEX polygon (`n` points, 3..12) : solid centroid-fan core + a feathered skirt
// (each vertex extruded outward from the centroid, alpha 0 at the rim) -> crisp AA silhouette on triangles,
// diamonds and chevrons, the same recipe as disc(). Call in the colour-quad state. Points in CW or CCW order.
void fill_poly_aa(u32 dev, const float* xy, int n, u32 col)
{
    if (n < 3 || n > 12) return;
    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < n; ++i) { cx += xy[2 * i]; cy += xy[2 * i + 1]; }
    cx /= (float)n; cy /= (float)n;
    const u32 c0 = col & 0x00FFFFFFu; const float f = 1.2f;
    VtxC fan[14];                                         // centroid + n verts + closing repeat = n+2 verts -> n tris
    fan[0] = { cx - 0.5f, cy - 0.5f, 0, 1, col };
    for (int i = 0; i <= n; ++i) { const int k = i % n; fan[i + 1] = { xy[2 * k] - 0.5f, xy[2 * k + 1] - 0.5f, 0, 1, col }; }
    dDrawUP(dev, D3DPT_TRIANGLEFAN, n, fan, sizeof(VtxC));
    VtxC ring[2 * 13];                                    // feathered skirt : vertex (solid) -> vertex + outward*f (alpha 0)
    for (int i = 0; i <= n; ++i) {
        const int k = i % n;
        float ox = xy[2 * k] - cx, oy = xy[2 * k + 1] - cy, len = sqrtf(ox * ox + oy * oy); if (len < 0.001f) len = 1.0f;
        const float nx = ox / len, ny = oy / len;
        ring[2 * i]     = { xy[2 * k] - 0.5f,          xy[2 * k + 1] - 0.5f,          0, 1, col };
        ring[2 * i + 1] = { xy[2 * k] - 0.5f + nx * f, xy[2 * k + 1] - 0.5f + ny * f, 0, 1, c0  };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * n, ring, sizeof(VtxC));
}

// feather the OUTER ARC (radius r, angles a0..a1, `seg` facets) : rim colour at r -> transparent at r+1.1px,
// an AA curved edge WITHOUT filling the interior. Call right after a solid quarter-disc fan sampled at the
// SAME segment count / angles so the feather sits exactly on the facet edge (rounded-rect corners).
void arc_feather(u32 dev, float cx, float cy, float r, float a0, float a1, int seg, u32 col)
{
    if (seg < 1) seg = 1; if (seg > 32) seg = 32;
    cx -= 0.5f; cy -= 0.5f;
    const float f = 1.1f; const u32 c0 = col & 0x00FFFFFFu;
    VtxC rim[2 * (32 + 1)];
    for (int i = 0; i <= seg; ++i) {
        const float a = a0 + (a1 - a0) * (float)i / (float)seg, ca = cosf(a), sa = sinf(a);
        rim[2 * i]     = { cx + r * ca,           cy + r * sa,           0, 1, col };
        rim[2 * i + 1] = { cx + (r + f) * ca,     cy + (r + f) * sa,     0, 1, c0  };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * seg, rim, sizeof(VtxC));
}

void seg_soft(u32 dev, float ax, float ay, float bx, float by, float th, u32 col)
{
    ax -= 0.5f; ay -= 0.5f; bx -= 0.5f; by -= 0.5f;
    float dx = bx - ax, dy = by - ay;
    float l = sqrtf(dx * dx + dy * dy); if (l < 0.01f) return;
    float nx = -dy / l * th * 0.5f, ny = dx / l * th * 0.5f;
    u32 c0 = col & 0x00FFFFFF;                                   // transparent at the edges
    VtxC q[6] = {
        { ax + nx, ay + ny, 0, 1, c0 },
        { bx + nx, by + ny, 0, 1, c0 },
        { ax,      ay,      0, 1, col },
        { bx,      by,      0, 1, col },
        { ax - nx, ay - ny, 0, 1, c0 },
        { bx - nx, by - ny, 0, 1, c0 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 4, q, sizeof(VtxC));       // 6 verts = 4 tris (two feathered halves)
}

void disc(u32 dev, float cx, float cy, float r, u32 col)
{
    cx -= 0.5f; cy -= 0.5f;                          // D3D half-pixel rule
    const int MAXN = 96;
    int N = (int)(r * 1.15f); if (N < 24) N = 24; if (N > MAXN) N = MAXN;   // segments scale with radius -> big discs stay ROUND (no facets)
    const float f = 1.3f;                           // feather width -> anti-aliased edge
    const u32 c0 = col & 0x00FFFFFF;                // same colour, alpha 0
    // solid core (triangle fan)
    VtxC fan[MAXN + 2];
    fan[0] = { cx, cy, 0, 1, col };
    for (int i = 0; i <= N; ++i) {
        float a = (float)i / N * 6.2831853f;
        fan[i + 1] = { cx + r * cosf(a), cy + r * sinf(a), 0, 1, col };
    }
    dDrawUP(dev, D3DPT_TRIANGLEFAN, N, fan, sizeof(VtxC));
    // feathered rim (triangle strip): opaque at r -> transparent at r+f
    VtxC ring[2 * (MAXN + 1)];
    for (int i = 0; i <= N; ++i) {
        float a = (float)i / N * 6.2831853f, ca = cosf(a), sa = sinf(a);
        ring[2 * i]     = { cx + r * ca,       cy + r * sa,       0, 1, col };
        ring[2 * i + 1] = { cx + (r + f) * ca, cy + (r + f) * sa, 0, 1, c0  };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * N, ring, sizeof(VtxC));
}

static const float PI_ = 3.14159265f;

// lerp_argb (per-channel ARGB interpolation, used for the vertical gradient across the rounded rect)
// lives in color.h -> one implementation shared with the rest of the renderer.

// a plain vertical-gradient rect (raw verts, half-pixel already applied by the caller).
static void vrect_raw(u32 dev, float x, float y, float w, float h, u32 cT, u32 cB)
{
    if (w <= 0.0f || h <= 0.0f) return;
    VtxC q[4] = {
        { x,     y,     0, 1, cT },
        { x + w, y,     0, 1, cT },
        { x,     y + h, 0, 1, cB },
        { x + w, y + h, 0, 1, cB },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(VtxC));
}

void rrect(u32 dev, float x, float y, float w, float h, float r, u32 cTop, u32 cBot, float feather)
{
    if (w <= 0.0f || h <= 0.0f) return;
    x -= 0.5f; y -= 0.5f;                                        // D3D half-pixel rule (applied ONCE here)
    if (r > w * 0.5f) r = w * 0.5f;
    if (r > h * 0.5f) r = h * 0.5f;
    // colour at any scanline y' (linear top->bottom across the whole box height)
    #define CY(yy) lerp_argb(cTop, cBot, ((yy) - y) / h)
    if (r < 0.75f && feather <= 0.0f) {                          // no radius, no feather -> a plain crisp rect
        vrect_raw(dev, x, y, w, h, cTop, cBot);
    } else {
        if (r < 0.75f) { r = 0.75f; if (r > w * 0.5f) r = w * 0.5f; if (r > h * 0.5f) r = h * 0.5f; }   // feather asked with ~no radius -> a hair of corner so the rounded path's CONSISTENT edge+corner feather applies (rule 2)
        // --- solid interior : centre column full-height + the two side bands + 4 quarter-disc corners ---
        vrect_raw(dev, x + r,     y,     w - 2 * r, h,         cTop,       cBot);          // centre column
        vrect_raw(dev, x,         y + r, r,         h - 2 * r, CY(y + r),  CY(y + h - r)); // left band
        vrect_raw(dev, x + w - r, y + r, r,         h - 2 * r, CY(y + r),  CY(y + h - r)); // right band
        const int NcMax = 24;                                   // stack-array bound (raised : big circles/discs -- e.g. the minimap brass lens -- were faceted at 14/corner)
        int Nc = (int)(r * 0.7f); if (Nc < 6) Nc = 6; if (Nc > NcMax) Nc = NcMax;   // more segments on bigger radii -> rounder
        struct Corner { float cx, cy, a0; };
        const Corner cs4[4] = {
            { x + r,     y + r,     PI_        },   // TL : 180 -> 270
            { x + w - r, y + r,     1.5f * PI_ },   // TR : 270 -> 360
            { x + w - r, y + h - r, 0.0f       },   // BR :   0 ->  90
            { x + r,     y + h - r, 0.5f * PI_ },   // BL :  90 -> 180
        };
        for (int k = 0; k < 4; ++k) {                            // ONE triangle-fan per corner (was Nc draws each)
            const float ccx = cs4[k].cx, ccy = cs4[k].cy, a0 = cs4[k].a0;
            VtxC fan[NcMax + 2];
            fan[0] = { ccx, ccy, 0, 1, CY(ccy) };                // fan centre
            for (int i = 0; i <= Nc; ++i) {
                const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc;
                const float vx = ccx + r * cosf(a), vy = ccy + r * sinf(a);
                fan[i + 1] = { vx, vy, 0, 1, CY(vy) };
            }
            dDrawUP(dev, D3DPT_TRIANGLEFAN, Nc, fan, sizeof(VtxC));   // Nc triangles, one submit
        }
        // --- feather ALL edges + corners CONSISTENTLY, so the silhouette extends by the same `feather`
        // everywhere. (If only the corners feathered, the rounded ends would reach ~1px further than the
        // straight centre -> the centre looks 1px short and the backdrop shows as a thin line there.) ---
        if (feather > 0.0f) {
            const float f = feather;
            { const u32 tA = cTop & 0x00FFFFFF;                                       // top edge : y -> y-f (alpha 0)
              VtxC T[4] = { { x + r, y - f, 0,1, tA }, { x + w - r, y - f, 0,1, tA },
                            { x + r, y,     0,1, cTop }, { x + w - r, y,     0,1, cTop } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, T, sizeof(VtxC)); }
            { const u32 bA = cBot & 0x00FFFFFF;                                       // bottom edge : y+h -> y+h+f
              VtxC B[4] = { { x + r, y + h,     0,1, cBot }, { x + w - r, y + h,     0,1, cBot },
                            { x + r, y + h + f, 0,1, bA },   { x + w - r, y + h + f, 0,1, bA } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, B, sizeof(VtxC)); }
            { const u32 c0 = CY(y + r), c1 = CY(y + h - r);                           // left + right edges
              VtxC L[4] = { { x - f, y + r, 0,1, c0 & 0x00FFFFFF }, { x, y + r, 0,1, c0 },
                            { x - f, y + h - r, 0,1, c1 & 0x00FFFFFF }, { x, y + h - r, 0,1, c1 } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, L, sizeof(VtxC));
              VtxC R[4] = { { x + w, y + r, 0,1, c0 }, { x + w + f, y + r, 0,1, c0 & 0x00FFFFFF },
                            { x + w, y + h - r, 0,1, c1 }, { x + w + f, y + h - r, 0,1, c1 & 0x00FFFFFF } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, R, sizeof(VtxC)); }
            // 4 corner arcs : radius r (full) -> r+f (alpha 0)
            for (int k = 0; k < 4; ++k) {
                const float ccx = cs4[k].cx, ccy = cs4[k].cy, a0 = cs4[k].a0;
                VtxC ring[2 * (NcMax + 1)];
                for (int i = 0; i <= Nc; ++i) {
                    const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc, ca = cosf(a), sa = sinf(a);
                    const u32 ci = CY(ccy + r * sa);
                    ring[2 * i]     = { ccx + r * ca,       ccy + r * sa,       0, 1, ci };
                    ring[2 * i + 1] = { ccx + (r + f) * ca, ccy + (r + f) * sa, 0, 1, ci & 0x00FFFFFF };
                }
                dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * Nc, ring, sizeof(VtxC));
            }
        }
    }
    #undef CY
}

void rrect_bordered(u32 dev, float x, float y, float w, float h, float r,
                    u32 cTop, u32 cBot, u32 border, float bt, float feather)
{
    rrect(dev, x, y, w, h, r, border, border, feather);
    const float ir = (r - bt > 0.0f) ? r - bt : 0.0f;
    rrect(dev, x + bt, y + bt, w - 2 * bt, h - 2 * bt, ir, cTop, cBot, feather);
}

// D3D8 stencil render states not in the d3d.h enum (passed as raw ids to dSetRS).
enum { RS_STENCILENABLE = 52, RS_STENCILFAIL = 53, RS_STENCILZFAIL = 54, RS_STENCILPASS = 55,
       RS_STENCILFUNC = 56, RS_STENCILREF = 57, RS_STENCILMASK = 58, RS_STENCILWRITEMASK = 59 };
void rrect_clip_begin(u32 dev, float x, float y, float w, float h, float r) {
    dSetVS(dev, FVF_XYZRHW_DIFFUSE); dSetTex(dev, 0, 0);
    dSetRS(dev, D3DRS_ALPHATESTENABLE, 0);
    dSetRS(dev, D3DRS_ALPHABLENDENABLE, 0);
    dSetRS(dev, RS_STENCILENABLE, 1);
    dSetRS(dev, RS_STENCILMASK, 0xFF); dSetRS(dev, RS_STENCILWRITEMASK, 0xFF);
    dSetRS(dev, D3DRS_COLORWRITEENABLE, 0);                                   // mask pass : stencil only, no colour
    dSetRS(dev, RS_STENCILFUNC, 8);                                          // ALWAYS
    dSetRS(dev, RS_STENCILFAIL, 3); dSetRS(dev, RS_STENCILZFAIL, 3); dSetRS(dev, RS_STENCILPASS, 3);   // REPLACE
    dSetRS(dev, RS_STENCILREF, 0);                                           // pass A : clear the region to 0
    grad_quad(dev, x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, 0, 0, 0, 0);
    dSetRS(dev, RS_STENCILREF, 1);                                          // pass B : set 1 inside the rounded rect
    rrect(dev, x, y, w, h, r, 0xFF000000, 0xFF000000, 0.0f);                 // feather 0 : mask must match the track EXACTLY (default 1.2px feather + alpha-test off would overflow the round cap ~1px)
    dSetRS(dev, D3DRS_COLORWRITEENABLE, 0x0000000F);                         // content : colour ONLY where stencil == 1
    dSetRS(dev, RS_STENCILFUNC, 3);                                         // EQUAL
    dSetRS(dev, RS_STENCILFAIL, 1); dSetRS(dev, RS_STENCILZFAIL, 1); dSetRS(dev, RS_STENCILPASS, 1);   // KEEP (don't touch stencil while drawing)
    dSetRS(dev, D3DRS_ALPHABLENDENABLE, 1);
}
void rrect_clip_end(u32 dev) {
    dSetRS(dev, RS_STENCILENABLE, 0);
    dSetRS(dev, D3DRS_COLORWRITEENABLE, 0x0000000F);
}

void rrect_left(u32 dev, float x, float y, float w, float h, float r, u32 cTop, u32 cBot, float feather)
{
    if (w <= 0.0f || h <= 0.0f) return;
    x -= 0.5f; y -= 0.5f;
    if (r > w) r = w;
    if (r > h * 0.5f) r = h * 0.5f;
    if (r < 0.0f) r = 0.0f;
    #define CY(yy) lerp_argb(cTop, cBot, ((yy) - y) / h)
    if (r < 0.75f) { vrect_raw(dev, x, y, w, h, cTop, cBot); }
    else {
        vrect_raw(dev, x + r, y,     w - r, h,         cTop,       cBot);          // body (flat right edge = the level)
        vrect_raw(dev, x,     y + r, r,     h - 2 * r, CY(y + r),  CY(y + h - r)); // left band
        const int NcMax = 14;
        int Nc = (int)(r * 0.9f); if (Nc < 6) Nc = 6; if (Nc > NcMax) Nc = NcMax;   // rounder on bigger radii
        const float cc[2][3] = { { x + r, y + r,     PI_ },          // TL : 180 -> 270
                                 { x + r, y + h - r, 0.5f * PI_ } };  // BL :  90 -> 180
        for (int k = 0; k < 2; ++k) {                            // ONE triangle-fan per (left) corner
            const float ccx = cc[k][0], ccy = cc[k][1], a0 = cc[k][2];
            VtxC fan[NcMax + 2];
            fan[0] = { ccx, ccy, 0, 1, CY(ccy) };                // fan centre
            for (int i = 0; i <= Nc; ++i) {
                const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc;
                const float vx = ccx + r * cosf(a), vy = ccy + r * sinf(a);
                fan[i + 1] = { vx, vy, 0, 1, CY(vy) };
            }
            dDrawUP(dev, D3DPT_TRIANGLEFAN, Nc, fan, sizeof(VtxC));
        }
        if (feather > 0.0f) {
            const float f = feather;
            const u32 c0 = CY(y + r), c1 = CY(y + h - r);
            VtxC L[4] = { { x, y + r, 0, 1, c0 }, { x, y + h - r, 0, 1, c1 },
                          { x - f, y + r, 0, 1, c0 & 0x00FFFFFF }, { x - f, y + h - r, 0, 1, c1 & 0x00FFFFFF } };
            dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, L, sizeof(VtxC));   // left edge feather
            // top + bottom edge feathers (x+r..x+w ; the flat RIGHT edge stays crisp = the level tip) so the
            // straight edges extend by the same `feather` as the rounded left cap (no 1px centre mismatch).
            { const u32 tA = cTop & 0x00FFFFFF;
              VtxC T[4] = { { x + r, y - f, 0,1, tA }, { x + w, y - f, 0,1, tA },
                            { x + r, y,     0,1, cTop }, { x + w, y,     0,1, cTop } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, T, sizeof(VtxC)); }
            { const u32 bA = cBot & 0x00FFFFFF;
              VtxC B[4] = { { x + r, y + h,     0,1, cBot }, { x + w, y + h,     0,1, cBot },
                            { x + r, y + h + f, 0,1, bA },   { x + w, y + h + f, 0,1, bA } };
              dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, B, sizeof(VtxC)); }
            for (int k = 0; k < 2; ++k) {
                const float ccx = cc[k][0], ccy = cc[k][1], a0 = cc[k][2];
                VtxC ring[2 * (NcMax + 1)];
                for (int i = 0; i <= Nc; ++i) {
                    const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc, ca = cosf(a), sa = sinf(a);
                    const u32 ci = CY(ccy + r * sa);
                    ring[2 * i]     = { ccx + r * ca,        ccy + r * sa,        0, 1, ci };
                    ring[2 * i + 1] = { ccx + (r + f) * ca,  ccy + (r + f) * sa,  0, 1, ci & 0x00FFFFFF };
                }
                dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * Nc, ring, sizeof(VtxC));
            }
        }
    }
    #undef CY
}

void rrect_glow(u32 dev, float x, float y, float w, float h, float r, u32 col, float glowW)
{
    if (w <= 0.0f || h <= 0.0f || glowW <= 0.0f) return;
    x -= 0.5f; y -= 0.5f;
    if (r > w * 0.5f) r = w * 0.5f;
    if (r > h * 0.5f) r = h * 0.5f;
    if (r < 0.0f) r = 0.0f;
    const float f = glowW;
    const u32 c0 = col & 0x00FFFFFF;
    // 4 straight edges : peak on the path, alpha 0 outward
    { VtxC v[4] = { { x + r, y, 0,1, col }, { x + w - r, y, 0,1, col },
                    { x + r, y - f, 0,1, c0 }, { x + w - r, y - f, 0,1, c0 } };            // top
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x + r, y + h, 0,1, col }, { x + w - r, y + h, 0,1, col },
                    { x + r, y + h + f, 0,1, c0 }, { x + w - r, y + h + f, 0,1, c0 } };      // bottom
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x, y + r, 0,1, col }, { x, y + h - r, 0,1, col },
                    { x - f, y + r, 0,1, c0 }, { x - f, y + h - r, 0,1, c0 } };              // left
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x + w, y + r, 0,1, col }, { x + w, y + h - r, 0,1, col },
                    { x + w + f, y + r, 0,1, c0 }, { x + w + f, y + h - r, 0,1, c0 } };      // right
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    // 4 corner arcs : radius r (peak) -> r+f (alpha 0)
    if (r > 0.25f) {
        const int Nc = 6;
        const float cs4[4][3] = {
            { x + r,     y + r,     PI_        },   // TL
            { x + w - r, y + r,     1.5f * PI_ },   // TR
            { x + w - r, y + h - r, 0.0f       },   // BR
            { x + r,     y + h - r, 0.5f * PI_ },   // BL
        };
        for (int k = 0; k < 4; ++k) {
            const float ccx = cs4[k][0], ccy = cs4[k][1], a0 = cs4[k][2];
            VtxC ring[2 * (Nc + 1)];
            for (int i = 0; i <= Nc; ++i) {
                const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc, ca = cosf(a), sa = sinf(a);
                ring[2 * i]     = { ccx + r * ca,       ccy + r * sa,       0, 1, col };
                ring[2 * i + 1] = { ccx + (r + f) * ca, ccy + (r + f) * sa, 0, 1, c0 };
            }
            dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * Nc, ring, sizeof(VtxC));
        }
    }
}

void disc_glow(u32 dev, float cx, float cy, float r, u32 col, float glowW)
{
    if (r <= 0.0f || glowW <= 0.0f) return;
    cx -= 0.5f; cy -= 0.5f;
    const int N = 32;
    const u32 c0 = col & 0x00FFFFFF;
    VtxC ring[2 * (N + 1)];
    for (int i = 0; i <= N; ++i) {
        const float a = (float)i / N * 6.2831853f, ca = cosf(a), sa = sinf(a);
        ring[2 * i]     = { cx + r * ca,             cy + r * sa,             0, 1, col };
        ring[2 * i + 1] = { cx + (r + glowW) * ca,   cy + (r + glowW) * sa,   0, 1, c0  };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * N, ring, sizeof(VtxC));
}

void rrect_stroke(u32 dev, float x, float y, float w, float h, float r, u32 col, float bt)
{
    if (w <= 0.0f || h <= 0.0f || bt <= 0.0f) return;
    x -= 0.5f; y -= 0.5f;
    if (r > w * 0.5f) r = w * 0.5f;
    if (r > h * 0.5f) r = h * 0.5f;
    if (r < 0.0f) r = 0.0f;
    const float ri = (r - bt > 0.0f) ? r - bt : 0.0f;
    const float fth = 1.0f;                                   // AA feather on the outer arc
    const u32 c0 = col & 0x00FFFFFF;
    // 4 straight edges (axis-aligned -> no aliasing, solid bands from the tangent points)
    { VtxC v[4] = { { x + r, y, 0,1, col }, { x + w - r, y, 0,1, col },
                    { x + r, y + bt, 0,1, col }, { x + w - r, y + bt, 0,1, col } };                 // top
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x + r, y + h - bt, 0,1, col }, { x + w - r, y + h - bt, 0,1, col },
                    { x + r, y + h, 0,1, col }, { x + w - r, y + h, 0,1, col } };                    // bottom
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x, y + r, 0,1, col }, { x + bt, y + r, 0,1, col },
                    { x, y + h - r, 0,1, col }, { x + bt, y + h - r, 0,1, col } };                   // left
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    { VtxC v[4] = { { x + w - bt, y + r, 0,1, col }, { x + w, y + r, 0,1, col },
                    { x + w - bt, y + h - r, 0,1, col }, { x + w, y + h - r, 0,1, col } };            // right
      dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(VtxC)); }
    // 4 corner arcs : a quarter annulus ri..r + a feathered rim r..r+fth
    if (r > 0.25f) {
        const int Nc = 8;
        const float cs4[4][3] = {
            { x + r,     y + r,     PI_        },   // TL
            { x + w - r, y + r,     1.5f * PI_ },   // TR
            { x + w - r, y + h - r, 0.0f       },   // BR
            { x + r,     y + h - r, 0.5f * PI_ },   // BL
        };
        for (int k = 0; k < 4; ++k) {
            const float ccx = cs4[k][0], ccy = cs4[k][1], a0 = cs4[k][2];
            VtxC ann[2 * (Nc + 1)], rim[2 * (Nc + 1)];
            for (int i = 0; i <= Nc; ++i) {
                const float a = a0 + (0.5f * PI_) * (float)i / (float)Nc, ca = cosf(a), sa = sinf(a);
                ann[2 * i]     = { ccx + r * ca,        ccy + r * sa,        0, 1, col };
                ann[2 * i + 1] = { ccx + ri * ca,       ccy + ri * sa,       0, 1, col };
                rim[2 * i]     = { ccx + r * ca,        ccy + r * sa,        0, 1, col };
                rim[2 * i + 1] = { ccx + (r + fth) * ca, ccy + (r + fth) * sa, 0, 1, c0 };
            }
            dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * Nc, ann, sizeof(VtxC));
            dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * Nc, rim, sizeof(VtxC));
        }
    }
}

void soft_blob(u32 dev, float cx, float cy, float hw, float hh, u32 col)
{
    u32 c0 = col & 0x00FFFFFF;                                   // transparent at the edges
    grad_quad(dev, cx - hw, cy - hh, hw, hh, c0, c0, c0, col);   // TL quadrant (bright at centre = BR)
    grad_quad(dev, cx,      cy - hh, hw, hh, c0, c0, col, c0);   // TR (bright at BL)
    grad_quad(dev, cx - hw, cy,      hw, hh, c0, col, c0, c0);   // BL (bright at TR)
    grad_quad(dev, cx,      cy,      hw, hh, col, c0, c0, c0);   // BR (bright at TL)
}

} // namespace aio
