// sandbox.cpp -- DIRECT D3D8, phase 3: richer liquid (organic noise + 3 parallax layers).
// Builds on the proven UV-scroll pipeline. Improvements:
//   - texture = a SEAMLESS organic noise (sum of sines with integer u/v freqs ->
//     tileable) instead of plain bands -> blobby, caustic-like.
//   - 3 LAYERS of the same texture, each scrolled at a different speed/scale/dir
//     and tinted via per-vertex DIFFUSE (FVF 0x144, COLOROP=MODULATE) -> parallax
//     depth: a dark body, a mid flow, a bright highlight, alpha-blended.
//
// FVF 0x144 = XYZRHW|DIFFUSE|TEX1, 28-byte vert = pos(16) + color(4) + uv(8).
// Iterate: //unload AioTest -> deploy.bat -> //load AioTest
#include "windower_plugin.h"
#include "windower_debug.h"
#include <math.h>
#include <string.h>

using namespace windower;

static PluginManager g_host;
static u32 g_tex[3] = { 0, 0, 0 };   // liquid textures (one per bar)
static u32 g_cap_front = 0, g_cap_back = 0, g_glow = 0, g_bubble = 0;
static const int CAP_W = 164, CAP_H = 280;
static const char* CAP_FRONT = "D:\\Windower Tetsouo\\plugins\\_aiohud_re\\sandbox_assets\\cap_front.bin";
static const char* CAP_BACK  = "D:\\Windower Tetsouo\\plugins\\_aiohud_re\\sandbox_assets\\cap_back.bin";
static float g_fill[3] = { 0.50f, 0.25f, 0.80f };   // HP / MP / TP fill (live via //aio commands)
static int   g_layers  = 4;                          // 1 = single-layer fluid; >=4 also draws bubbles/halo (//aio lay N). Default = full look.

enum {
    D3DPT_TRIANGLESTRIP = 5,
    FVF_XYZRHW_DIFFUSE      = 0x44,
    FVF_XYZRHW_DIFFUSE_TEX1 = 0x144,
    D3DSBT_ALL = 1, D3DFMT_A8R8G8B8 = 21, D3DPOOL_MANAGED = 1,
    D3DRS_ZENABLE = 7, D3DRS_SRCBLEND = 19, D3DRS_DESTBLEND = 20, D3DRS_CULLMODE = 22,
    D3DRS_ALPHABLENDENABLE = 27, D3DRS_LIGHTING = 137,
    D3DRS_ALPHATESTENABLE = 15, D3DRS_FOGENABLE = 28, D3DRS_SPECULARENABLE = 29,
    D3DRS_COLORVERTEX = 141, D3DRS_COLORWRITEENABLE = 168, D3DRS_TEXTUREFACTOR = 60,
    D3DRS_WRAP0 = 128, D3DRS_BLENDOP = 171, D3DBLENDOP_ADD = 1,
    D3DTSS_TEXCOORDINDEX = 11, D3DTSS_TEXTURETRANSFORMFLAGS = 24, D3DTTFF_DISABLE = 0,
    D3DCULL_NONE = 1, D3DBLEND_ZERO = 1, D3DBLEND_ONE = 2, D3DBLEND_SRCALPHA = 5, D3DBLEND_INVSRCALPHA = 6,
    D3DTSS_COLOROP = 1, D3DTSS_COLORARG1 = 2, D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4, D3DTSS_ALPHAARG1 = 5, D3DTSS_ALPHAARG2 = 7,
    D3DTSS_ADDRESSU = 13, D3DTSS_ADDRESSV = 14,
    D3DTSS_MAGFILTER = 16, D3DTSS_MINFILTER = 17, D3DTSS_MIPFILTER = 18,
    D3DTEXF_NONE = 0, D3DTEXF_LINEAR = 2,
    D3DTOP_DISABLE = 1, D3DTOP_SELECTARG1 = 2, D3DTOP_MODULATE = 4, D3DTA_DIFFUSE = 0, D3DTA_TEXTURE = 2,
    D3DTADDRESS_WRAP = 1, D3DTADDRESS_CLAMP = 3,
};

struct Vtx  { float x, y, z, rhw; u32 color; float u, v; };  // 28 bytes, FVF 0x144 (liquid)
struct VtxC { float x, y, z, rhw; u32 color; };              // 20 bytes, FVF 0x44  (glass)

static void dSetRS (u32 d, u32 s, u32 v)          { auto f = vmethod<long(__stdcall*)(u32,u32,u32)>(d,50);     if(f) f(d,s,v); }
static void dSetTSS(u32 d, u32 st, u32 ty, u32 v) { auto f = vmethod<long(__stdcall*)(u32,u32,u32,u32)>(d,63);  if(f) f(d,st,ty,v); }
static void dSetTex(u32 d, u32 st, u32 tex)       { auto f = vmethod<long(__stdcall*)(u32,u32,u32)>(d,61);      if(f) f(d,st,tex); }
static void dSetVS (u32 d, u32 fvf)               { auto f = vmethod<long(__stdcall*)(u32,u32)>(d,76);          if(f) f(d,fvf); }
static void dDrawUP(u32 d, u32 pt, u32 n, const void* v, u32 st){ auto f = vmethod<long(__stdcall*)(u32,u32,u32,const void*,u32)>(d,72); if(f) f(d,pt,n,v,st); }
static u32  dCreateSB(u32 d, u32 type)            { auto f = vmethod<long(__stdcall*)(u32,u32,u32*)>(d,57); u32 t=0; if(f) f(d,type,&t); return t; }
static void dApplySB(u32 d, u32 tok)             { auto f = vmethod<long(__stdcall*)(u32,u32)>(d,54);          if(f) f(d,tok); }
static void dDelSB  (u32 d, u32 tok)             { auto f = vmethod<long(__stdcall*)(u32,u32)>(d,56);          if(f) f(d,tok); }

// seamless organic noise (integer u/v freqs -> tiles perfectly at u/v = 0..1)
static float noise(float u, float v)
{
    const float TAU = 6.2831853f;
    float n = 0.50f * sinf(TAU * (2*u + 1*v) + 0.0f)
            + 0.25f * sinf(TAU * (3*u - 2*v) + 1.7f)
            + 0.15f * sinf(TAU * (1*u + 4*v) + 3.1f)
            + 0.10f * sinf(TAU * (5*u + 3*v) + 4.6f);
    return 0.5f + 0.5f * n;   // 0..1
}

// a SECOND seamless noise (also integer freqs -> tileable).
static float noise2(float u, float v)
{
    const float TAU = 6.2831853f;
    float n = 0.50f * sinf(TAU * (3*u + 2*v) + 0.9f)
            + 0.30f * sinf(TAU * (2*u - 3*v) + 2.4f)
            + 0.20f * sinf(TAU * (4*u + 5*v) + 5.1f);
    return 0.5f + 0.5f * n;   // 0..1
}

// HIGH-frequency seamless noise (integer freqs) -> for tiny speckles.
static float noise3(float u, float v)
{
    const float TAU = 6.2831853f;
    float n = 0.50f * sinf(TAU * (8*u + 6*v)  + 0.5f)
            + 0.30f * sinf(TAU * (11*u - 7*v) + 2.0f)
            + 0.20f * sinf(TAU * (9*u + 13*v) + 4.0f);
    return 0.5f + 0.5f * n;   // 0..1
}

// deterministic hash -> [0,1).
static float hash2(int i, int j)
{
    float s = sinf(i * 12.9898f + j * 78.233f) * 43758.5453f;
    return s - floorf(s);
}

// --- proper organic noise (hash lattice, smooth-interpolated, TILEABLE at grid G) ---
static float vhash(int xi, int yi, int G) { xi = ((xi % G) + G) % G; yi = ((yi % G) + G) % G; return hash2(xi + 1, yi * 3 + 1); }
static float vnoise(float u, float v, int G)
{
    float fx = u * G, fy = v * G;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float tx = fx - x0, ty = fy - y0;
    tx = tx * tx * (3.0f - 2.0f * tx); ty = ty * ty * (3.0f - 2.0f * ty);   // smoothstep
    float a = vhash(x0, y0, G),     b = vhash(x0 + 1, y0, G);
    float c = vhash(x0, y0 + 1, G), d = vhash(x0 + 1, y0 + 1, G);
    float top = a + (b - a) * tx, bot = c + (d - c) * tx;
    return top + (bot - top) * ty;   // 0..1
}
static float fbm(float u, float v)   // fractal sum -> cloudy organic, tileable
{
    return 0.55f * vnoise(u, v, 4) + 0.30f * vnoise(u, v, 8) + 0.15f * vnoise(u, v, 16);
}
// tileable cellular (Worley) : distance to nearest jittered feature point.
// `seed` shifts the jitter; `G` = grid resolution (bigger = finer/denser cells).
static float cellular(float u, float v, int seed, int G)
{
    float fx = u * G, fy = v * G;
    int cx = (int)floorf(fx), cy = (int)floorf(fy);
    float md = 9.0f;
    for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
        int ci = cx + di, cj = cy + dj;
        int wi = ((ci % G) + G) % G, wj = ((cj % G) + G) % G;
        float jx = hash2(wi + 5  + seed * 31, wj + 9  + seed * 17);
        float jy = hash2(wi + 19 + seed * 13, wj + 23 + seed * 29);
        float dx = (ci + jx) - fx, dy = (cj + jy) - fy;
        float d = dx * dx + dy * dy;
        if (d < md) md = d;
    }
    float r = sqrtf(md); return r > 1.0f ? 1.0f : r;   // 0 at cell centre
}

// ANISOTROPIC tileable cellular: separate cell counts on each axis. Gx < Gy makes
// cells WIDE (elongated horizontally) -> laminar "flow line" streaks along the tube.
static float cellular2(float u, float v, int seed, int Gx, int Gy)
{
    float fx = u * Gx, fy = v * Gy;
    int cx = (int)floorf(fx), cy = (int)floorf(fy);
    float md = 9.0f;
    for (int dj = -1; dj <= 1; dj++) for (int di = -1; di <= 1; di++) {
        int ci = cx + di, cj = cy + dj;
        int wi = ((ci % Gx) + Gx) % Gx, wj = ((cj % Gy) + Gy) % Gy;
        float jx = hash2(wi + 5  + seed * 31, wj + 9  + seed * 17);
        float jy = hash2(wi + 19 + seed * 13, wj + 23 + seed * 29);
        float dx = (ci + jx) - fx, dy = (cj + jy) - fy;
        float d = dx * dx + dy * dy;
        if (d < md) md = d;
    }
    float r = sqrtf(md); return r > 1.0f ? 1.0f : r;
}

