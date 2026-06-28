// color.h -- ARGB colour math used by the renderer (scale / fade / interpolate).
// Colours are packed 0xAARRGGBB. Header-only (tiny, used everywhere).
#pragma once
#include "d3d.h"

namespace aio {

// scale a colour's RGB by f (keep alpha) -> lighten (>1) / darken (<1).
inline u32 scale_rgb(u32 c, float f) {
    int r = (int)(((c >> 16) & 0xFF) * f); if (r > 255) r = 255; if (r < 0) r = 0;
    int g = (int)(((c >> 8)  & 0xFF) * f); if (g > 255) g = 255; if (g < 0) g = 0;
    int b = (int)(( c        & 0xFF) * f); if (b > 255) b = 255; if (b < 0) b = 0;
    return (c & 0xFF000000) | (r << 16) | (g << 8) | b;
}

// scale only the ALPHA byte by f (keep RGB).
inline u32 mulA(u32 c, float f) { u32 a = (u32)(((c >> 24) & 0xFF) * f); if (a > 255) a = 255; return (a << 24) | (c & 0xFFFFFF); }

// linear-interpolate one byte channel (at bit shift `sh`) between a and b.
inline u32 lerp_ch(u32 a, u32 b, int sh, float t) {
    int ca = (a >> sh) & 0xFF, cb = (b >> sh) & 0xFF;
    int c = (int)(ca + (cb - ca) * t + 0.5f);
    if (c < 0) c = 0; if (c > 255) c = 255;
    return (u32)c << sh;
}

// linear-interpolate a full ARGB colour.
inline u32 lerp_argb(u32 a, u32 b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    return lerp_ch(a, b, 0, t) | lerp_ch(a, b, 8, t) | lerp_ch(a, b, 16, t) | lerp_ch(a, b, 24, t);
}

} // namespace aio
