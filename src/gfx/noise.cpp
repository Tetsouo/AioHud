// noise.cpp -- see noise.h.
#include "noise.h"
#include <math.h>

namespace aio {

// fast INTEGER hash -> [0,1). Was a sinf()-based hash : at 60 fbm-sinf per nebula pixel that cost
// ~15M sinf calls for a 512^2 field (the //aio config open freeze). This is ~10x cheaper and sinf-free.
float hash2(int i, int j)
{
    unsigned int n = (unsigned int)(i * 374761393 + j * 668265263);
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= (n >> 16);
    return (float)(n & 0x00FFFFFFu) / (float)0x01000000;
}

// hashed lattice value, wrapped to [0,G) on each axis -> the noise tiles at G.
static float vhash(int xi, int yi, int G) { xi = ((xi % G) + G) % G; yi = ((yi % G) + G) % G; return hash2(xi + 1, yi * 3 + 1); }

float vnoise(float u, float v, int G)
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

float fbm(float u, float v)
{
    return 0.55f * vnoise(u, v, 4) + 0.30f * vnoise(u, v, 8) + 0.15f * vnoise(u, v, 16);
}

float hash01(int n)
{
    n = (n << 13) ^ n;
    int m = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return m / 2147483647.0f;
}

} // namespace aio
