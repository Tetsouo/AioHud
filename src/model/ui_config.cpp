// ui_config.cpp -- see ui_config.h.
#include "model/ui_config.h"

namespace aio {

UiConfig& ui_config() { static UiConfig c; return c; }

// index 0 = Default (keep the layout face). The rest are common Windows GDI faces.
static const char* FACE_LABEL[] = {
    "Default", "Segoe UI", "Arial", "Tahoma", "Verdana", "Calibri", "Trebuchet MS", "Consolas", "Georgia",
};
static const char* FACE_NAME[] = {
    "",        "Segoe UI", "Arial", "Tahoma", "Verdana", "Calibri", "Trebuchet MS", "Consolas", "Georgia",
};
static const int NFACE = (int)(sizeof(FACE_LABEL) / sizeof(FACE_LABEL[0]));

int         ui_font_count()        { return NFACE; }
const char* ui_font_label(int i)   { return (i >= 0 && i < NFACE) ? FACE_LABEL[i] : ""; }
const char* ui_font_face(int i)    { return (i >= 0 && i < NFACE) ? FACE_NAME[i]  : ""; }

} // namespace aio