// alpha-byte scale / RGB brighten helpers for live-tinted layers
static u32 mulA(u32 c, float f) { u32 a = (u32)(((c >> 24) & 0xFF) * f); if (a > 255) a = 255; return (a << 24) | (c & 0xFFFFFF); }
static u32 brighten(u32 c, float f) {
    u32 a = (c >> 24) & 0xFF, r = (u32)(((c >> 16) & 0xFF) * f), g = (u32)(((c >> 8) & 0xFF) * f), b = (u32)((c & 0xFF) * f);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// per-bar texture: 0 HP = organic cells, 1 MP = swirly arcane volutes (warped
// FBM), 2 TP = fine dense cells (energy charge). Seamless.
static u32 make_texture(u32 dev, int variant)
{
    const int W = 512, H = 64;   // ~8:1, MATCHES the bar's aspect -> shown 1x (no tiling, no repeat)
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    u32 tex = 0;
    if (!fCreate || fCreate(dev, W, H, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) < 0 || !valid_ptr(tex))
        return 0;
    struct LR { int Pitch; void* pBits; } lr = { 0, 0 };
    auto fLock = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
    if (fLock && fLock(tex, 0, &lr, 0, 0) >= 0 && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float u = (float)x / W, v = (float)y / H;
                // CAUSTIC LIGHT FILAMENTS (not blobs): a smooth dark body crossed by
                // THIN bright wavy streaks -> reads as light refracting through moving
                // liquid, the way the bottom of a pool looks. Domain-warped RIDGED fbm
                // (ridge = 1-|2n-1| -> bright where the noise crosses 0.5), stretched
                // horizontally (vertical freq >> horizontal) so the filaments run ALONG
                // the tube. Integer domain scales + tileable warp keep it seamless.
                // SMOOTH liquid: broad, soft, LOW-CONTRAST light variation -- no thin
                // filaments, no busy detail. The "liquid" feel comes from the MOTION
                // (flow + traveling specular + meniscus), not from a detailed texture.
                float bright;
                if (variant == 1) {                                  // MP : slow arcane swirl (gentle, low freq)
                    float wx = fbm(1 * u, 1 * v + 0.30f) - 0.5f;
                    float n  = fbm(3 * u + 0.6f * wx, 1 * v);
                    bright = 0.45f + 0.48f * n;
                } else {                                             // HP / TP : soft broad liquid (8:1 texture)
                    int fu = (variant == 2) ? 5 : 3;                 // horizontal cycles (texture is 8:1 -> gentle streak)
                    int fv = 1;                                      // 1 -> large soft vertical variation (no tiny detail)
                    float n  = fbm(fu * u, fv * v);
                    float c  = (variant == 2) ? 0.50f : 0.42f;       // TP a touch more contrast (energetic)
                    bright = (0.5f - 0.5f * c) + c * n;
                }
                bright = 0.62f + 0.38f * bright;                     // HIGH floor -> the grey texture barely dims the vivid colour
                if (bright > 1.0f) bright = 1.0f; if (bright < 0.0f) bright = 0.0f;
                u32 b = (u32)(bright * 255.0f);
                // mostly opaque -> colour reads saturated over the dark game background.
                float al = 0.74f + 0.24f * bright;
                u32 av = (u32)(al * 255.0f);
                row[x] = (av << 24) | (b << 16) | (b << 8) | b;
            }
        }
        auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);
        if (fUnlock) fUnlock(tex, 0);
    }
    return tex;
}

// VIVID liquid palettes (saturated to punch through the glass shading). Each =
// {body, mid, additive-highlight}. Order 1/2/3 = HP red-pink / MP blue-cyan /
// TP yellow-green. body alpha 0xD8 (present, slightly translucent).
// JRPG-vivid palettes (saturated + bright -> "pop" through the glass). {body, mid, highlight}.
// POTION palettes = {DEEP base (bottom), mid (caustic), BRIGHT accent (top/specular)}.
// The body draws a two-tone gradient deep->bright -> luminous magic-potion look.
static const u32 PAL_HP[3] = { 0xE0148C2D, 0x965ADC5A, 0xC8A0FF50 };  // 1 HP  deep emerald -> bright LIME
static const u32 PAL_MP[3] = { 0xE0005ADC, 0x9628B4FF, 0xC88CF5FF };  // 2 MP  vivid MAGICAL CYAN (deep -> bright)
static const u32 PAL_TP[3] = { 0xE05A0FBE, 0x969632F0, 0x88CD6EFF };  // 3 TP  deep violet -> bright MAGENTA-violet

// HP changes colour with its level. FFXI undead "low-HP" aggro triggers at HP
// <= 75%, and the game's HP number goes red at 25%. So: >=75% = full pink, blends
// to ORANGE through the 25-75% undead-aggro zone, to RED below 25%.
static const u32 HP_YELLOW[3] = { 0xE0C88200, 0x96EBC828, 0xC8FFF05A };  // 25-75% deep amber -> bright YELLOW
static const u32 HP_RED[3]    = { 0xE0960A0A, 0x96E62828, 0xC8FF5A46 };  // <=25%   deep blood -> bright RED
static u32 lerp_ch(u32 a, u32 b, int sh, float t)
{
    int ca = (a >> sh) & 0xFF, cb = (b >> sh) & 0xFF;
    int c = (int)(ca + (cb - ca) * t + 0.5f);
    if (c < 0) c = 0; if (c > 255) c = 255;
    return (u32)c << sh;
}
static u32 lerp_argb(u32 a, u32 b, float t)
{
    if (t < 0) t = 0; if (t > 1) t = 1;
    return lerp_ch(a, b, 0, t) | lerp_ch(a, b, 8, t) | lerp_ch(a, b, 16, t) | lerp_ch(a, b, 24, t);
}
static void hp_palette(float fill, u32 out[3])
{
    // Hard 3-zone switch (no gradient): green >75%, ORANGE 25-75% (undead aggro),
    // RED <=25%.
    const u32* p = (fill > 0.75f) ? PAL_HP : (fill > 0.25f) ? HP_YELLOW : HP_RED;
    out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
}

// TP weaponskill tiers (read at a glance): 0-999 gray+faded (charging), 1000 WS1
// silver, 2000 WS2 gold, 3000 WS3 radiant. Glow intensity ramps per tier.
// ONE violet hue as a deep->bright potion gradient, ramping DULL -> VIVID as TP charges.
static const u32 TP_T0[3] = { 0xA032284B, 0x605F5082, 0x208C7DA5 };  // 0-999     dull greyed violet (charging, no glow)
static const u32 TP_T1[3] = { 0xD8461496, 0x96823CD2, 0xC8B478F5 };  // 1000-1999 muted violet, waking up (WS1)
static const u32 TP_T2[3] = { 0xE05A0FBE, 0x969632F0, 0xC8CD6EFF };  // 2000-2999 vivid violet (WS2)
static const u32 TP_T3[3] = { 0xF06E0AE1, 0xB4AA28FF, 0xCCEB8CFF };  // 3000      ELECTRIC violet, max vivid (WS3)
static int tp_tier(float fill)
{
    int tp = (int)(fill * 3000.0f + 0.5f);
    return (tp >= 3000) ? 3 : (tp >= 2000) ? 2 : (tp >= 1000) ? 1 : 0;
}
// CONTINUOUS irradiation: instead of snapping to a tier, interpolate smoothly through
// the 4 anchor palettes (TP 0 / 1000 / 2000 / 3000) -> the liquid colour & opacity
// ramp up continuously as TP charges (more TP = more irradiated).
static void tp_palette(float fill, u32 out[3])
{
    const u32* K[4] = { TP_T0, TP_T1, TP_T2, TP_T3 };
    float c = fill * 3.0f; if (c < 0.0f) c = 0.0f; if (c > 3.0f) c = 3.0f;   // 0..3
    int i0 = (int)c; if (i0 > 2) i0 = 2;
    float tt = c - (float)i0;
    out[0] = lerp_argb(K[i0][0], K[i0 + 1][0], tt);
    out[1] = lerp_argb(K[i0][1], K[i0 + 1][1], tt);
    out[2] = lerp_argb(K[i0][2], K[i0 + 1][2], tt);
}

