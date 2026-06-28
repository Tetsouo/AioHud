// draw.cpp -- see draw.h.
#include "draw.h"
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
    const int N = 28;
    const float f = 1.3f;                           // feather width -> anti-aliased edge
    const u32 c0 = col & 0x00FFFFFF;                // same colour, alpha 0
    // solid core (triangle fan)
    VtxC fan[N + 2];
    fan[0] = { cx, cy, 0, 1, col };
    for (int i = 0; i <= N; ++i) {
        float a = (float)i / N * 6.2831853f;
        fan[i + 1] = { cx + r * cosf(a), cy + r * sinf(a), 0, 1, col };
    }
    dDrawUP(dev, D3DPT_TRIANGLEFAN, N, fan, sizeof(VtxC));
    // feathered rim (triangle strip): opaque at r -> transparent at r+f
    VtxC ring[2 * (N + 1)];
    for (int i = 0; i <= N; ++i) {
        float a = (float)i / N * 6.2831853f, ca = cosf(a), sa = sinf(a);
        ring[2 * i]     = { cx + r * ca,       cy + r * sa,       0, 1, col };
        ring[2 * i + 1] = { cx + (r + f) * ca, cy + (r + f) * sa, 0, 1, c0  };
    }
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2 * N, ring, sizeof(VtxC));
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
