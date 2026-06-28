// gamestate.h -- the data the HUD reads to draw itself.
//
// This is the SEAM between "where the numbers come from" and "how they look".
// Today the fills are pushed in by the //aio command (see plugin/aiohud.cpp).
// When the ffxi data interface (host vtbl[7]) is reversed, a poller will fill the
// same struct from live game data and nothing in the rendering layer changes.
#pragma once

namespace aio {

struct GameState {
    float hp = 0.50f;   // 0..1  (fraction of max HP)
    float mp = 0.25f;   // 0..1  (fraction of max MP)
    float tp = 0.80f;   // 0..1  (TP / 3000)
};

} // namespace aio
