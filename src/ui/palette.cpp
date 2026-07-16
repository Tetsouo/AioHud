// palette.cpp -- see palette.h.
#include "palette.h"
#include "gfx/color.h"

namespace aio {

// VIVID liquid palettes (saturated to punch through the glass shading). Each =
// {DEEP base (bottom), mid (caustic), BRIGHT accent (top/specular)}.
const u32 PAL_HP[3] = { 0xE0148C2D, 0x965ADC5A, 0xC8A0FF50 };  // HP  deep emerald -> bright LIME
const u32 PAL_MP[3] = { 0xE0005ADC, 0x9628B4FF, 0xC88CF5FF };  // MP  vivid MAGICAL CYAN
const u32 PAL_TP[3] = { 0xE05A0FBE, 0x969632F0, 0x88CD6EFF };  // TP  deep violet -> bright MAGENTA-violet

// HP changes colour with its level: >=75% full green, ORANGE through the 25-75%
// undead-aggro zone, RED below 25% (matches the game's red HP number at 25%).
static const u32 HP_YELLOW[3] = { 0xE0C88200, 0x96EBC828, 0xC8FFF05A };  // 25-75% deep amber -> bright YELLOW
static const u32 HP_RED[3]    = { 0xE0960A0A, 0x96E62828, 0xC8FF5A46 };  // <=25%   deep blood -> bright RED

// TP weaponskill tiers as a deep->bright violet gradient, DULL -> VIVID as it charges.
static const u32 TP_T0[3] = { 0xA032284B, 0x605F5082, 0x208C7DA5 };  // 0-999     dull greyed violet (charging, no glow)
static const u32 TP_T1[3] = { 0xD8461496, 0x96823CD2, 0xC8B478F5 };  // 1000-1999 muted violet, waking up (WS1)
static const u32 TP_T2[3] = { 0xE05A0FBE, 0x969632F0, 0xC8CD6EFF };  // 2000-2999 vivid violet (WS2)
static const u32 TP_T3[3] = { 0xF06E0AE1, 0xB4AA28FF, 0xCCEB8CFF };  // 3000      ELECTRIC violet, max vivid (WS3)

int tp_tier(float fill)
{
    int tp = (int)(fill * 3000.0f + 0.5f);
    return (tp >= 3000) ? 3 : (tp >= 2000) ? 2 : (tp >= 1000) ? 1 : 0;
}

void hp_palette(float fill, u32 out[3])
{
    // Hard 3-zone switch (no gradient): green >75%, ORANGE 25-75%, RED <=25%.
    const u32* p = (fill > 0.75f) ? PAL_HP : (fill > 0.25f) ? HP_YELLOW : HP_RED;
    out[0] = p[0]; out[1] = p[1]; out[2] = p[2];
}

void tp_palette(float fill, u32 out[3])
{
    // interpolate smoothly through the 4 anchor palettes (0 / 1000 / 2000 / 3000)
    // -> colour & opacity ramp up continuously as TP charges (more TP = more irradiated).
    const u32* K[4] = { TP_T0, TP_T1, TP_T2, TP_T3 };
    float c = fill * 3.0f; if (c < 0.0f) c = 0.0f; if (c > 3.0f) c = 3.0f;   // 0..3
    int i0 = (int)c; if (i0 > 2) i0 = 2;
    float tt = c - (float)i0;
    out[0] = lerp_argb(K[i0][0], K[i0 + 1][0], tt);
    out[1] = lerp_argb(K[i0][1], K[i0 + 1][1], tt);
    out[2] = lerp_argb(K[i0][2], K[i0 + 1][2], tt);
}

} // namespace aio