// a textured quad with explicit UVs and left/right vertex colours (h-gradient)
static void tquad(u32 dev, float x, float y, float w, float h,
                  float u0, float u1, float v0, float v1, u32 cL, u32 cR)
{
    Vtx q[4] = {
        { x,     y,     0, 1, cL, u0, v0 },
        { x + w, y,     0, 1, cR, u1, v0 },
        { x,     y + h, 0, 1, cL, u0, v1 },
        { x + w, y + h, 0, 1, cR, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// scale a colour's RGB by f (keep alpha) -> lighten (>1) / darken (<1).
static u32 scale_rgb(u32 c, float f)
{
    int r = (int)(((c >> 16) & 0xFF) * f); if (r > 255) r = 255; if (r < 0) r = 0;
    int g = (int)(((c >> 8)  & 0xFF) * f); if (g > 255) g = 255; if (g < 0) g = 0;
    int b = (int)(( c        & 0xFF) * f); if (b > 255) b = 255; if (b < 0) b = 0;
    return (c & 0xFF000000) | (r << 16) | (g << 8) | b;
}

// textured quad with a VERTICAL colour gradient (top vs bottom) -> liquid volume.
static void tquad_v(u32 dev, float x, float y, float w, float h,
                    float u0, float u1, float v0, float v1, u32 cTop, u32 cBot)
{
    Vtx q[4] = {
        { x,     y,     0, 1, cTop, u0, v0 },
        { x + w, y,     0, 1, cTop, u1, v0 },
        { x,     y + h, 0, 1, cBot, u0, v1 },
        { x + w, y + h, 0, 1, cBot, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// textured quad with FOUR independent corner colours (h+v gradient) + explicit UVs.
static void tquad4(u32 dev, float x, float y, float w, float h,
                   float u0, float u1, float v0, float v1,
                   u32 cTL, u32 cTR, u32 cBL, u32 cBR)
{
    Vtx q[4] = {
        { x,     y,     0, 1, cTL, u0, v0 },
        { x + w, y,     0, 1, cTR, u1, v0 },
        { x,     y + h, 0, 1, cBL, u0, v1 },
        { x + w, y + h, 0, 1, cBR, u1, v1 },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// one tinted layer of the bar at a given UV offset (uOff,vOff). Clean hard right
// edge (sharp fill level). The motion is driven by the offsets the caller passes.
static void layer(u32 dev, float x, float y, float w, float h,
                  float uScale, float vScale, float uOff, float vOff, u32 tint)
{
    tquad(dev, x, y, w, h, uOff, uOff + uScale, vOff, vOff + vScale, tint, tint);
}

// a liquid bar filled to `fill` (0..1) from the left. The liquid occupies width
// fw = w*fill; U span is scaled by fill so the texel density stays constant (the
// pattern isn't squished). The empty part is left clear -> the glass shows there.
static void draw_bar(u32 dev, float x, float y, float w, float h, float t, const u32* pal, float fill, int kind)
{
    float fw = w * fill;
    if (fw < 1.0f) return;

    // Per-resource animation personality:
    //  HP (0) : panics as it drains  -> low = fast & chaotic.
    //  MP (1) : magic fades as it drains -> low = slow & dim glow (no colour change).
    //  TP (2) : charges up -> more TP = more energetic.
    float spd, ampf, glowf;
    if (kind == 1) {                          // MP
        spd   = 0.30f + 1.70f * fill;
        ampf  = 0.50f + 0.90f * fill;
        glowf = 0.22f + 0.78f * fill;         // the magic effect fades at low MP
    } else if (kind == 2) {                   // TP
        spd   = 0.40f + 1.60f * fill;
        ampf  = 0.80f + 0.55f * fill;
        glowf = 1.0f;
    } else {                                  // HP
        float low = 1.0f - fill; if (low < 0) low = 0; if (low > 1) low = 1;
        spd   = 0.45f + 2.60f * low;
        ampf  = 1.00f + 1.30f * low;
        glowf = 1.0f;
    }
    float ts = t * spd;

    // HORIZONTAL FLOW is now the dominant motion: the liquid STREAMS along the tube.
    // constant-velocity u-scroll + WRAP addressing = endless flow (never resets) ->
    // this is the fix for the old "static blob that tremored in place".
    float flow  = t * (0.05f + 0.10f * spd);             // body flow speed (scales with personality)
    float vWob  = 0.018f * ampf * sinf(ts * 0.7f);       // gentle vertical breathing only
    // The texture is now ~8:1 (matches the bar), so we show exactly ONE tile across
    // the length (no repeat -> looks natural) and texels stay ~square (no smear).
    // uSpan scales with fill so density is constant; <=1 so the pattern never repeats.
    float uSpan = 1.0f * fill;

    // --- BODY : vertical gradient (light top / dark bottom) = volume ---
    float pul = 0.94f + 0.06f * sinf(ts * 1.6f);
    // TWO-TONE POTION gradient: BRIGHT accent (pal[2]) at the top surface, DEEP
    // saturated base (pal[0]) at the bottom -> luminous magic-fluid look. Both at the
    // body's opacity (pal[0] alpha).
    u32 aBody = pal[0] & 0xFF000000;
    if (kind == 2 && tp_tier(fill) < 1) {                // TP below 1000: more TRANSPARENT (charging), opaque at 1000
        float op = 0.32f + 0.18f * (fill / 0.3333f);     // ~0.32 empty -> ~0.50 at 999, then jumps to 1.0 at 1000
        u32 ab = (u32)(((aBody >> 24) & 0xFF) * op);
        aBody = (ab << 24) | (aBody & 0x00FFFFFF);
    }
    u32 deep  = scale_rgb(pal[0], 1.30f);                // LIFTED deep base -> tonal dips stay colourful, not heavy/black
    // MP : the blue STRENGTH tracks the MP level (full = vivid, low = faded) with NO pulse.
    // HP/TP keep the subtle body pulse.
    float bodyMul = (kind == 1) ? (0.42f + 0.58f * fill) : pul;
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    // TONAL VARIATION along the length: split the body into segments, each shaded a
    // little lighter/darker (BRIGHTNESS only -> stays exactly in the hue), drifting
    // with the flow -> the liquid has lighter/darker zones, not one flat colour. The
    // tone is sampled per segment BOUNDARY so it's continuous (no banding).
    const int NS = 12;
    const float TAU = 6.2831853f;
    float ph = (float)kind * 2.10f;                      // per-bar PHASE -> the 3 fioles never animate in sync
    for (int s = 0; s < NS; s++) {
        float fL = (float)s / NS, fR = (float)(s + 1) / NS;
        // 0..1 mix position, drifting along the length (two octaves) + per-bar phase
        float vL = 0.5f + 0.32f * sinf(fL * TAU * 1.5f + t * 0.9f + ph) + 0.18f * sinf(fL * TAU * 3.0f - t * 1.3f + ph * 1.7f);
        float vR = 0.5f + 0.32f * sinf(fR * TAU * 1.5f + t * 0.9f + ph) + 0.18f * sinf(fR * TAU * 3.0f - t * 1.3f + ph * 1.7f);
        if (vL < 0) vL = 0; if (vL > 1) vL = 1; if (vR < 0) vR = 0; if (vR > 1) vR = 1;
        // MIX THROUGH THE PALETTE'S OWN COLOURS (deep -> mid -> bright): the variation is
        // a gradient of the HUE (deep green<->bright green, deep blue<->bright, ...), all
        // saturated -> stays colourful, NEVER goes to black. Top row mid..bright, bottom
        // row deep..mid -> keeps the vertical volume too.
        u32 cTL = aBody | (scale_rgb(lerp_argb(pal[1], pal[2], vL), bodyMul) & 0x00FFFFFF);
        u32 cTR = aBody | (scale_rgb(lerp_argb(pal[1], pal[2], vR), bodyMul) & 0x00FFFFFF);
        u32 cBL = aBody | (scale_rgb(lerp_argb(deep, pal[1], vL), bodyMul) & 0x00FFFFFF);
        u32 cBR = aBody | (scale_rgb(lerp_argb(deep, pal[1], vR), bodyMul) & 0x00FFFFFF);
        float sx = x + fw * fL, sw = fw * (fR - fL);
        float su0 = flow + uSpan * fL, su1 = flow + uSpan * fR;
        tquad4(dev, sx, y, sw, h, su0, su1, vWob, vWob + 1.0f, cTL, cTR, cBL, cBR);
    }

    // --- DEPTH CAUSTIC : a fainter 2nd layer of the same texture scrolling the
    // OTHER way + a V-offset -> parallax shimmer (gives the liquid a sense of depth
    // and internal motion). alpha comes from the VERTEX (caller set ALPHAOP=
    // SELECTARG1/DIFFUSE) so it stays immune to the zoning alpha mis-sample. ---
    u32 cawT = mulA(scale_rgb(pal[1], 1.10f), 0.26f);
    u32 cawB = mulA(scale_rgb(pal[1], 0.62f), 0.26f);
    float flow2 = 0.37f - t * (0.04f + 0.07f * spd);
    tquad_v(dev, x, y, fw, h, flow2, flow2 + uSpan * 1.3f, 0.23f - vWob, 1.23f - vWob, cawT, cawB);

    // --- TRAVELING SPECULAR : a bright top-third band, ADDITIVE, scrolling FASTER
    // than the body. The texture's bright cells become light glints sliding through
    // the liquid -> the #1 "this is fluid, not a painted bar" cue. ---
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);
    float specS = t * (0.16f + 0.18f * spd);
    int sa = (int)(0x28 * glowf); if (sa > 255) sa = 255; if (sa < 0) sa = 0;
    u32 spec  = ((u32)sa << 24) | (pal[2] & 0x00FFFFFF);
    u32 spec0 = spec & 0x00FFFFFF;                        // alpha 0 -> FEATHERED ends (no hard horizontal edge)
    float su1 = specS + uSpan * 0.8f;
    tquad_v(dev, x, y + h * 0.04f, fw, h * 0.20f, specS, su1, 0.05f, 0.25f, spec0, spec);  // fade IN  (0 -> bright)
    tquad_v(dev, x, y + h * 0.24f, fw, h * 0.22f, specS, su1, 0.25f, 0.45f, spec, spec0);  // fade OUT (bright -> 0, no line)

    // --- EMISSIVE bloom (TP ONLY): the violet fluid itself GLOWS from within (additive,
    // strong, modulated by the texture), pulsing in sync with the halo. Off below 1000
    // (not WS-ready), steady throb 1000-2999, special fast flash at 3000. ---
    if (kind == 2) {
        // CONTINUOUS irradiation: the inner glow GROWS with the charge (a little even
        // below 1000 -> "matter charging"), pulsing; special fast flash at 3000.
        int tier = tp_tier(fill);
        float ep;
        if (tier >= 3) { float s = 0.5f + 0.5f * sinf(t * 5.0f); s = s * s * s; ep = 0.40f + 0.60f * s; }
        else           { ep = 0.50f + 0.50f * sinf(t * 3.0f); }
        float eb = 0x28 + 0xA8 * fill;                                  // irradiation grows with TP (0..1)
        if (tier < 1) eb *= 0.30f;                                      // dormant while charging (<1000) -> 1000 ignites
        int ea = (int)(eb * ep); if (ea > 255) ea = 255; if (ea < 0) ea = 0;
        u32 eTop = ((u32)ea           << 24) | (pal[2] & 0x00FFFFFF);   // bright accent, top
        u32 eBot = ((u32)(ea * 2 / 3) << 24) | (pal[1] & 0x00FFFFFF);   // mid, bottom
        tquad_v(dev, x, y, fw, h, flow, flow + uSpan, vWob, vWob + 1.0f, eTop, eBot);
    }

    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);  // leave a known blend state for the next bar
}

// a vertex-coloured quad (4 corner colours = a linear gradient). FVF 0x44.
static void grad_quad(u32 dev, float x, float y, float w, float h, u32 cTL, u32 cTR, u32 cBL, u32 cBR)
{
    VtxC q[4] = {
        { x,     y,     0, 1, cTL },
        { x + w, y,     0, 1, cTR },
        { x,     y + h, 0, 1, cBL },
        { x + w, y + h, 0, 1, cBR },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(VtxC));
}

// a soft coloured glow centred on the liquid surface (sx), fading both ways. FVF 0x44.
static void surface_glow(u32 dev, float sx, float y, float h, u32 c)
{
    // a thin DARK "thickness" band just behind the surface -> the liquid reads as
    // DENSER at the meniscus (alpha-blend darkens, fading to 0 away from the front).
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    grad_quad(dev, sx - 7.0f, y, 7.0f, h, 0x00000000, 0x55000000, 0x00000000, 0x55000000);
    // additive coloured glow fading both ways from the surface...
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);   // additive
    const float gw = 6.0f;
    u32 c0 = c & 0x00FFFFFF;                       // same colour, alpha 0
    grad_quad(dev, sx - gw, y, gw, h, c0, c,  c0, c);    // fade up to the surface (from the liquid side)
    grad_quad(dev, sx,      y, gw, h, c,  c0, c,  c0);   // fade away past it (into the empty glass)
    // ...plus a crisp bright CORE line = read the exact level at a glance.
    u32 core = 0x88000000u | (scale_rgb(c, 1.6f) & 0x00FFFFFF);
    grad_quad(dev, sx - 1.0f, y, 2.0f, h, core, core, core, core);
}

// a lab-vial GRADUATION : a thin mark etched into the glass, hanging from the TOP
// rim and fading downward (stays inside the fiole). FVF 0x44.
static void tick(u32 dev, float tx, float y, float h)
{
    const float tw = 3.5f, th = h * 0.34f, hx = tw * 0.5f, y0 = y + 2.0f;
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);             // engraved groove (dark, fades down)
    grad_quad(dev, tx - hx, y0, tw, th, 0x78000000, 0x78000000, 0x00000000, 0x00000000);
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                     // glass highlight on the groove edge
    grad_quad(dev, tx + hx * 0.3f, y0, 1.6f, th, 0x90FFFFFF, 0x90FFFFFF, 0x00000000, 0x00000000);
}

// soft RADIAL glow texture. The falloff is stored in the RGB (grayscale), NOT the
// alpha: a texture's alpha mis-samples as ~255 while a zone loads (see the zoning
// fix), but its RGB stays correct. Drawn ADDITIVE with COLOROP=MODULATE(tex*tint)
// and ALPHAOP=SELECTARG1(diffuse) -> edges are black (=add nothing), shape immune.
static u32 make_glow(u32 dev)
{
    const int W = 128, H = 64;
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    u32 tex = 0;
    if (!fCreate || fCreate(dev, W, H, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) < 0 || !valid_ptr(tex))
        return 0;
    struct LR { int Pitch; void* pBits; } lr = { 0, 0 };
    auto fLock = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
    if (fLock && fLock(tex, 0, &lr, 0, 0) >= 0 && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float nx = ((x + 0.5f) / W) * 2.0f - 1.0f;
                float ny = ((y + 0.5f) / H) * 2.0f - 1.0f;
                // CAPSULE / stadium falloff: flat bright straight core with SEMICIRCULAR
                // ROUNDED ends (not square). cx = straight half-length (covers the bar);
                // kx compresses the end caps in texture-x so they look ROUND once the quad
                // is stretched (~3.6x) wide on screen. Past cx the iso-contours are
                // ellipses -> rounded caps; the ny term keeps the long sides fading softly.
                const float cx = 0.72f, kx = 3.6f;
                float dx = fabsf(nx) - cx; if (dx < 0) dx = 0;
                float d  = sqrtf((dx * kx) * (dx * kx) + ny * ny);   // 0 core .. 1 edges, rounded ends
                float a  = 1.0f - d; if (a < 0) a = 0; if (a > 1) a = 1;
                a = a * (2.0f - a);                          // ease-out -> SOFT tail, melts to 0 (no abrupt stop)
                u32 av = (u32)(a * 255.0f);
                row[x] = 0xFF000000 | (av << 16) | (av << 8) | av;   // shape in RGB, alpha opaque
            }
        }
        auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);
        if (fUnlock) fUnlock(tex, 0);
    }
    return tex;
}

// a small BUBBLE sprite : faint disc + a brighter rim (reads as a bubble). 32x32.
// Shape in RGB (not alpha) so it survives zoning -- same reason as make_glow.
static u32 make_bubble(u32 dev)
{
    const int W = 32, H = 32;
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    u32 tex = 0;
    if (!fCreate || fCreate(dev, W, H, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) < 0 || !valid_ptr(tex))
        return 0;
    struct LR { int Pitch; void* pBits; } lr = { 0, 0 };
    auto fLock = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
    if (fLock && fLock(tex, 0, &lr, 0, 0) >= 0 && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float nx = ((x + 0.5f) / W) * 2.0f - 1.0f;
                float ny = ((y + 0.5f) / H) * 2.0f - 1.0f;
                float d = sqrtf(nx * nx + ny * ny);
                float body = 1.0f - d; if (body < 0) body = 0; body = body * body * 0.45f; // faint fill
                float rim = 1.0f - fabsf(d - 0.72f) * 4.5f; if (rim < 0) rim = 0;           // bright thin ring
                float a = body + rim * 0.95f; if (a > 1) a = 1; if (a < 0) a = 0;
                u32 av = (u32)(a * 255.0f);
                row[x] = 0xFF000000 | (av << 16) | (av << 8) | av;   // shape in RGB, alpha opaque
            }
        }
        auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);
        if (fUnlock) fUnlock(tex, 0);
    }
    return tex;
}

