// ui_config.h -- live, user-editable HUD settings (edited by the //aio config page, read by the
// widgets + Hud each frame). A process-wide singleton, like party(). Start small (the Party/Alliance
// basics) ; grow as more controls are added. Not yet persisted to disk.
#pragma once

namespace aio {

struct UiConfig {
    // ---- Party / Alliance ----
    int   skinTheme  = 0;      // window-skin theme index (the Hud applies it -> all boxes)
    int   fontFace   = 0;      // 0 = layout default ; >0 = override every party/alliance text face
    float partyScale = 1.0f;   // size multiplier on the party/alliance boxes (font + geometry)
};

UiConfig& ui_config();         // the singleton

// selectable font faces for the Font control (index 0 = "Default" = keep the layout font).
int         ui_font_count();
const char* ui_font_label(int idx);   // display label
const char* ui_font_face(int idx);    // GDI face name ("" for Default)

} // namespace aio
