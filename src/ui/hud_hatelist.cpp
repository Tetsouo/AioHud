// hud_hatelist.cpp -- split out of hud.cpp (pure move). Hate List box renderer.
#include "ui/hud.h"
#include "ui/hud_internal.h"
#include "model/ui_config.h"
#include "ui/text_style.h"
#include "ui/box_style.h"
#include "model/party_state.h"
#include "gfx/draw.h"
#include "gfx/d3d.h"

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace aio {

// ============================ HATE LIST box ============================
// per-element typography for the Hate List (Distance / Name / HP% / Target), like the treasure/skillchains helpers.
static Font* hl_font(const Frame& f, int e) { return te_font(f, ui_config().hlText[e]); }
static inline float hl_sz(int e, float base) { return te_sz(ui_config().hlText[e], base); }
static inline float hl_ow(int e, float base) { return te_ow(ui_config().hlText[e], base); }
static inline u32   hl_col(int e, u32 base)  { return te_col(ui_config().hlText[e], base); }
static const char*  hl_up(int e, const char* s, char* buf, int cap) { return te_upper(ui_config().hlText[e], s, buf, cap); }
// the hate list keeps its OWN continuous HP ramp (green 100% -> yellow 50% -> red 0%), like the reference addon
// (deliberately distinct from the party/target gauge colour). p in 0..1.
static u32 hl_hp_color(float p) {
    if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
    float r, g;
    if (p > 0.5f) { r = (1.0f - p) * 2.0f; g = 1.0f; } else { r = 1.0f; g = p * 2.0f; }
    const int R = (int)(45.0f + r * 185.0f), G = (int)(45.0f + g * 180.0f), B = 55;
    return 0xFF000000u | ((u32)R << 16) | ((u32)G << 8) | (u32)B;
}
// a small VECTOR ">>" double-chevron, vertically centred at cy (the mockup's dblarrow icon ; the font atlas is
// Latin-1 only so no U+00BB glyph is guaranteed). Caller is in colour-quad state ; returns the width consumed.
static float hl_dblarrow(u32 dev, float x, float cy, float sz, u32 col) {
    const float th = 1.3f + sz * 0.05f, hs = sz * 0.24f, step = sz * 0.30f;
    for (int k = 0; k < 2; ++k) {   // two chevrons
        const float bx = x + k * step;
        seg_soft(dev, bx, cy - hs, bx + hs, cy, th, col);
        seg_soft(dev, bx + hs, cy, bx, cy + hs, th, col);
    }
    return step + hs;
}

// truncate `s` (CAPS already applied) with a ".." suffix until it fits maxW at size sz (like the mockup's
// text-overflow: ellipsis, but the font atlas has no U+2026 glyph so ".." stands in). Returns s if it already fits.
static const char* hl_fit(Font* fo, const char* s, float sz, float maxW, char* buf, int cap) {
    if (!s || fo->measure(s, sz) <= maxW) return s;
    int n = 0; while (s[n]) ++n;
    for (int len = n - 1; len >= 1; --len) {
        int c = 0; for (; c < len && c < cap - 3; ++c) buf[c] = s[c];
        buf[c++] = '.'; buf[c++] = '.'; buf[c] = 0;
        if (fo->measure(buf, sz) <= maxW) return buf;
    }
    buf[0] = s[0]; buf[1] = 0; return buf;
}

// The mobs that have aggro on you / your party, one HP bar (fiole) per mob : "[dist]  name .... HP%  >> PC".
// Rows come from party().hate_rows() (fed by the 0x028 enmity tracker), sorted by HP ascending ; the row you're
// TARGETING is framed gold (red when the mob is claimed). A sample list in preview / edit. Placed via //aio edit
// (hlX = horizontal centre, hlY = top), like the other boxes.
// Core renderer, extracted as a FREE function so the config PREVIEW and the Help sample reuse the EXACT same
// config-aware draw with no Hud instance. maxRowsCap > 0 limits the row count (the Help sample shows a few).
void hatelist_draw(const Frame& f, bool preview, float ovX, float ovY, float ovS, float screenW, float screenH, int maxRowsCap) {
    const UiConfig& C = ui_config();
    if (!C.hlShow) return;
    const bool editing = C.editLayout && !preview;
    u32 dev = f.dev;
    if (!f.font && !f.fonts) return;

    struct HRow { char dist[8]; char mob[24]; char pct[6]; char pc[20]; int hpp; unsigned char red, target; };
    HRow rows[24]; int nrows = 0;
    int maxN = (C.hlCount < 1) ? 1 : (C.hlCount > 20 ? 20 : C.hlCount);
    if (maxRowsCap > 0 && maxN > maxRowsCap) maxN = maxRowsCap;   // Help sample : only a few rows
    if (preview || editing) {
        static const struct { const char* n; int hpp; float dist; const char* pc; int red; int tgt; } SMP[20] = {
            {"Hpemde", 8, 22.4f, "Tetsouo", 0, 0}, {"Cunning Sammael", 14, 11.8f, 0, 0, 0}, {"Apex Bat", 21, 5.2f, "Kaories", 1, 1},
            {"Locus Ghost", 28, 18.6f, 0, 0, 0}, {"Gabbrath", 35, 9.1f, "Apururu", 0, 0}, {"Bztavian", 42, 14.3f, 0, 0, 0},
            {"Naga", 48, 7.7f, "Trion", 0, 0}, {"Velkk", 54, 25.0f, 0, 0, 0}, {"Matamata", 60, 3.4f, "Ayame", 0, 0},
            {"Yumcax", 66, 33.0f, 0, 0, 0}, {"Hennetiel Tiger", 72, 12.0f, "Curilla", 0, 0}, {"Leech", 78, 6.0f, 0, 0, 0},
            {"Gigas Bonze", 82, 14.1f, "Brakuk", 0, 0}, {"Hippogryph", 86, 9.7f, 0, 0, 0}, {"Wespe", 89, 18.3f, 0, 1, 0},
            {"Buccaboo", 92, 7.4f, 0, 0, 0}, {"Wild Karakul", 94, 20.5f, "Selene", 0, 0}, {"Snoll", 96, 13.6f, 0, 0, 0},
            {"Peiste", 98, 19.5f, 0, 0, 0}, {"Greater Colibri", 100, 21.7f, 0, 0, 0} };
        for (int i = 0; i < 20 && nrows < maxN; ++i) { HRow& r = rows[nrows];
            sprintf(r.dist, "%.1f", SMP[i].dist);
            int c = 0; for (; SMP[i].n[c] && c < 23; ++c) r.mob[c] = SMP[i].n[c]; r.mob[c] = 0;
            sprintf(r.pct, "%d%%", SMP[i].hpp);
            if (SMP[i].pc) { c = 0; for (; SMP[i].pc[c] && c < 19; ++c) r.pc[c] = SMP[i].pc[c]; r.pc[c] = 0; } else r.pc[0] = 0;
            r.hpp = SMP[i].hpp; r.red = (unsigned char)SMP[i].red; r.target = (unsigned char)SMP[i].tgt; ++nrows; }
    } else {
        int hn = 0; const HateRow* hr = party().hate_rows(hn);
        for (int i = 0; i < hn && nrows < maxN; ++i) {
            const HateRow& s = hr[i]; HRow& r = rows[nrows];
            sprintf(r.dist, "%.1f", s.dist);
            int c = 0; for (; s.mob[c] && c < 23; ++c) r.mob[c] = s.mob[c]; r.mob[c] = 0;
            sprintf(r.pct, "%d%%", s.hpp);
            c = 0; for (; s.pc[c] && c < 19; ++c) r.pc[c] = s.pc[c]; r.pc[c] = 0;
            r.hpp = s.hpp; r.red = s.red; r.target = s.target; ++nrows;
        }
    }
    if (nrows == 0) return;

    float sscl = C.hlScale; if (sscl < 0.5f) sscl = 0.5f; if (sscl > 2.0f) sscl = 2.0f;
    const float S = (ovS > 0.0f) ? ovS : (screenH / 1000.0f) * sscl;
    // mockup-faithful metrics (hate.css) : bar 126x16, fonts 10px, gaps 5px, rows 3px apart, gold caps 5px.
    const float pad = (ui_config().hlBox.on ? 5.0f : 0.0f) * S, gap = 5.0f * S;   // no box chrome -> hug the content (precise placement)
    const u32 white = 0xFFFFFFFFu, dim = 0xFFB4B9C8u, gold = 0xFFE8C55Au, red = 0xFFFF4646u, strk = 0xFF000000u;
    const bool shDist = C.hlDist != 0, shTgt = C.hlTgt != 0;

    Font* fDist = hl_font(f, HL_DIST); Font* fName = hl_font(f, HL_NAME); Font* fPct = hl_font(f, HL_PCT); Font* fPc = hl_font(f, HL_TARGET);
    const float zDist = hl_sz(HL_DIST, 10.0f) * S, zName = hl_sz(HL_NAME, 10.0f) * S, zPct = hl_sz(HL_PCT, 10.0f) * S, zPc = hl_sz(HL_TARGET, 10.0f) * S;
    const float oDist = hl_ow(HL_DIST, 1.0f) * S, oName = hl_ow(HL_NAME, 1.0f) * S, oPct = hl_ow(HL_PCT, 1.0f) * S, oPc = hl_ow(HL_TARGET, 1.0f) * S;
    const u32 distCol = hl_col(HL_DIST, dim), nameCol = hl_col(HL_NAME, white), pctCol = hl_col(HL_PCT, white), pcCol = hl_col(HL_TARGET, 0xFFC8AA6Eu);

    const float barW = 126.0f * S, barH = 16.0f * S;
    const float capW = 5.0f * S;                          // gold end caps (z-index over the fill)
    const float nameInsetL = 10.0f * S, pctInsetR = 9.0f * S, pctReserve = 37.0f * S;   // css : name left:10 right:37 ; HP% right:9
    const float rowH = barH + 3.0f * S;                   // css : .haterow + .haterow { margin-top: 3px }
    const float arrowW = zPc * 0.72f;

    // ---- measure the side columns (each with its own font + CAPS) ----
    char db[8], nb[24], pb[6], cb[20];
    float wDist = 0, wTgt = 0;
    for (int i = 0; i < nrows; ++i) {
        if (shDist) { float a = fDist->measure(hl_up(HL_DIST, rows[i].dist, db, 8), zDist); if (a > wDist) wDist = a; }
        if (shTgt && rows[i].pc[0]) { float a = arrowW + gap * 0.6f + fPc->measure(hl_up(HL_TARGET, rows[i].pc, cb, 20), zPc); if (a > wTgt) wTgt = a; }
    }
    const float contentW = (shDist ? wDist + gap : 0.0f) + barW + (wTgt > 0 ? gap + wTgt : 0.0f);
    const float boxW = contentW + 2.0f * pad;
    const float boxH = nrows * rowH + 2.0f * pad;

    // ---- position (+ edit drag) : hlX = horizontal centre, hlY = top ----
    float px, py;
    if (ovS > 0.0f) { px = ovX - boxW * 0.5f; py = ovY - boxH * 0.5f; }
    else            { px = C.hlX * screenW - boxW * 0.5f; py = C.hlY * screenH; }
    if (editing) { static EditBox g_hlEdit; box_edit(f, g_hlEdit, EDITBOX_HATE, px, py, boxW, boxH, ui_config().hlScale, ui_config().hlX, ui_config().hlY, true); }

    // ---- box chrome ----
    dColorQuadState(dev);
    const float r0 = 5.0f * S;
    draw_themed_box(dev, f.skin, px, py, boxW, boxH, ui_config().hlBox, 1.0f, S);   // shared themed chrome (frame/transp/theme)

    const float xDistR = px + pad + wDist;                     // distance column : right-aligned
    const float xBar   = px + pad + (shDist ? wDist + gap : 0.0f);
    const float xTgt   = xBar + barW + gap;
    const float br     = barH * 0.5f;
    char fitb[24];

    float rowTop = py + pad;
    for (int i = 0; i < nrows; ++i) {
        const HRow& R = rows[i];
        const float cy = rowTop + rowH * 0.5f, by = cy - barH * 0.5f;

        // the row you're targeting : thin frame hugging the bar (gold ; red when the mob is claimed)
        if (R.target) { dColorQuadState(dev); rrect_stroke(dev, xBar - 2.0f * S, by - 2.0f * S, barW + 4.0f * S, barH + 4.0f * S, br + 2.0f * S, R.red ? red : gold, 1.3f * S); }

        // distance (left)
        if (shDist) { const char* sd = hl_up(HL_DIST, R.dist, db, 8); fDist->begin(dev); fDist->draw_lc(dev, xDistR - fDist->measure(sd, zDist), cy, sd, zDist, distCol, strk, oDist); dColorQuadState(dev); }

        // HP fiole : a GOLD capsule whose rounded ends ARE the metal caps (mockup : caps rounded 8px on the outer
        // side), a dark glass tube inset by capW over it, the HP fill inside the tube, a thin bluish rim. Drawing the
        // caps as the capsule's ends (not separate 5px rrects) makes them hug the tube's rounding -> no truncated corner.
        rrect(dev, xBar, by, barW, barH, br, gold, 0xFFA07A14u, 1.0f);                    // full gold capsule (ends = caps)
        const float tx = xBar + capW, tw = barW - 2.0f * capW;                            // dark tube, inset by the cap width
        rrect(dev, tx, by, tw, barH, br, 0xFF20222Cu, 0xFF16181Fu, 1.0f);                 // #1a1c24 glass tube
        const float frac = R.hpp / 100.0f; const float avail = tw - 2.0f * S;
        const float fw = avail * (frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac));
        if (fw > 0.5f) {
            const u32 hc = hl_hp_color(frac);
            if (fw >= avail - 0.5f) rrect(dev, tx + 1.0f * S, by + 1.0f * S, fw, barH - 2.0f * S, br - 1.0f * S, hc, hc, 1.0f);
            else                    rrect_left(dev, tx + 1.0f * S, by + 1.0f * S, fw, barH - 2.0f * S, br - 1.0f * S, hc, hc, 1.0f);
        }
        rrect_stroke(dev, tx, by, tw, barH, br, 0x94A5BEFFu, 1.2f * S);                   // bluish tube rim (css 2px rgba(165,190,255,.58))

        // name (over the bar, left ; truncated with ".." to fit) + HP% (over the bar, right)
        const char* sp = hl_up(HL_PCT, R.pct, pb, 6);
        const float nameW = barW - nameInsetL - pctReserve;
        const char* nm = hl_fit(fName, hl_up(HL_NAME, R.mob, nb, 24), zName, nameW, fitb, 24);
        fName->begin(dev); fName->draw_lc(dev, xBar + nameInsetL, cy, nm, zName, nameCol, strk, oName);
        fPct->begin(dev);  fPct->draw_lc(dev, xBar + barW - pctInsetR - fPct->measure(sp, zPct), cy, sp, zPct, pctCol, strk, oPct);

        // target column (right) : ">> PCname"
        if (shTgt && R.pc[0]) {
            dColorQuadState(dev);
            float x = xTgt;
            x += hl_dblarrow(dev, x, cy, zPc, R.red ? red : gold) + gap * 0.6f;
            const char* pc = hl_up(HL_TARGET, R.pc, cb, 20);
            fPc->begin(dev); fPc->draw_lc(dev, x, cy, pc, zPc, pcCol, strk, oPc);
        }
        rowTop += rowH;
    }
}

// Live / edit path : the Hud draws the box at its configured screen position.
void Hud::draw_hate_list(const Frame& f, bool preview, float ovX, float ovY, float ovS) {
    hatelist_draw(f, preview, ovX, ovY, ovS, (float)screenW_, (float)screenH_, 0);
}

// Help sample : a few config-styled fiole rows (the REAL renderer in preview mode), centred at (cx,cy) at scale `s`.
void hatelist_help_box(const Frame& f, float cx, float cy, float s) {
    hatelist_draw(f, true, cx, cy, s, 0.0f, 0.0f, 4);
}

} // namespace aio