// load a raw BGRA blob (W*H*4, straight alpha) into a D3D texture.
static u32 load_raw_texture(u32 dev, const char* path, int W, int H)
{
    HANDLE hf = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hf == INVALID_HANDLE_VALUE) return 0;
    DWORD need = (DWORD)(W * H * 4), got = 0;
    char* buf = (char*)HeapAlloc(GetProcessHeap(), 0, need);
    if (!buf) { CloseHandle(hf); return 0; }
    BOOL ok = ReadFile(hf, buf, need, &got, 0);
    CloseHandle(hf);
    if (!ok || got != need) { HeapFree(GetProcessHeap(), 0, buf); return 0; }

    u32 tex = 0;
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    if (fCreate && fCreate(dev, W, H, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) >= 0 && valid_ptr(tex)) {
        struct LR { int Pitch; void* pBits; } lr = { 0, 0 };
        auto fLock = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
        if (fLock && fLock(tex, 0, &lr, 0, 0) >= 0 && lr.pBits) {
            for (int y = 0; y < H; y++) {
                u32* dpix = (u32*)((char*)lr.pBits + y * lr.Pitch);
                u32* spix = (u32*)(buf + y * W * 4);
                for (int xx = 0; xx < W; xx++) dpix[xx] = spix[xx];
            }
            auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);
            if (fUnlock) fUnlock(tex, 0);
        }
    } else tex = 0;
    HeapFree(GetProcessHeap(), 0, buf);
    return tex;
}

// a cap quad (true texture colours via white diffuse), UV 0..1, optional mirror.
static void cap_quad(u32 dev, float x, float y, float w, float h, bool flip)
{
    float u0 = flip ? 1.0f : 0.0f, u1 = flip ? 0.0f : 1.0f;
    Vtx q[4] = {
        { x,     y,     0, 1, 0xFFFFFFFF, u0, 0.0f },
        { x + w, y,     0, 1, 0xFFFFFFFF, u1, 0.0f },
        { x,     y + h, 0, 1, 0xFFFFFFFF, u0, 1.0f },
        { x + w, y + h, 0, 1, 0xFFFFFFFF, u1, 1.0f },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// a textured quad with a diffuse tint (UV 0..1). For the radial glow.
static void glow_quad(u32 dev, float x, float y, float w, float h, u32 color)
{
    Vtx q[4] = {
        { x,     y,     0, 1, color, 0.0f, 0.0f },
        { x + w, y,     0, 1, color, 1.0f, 0.0f },
        { x,     y + h, 0, 1, color, 0.0f, 1.0f },
        { x + w, y + h, 0, 1, color, 1.0f, 1.0f },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vtx));
}

// deterministic per-index pseudo-random in [0,1).
static float hash01(int n)
{
    n = (n << 13) ^ n;
    int m = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return m / 2147483647.0f;
}

// rising BUBBLES (boiling look) inside a bar's FILLED area. Additive, tinted.
// Uses g_bubble; assumes textured FVF/modulate stage already set.
static void draw_bubbles(u32 dev, float x, float y, float w, float h, float fill, float t, u32 rgb, float countMul, float sizeMul)
{
    if (!g_bubble) return;
    float fw = w * fill;
    if (fw < 6.0f) return;
    dSetTex(dev, 0, g_bubble);
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                  // additive
    dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);        // blend alpha from VERTEX (shape is in tex RGB)
    dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);            // -> immune to the zoning alpha mis-sample
    int N = (int)((2.0f + fill * 24.0f) * countMul); if (N > 28) N = 28; // MORE bubbles when fuller -> boils harder
    float spdScale = 0.35f + 0.75f * fill;                       // more vigorous boil when full
    for (int i = 0; i < N; i++) {
        float baseSz = (2.6f + hash01(i * 7 + 3) * 4.2f) * sizeMul;     // bubble base size
        float spd = (0.16f + hash01(i * 5 + 2) * 0.30f) * spdScale; // rise speed (scales with MP)
        float ph  = hash01(i * 3 + 5);
        float bx  = hash01(i * 2 + 1);                          // base x fraction
        float wob = 0.6f + hash01(i * 11 + 1) * 1.6f;
        float p = t * spd + ph; p -= floorf(p);                 // 0..1 rise (loops)
        // SWELL (grows as it climbs) + PULSE (breathes like a real bubble)
        float swell = 0.30f + 0.70f * p;
        float pulse = 1.0f + 0.22f * sinf(t * (3.0f + hash01(i * 13 + 2) * 3.0f) + i * 2.1f);
        float sz = baseSz * swell * pulse;
        // organic wobble (two frequencies) -> drifts like a bubble
        float cx = x + bx * fw + sinf(t * wob + i * 1.7f) * 6.0f + sinf(t * wob * 2.3f + i) * 2.5f;
        float cy = (y + h - baseSz) - p * (h - 2.0f * baseSz);  // rise bottom -> top
        float minx = x + baseSz, maxx = x + fw - baseSz; if (maxx < minx) maxx = minx;
        if (cx < minx) cx = minx; if (cx > maxx) cx = maxx;
        float af;                                               // fade in fast, then POP out near the top
        if (p < 0.10f) af = p / 0.10f; else if (p > 0.70f) af = (1.0f - p) / 0.30f; else af = 1.0f;
        if (af < 0) af = 0;
        int a = (int)(0x90 * af * (0.4f + 0.6f * fill)); if (a > 255) a = 255; if (a < 0) a = 0;
        u32 col = ((u32)a << 24) | (rgb & 0x00FFFFFF);
        glow_quad(dev, cx - sz, cy - sz, sz * 2.0f, sz * 2.0f, col);
    }
    dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);          // restore texture-alpha pipeline
    dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
}

