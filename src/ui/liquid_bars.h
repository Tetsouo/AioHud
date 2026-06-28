// liquid_bars.h -- the three animated liquid gauges (HP / MP / TP).
//
// This is the first real HUD widget: the "vial" look proven in the sandbox, now a
// self-contained Widget. It owns its liquid/cap/glow/bubble textures, reads its
// fills from the shared GameState, and draws the full composition (back caps, TP
// halo, liquid bars, bubbles, glass, electricity, front caps) inside one frame.
//
// `layers` is the effect level: 1 = liquid only, >=4 = bubbles + halo + arcs (the
// full look). It is the only knob the //aio command exposes today.
#pragma once
#include "widget.h"

namespace aio {

struct GameState;

class LiquidBars : public Widget {
public:
    // default origin = the proven sandbox position, so the bars look unchanged if no
    // layout is applied. The HUD overrides px_/py_ from the descriptor via set_place().
    explicit LiquidBars(const GameState* state) : state_(state) { px_ = 1000.0f; py_ = 520.0f; }

    // this widget IS the HP/MP/TP fioles of the Player Hub (placeholder for the full hub).
    const char* type_name() const override { return "PlayerHub"; }
    // content size of the 3 stacked bars (3*70 body + 2*44 gap) -> used for anchoring.
    void measure(float& w, float& h) const override { w = 560.0f; h = 298.0f; }

    void ensure(u32 dev) override;
    void draw(const Frame& f) override;
    void on_device_lost() override;
    void dispose() override;

    void set_layers(int n) { if (n < 0) n = 0; if (n > 4) n = 4; layers_ = n; }
    int  layers() const { return layers_; }

private:
    const GameState* state_;
    int layers_ = 4;                 // 1 = liquid only; >=4 = full effects

    u32 tex_[3]   = { 0, 0, 0 };     // per-bar liquid textures (HP / MP / TP)
    u32 cap_front_ = 0, cap_back_ = 0, glow_ = 0, bubble_ = 0;

    // TP "tier unlocked" flash (was function-static in the sandbox render).
    int   flash_prev_tier_ = -1;     // -1 = uninitialised (first frame on this device)
    float flash_ = 0.0f;
};

} // namespace aio
