// text_style.h -- the ONE implementation of "resolve a TextStyle for drawing", shared by every module.
// Each module used to re-declare an identical <m>_font/_sz/_ow/_col quartet over its own ui_config().<m>Text[]
// array (~30 copies). Now those are one-line wrappers around te_* : the font-resolution / size / outline /
// colour logic lives here once. Behaviour is byte-for-byte what the per-module copies did.
#pragma once
#include "gfx/font.h"
#include "model/ui_config.h"   // TextStyle, ui_font_face
#include "ui/widget.h"         // Frame

namespace aio {

// the element's Font : configured face (0 = layout default), 700/400 weight from bold, italic. Null-safe fallback.
inline Font* te_font(const Frame& f, const TextStyle& t) {
    if (!f.fonts) return f.font;
    Font* r = f.fonts->get(t.face > 0 ? ui_font_face(t.face) : 0, t.bold ? 700 : 400, t.italic);
    return r ? r : f.font;
}
inline float te_sz (const TextStyle& t, float base) { return base * t.size; }         // scaled base size
inline float te_ow (const TextStyle& t, float base) { return base * t.outline; }      // scaled outline width
inline u32   te_col(const TextStyle& t, u32 base)   { return t.colorOn ? t.color : base; }   // custom colour override
// UPPERCASE `s` into `buf` (cap = buffer size incl. NUL) when the element's style wants CAPS ; else return `s`
// unchanged. Same ASCII a-z -> A-Z transform every module's per-element "_up" helper used to inline.
inline const char* te_upper(const TextStyle& t, const char* s, char* buf, int cap) {
    if (!t.upper || !s) return s;
    int i = 0; for (; s[i] && i < cap - 1; ++i) { char c = s[i]; buf[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; } buf[i] = 0; return buf;
}

} // namespace aio