// draw `tex` (back or front cap) at both ends of every bar. Caller sets blend/FVF.
static void draw_cap_pass(u32 dev, u32 tex, const float* ys, float x, float w, float h)
{
    if (!tex) return;
    dSetTex(dev, 0, tex);
    float capH = h * 1.42f, capW = capH * ((float)CAP_W / (float)CAP_H);
    for (int i = 0; i < 3; i++) {
        float cy = ys[i] + h * 0.5f - capH * 0.5f;
        cap_quad(dev, x - capW * 0.70f,     cy, capW, capH, true);    // left end (mirrored)
        cap_quad(dev, x + w - capW * 0.30f, cy, capW, capH, false);   // right end
    }
}

// outer RADIATION halo around a bar (additive gold light spilling OUTSIDE the
// glass), brightest at the bar edge and fading out. Intensity 0..1 (e.g. TP fill).
static void outer_glow(u32 dev, float x, float y, float w, float h, float intensity, float t)
{
    if (intensity <= 0.02f) return;
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);             // additive
    float pulse = 0.65f + 0.35f * sinf(t * 2.0f);
    int a = (int)(0x98 * intensity * pulse); if (a > 255) a = 255; if (a < 0) a = 0;
    u32 c  = ((u32)a << 24) | 0x00FFDC78;                  // gold (255,220,120)
    u32 c0 = c & 0x00FFFFFF;                               // same colour, alpha 0
    // Only TOP/BOTTOM radiation (the broad faces of the tube); the caps cover the
    // ends, so no left/right bands -> no square corners. Inset a touch so the
    // glow's vertical ends sit under the caps.
    const float G = 15.0f, e = w * 0.04f;
    grad_quad(dev, x + e, y - G, w - 2 * e, G, c0, c0, c,  c );    // top
    grad_quad(dev, x + e, y + h, w - 2 * e, G, c,  c,  c0, c0);    // bottom
}

// GLASS = just the FRONT FACE of a horizontal glass TUBE. Pure curvature shading,
// NO border/rim/frame. A horizontal cylinder lit from above: top & bottom edges
// curve away (darken), a bright specular band runs the full length near the top,
// a faint bounce band sits low. Every gradient fades to alpha 0 at its inner edge
// so there are no hard lines -- it reads as a smooth curved surface. FVF 0x44.
static void glass_overlay(u32 dev, float x, float y, float w, float h)
{
    // --- tinted glass BODY + cylinder curvature (alpha blend, darkens) ---
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    grad_quad(dev, x, y,             w, h,         0x14000000, 0x14000000, 0x14000000, 0x14000000); // uniform tint (light -> colour pops)
    // ONE smooth full-height darken (no gradient that STARTS mid-bar) -> removes the
    // abrupt slope change at the centre that read as a faint 1px horizontal line.
    grad_quad(dev, x, y,             w, h,         0x00000000, 0x00000000, 0x90000000, 0x90000000); // smooth curve darken to bottom (lighter)
    grad_quad(dev, x, y,             w, h * 0.40f, 0x66000000, 0x66000000, 0x00000000, 0x00000000); // soft top rim only

    // --- specular band (additive), peak ~20% from top, soft both ways, full length ---
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);
    float pk = h * 0.20f;
    grad_quad(dev, x, y,      w, pk,         0x00FFFFFF, 0x00FFFFFF, 0x6CFFFFFF, 0x6CFFFFFF); // rise to peak (softer -> less white wash)
    grad_quad(dev, x, y + pk, w, h * 0.22f,  0x6CFFFFFF, 0x6CFFFFFF, 0x00FFFFFF, 0x00FFFFFF); // fall to 0

    // --- faint bounce band low down (light reflecting off the inside) ---
    float by = y + h * 0.72f, bh = h * 0.22f;
    grad_quad(dev, x, by,             w, bh * 0.5f, 0x00FFFFFF, 0x00FFFFFF, 0x1CFFFFFF, 0x1CFFFFFF);
    grad_quad(dev, x, by + bh * 0.5f, w, bh * 0.5f, 0x1CFFFFFF, 0x1CFFFFFF, 0x00FFFFFF, 0x00FFFFFF);
}

// ============================ TP ELECTRICITY ⚡ =============================
// a thin bright SEGMENT (rectangle) from A to B, `th` px wide. Untextured diffuse
// (FVF 0x44), drawn additive by the caller. Used to build lightning bolts.
static void seg_quad(u32 dev, float ax, float ay, float bx, float by, float th, u32 col)
{
    float dx = bx - ax, dy = by - ay;
    float l = sqrtf(dx * dx + dy * dy); if (l < 0.01f) return;
    float nx = -dy / l * th * 0.5f, ny = dx / l * th * 0.5f;   // perpendicular half-width
    VtxC q[4] = {
        { ax + nx, ay + ny, 0, 1, col },
        { bx + nx, by + ny, 0, 1, col },
        { ax - nx, ay - ny, 0, 1, col },
        { bx - nx, by - ny, 0, 1, col },
    };
    dDrawUP(dev, D3DPT_TRIANGLESTRIP, 2, q, sizeof(VtxC));
}

// a SOFT (feathered) segment: alpha 0 at the two long edges, full at the centre line
// -> a glowing band instead of a hard rectangle (no "drawn in Paint" crisp edges).
static void seg_soft(u32 dev, float ax, float ay, float bx, float by, float th, u32 col)
{
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

// build a jagged bolt polyline (N+1 points) from (x0,y0) to (x1,y1) into px/py.
// Points displaced perpendicular by smoothed low-freq noise, tapered to 0 at the ends.
static void build_bolt(float x0, float y0, float x1, float y1, int seed, float amp, float* px, float* py, int N,
                       float clx0, float cly0, float clx1, float cly1)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy); if (len < 0.001f) len = 0.001f;
    float nx = -dy / len, ny = dx / len;
    for (int i = 0; i <= N; i++) {
        float tt = (float)i / N;
        float taper = sinf(tt * 3.14159265f);
        float r = (hash01(seed * 97 + (i / 2)) * 2.0f - 1.0f) * 0.44f   // gentle mid-freq bends
                + (hash01(seed * 151 + (i / 4)) * 2.0f - 1.0f) * 0.66f; // dominant LOW-freq -> smooth wave
        float off = r * amp * taper;
        float ux = x0 + dx * tt + nx * off;
        float uy = y0 + dy * tt + ny * off;
        if (ux < clx0) ux = clx0; if (ux > clx1) ux = clx1;            // CLIP to the bar frame
        if (uy < cly0) uy = cly0; if (uy > cly1) uy = cly1;
        px[i] = ux; py[i] = uy;
    }
}

// draw ONE width pass of a precomputed polyline, up to `grow` (0..1) of its length.
// `tipTaper` thins the stroke toward the tip (i=N): 0 = uniform, 0.6 = down to 40%.
static void draw_pass(u32 dev, const float* px, const float* py, int N, u32 col, float th, float grow, int reverse, float tipTaper)
{
    int drawn = (int)(grow * N + 0.999f); if (drawn < 1) drawn = 1; if (drawn > N) drawn = N;
    if (!reverse) { for (int i = 0; i < drawn; i++)          { float ww = th * (1.0f - tipTaper * ((i + 0.5f) / N)); seg_soft(dev, px[i], py[i], px[i + 1], py[i + 1], ww, col); } }
    else          { for (int i = N - 1; i >= N - drawn; i--) { float ww = th * (1.0f - tipTaper * ((i + 0.5f) / N)); seg_soft(dev, px[i], py[i], px[i + 1], py[i + 1], ww, col); } }
}

