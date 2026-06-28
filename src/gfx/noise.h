// noise.h -- tileable value noise + a per-index PRNG, for procedural textures
// and animated effects. All functions are seamless at the unit cell so textures
// tile without a visible seam.
#pragma once

namespace aio {

// deterministic hash -> [0,1).
float hash2(int i, int j);

// smooth value noise on a hash lattice, TILEABLE at grid resolution G.
float vnoise(float u, float v, int G);

// fractal sum of vnoise (3 octaves) -> cloudy organic, tileable.
float fbm(float u, float v);

// deterministic per-index pseudo-random in [0,1) (for particles / bolts).
float hash01(int n);

} // namespace aio
