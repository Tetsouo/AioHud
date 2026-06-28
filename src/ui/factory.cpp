// factory.cpp -- see factory.h.
#include "ui/factory.h"
#include "ui/liquid_bars.h"
#include "ui/party.h"

namespace aio {

Widget* make_widget(const std::string& type, const GameState* state) {
    if (type == "PlayerHub") return new LiquidBars(state);   // HP/MP/TP fioles (placeholder for the full hub)
    if (type == "PartyList")    return new Party();          // main party list (text + bars)
    if (type == "AllianceList") return new Party();          // alliance party box (same widget, tier set via config)
    // TODO: "TargetBar", "HateList", "VanaClock", ... as each is implemented.
    return nullptr;                                          // unknown/unimplemented -> HUD skips it
}

} // namespace aio