// a full LIGHTNING bolt: multi-width glow passes + RECURSIVE branches that fork off
// the path once the main bolt has grown past their fork point -> a real lightning
// TREE, not a single line. `grow` builds it pole-to-pole; `inten` scales brightness,
// `wscale` the widths, `depth` the remaining branch levels.
static void draw_lightning(u32 dev, float x0, float y0, float x1, float y1, int seed,
                           float amp, float grow, int reverse, int depth, float inten, float wscale,
                           float tipTaper, float clx0, float cly0, float clx1, float cly1)
{
    const int N = 14;
    float px[N + 1], py[N + 1];
    build_bolt(x0, y0, x1, y1, seed, amp, px, py, N, clx0, cly0, clx1, cly1);

    int a;
    a = (int)(0x1E * inten); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x004C82F0, 30.0f * wscale, grow, reverse, tipTaper);
    a = (int)(0x3A * inten); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x004078E8, 16.0f * wscale, grow, reverse, tipTaper);
    a = (int)(0x52 * inten); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x003A86FF,  7.0f * wscale, grow, reverse, tipTaper);
    a = (int)(0x6E * inten); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x0070A8FF,  3.4f * wscale, grow, reverse, tipTaper);
    a = (int)(0xF0 * inten); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x00DCEEFF,  1.5f * wscale, grow, reverse, tipTaper);

    if (depth <= 0) return;
    int   nb   = (depth >= 2) ? 3 : 2;                                 // forks at this level
    float mlen = sqrtf((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    float mdx  = (x1 - x0) / (mlen + 0.001f), mdy = (y1 - y0) / (mlen + 0.001f);
    for (int b = 0; b < nb; b++) {
        float ff = 0.26f + 0.56f * hash01(seed * 311 + b * 97);        // fork at this fraction along
        if (grow < ff) continue;                                       // only once the main bolt reaches it
        int idx = (int)(ff * N + 0.5f); if (idx < 1) idx = 1; if (idx > N - 1) idx = N - 1;
        float ox = px[idx], oy = py[idx];
        float ang  = (hash01(seed * 71 + b * 29) - 0.5f) * 1.5f;       // FORWARD-biased fork angle
        float blen = (0.16f + 0.22f * hash01(seed * 53 + b * 41)) * mlen;  // shorter sub-bolts
        float ca = cosf(ang), sa = sinf(ang);
        float ddx = mdx * ca - mdy * sa, ddy = mdx * sa + mdy * ca;
        float ex = ox + ddx * blen, ey = oy + ddy * blen;
        float bgrow = (grow - ff) / (1.0f - ff); if (bgrow > 1.0f) bgrow = 1.0f;
        // branches: thinner (taper 0.6), dimmer (inten 0.62 -> branch fading), same clip frame.
        draw_lightning(dev, ox, oy, ex, ey, seed * 7 + b * 131 + 3, amp * 0.5f, bgrow, reverse,
                       depth - 1, inten * 0.72f, wscale * 0.55f, 0.6f, clx0, cly0, clx1, cly1);
    }
}

// an electric arc ENCIRCLING the round cross-section of the horizontal glass tube:
// a VERTICAL arc from top to bottom at x=xc, curved horizontally (max bulge at the
// front = mid-height, zero at the tube edges) + a small wriggle -> reads as energy
// wrapping AROUND the cylinder. `grow`/`reverse` sweep it top->bottom or bottom->top.
static void draw_ring(u32 dev, float xc, float y, float h, float bulge, float tilt, int seed, float t,
                      float grow, int reverse, float inten, float lf,
                      float clx0, float cly0, float clx1, float cly1)
{
    const int N = 20;
    const float TAU = 6.2831853f;
    float px[N + 1], py[N + 1];
    float side = (hash01(seed * 11 + 5) > 0.5f) ? 1.0f : -1.0f;     // wrap to the front-left or front-right
    float fS = 2.0f + 2.5f * hash01(seed * 3 + 1);
    float sS = 1.2f + 1.6f * hash01(seed * 7 + 3);                  // crawl/wriggle speed (snappier)
    float ph = hash01(seed * 13 + 6) * TAU;
    for (int i = 0; i <= N; i++) {
        float vv = (float)i / N;                                    // 0 top .. 1 bottom (around the cross-section)
        float prof = sinf(vv * 3.14159265f);                        // 0 at the tube edges, 1 at the front (mid-height)
        float xoff = tilt * (vv - 0.5f)                             // DIAGONAL slant (less vertical, reads as wrapping)
                   + side * bulge * prof
                   + bulge * 0.4f * prof * sinf(vv * fS * TAU + t * sS + ph); // round bulge + wriggle
        float ux = xc + xoff;
        float uy = y + h * vv;
        if (ux < clx0) ux = clx0; if (ux > clx1) ux = clx1;
        if (uy < cly0) uy = cly0; if (uy > cly1) uy = cly1;
        px[i] = ux; py[i] = uy;
    }
    int a;
    a = (int)(0x30 * inten * lf); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x004018C8, 46.0f, grow, reverse, 0.0f); // HUGE broad VIOLET glow (in the liquid)
    a = (int)(0x4C * inten * lf); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x006A28E6, 24.0f, grow, reverse, 0.0f); // strong violet glow
    a = (int)(0x60 * inten * lf); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x00A878FF,  7.0f, grow, reverse, 0.0f); // mid violet
    a = (int)(0xB4 * inten * lf); if (a > 255) a = 255; if (a > 0) draw_pass(dev, px, py, N, ((u32)a << 24) | 0x00E2CCFF,  1.8f, grow, reverse, 0.0f); // violet-white core
}

// a soft glowing BLOB centred at (cx,cy): bright at the centre, fading to 0 in every
// direction (bilinear) -> a soft light lozenge, not a hard bar. FVF 0x44 (grad_quad).
static void soft_blob(u32 dev, float cx, float cy, float hw, float hh, u32 col)
{
    u32 c0 = col & 0x00FFFFFF;                                   // transparent at the edges
    grad_quad(dev, cx - hw, cy - hh, hw, hh, c0, c0, c0, col);   // TL quadrant (bright at centre = BR)
    grad_quad(dev, cx,      cy - hh, hw, hh, c0, c0, col, c0);   // TR (bright at BL)
    grad_quad(dev, cx - hw, cy,      hw, hh, c0, col, c0, c0);   // BL (bright at TR)
    grad_quad(dev, cx,      cy,      hw, hh, col, c0, c0, c0);   // BR (bright at TL)
}

// energy WAVES sweeping along the TP tube (unlocked at 2000): soft light blobs that
// travel left<->right -> energy coursing through the liquid. Count/brightness grow to 3000.
static void draw_tp_waves(u32 dev, float x, float y, float w, float h, float fill, float t)
{
    if (tp_tier(fill) < 2) return;                                 // only from 2000
    float fwT = w * fill; if (fwT < 12.0f) return;
    float c = (fill - 0.6667f) / 0.3333f; if (c < 0) c = 0; if (c > 1) c = 1;   // 2000 -> 3000 ramp
    int   nWaves = 2 + (int)(2.0f * c + 0.5f);                    // 2-4 waves
    float speed  = 0.10f + 0.10f * c;                            // sweep faster toward 3000
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                   // additive
    for (int k = 0; k < nWaves; k++) {
        int   dir = (k & 1) ? -1 : 1;                            // alternate sweep direction
        float p = t * speed * (float)dir + (float)k / nWaves + hash01(k * 7 + 3);
        p -= floorf(p);                                          // 0..1 position along the tube
        float wx = x + p * fwT;
        float edge = sinf(p * 3.14159265f);                      // softer near the ends
        float cy = y + h * 0.5f;
        int aG = (int)(0x2A * (0.4f + 0.6f * c) * edge); if (aG > 255) aG = 255;
        int aC = (int)(0x5E * (0.4f + 0.6f * c) * edge); if (aC > 255) aC = 255;
        u32 cG = ((u32)aG << 24) | 0x002E6CFF;                   // wide soft bloom
        u32 cC = ((u32)aC << 24) | 0x0090C8FF;                   // brighter core
        soft_blob(dev, wx, cy, 30.0f, h * 0.55f, cG);            // soft glowing wavefront (no hard bar)
        soft_blob(dev, wx, cy, 15.0f, h * 0.42f, cC);
    }
}

// rising magical CYAN sparks inside the MP liquid (same idea as the TP sparks but
// softer & slower = magical motes): they rise and fade, count/brightness grow with MP.
static void draw_mp_sparks(u32 dev, float x, float y, float w, float h, float fill, float t)
{
    float fwT = w * fill; if (fwT < 8.0f) return;
    int N = (int)(2 + 14 * fill); if (N < 2) N = 2;
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                    // additive
    for (int i = 0; i < N; i++) {
        float spd = 0.28f + 0.42f * hash01(i * 5 + 2);            // slow magical drift
        float p = t * spd + hash01(i * 3 + 1); p -= floorf(p);    // 0..1 rise
        float sx = x + (0.06f + 0.88f * hash01(i * 7 + 3)) * fwT
                     + sinf(t * (0.7f + hash01(i * 11 + 2)) + i) * 5.0f;
        float sy = (y + h - 2.0f) - p * (h - 4.0f);               // bottom -> top
        float fade = sinf(p * 3.14159265f);
        int a = (int)(0xA0 * fade * (0.35f + 0.65f * fill)); if (a > 255) a = 255;
        if (a < 4) continue;
        if (sx < x + 2.0f) sx = x + 2.0f; if (sx > x + fwT - 2.0f) sx = x + fwT - 2.0f;
        u32 col = ((u32)a << 24) | 0x0080F0FF;                    // magical cyan
        float len = 3.0f + 4.0f * hash01(i * 13 + 5);
        seg_soft(dev, sx, sy, sx, sy - len, 2.6f, col);
    }
}

// TP electricity = arcs ENCIRCLING the glass tube, drawn INSIDE the liquid (the glass
// is painted OVER them) -> reads as contained in the fiole. ONLY at 3000 (the ultimate
// state). Each arc lights the liquid LOCALLY around itself, not the whole fill.
static void draw_tp_lightning(u32 dev, float x, float y, float w, float h, float fill, float t)
{
    if (tp_tier(fill) < 2) return;                                 // from 2000, ramping up to 3000
    float fwT = w * fill; if (fwT < 12.0f) return;
    float c = (fill - 0.6667f) / 0.3333f; if (c < 0) c = 0; if (c > 1) c = 1;  // 2000 -> 3000 ramp
    int   nArcs    = (c > 0.55f) ? 2 : 1;                         // 1 small arc at 2000, 2 toward 3000
    float intenMul = 0.20f + 0.80f * c;                          // much fainter at 2000
    float curveMul = 0.40f + 0.60f * c;                          // much smaller bulge/tilt at 2000

    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                    // additive

    for (int k = 0; k < nArcs; k++) {
        float life  = (0.70f + 0.50f * hash01(k * 17 + 3)) * (1.6f - 0.6f * c);  // rarer at 2000
        float phase = t / life + hash01(k * 29 + 7) * 13.0f;
        int   strike = (int)phase;
        float fr = phase - (float)strike;
        if (fr > 0.36f) continue;
        int   seed = k * 1009 + strike * 131 + 1;
        float grow, inten;
        if (fr < 0.14f) { grow = fr / 0.14f;             inten = 0.6f + 0.4f * grow; }
        else            { grow = 1.0f;                   inten = 1.0f - (fr - 0.14f) / 0.22f; }
        if (inten < 0.0f) inten = 0.0f;
        inten *= (0.65f + 0.35f * hash01(strike * 53 + k * 7)) * intenMul;   // fainter at 2000
        int   reverse = (hash01(seed * 23 + 8) > 0.5f) ? 1 : 0;

        float xc    = x + (0.12f + 0.76f * hash01(seed * 5 + 2)) * fwT;
        float bulge = h * (0.18f + 0.16f * hash01(seed * 11 + 4)) * curveMul; // smaller at 2000
        float tilt  = (hash01(seed * 19 + 9) - 0.5f) * 1.7f * h * curveMul;

        // LOCALISED liquid impact: a soft blue glow column right where THIS arc is
        // (not the whole liquid) -> the fluid lights up around the arc only.
        int la = (int)(0x58 * inten); if (la > 255) la = 255;
        u32 lc = ((u32)la << 24) | 0x005A24DC;                    // VIOLET impact (matches the arcs, inside the liquid)
        seg_soft(dev, xc, y, xc, y + h, h * 0.85f, lc);

        draw_ring(dev, xc, y, h, bulge, tilt, seed, t, grow, reverse, inten, 1.0f, x, y, x + fwT, y + h);
    }
}

