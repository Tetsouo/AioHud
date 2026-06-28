// config_page.h -- the full-screen, web-style configuration overlay (//aio config).
//
// Drawn on TOP of everything by the HUD when open : a dimmed screen + a centred page panel
// (FFXI window skin) with TABS (Configuration / Profil / Help) and, in Configuration, a LEFT
// sidebar of config sections (Party/Alliance ; more later : treasure pool, hate list, ...).
// Interaction (mouse clicks on tabs / sections / controls) is wired separately -- this is the
// layout + state. Tabs/sections can also be driven from //aio config <args>.
#pragma once
#include "ui/widget.h"   // Frame

namespace aio {

class ConfigPage {
public:
    void toggle()         { open_ = !open_; }
    void set_open(bool o) { open_ = o; }
    bool is_open() const  { return open_; }
    void set_tab(int t);
    void set_section(int s);

    // draw the overlay for the current frame (screen size in the HUD coord space). No-op if closed.
    void draw(const Frame& f, float sw, float sh);

private:
    bool open_    = false;
    int  tab_     = 0;   // 0 = Configuration, 1 = Profil, 2 = Help
    int  section_ = 0;   // Configuration sidebar : 0 = Party/Alliance (more later)
};

} // namespace aio
