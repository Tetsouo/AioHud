// config_page.cpp -- see config_page.h.
#include "ui/config_page.h"
#include "gfx/draw.h"      // grad_quad
#include "gfx/font.h"
#include "gfx/window.h"

namespace aio {

static inline float snap(float v) { return (float)(int)(v + 0.5f); }
static inline u32   solid(u32 c)  { return c; }

static const char* TABS[]     = { "Configuration", "Profil", "Help" };
static const int   NTABS      = 3;
static const char* SECTIONS[] = { "Party / Alliance" };
static const int   NSECT      = 1;

// palette (ARGB)
static const u32 C_TEXT   = 0xFFFFFFFF;
static const u32 C_DIM    = 0xFFB8C2D0;
static const u32 C_MUTE   = 0xFF8FA0B8;
static const u32 C_STROKE = 0xFF000000;
static const u32 C_ACCENT = 0xFF2E78D6;   // active tab / selected row

void ConfigPage::set_tab(int t)     { if (t >= 0 && t < NTABS) tab_ = t; }
void ConfigPage::set_section(int s) { if (s >= 0 && s < NSECT) section_ = s; }

static void flat(u32 dev, float x, float y, float w, float h, u32 c) {
    grad_quad(dev, x, y, w, h, c, c, c, c);
}

void ConfigPage::draw(const Frame& f, float sw, float sh) {
    if (!open_) return;
    u32 dev = f.dev;
    Font* fo = f.font;
    if (!fo || !fo->ready() || sw <= 0 || sh <= 0) return;

    // 1) dim the whole screen behind the page
    flat(dev, 0, 0, sw, sh, 0xCC05070C);

    // 2) the page panel : FULL SCREEN, drawn with the FFXI window skin
    const float px = 0.0f, py = 0.0f;
    const float pw = sw,   ph = sh;
    if (f.skin && f.skin->ready()) draw_window(dev, *f.skin, px, py, pw, ph, 0xFFFFFFFF, 1.0f);
    else { flat(dev, px, py, pw, ph, 0xF00E1422); flat(dev, px, py, pw, 2, 0x80FFFFFF); }

    const float pad = snap(22.0f);

    // 3) title
    fo->begin(dev);
    fo->draw_lc(dev, px + pad, py + snap(24.0f), "AioHUD  \xC2\xB7  Configuration", snap(22.0f), C_TEXT, C_STROKE, 1.6f);

    // 4) tab bar
    const float tabY = py + snap(48.0f), tabH = snap(30.0f), tabW = snap(150.0f), tabGap = snap(6.0f);
    for (int i = 0; i < NTABS; ++i) {
        const float tx = px + pad + i * (tabW + tabGap);
        const bool active = (i == tab_);
        flat(dev, tx, tabY, tabW, tabH, active ? C_ACCENT : 0x33FFFFFF);
        if (active) flat(dev, tx, tabY + tabH - snap(2.0f), tabW, snap(2.0f), 0xFFBFE0FF);   // underline
        fo->begin(dev);
        fo->draw_c(dev, tx + tabW * 0.5f, tabY + tabH * 0.5f, TABS[i], snap(15.0f), active ? C_TEXT : C_DIM, C_STROKE, 1.0f);
    }

    // 5) divider under the tabs
    const float bodyY = tabY + tabH + snap(12.0f);
    flat(dev, px + snap(14.0f), bodyY - snap(8.0f), pw - snap(28.0f), 1, 0x40FFFFFF);

    const float bottomY = py + ph - pad;

    if (tab_ == 0) {
        // --- Configuration : left sidebar of sections + content on the right ---
        const float sbX = px + pad, sbY = bodyY, sbW = snap(210.0f), sbH = bottomY - bodyY;
        flat(dev, sbX, sbY, sbW, sbH, 0x33000000);
        for (int i = 0; i < NSECT; ++i) {
            const float rowY = sbY + snap(6.0f) + i * snap(34.0f), rowH = snap(30.0f);
            const bool active = (i == section_);
            if (active) { flat(dev, sbX, rowY, sbW, rowH, 0x66 << 24 | (C_ACCENT & 0x00FFFFFF));
                          flat(dev, sbX, rowY, snap(3.0f), rowH, 0xFFBFE0FF); }                 // active marker bar
            fo->begin(dev);
            fo->draw_lc(dev, sbX + snap(16.0f), rowY + rowH * 0.5f, SECTIONS[i], snap(15.0f),
                        active ? C_TEXT : C_DIM, C_STROKE, 1.0f);
        }

        // content panel
        const float coX = sbX + sbW + snap(18.0f), coY = bodyY, coW = px + pw - pad - coX, coH = sbH;
        flat(dev, coX, coY, coW, coH, 0x22000000);
        fo->begin(dev);
        fo->draw_lc(dev, coX + snap(16.0f), coY + snap(22.0f), SECTIONS[section_], snap(18.0f), C_TEXT, C_STROKE, 1.3f);
        fo->draw_lc(dev, coX + snap(16.0f), coY + snap(50.0f),
                    "Controles a venir : taille, position, opacite, couleurs, buffs...", snap(13.0f), C_DIM, C_STROKE, 1.0f);
    } else if (tab_ == 1) {
        fo->begin(dev);
        fo->draw_lc(dev, px + pad, bodyY + snap(20.0f), "Profil  (a venir)", snap(16.0f), C_DIM, C_STROKE, 1.0f);
    } else {
        fo->begin(dev);
        fo->draw_lc(dev, px + pad, bodyY + snap(20.0f), "Help  (a venir)", snap(16.0f), C_DIM, C_STROKE, 1.0f);
    }

    // 6) footer hint
    fo->begin(dev);
    fo->draw_lc(dev, px + pad, bottomY + snap(2.0f), "//aio config  pour fermer", snap(12.0f), C_MUTE, C_STROKE, 1.0f);
    (void)solid;
}

} // namespace aio