// read a MSVC std::string (0x18 bytes: buf/ptr @+0, size @+0x10, cap @+0x14).
static void read_stdstr(u32 saddr, char* out, int osz)
{
    out[0] = 0;
    u32 cap = 0, size = 0;
    if (!safe_read(saddr + 0x14, &cap) || !safe_read(saddr + 0x10, &size)) return;
    u32 src = saddr;
    if (cap >= 16) { if (!safe_read(saddr, &src)) return; }
    if (size > (u32)(osz - 1)) size = osz - 1;
    if (size > 300) return;
    __try { for (u32 i = 0; i < size; i++) out[i] = *((volatile char*)(src + i)); out[size] = 0; }
    __except (EXCEPTION_EXECUTE_HANDLER) { out[0] = 0; }
}

// Walk Hook's live plugin table and log each plugin's name + command aliases.
// Chain (from the reversed dispatcher): *(Hook+0x1cbe4c) -> +0xc = manager;
// manager+0xa14a8/0xa14ac = plugin vector; entry stride 100; +0x08 = name,
// +0x58/0x5c = alias vector<std::string> (stride 0x18).
static void dump_plugin_table()
{
    __try {
        u32 hb = (u32)GetModuleHandleA("Hook.dll");
        debug::log("--- plugin table dump (Hook base %08X) ---", hb);
        u32 P = 0; safe_read(hb + 0x1cbe4c, &P);
        u32 mgr = 0; safe_read(P + 0xc, &mgr);
        u32 begin = 0, end = 0;
        safe_read(mgr + 0xa14a8, &begin); safe_read(mgr + 0xa14ac, &end);
        debug::log("P=%08X mgr=%08X list=%08X..%08X (%d)", P, mgr, begin, end,
                   (begin && end && end > begin) ? (int)((end - begin) / 100) : -1);
        int guard = 0;
        for (u32 e = begin; e < end && guard < 40; e += 100, guard++) {
            char name[80]; read_stdstr(e + 8, name, 80);
            u32 ab = 0, ae = 0; safe_read(e + 0x58, &ab); safe_read(e + 0x5c, &ae);
            int nal = (ab && ae && ae > ab) ? (int)((ae - ab) / 0x18) : 0;
            debug::log("[%08X] name='%s'  aliases=%d", e, name, nal);
            int ag = 0;
            for (u32 a = ab; a < ae && ag < 20; a += 0x18, ag++) {
                char al[80]; read_stdstr(a, al, 80);
                debug::log("      alias='%s'", al);
            }
        }
        debug::log("--- end table dump ---");
    } __except (EXCEPTION_EXECUTE_HANDLER) { debug::log("table dump threw"); }
}

void aio_plugin_init(PluginManager host)
{
    g_host = host;
    debug::clear();
    debug::log("D3D PoC ph3: device = 0x%08X", host.service_raw(2));
    host.console().print(">>> AioTest D3D liquid -- command: //fiole hp|mp|tp 0-100 <<<");
}

// --- live HP/MP/TP fill via a polled config file (a native plugin can't receive
// // commands in Windower 4, so a tiny Lua addon writes plugins\aio_fill.txt) ---
static void set_fill(int idx, int v)
{
    if (idx < 0 || idx > 2) return;
    if (idx == 2) {                                   // TP : 0..3000
        if (v < 0) v = 0; if (v > 3000) v = 3000;
        g_fill[2] = v / 3000.0f;
    } else {                                          // HP / MP : 0..100 %
        if (v < 0) v = 0; if (v > 100) v = 100;
        g_fill[idx] = v / 100.0f;
    }
}
static void parse_one(const char* s, const char* tok, int idx)
{
    const char* p = strstr(s, tok);
    if (!p) return;
    p += 2;
    while (*p && (*p < '0' || *p > '9')) p++;
    if (!*p) return;
    int n = 0; while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; }
    set_fill(idx, n);
}
static void parse_fill_string(const char* s)
{
    if (strstr(s, "hp") || strstr(s, "mp") || strstr(s, "tp")) {
        parse_one(s, "hp", 0); parse_one(s, "mp", 1); parse_one(s, "tp", 2);   // keyword form
    } else {
        const char* p = s;                                                     // positional: HP MP TP
        for (int i = 0; i < 3; i++) {
            while (*p && (*p < '0' || *p > '9')) p++;
            if (!*p) break;
            int n = 0; while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; }
            set_fill(i, n);
        }
    }
}

void aio_plugin_render() {}   // slot 5 : unused (fills are set by the native //fiole command)

// slot 11 : incoming packets. Unused now -- the zoning glitch is fixed at the
// render level (see docs/REFERENCE.md §3c), so no zone-load detection is needed.
void aio_plugin_packet_in(u32 a, u32 b, u32 c, u32 d) { (void)a; (void)b; (void)c; (void)d; }

void aio_plugin_render6()
{
    u32 dev = g_host.service_raw(2);
    if (!valid_ptr(dev)) return;

    // If the game reset/recreated the D3D device (it does this around zoning), our
    // textures belong to the OLD device -> drop them so they rebuild on the NEW one.
    static u32 s_last_dev = 0;
    if (dev != s_last_dev) {
        if (s_last_dev) debug::log("DEV CHANGED %08X -> %08X (rebuild)", s_last_dev, dev);
        s_last_dev = dev;
        g_tex[0] = g_tex[1] = g_tex[2] = 0;                       // NULL (old device may be dead -> don't Release)
        g_cap_front = g_cap_back = g_glow = g_bubble = 0;
    }
    if (!g_tex[0]) {
        __try { for (int vv = 0; vv < 3; vv++) g_tex[vv] = make_texture(dev, vv); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        debug::log("textures -> %08X %08X %08X", g_tex[0], g_tex[1], g_tex[2]);
        if (!g_tex[0] || !g_tex[1] || !g_tex[2]) return;
    }
    if (!g_cap_front) {
        __try {
            g_cap_front = load_raw_texture(dev, CAP_FRONT, CAP_W, CAP_H);
            g_cap_back  = load_raw_texture(dev, CAP_BACK,  CAP_W, CAP_H);
            g_glow      = make_glow(dev);
            g_bubble    = make_bubble(dev);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        debug::log("caps -> front %08X back %08X glow %08X", g_cap_front, g_cap_back, g_glow);
    }

    const float x = 1000.0f, w = 560.0f, h = 70.0f;
    float t = (float)(GetTickCount() % 1000000) / 1000.0f;

    __try {
        u32 tok = dCreateSB(dev, D3DSBT_ALL);

        dSetVS(dev, FVF_XYZRHW_DIFFUSE_TEX1);
        // texture is set per bar below (each bar shows a different variant)
        dSetRS(dev, D3DRS_ZENABLE, 0);
        dSetRS(dev, D3DRS_CULLMODE, D3DCULL_NONE);
        dSetRS(dev, D3DRS_LIGHTING, 0);
        // neutralise the game's environment so it can't tint our quads (fog/specular/
        // alpha-test during zoning etc.). The state block restores all this afterwards.
        dSetRS(dev, D3DRS_FOGENABLE, 0);
        dSetRS(dev, D3DRS_SPECULARENABLE, 0);
        dSetRS(dev, D3DRS_ALPHATESTENABLE, 0);
        dSetRS(dev, D3DRS_COLORWRITEENABLE, 0x0000000F);   // write RGBA
        dSetRS(dev, D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
        dSetRS(dev, D3DRS_BLENDOP, D3DBLENDOP_ADD);        // force ADD (game may leave MIN/MAX/SUBTRACT mid-load)
        dSetRS(dev, D3DRS_WRAP0, 0);                       // no cylindrical wrap on our tiled UVs
        dSetRS(dev, D3DRS_ALPHABLENDENABLE, 1);
        dSetRS(dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        // final = texture * diffuse  (both colour and alpha)
        dSetTSS(dev, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        dSetTSS(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        dSetTSS(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        dSetTSS(dev, 0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP);
        dSetTSS(dev, 0, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP);
        // force our own texture filtering so the game's filter state (esp. MIP, which
        // mangles our single-mip textures while a zone loads) can't affect our sampling.
        dSetTSS(dev, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        dSetTSS(dev, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        dSetTSS(dev, 0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
        dSetTSS(dev, 0, D3DTSS_TEXCOORDINDEX, 0);                     // use our UV set, no game transform
        dSetTSS(dev, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        dSetTSS(dev, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);              // kill stage 1 (game may leave multitexturing on)
        dSetTSS(dev, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

        const float ys[3]    = { 520.0f, 634.0f, 748.0f };   // spaced so the caps don't touch
        const u32*  pals[3]  = { PAL_HP, PAL_MP, PAL_TP };
        const float toff[3]  = { 0.0f, 13.7f, 27.3f };   // per-bar time offset -> desynced motion

        // BACK cap rim (behind the liquid). CLAMP addressing for cap art.
        dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        draw_cap_pass(dev, g_cap_back, ys, x, w, h);
        dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);   // wrap again for the liquid
        dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);

        // TP radiation : a rounded radial glow BEHIND the liquid (additive, gold).
        {
            int tt = tp_tier(g_fill[2]);
            float gIc = (tt < 1) ? 0.0f : (0.45f + 0.55f * g_fill[2]);   // off <1000, grows CONTINUOUSLY with the charge
            if (g_layers >= 4 && g_glow && gIc > 0.02f) {
                // 1000-2999 = "WS ready": ONE steady throb, the SAME for both tiers.
                // 3000 = "Aftermath ready" (kept up almost always): a SPECIAL fast, sharp
                // FLASHING pulse with a distinct rhythm -> instantly readable.
                // Phase the HALO to LAG the liquid's emissive pulse -> the halo arrives
                // toward the END of the liquid's pulse peak (liquid charges, then halo
                // bursts). Share the liquid's time base (t + toff[2]) for a stable phase.
                float tg = t + toff[2];
                float gp;
                if (tt >= 3) {
                    float s = 0.5f + 0.5f * sinf((tg - 0.14f) * 5.0f); s = s * s * s;  // flash, lagging the liquid flash
                    gp = 0.32f + 0.52f * s;
                } else {
                    gp = 0.52f + 0.46f * sinf(tg * 3.0f - 1.0f);            // peak ~1 rad AFTER the liquid -> end of its peak
                }
                int ga = (int)(0xCC * gIc * gp); if (ga > 255) ga = 255; if (ga < 0) ga = 0;
                u32 gdyn[3]; tp_palette(g_fill[2], gdyn);                // glow tint follows the continuous palette
                // toward 3000 the halo shifts from violet to ELECTRIC BLUE (charged state).
                float bblend = (g_fill[2] - 0.6667f) / 0.3333f; if (bblend < 0) bblend = 0; if (bblend > 1) bblend = 1;
                u32 haloRGB = lerp_argb(gdyn[2] & 0x00FFFFFF, 0x0090DCFF, bblend) & 0x00FFFFFF;
                u32 gc = ((u32)ga << 24) | haloRGB;
                dSetTex(dev, 0, g_glow);
                dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);             // additive
                dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);   // blend alpha from VERTEX (shape in tex RGB)
                dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);       // -> immune to the zoning alpha mis-sample
                const float mx = 46.0f, my = 12.0f;                    // much less vertical spill above/below the bar
                float fwT = w * g_fill[2];                              // glow only over the FILLED part
                int reps = 1;                                          // no stacking -> 3000 peak no longer towers
                for (int r = 0; r < reps; r++)
                    glow_quad(dev, x - mx, ys[2] - my, fwT + 2 * mx, h + 2 * my, gc);
                dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);     // restore texture-alpha pipeline
                dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
                dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }

        // keep our MANAGED fluid textures resident: the game evicts them from VRAM
        // under pressure while loading a zone, so they'd sample wrong (alpha/lum).
        // PreLoad (IDirect3DResource8 vtbl[9]) re-homes them before we draw.
        for (int i = 0; i < 3; i++) {
            auto fPre = vmethod<void(__stdcall*)(u32)>(g_tex[i], 9);
            if (fPre) __try { fPre(g_tex[i]); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // liquid bars (each a different cell layout, desynced)
        dSetTSS(dev, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        dSetTSS(dev, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        // ZONING-GLITCH FIX (see docs/REFERENCE.md §3c): a texture's ALPHA channel
        // mis-samples as ~255 while a zone loads (its RGB stays correct), so a
        // texture-alpha-blended liquid renders too opaque/bright until the load ends.
        // Take the liquid's blend alpha from the VERTEX instead. RGB stays MODULATE
        // (texture * diffuse). Caps keep texture alpha (they need it for shape and
        // are stable: their alpha is at the safe 0/255 extremes).
        dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        for (int i = 0; i < 3; i++) {
            const u32* pal = pals[i];
            u32 dyn[3];
            if      (i == 0) { hp_palette(g_fill[0], dyn); pal = dyn; }   // HP: green/orange/red
            else if (i == 2) { tp_palette(g_fill[2], dyn); pal = dyn; }   // TP: 3 tiers (1000/2000/3000)
            dSetTex(dev, 0, g_tex[i]);
            draw_bar(dev, x, ys[i], w, h, t + toff[i], pal, g_fill[i], i);
        }
        dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);   // restore texture-alpha for bubbles/caps
        dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

        // RISING BUBBLES inside the liquid (textured, under the glass = in the fiole):
        //  MP = boiling magic bubbles (cyan, more as it fills);
        //  TP = small AIR bubbles rising along the tube, from 2000 (fewer & smaller).
        if (g_layers >= 4)
            draw_bubbles(dev, x, ys[1], w, h, g_fill[1], t, 0x00A0E8FF, 1.0f, 1.0f);   // MP : boiling magic bubbles

        // --- GLASS : untextured, pure diffuse geometry ---
        dSetVS(dev, FVF_XYZRHW_DIFFUSE);
        dSetTex(dev, 0, 0);
        dSetTSS(dev, 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
        dSetTSS(dev, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

        // --- TP ELECTRICITY drawn UNDER the glass (so it reads as INSIDE the fiole) ---
        // sparks (>=2000) + encircling arcs (3000). FVF 0x44 / SELECTARG1-DIFFUSE is set.
        if (g_layers >= 4) {
            draw_tp_lightning(dev, x, ys[2], w, h, g_fill[2], t);  // TP : encircling arcs (ONLY 3000)
        }
        // UNLOCK FLASH: a bright blue-white pulse over the TP fill when you cross a WS
        // threshold (1000/2000/3000) upward -> satisfying "tier unlocked" feedback.
        {
            static int s_prevTier = -1; static float s_flash = 0.0f; static u32 s_lastDev = 0;
            int curTier = tp_tier(g_fill[2]);
            if (dev != s_lastDev) { s_prevTier = curTier; s_flash = 0.0f; s_lastDev = dev; }
            if (s_prevTier >= 0 && curTier > s_prevTier) s_flash = 1.0f;     // crossed a threshold up
            s_prevTier = curTier;
            if (s_flash > 0.0f) {
                float fwT = w * g_fill[2];
                int fa = (int)(0xC0 * s_flash * s_flash); if (fa > 255) fa = 255;
                u32 c  = ((u32)fa << 24) | 0x00C8E0FF;                       // bright blue-white
                u32 c0 = c & 0x00FFFFFF;
                dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);
                grad_quad(dev, x, ys[2],            fwT, h * 0.5f, c0, c0, c,  c );
                grad_quad(dev, x, ys[2] + h * 0.5f, fwT, h * 0.5f, c,  c,  c0, c0);
                s_flash -= 0.045f; if (s_flash < 0.0f) s_flash = 0.0f;       // ~0.35s decay
            }
        }

        for (int i = 0; i < 3; i++) {
            glass_overlay(dev, x, ys[i], w, h);

            // 3000 = the GLASS itself is CHARGED with electricity: a flickering electric-blue
            // rim glow on the top & bottom edges + a faint blue sheen over the whole tube.
            if (i == 2 && tp_tier(g_fill[2]) >= 3) {
                // the charge clings to the glass EDGES (Saint-Elmo's fire) with an erratic
                // electric flicker -> bright crackling rim lines + soft inward glow.
                float fl = 0.40f + 0.30f * sinf(t * 12.0f) + 0.20f * sinf(t * 29.0f + 2.1f);
                fl *= 0.65f + 0.70f * hash01((int)(t * 28.0f) * 7 + 3);              // stepped crackle
                if (fl < 0.10f) fl = 0.10f; if (fl > 1.0f) fl = 1.0f;
                float fwT = w * g_fill[2];
                dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_ONE);                          // additive
                int ag = (int)(0x26 * fl); if (ag > 255) ag = 255;     // subtle / transparent
                int ac = (int)(0x5C * fl); if (ac > 255) ac = 255;
                u32 cg = ((u32)ag << 24) | 0x002C9CFF, cg0 = cg & 0x00FFFFFF;        // soft inward glow
                u32 cc = ((u32)ac << 24) | 0x0090DCFF;                               // bright crackling edge line
                grad_quad(dev, x, ys[2],             fwT, h * 0.16f, cg,  cg,  cg0, cg0); // top: glow fades inward
                grad_quad(dev, x, ys[2],             fwT, 2.0f,      cc,  cc,  cc,  cc ); // top: bright edge line
                grad_quad(dev, x, ys[2] + h * 0.84f, fwT, h * 0.16f, cg0, cg0, cg,  cg ); // bottom: glow
                grad_quad(dev, x, ys[2] + h - 2.0f,  fwT, 2.0f,      cc,  cc,  cc,  cc ); // bottom: bright edge line
            }

            // meniscus : a bright line at the liquid surface = read the level at a glance
            float fwi = w * g_fill[i];
            if (fwi > 2.0f && fwi < w - 1.0f) {
                u32 mp[3]; const u32* pl = pals[i];
                if      (i == 0) { hp_palette(g_fill[0], mp); pl = mp; }
                else if (i == 2) { tp_palette(g_fill[2], mp); pl = mp; }
                u32 men = 0x40000000u | (scale_rgb(pl[1], 1.3f) & 0x00FFFFFF);
                surface_glow(dev, x + fwi, ys[i], h, men);   // locked at the EXACT fill level (no jitter)
            }
        }

        // FRONT cap (over everything). Back to textured FVF + modulate + clamp.
        dSetVS(dev, FVF_XYZRHW_DIFFUSE_TEX1);
        dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        dSetTSS(dev, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
        dSetTSS(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        dSetTSS(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        dSetTSS(dev, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        dSetTSS(dev, 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
        dSetTSS(dev, 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
        draw_cap_pass(dev, g_cap_front, ys, x, w, h);

        if (tok) { dApplySB(dev, tok); dDelSB(dev, tok); }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        static bool logged = false;
        if (!logged) { logged = true; debug::log("D3D draw threw (SEH)"); }
    }
}

void aio_plugin_unload()
{
    u32 all[7] = { g_tex[0], g_tex[1], g_tex[2], g_cap_front, g_cap_back, g_glow, g_bubble };
    for (int i = 0; i < 7; i++) {
        if (all[i]) {
            auto fRelease = vmethod<long(__stdcall*)(u32)>(all[i], 2);
            if (fRelease) __try { fRelease(all[i]); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }
    g_tex[0] = g_tex[1] = g_tex[2] = 0; g_cap_front = 0; g_cap_back = 0; g_glow = 0; g_bubble = 0;
}

// slot 7 HandleCommand (kept in case a future Windower routes it; today it does
// not fire for native plugins, so the file poll above is the real path).
void aio_plugin_command(const char* cmd)
{
    if (!cmd) return;
    // lower-case a copy so sub-commands/keywords are case-insensitive
    // (//AIO FIOLE HP 50 == //aio fiole hp 50). The "fiole" sub-command word is
    // harmlessly ignored by the fill parser (no digits, no hp/mp/tp).
    char buf[256]; int i = 0;
    for (; cmd[i] && i < 255; i++) { char c = cmd[i]; buf[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
    buf[i] = 0;
    const char* lp = strstr(buf, "lay");                 // //aio lay N  -> liquid effect layers (1 = liquid only; >=4 adds bubbles/halo)
    if (lp) {
        const char* p = lp + 3; while (*p && (*p < '0' || *p > '9')) p++;
        if (*p) { int n = *p - '0'; if (n < 0) n = 0; if (n > 4) n = 4; g_layers = n; }
        return;
    }
    parse_fill_string(buf);
}

const char* aio_plugin_name()        { return "AioTest"; }
// NB: Windower uses GetDescription (lowercased) as the plugin's CONSOLE COMMAND
// alias. So this string IS the //command name -> keep it a single clean word.
const char* aio_plugin_description() { return "aio"; }
