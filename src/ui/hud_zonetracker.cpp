// hud_zonetracker.cpp -- split out of hud.cpp (pure move). Zone Tracker box renderer.
#include "ui/hud.h"
#include "ui/hud_internal.h"
#include "model/ui_config.h"
#include "ui/text_style.h"
#include "ui/box_style.h"
#include "model/party_state.h"
#include "gfx/draw.h"
#include "gfx/d3d.h"
#include "model/paths.h"
#include "gfx/texture.h"
#include "model/zones.h"
#include "model/resistances.h"
#include "model/gamestate.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace aio {

static const char* WEAPON_ICONS_PATH() { static char b[260]; if (!b[0]) plugin_path(b, 260, "assets\\weapon_icons.raw"); return b; }
// ============================ ZONE TRACKER box (Dynamis / Abyssea) ============================
static Font* zt_font(const Frame& f, int e) { return te_font(f, ui_config().ztText[e]); }
static inline float zt_sz(int e, float base) { return te_sz(ui_config().ztText[e], base); }
static inline float zt_ow(int e, float base) { return te_ow(ui_config().ztText[e], base); }
static inline u32   zt_col(int e, u32 base)  { return te_col(ui_config().ztText[e], base); }
// green (full) -> yellow -> red (empty), like the mockup timeRamp.
static u32 zt_time_col(float p) {
    if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
    float r, g; if (p > 0.5f) { r = (1.0f - p) * 2.0f; g = 1.0f; } else { r = 1.0f; g = p * 2.0f; }
    return 0xFF000000u | ((u32)(45 + r * 185) << 16) | ((u32)(45 + g * 180) << 8) | 55u;
}

// The Zone Tracker : Dynamis (run timer + 5 granule Key Items) or Abyssea (visitant timer + 7 lights), from
// party().zone_tracker() (packet-fed). Appears only in those zones ; a sample (per ztVariant) in preview/edit.
// Core renderer, extracted as a FREE function so the config PREVIEW and the Help sample reuse the EXACT same
// config-aware draw with no Hud instance. `weaponTex` = the Odyssey weapon-icons sheet (0 = none), from the caller.
void zonetracker_draw(const Frame& f, bool preview, float ovX, float ovY, float ovS, float screenW, float screenH,
                      u32 weaponTex, bool measureOnly = false, float* outW = 0, float* outH = 0, int variantOverride = -1) {
    const UiConfig& C = ui_config();
    const int vz = (variantOverride >= 0) ? variantOverride : C.ztVariant;   // Help forces a specific content variant (0 Dyn / 1 Aby / 2 Omen / 3 Nyzul / 4 Odyssey)
    if (!C.ztShow && variantOverride < 0) return;   // Help sample ignores the Show toggle
    const bool showHdr = (C.ztHeader != 0);   // the box TITLE row (Dynamis / Abyssea / Omen / Nyzul / Sheol) -- toggle
    const bool editing = C.editLayout && !preview;
    u32 dev = f.dev;
    if (!f.font && !f.fonts) return;

    // ===== OMEN (mode 3, Reisenjima Henge) : header + floor objective + bonus timer + up to 10 objective rows =====
    if ((preview || editing) ? (vz == 2) : (party().zone_tracker().mode == 3)) {
        struct ORow { char label[16]; int cur, req; unsigned char done, failed; };
        ORow orows[10]; int nrows = 0;
        char floorObj[48] = {0}; int omens = 0, bonusSec = 0;
        if (preview || editing) {
            const char* fo = "Vanquish all transcended foes"; int k = 0; for (; fo[k] && k < 47; ++k) floorObj[k] = fo[k]; floorObj[k] = 0;
            omens = 5; bonusSec = 272;
            static const struct { const char* n; int c, r; } SMP[6] = { {"WS Damage",15000,15000},{"Kills",8,8},{"MB Damage",12000,15000},{"Critical Hits",40,60},{"Magic Bursts",2,5},{"Skillchains",1,3} };
            for (int i = 0; i < 6; ++i) { ORow& r = orows[nrows]; int j = 0; for (; SMP[i].n[j] && j < 15; ++j) r.label[j] = SMP[i].n[j]; r.label[j] = 0; r.cur = SMP[i].c; r.req = SMP[i].r; r.done = (r.cur >= r.req); r.failed = 0; ++nrows; }
        } else {
            const ZoneTracker& zt = party().zone_tracker();
            int k = 0; for (; zt.floorObj[k] && k < 47; ++k) floorObj[k] = zt.floorObj[k]; floorObj[k] = 0;
            omens = zt.omens;
            bonusSec = zt.omenBonusSec - (int)((GetTickCount() - zt.omenBonusMs) / 1000u); if (bonusSec < 0) bonusSec = 0;
            for (int i = 0; i < 10; ++i) if (zt.omen[i].type) { ORow& r = orows[nrows]; const char* nm = PartyState::omen_short(zt.omen[i].type);
                int j = 0; for (; nm[j] && j < 15; ++j) r.label[j] = nm[j]; r.label[j] = 0;
                r.cur = zt.omen[i].cur; r.req = zt.omen[i].req;
                r.done = (zt.omen[i].req > 0 && r.cur >= zt.omen[i].req);
                r.failed = (bonusSec < 1 && zt.omen[i].req > 0 && r.cur < zt.omen[i].req); ++nrows; }
        }
        float sscl = C.ztScale; if (sscl < 0.5f) sscl = 0.5f; if (sscl > 2.0f) sscl = 2.0f;
        const float S = (ovS > 0.0f) ? ovS : (screenH / 1000.0f) * sscl;
        const float pad = (ui_config().ztBox.on ? 8.0f : 0.0f) * S, gap = 5.0f * S;   // no box chrome -> hug the content
        const u32 white = 0xFFEAF0FFu, gold = 0xFFE8C55Au, strk = 0xFF000000u, orange = 0xFFEB9660u, dim = 0xFFB4B9C8u, green = 0xFF6BE06Bu, red = 0xFFF06060u;
        Font* fH = zt_font(f, ZT_HEADER); Font* fB = zt_font(f, ZT_BODY);
        const float zH = zt_sz(ZT_HEADER, 15.0f) * S, zB = zt_sz(ZT_BODY, 12.0f) * S;
        const float oH = zt_ow(ZT_HEADER, 1.1f) * S, oB = zt_ow(ZT_BODY, 1.0f) * S;
        const float lineH = zB + 5.0f * S, headH = zH + 6.0f * S;
        char sb[48], hb[40]; sprintf(hb, "Omens: %d   Bonus %d:%02d", omens, bonusSec / 60, bonusSec % 60);
        #define OMEN_FMT(i) (orows[i].req < 0 ? (sprintf(sb, "%d: %.14s [%d/???]", (i) + 1, orows[i].label, orows[i].cur), sb) : (sprintf(sb, "%d: %.14s [%d/%d]", (i) + 1, orows[i].label, orows[i].cur, orows[i].req), sb))
        float contentW = fH->measure("Omen", zH);
        if (fB->measure(floorObj, zB) > contentW) contentW = fB->measure(floorObj, zB);
        if (fB->measure(hb, zB) > contentW) contentW = fB->measure(hb, zB);
        for (int i = 0; i < nrows; ++i) { float w = fB->measure(OMEN_FMT(i), zB); if (w > contentW) contentW = w; }
        if (contentW < 150.0f * S) contentW = 150.0f * S;
        const float boxW = contentW + 2.0f * pad;
        const float boxH = pad + (showHdr ? headH + gap : 0.0f) +lineH + gap * 0.5f + lineH + 4.0f * S + nrows * lineH + pad;
        if (measureOnly) { if (outW) *outW = boxW; if (outH) *outH = boxH; return; }
        float px, py;
        if (ovS > 0.0f) { px = ovX - boxW * 0.5f; py = ovY - boxH * 0.5f; }
        else            { px = C.ztX * screenW - boxW * 0.5f; py = C.ztY * screenH; }
        if (editing) { static EditBox g_ztEdit; box_edit(f, g_ztEdit, EDITBOX_ZONETRACKER, px, py, boxW, boxH, ui_config().ztScale, ui_config().ztX, ui_config().ztY, true); }
        dColorQuadState(dev);
        const float r0 = 6.0f * S;
        draw_themed_box(dev, f.skin, px, py, boxW, boxH, ui_config().ztBox, 1.0f, S);   // shared themed chrome (frame/transp/theme)
        const float cx = px + boxW * 0.5f, x0 = px + pad;
        float cy = py + pad;
        if (showHdr) { fH->begin(dev); fH->draw_c(dev, cx, cy + headH * 0.5f, "Omen", zH, zt_col(ZT_HEADER, orange), strk, oH); cy += headH + gap; }
        fB->begin(dev); fB->draw_c(dev, cx, cy + lineH * 0.5f, floorObj, zB, white, strk, oB); cy += lineH + gap * 0.5f;
        fB->draw_c(dev, cx, cy + lineH * 0.5f, hb, zB, dim, strk, oB); cy += lineH + 4.0f * S;
        for (int i = 0; i < nrows; ++i) { const char* rs = OMEN_FMT(i);
            const u32 col = orows[i].done ? green : (orows[i].failed ? red : zt_col(ZT_BODY, white));
            fB->begin(dev); fB->draw_lc(dev, x0, cy + lineH * 0.5f, rs, zB, col, strk, oB); cy += lineH; }
        #undef OMEN_FMT
        return;
    }

    // ===== NYZUL ISLE (mode 4) : header + Floor / Time + Objective / Restriction / Completed / Reward Rate / Tokens =====
    if ((preview || editing) ? (vz == 3) : (party().zone_tracker().mode == 4)) {
        int floor = 0, remain = 0, completed = 0, rate = 100; long tokens = 0;
        char objective[48] = {0}, restriction[40] = {0}; unsigned char objPending = 1, restrFail = 0;
        if (preview || editing) {
            floor = 42; remain = 48; completed = 7; rate = 110; tokens = 1540;
            const char* o = "Defeat all enemies"; int k = 0; for (; o[k] && k < 47; ++k) objective[k] = o[k]; objective[k] = 0;
            const char* rr = "No magic"; k = 0; for (; rr[k] && k < 39; ++k) restriction[k] = rr[k]; restriction[k] = 0;
        } else {
            const ZoneTracker& zt = party().zone_tracker();
            floor = zt.nyFloor; remain = party().nyzul_remaining(); if (remain < 0) remain = 0;
            completed = zt.nyCompleted; tokens = (long)(zt.nyTokens + 0.5);
            double rd = 1.0; if (zt.nyArmband) rd += 0.1; if (zt.nyPartySize > 3) rd -= (zt.nyPartySize - 3) * 0.1;   // live reward rate
            rate = (int)(rd * 100.0 + 0.5);
            int k = 0; for (; zt.nyObjective[k] && k < 47; ++k) objective[k] = zt.nyObjective[k]; objective[k] = 0;
            k = 0; for (; zt.nyRestriction[k] && k < 39; ++k) restriction[k] = zt.nyRestriction[k]; restriction[k] = 0;
            objPending = zt.nyObjPending; restrFail = zt.nyRestrFail;
        }
        const bool hasRestr = restriction[0] != 0;
        float sscl = C.ztScale; if (sscl < 0.5f) sscl = 0.5f; if (sscl > 2.0f) sscl = 2.0f;
        const float S = (ovS > 0.0f) ? ovS : (screenH / 1000.0f) * sscl;
        const float pad = (ui_config().ztBox.on ? 8.0f : 0.0f) * S, gap = 5.0f * S;   // no box chrome -> hug the content
        const u32 white = 0xFFEAF0FFu, gold = 0xFFE8C55Au, strk = 0xFF000000u, orange = 0xFFEB9660u;
        const u32 yellow = 0xFFFFFA78u, green = 0xFF6BE06Bu, red = 0xFFF06060u, orst = 0xFFFFA500u;
        Font* fH = zt_font(f, ZT_HEADER); Font* fB = zt_font(f, ZT_BODY);
        const float zH = zt_sz(ZT_HEADER, 15.0f) * S, zB = zt_sz(ZT_BODY, 12.0f) * S;
        const float oH = zt_ow(ZT_HEADER, 1.1f) * S, oB = zt_ow(ZT_BODY, 1.0f) * S;
        const float lineH = zB + 5.0f * S, headH = zH + 6.0f * S;
        char rFloor[24], rTime[24], rComp[24], rRate[24], rTok[28];
        sprintf(rFloor, "Floor: %d", floor);
        sprintf(rTime, "Time:  %d:%02d", remain / 60, remain % 60);
        sprintf(rComp, "Completed: %d", completed);
        sprintf(rRate, "Reward Rate: %d%%", rate);
        sprintf(rTok,  "Tokens: %ld", tokens);
        // content width : header + centred Floor/Time + the left rows (objective/restriction measured label+value together)
        char rObj[64], rRestr[60];
        sprintf(rObj,   "Objective: %.40s", objective[0] ? objective : "-");
        sprintf(rRestr, "Restriction: %.36s", restriction);
        float contentW = fH->measure("Nyzul Isle", zH);
        const char* MW[7] = { rFloor, rTime, rObj, rComp, rRate, rTok, rRestr };
        for (int i = 0; i < (hasRestr ? 7 : 6); ++i) { float w = fB->measure(MW[i], zB); if (w > contentW) contentW = w; }
        if (contentW < 150.0f * S) contentW = 150.0f * S;
        const float boxW = contentW + 2.0f * pad;
        const int leftRows = hasRestr ? 5 : 4;                         // Objective [+Restriction] Completed Rate Tokens
        const float blank = lineH * 0.6f;
        const float boxH = pad + (showHdr ? headH + gap : 0.0f) +2.0f * lineH + blank + leftRows * lineH + pad;
        if (measureOnly) { if (outW) *outW = boxW; if (outH) *outH = boxH; return; }
        float px, py;
        if (ovS > 0.0f) { px = ovX - boxW * 0.5f; py = ovY - boxH * 0.5f; }
        else            { px = C.ztX * screenW - boxW * 0.5f; py = C.ztY * screenH; }
        if (editing) { static EditBox g_ztEdit; box_edit(f, g_ztEdit, EDITBOX_ZONETRACKER, px, py, boxW, boxH, ui_config().ztScale, ui_config().ztX, ui_config().ztY, true); }
        dColorQuadState(dev);
        const float r0 = 6.0f * S;
        draw_themed_box(dev, f.skin, px, py, boxW, boxH, ui_config().ztBox, 1.0f, S);   // shared themed chrome (frame/transp/theme)
        const float cx = px + boxW * 0.5f, x0 = px + pad;
        float cy = py + pad;
        if (showHdr) { fH->begin(dev); fH->draw_c(dev, cx, cy + headH * 0.5f, "Nyzul Isle", zH, zt_col(ZT_HEADER, orange), strk, oH); cy += headH + gap; }
        fB->begin(dev);
        fB->draw_c(dev, cx, cy + lineH * 0.5f, rFloor, zB, zt_col(ZT_BODY, white), strk, oB); cy += lineH;
        { const u32 tc = (remain < 60) ? red : white; fB->draw_c(dev, cx, cy + lineH * 0.5f, rTime, zB, tc, strk, oB); } cy += lineH + blank;
        { const char* lbl = "Objective: "; const u32 vc = objPending ? yellow : green;   // label default, value coloured by state
          fB->draw_lc(dev, x0, cy + lineH * 0.5f, lbl, zB, zt_col(ZT_BODY, white), strk, oB);
          fB->draw_lc(dev, x0 + fB->measure(lbl, zB), cy + lineH * 0.5f, objective[0] ? objective : "-", zB, vc, strk, oB); cy += lineH; }
        if (hasRestr) { const char* lbl = "Restriction: "; const u32 vc = restrFail ? red : orst;
          fB->draw_lc(dev, x0, cy + lineH * 0.5f, lbl, zB, zt_col(ZT_BODY, white), strk, oB);
          fB->draw_lc(dev, x0 + fB->measure(lbl, zB), cy + lineH * 0.5f, restriction, zB, vc, strk, oB); cy += lineH; }
        fB->draw_lc(dev, x0, cy + lineH * 0.5f, rComp, zB, zt_col(ZT_BODY, white), strk, oB); cy += lineH;
        fB->draw_lc(dev, x0, cy + lineH * 0.5f, rRate, zB, zt_col(ZT_BODY, white), strk, oB); cy += lineH;
        fB->draw_lc(dev, x0, cy + lineH * 0.5f, rTok,  zB, zt_col(ZT_BODY, white), strk, oB); cy += lineH;
        return;
    }

    // ===== SHEOL / ODYSSEY (mode 5) : Sheol A/B/C header + segment counter + the target's resistances (weapons /
    // elements / Cruel Joke), ported from the SheolHelper addon. =====
    if ((preview || editing) ? (vz == 4) : (party().zone_tracker().mode == 5)) {
        int segs = 0, sheol = 0; bool lastRun = false;
        if (preview || editing) { segs = 1234; sheol = 2; }
        else { const ZoneTracker& zt = party().zone_tracker(); segs = zt.segments; sheol = zt.sheolzone; lastRun = (zt.segLastRun != 0); }
        const bool showSeg = (C.ztSheolSeg != 0);
        // ---- resistances of the current target (cached by name ; recomputed only when the target changes) ----
        static ResData rd; static char rdName[24] = {0};
        bool haveRes = false;
        if (preview || editing) { haveRes = (C.ztSheolRes != 0) && compute_resistances("Tiger", 2, rd); }
        else if (C.ztSheolRes && f.game && f.game->target.valid && f.game->target.spawnType == 0x10 && !lastRun) {
            const char* tn = f.game->target.name;
            if (strcmp(tn, rdName) != 0) { compute_resistances(tn, sheol, rd); int k = 0; for (; tn[k] && k < 23; ++k) rdName[k] = tn[k]; rdName[k] = 0; }
            haveRes = rd.valid;
        }
        const bool showJoke = haveRes && (C.ztSheolJoke != 0) && (rd.jokeVuln >= 0);
        float sscl = C.ztScale; if (sscl < 0.5f) sscl = 0.5f; if (sscl > 2.0f) sscl = 2.0f;
        const float S = (ovS > 0.0f) ? ovS : (screenH / 1000.0f) * sscl;
        const float pad = (ui_config().ztBox.on ? 8.0f : 0.0f) * S, gap = 5.0f * S;   // no box chrome -> hug the content
        const u32 white = 0xFFEAF0FFu, gold = 0xFFE8C55Au, strk = 0xFF000000u, orange = 0xFFEB9660u, green = 0xFF6BE06Bu, red = 0xFFF06060u, dim = 0xFFB4B9C8u;
        Font* fH = zt_font(f, ZT_HEADER); Font* fB = zt_font(f, ZT_BODY);
        const float zH = zt_sz(ZT_HEADER, 15.0f) * S, zB = zt_sz(ZT_BODY, 12.0f) * S;
        const float oH = zt_ow(ZT_HEADER, 1.1f) * S, oB = zt_ow(ZT_BODY, 1.0f) * S;
        const float lineH = zB + 5.0f * S, headH = zH + 6.0f * S;
        // physical damage-type ICONS (Slashing/Piercing/Blunt) -- lazy-load the 3-cell atlas ; text fallback if absent.
        const bool wIcons = (weaponTex != 0);
        const float iconSz = 15.0f * S;
        #define RESCOL(c) ((c) == 1 ? green : (c) == 2 ? red : white)
        char hdr[16]; if (sheol >= 1 && sheol <= 3) sprintf(hdr, "Sheol %c", (char)('A' + sheol - 1)); else sprintf(hdr, "Odyssey");
        char sb[40]; sprintf(sb, "Segments: %d%s", segs, lastRun ? " (last run)" : "");
        // measure : header, segments, then (weapons / element 2-col / joke)
        float contentW = fH->measure(hdr, zH); if (showSeg && fB->measure(sb, zB) > contentW) contentW = fB->measure(sb, zB);
        char wl[24], vb[12];
        const float dotD = 9.0f * S, dotR = dotD * 0.5f;   // element colour puck
        const float subGap = gap, colGap = gap * 6.0f;     // between the 2 element sub-cols / (wider) between weapons and elements
        float wCellW = 0.0f, eValW = 0.0f, eCellW = 0.0f, resW = 0.0f;
        if (haveRes) {
            if (fB->measure(rd.family, zB) > contentW) contentW = fB->measure(rd.family, zB);
            float wValW = 0.0f;
            for (int i = 0; i < 3; ++i) { sprintf(vb, "%d%%", rd.weapon[i].pct); const float w = fB->measure(vb, zB); if (w > wValW) wValW = w; }
            if (wIcons) wCellW = iconSz + gap * 0.5f + wValW;   // [icon] value%
            else for (int i = 0; i < 3; ++i) { sprintf(wl, "%-9s%4d%%", res_weapon_name(i), rd.weapon[i].pct); const float w = fB->measure(wl, zB); if (w > wCellW) wCellW = w; }
            for (int i = 0; i < 8; ++i) { sprintf(vb, "%d%%", rd.elem[i].pct); const float w = fB->measure(vb, zB); if (w > eValW) eValW = w; }
            eCellW = dotD + gap * 0.5f + eValW;             // element cell = [colour puck] value%
            resW = wCellW + colGap + eCellW + subGap + eCellW;   // PHYSICAL column | MAGICAL 2-col, side by side
            if (resW > contentW) contentW = resW;
            if (showJoke && fB->measure("Cruel Joke", zB) > contentW) contentW = fB->measure("Cruel Joke", zB);
        }
        if (contentW < 130.0f * S) contentW = 130.0f * S;
        int bodyLines = (showSeg ? 1 : 0) + (haveRes ? (1 + 4 + (showJoke ? 1 : 0)) : 0);   // seg + family + 4-row resist block + joke
        const float boxW = contentW + 2.0f * pad, boxH = pad + (showHdr ? headH + gap : 0.0f) + bodyLines * lineH + pad;   // bottom margin = pad (was + an extra gap -> too much space under Cruel Joke)
        if (measureOnly) { if (outW) *outW = boxW; if (outH) *outH = boxH; return; }
        float px, py;
        if (ovS > 0.0f) { px = ovX - boxW * 0.5f; py = ovY - boxH * 0.5f; }
        else            { px = C.ztX * screenW - boxW * 0.5f; py = C.ztY * screenH; }
        if (editing) { static EditBox g_ztEditS; box_edit(f, g_ztEditS, EDITBOX_ZONETRACKER, px, py, boxW, boxH, ui_config().ztScale, ui_config().ztX, ui_config().ztY, true); }
        dColorQuadState(dev);
        const float r0 = 6.0f * S;
        draw_themed_box(dev, f.skin, px, py, boxW, boxH, ui_config().ztBox, 1.0f, S);   // shared themed chrome (frame/transp/theme)
        const float cx = px + boxW * 0.5f, x0 = px + pad;
        float cy = py + pad;
        if (showHdr) { fH->begin(dev); fH->draw_c(dev, cx, cy + headH * 0.5f, hdr, zH, zt_col(ZT_HEADER, orange), strk, oH); cy += headH + gap; }
        fB->begin(dev);
        if (showSeg) { fB->draw_c(dev, cx, cy + lineH * 0.5f, sb, zB, zt_col(ZT_BODY, white), strk, oB); cy += lineH; }
        if (haveRes) {
            fB->draw_c(dev, cx, cy + lineH * 0.5f, rd.family, zB, orange, strk, oB); cy += lineH;
            // PHYSICAL (weapons) in a LEFT column | MAGICAL (elements) as a compact 2-col block on the RIGHT: a colour
            // PUCK per element (puck = element colour) + the value% (coloured by resistance). Side by side, top-aligned,
            // the whole block centred. 4 rows tall (weapons fill the first 3).
            const float blockX = cx - resW * 0.5f;
            const float xE = blockX + wCellW + colGap, xE2 = xE + eCellW + subGap;
            if (wIcons) {   // physical : the Slashing/Piercing/Blunt ICON + value% (right-aligned)
                for (int i = 0; i < 3; ++i) draw_icon_cell(dev, weaponTex, blockX, cy + i * lineH + (lineH - iconSz) * 0.5f, iconSz, iconSz, i / 3.0f, (i + 1) / 3.0f);
                fB->begin(dev);
                for (int i = 0; i < 3; ++i) { sprintf(vb, "%d%%", rd.weapon[i].pct); const float vw = fB->measure(vb, zB);
                    fB->draw_lc(dev, blockX + wCellW - vw, cy + i * lineH + lineH * 0.5f, vb, zB, RESCOL(rd.weapon[i].color), strk, oB); }
            } else {        // text fallback (atlas missing)
                for (int i = 0; i < 3; ++i) { sprintf(wl, "%-9s%4d%%", res_weapon_name(i), rd.weapon[i].pct);
                    fB->begin(dev); fB->draw_lc(dev, blockX, cy + i * lineH + lineH * 0.5f, wl, zB, RESCOL(rd.weapon[i].color), strk, oB); }
            }
            dColorQuadState(dev);                                       // element colour pucks (primitives) first
            for (int r = 0; r < 4; ++r) for (int c = 0; c < 2; ++c) { const int i = r * 2 + c; const float ex = (c == 0) ? xE : xE2;
                disc(dev, ex + dotR, cy + r * lineH + lineH * 0.5f, dotR, res_elem_color(i)); }
            fB->begin(dev);                                            // then the value%, right-aligned in the cell
            for (int r = 0; r < 4; ++r) for (int c = 0; c < 2; ++c) { const int i = r * 2 + c; const float ex = (c == 0) ? xE : xE2;
                sprintf(vb, "%d%%", rd.elem[i].pct); const float vw = fB->measure(vb, zB);
                fB->draw_lc(dev, ex + eCellW - vw, cy + r * lineH + lineH * 0.5f, vb, zB, RESCOL(rd.elem[i].color), strk, oB); }
            cy += 4 * lineH;
            if (showJoke) { fB->begin(dev); fB->draw_c(dev, cx, cy + lineH * 0.5f, "Cruel Joke", zB, rd.jokeVuln ? green : red, strk, oB); cy += lineH; }
        }
        #undef RESCOL
        return;
    }

    int mode; int remainSec = 0, limitSec = 1; unsigned char ki[5] = {0}; int lights[7] = {0}; int visRemainSec = 0, visMax = 7200;
    if (preview || editing) {
        mode = (vz == 0) ? 1 : 2;
        if (mode == 1) { limitSec = 3600; remainSec = 3510; unsigned char k[5] = {1,1,0,0,1}; for (int i = 0; i < 5; ++i) ki[i] = k[i]; }
        else { int l[7] = {180,240,90,200,150,60,30}; for (int i = 0; i < 7; ++i) lights[i] = l[i]; visRemainSec = 18 * 60 + 42; }
    } else {
        const ZoneTracker& zt = party().zone_tracker();
        if (zt.mode == 0) return;
        mode = zt.mode;
        const unsigned now = GetTickCount();
        if (mode == 1) {
            limitSec = zt.dynLimitSec > 0 ? zt.dynLimitSec : 1;
            remainSec = limitSec - (int)((now - zt.dynEntryMs) / 1000u); if (remainSec < 0) remainSec = 0;
            for (int i = 0; i < 5; ++i) ki[i] = zt.ki[i];
        } else {
            for (int i = 0; i < 7; ++i) lights[i] = zt.lights[i];
            visRemainSec = zt.visitantMin * 60 - (int)((now - zt.visitantMs) / 1000u); if (visRemainSec < 0) visRemainSec = 0;
        }
    }

    float sscl = C.ztScale; if (sscl < 0.5f) sscl = 0.5f; if (sscl > 2.0f) sscl = 2.0f;
    const float S = (ovS > 0.0f) ? ovS : (screenH / 1000.0f) * sscl;
    const float pad = (ui_config().ztBox.on ? 8.0f : 0.0f) * S, gap = 6.0f * S;   // no box chrome -> hug the content
    const u32 white = 0xFFEAF0FFu, gold = 0xFFE8C55Au, strk = 0xFF000000u, orange = 0xFFEB9660u;
    Font* fH = zt_font(f, ZT_HEADER); Font* fB = zt_font(f, ZT_BODY);
    const float zH = zt_sz(ZT_HEADER, 15.0f) * S, zB = zt_sz(ZT_BODY, 12.0f) * S;
    const float oH = zt_ow(ZT_HEADER, 1.1f) * S, oB = zt_ow(ZT_BODY, 1.0f) * S;
    const float headH = zH + 6.0f * S, barH = zB + 6.0f * S;

    // ---- measure -> content size (per mode) ----
    static const char* KI_NM[5] = { "Crimson", "Azure", "Amber", "Alabaster", "Obsidian" };
    static const char* LT_AB[7] = { "Pe", "Az", "Ru", "Am", "Go", "Si", "Eb" };
    static const u32    LT_CO[7] = { 0xFFEBEBF5u, 0xFF5096FFu, 0xFFE63C3Cu, 0xFFE6B43Cu, 0xFFFFD764u, 0xFFB4BEC8u, 0xFF8C78AAu };
    static const int    LT_CAP[7] = { 230, 255, 255, 255, 200, 200, 200 };
    const char* title = (mode == 1) ? "Dynamis" : "Abyssea";
    const float dotD = 8.0f * S, lightH = 42.0f * S, colW = 22.0f * S;
    float contentW;
    if (mode == 1) {
        float w = fB->measure("Alabaster", zB) + gap + dotD;   // widest KI row
        if (fH->measure(title, zH) > w) w = fH->measure(title, zH);
        if (w < 96.0f * S) w = 96.0f * S;
        contentW = w;
    } else {
        contentW = 7.0f * colW + 6.0f * (gap * 0.5f);
        if (fH->measure(title, zH) > contentW) contentW = fH->measure(title, zH);
    }
    const float bodyH = (mode == 1) ? (5.0f * (zB + 5.0f * S)) : (lightH + zB + 4.0f * S);
    const float boxW = contentW + 2.0f * pad;
    const float boxH = pad + (showHdr ? headH + gap : 0.0f) +barH + gap + bodyH + pad;
    if (measureOnly) { if (outW) *outW = boxW; if (outH) *outH = boxH; return; }

    // ---- position (+ edit drag) ----
    float px, py;
    if (ovS > 0.0f) { px = ovX - boxW * 0.5f; py = ovY - boxH * 0.5f; }
    else            { px = C.ztX * screenW - boxW * 0.5f; py = C.ztY * screenH; }
    if (editing) { static EditBox g_ztEdit; box_edit(f, g_ztEdit, EDITBOX_ZONETRACKER, px, py, boxW, boxH, ui_config().ztScale, ui_config().ztX, ui_config().ztY, true); }

    // ---- chrome ----
    dColorQuadState(dev);
    const float r0 = 6.0f * S;
    draw_themed_box(dev, f.skin, px, py, boxW, boxH, ui_config().ztBox, 1.0f, S);   // shared themed chrome (frame/transp/theme)

    const float cx = px + boxW * 0.5f, x0 = px + pad;
    float cy = py + pad;
    // header
    if (showHdr) { fH->begin(dev); fH->draw_c(dev, cx, cy + headH * 0.5f, title, zH, zt_col(ZT_HEADER, orange), strk, oH); cy += headH + gap; }
    // time bar
    {
        const int tsec = (mode == 1) ? remainSec : visRemainSec;
        const int tmax = (mode == 1) ? limitSec : (visMax > 0 ? visMax : 1);
        const float frac = (float)tsec / (float)tmax;
        const float br = barH * 0.5f;
        rrect(dev, x0, cy, contentW, barH, br, 0xFF20222Cu, 0xFF16181Fu, 1.0f);
        const float fw = (contentW - 2.0f * S) * (frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac));
        const u32 tc = zt_time_col(frac);
        if (fw > 1.0f) { if (fw >= contentW - 2.0f * S - 0.5f) rrect(dev, x0 + 1.0f * S, cy + 1.0f * S, fw, barH - 2.0f * S, br - 1.0f * S, tc, tc, 1.0f);
                         else rrect_left(dev, x0 + 1.0f * S, cy + 1.0f * S, fw, barH - 2.0f * S, br - 1.0f * S, tc, tc, 1.0f); }
        char tb[12]; sprintf(tb, "%d:%02d", tsec / 60, tsec % 60);
        fB->begin(dev); fB->draw_c(dev, cx, cy + barH * 0.5f, tb, zB, white, strk, oB);
    }
    cy += barH + gap;
    // body
    if (mode == 1) {   // 5 Key-Item rows : coloured dot (green owned / red missing) + name
        const float rowH = zB + 5.0f * S; char nb[16];
        for (int i = 0; i < 5; ++i) {
            const float ry = cy + rowH * 0.5f;
            const u32 dc = ki[i] ? 0xFF6BE06Bu : 0xFFF06060u;
            dColorQuadState(dev); rrect(dev, x0, ry - dotD * 0.5f, dotD, dotD, dotD * 0.5f, dc, dc, 1.0f);
            const char* nm = KI_NM[i]; int c2 = 0; for (; nm[c2] && c2 < 15; ++c2) nb[c2] = nm[c2]; nb[c2] = 0;
            fB->begin(dev); fB->draw_lc(dev, x0 + dotD + gap, ry, nb, zB, zt_col(ZT_BODY, ki[i] ? white : 0xFFB4B9C8u), strk, oB);
            cy += rowH;
        }
    } else {   // 7 vertical light bars : label (top) + bar (value/cap) + value (bottom)
        const float step = (contentW - colW) / 6.0f;
        for (int i = 0; i < 7; ++i) {
            const float lcx = x0 + colW * 0.5f + step * i;
            fB->begin(dev); fB->draw_c(dev, lcx, cy + zB * 0.5f, LT_AB[i], zB, LT_CO[i], strk, oB);
            const float bx = lcx - 4.0f * S, byy = cy + zB + 3.0f * S, bw = 8.0f * S, bh = lightH - zB - 3.0f * S;
            dColorQuadState(dev);
            rrect(dev, bx, byy, bw, bh, bw * 0.5f, 0xFF20222Cu, 0xFF16181Fu, 1.0f);
            float fr = (float)lights[i] / (float)LT_CAP[i]; if (fr < 0.0f) fr = 0.0f; if (fr > 1.0f) fr = 1.0f;
            const float fh = (bh - 2.0f * S) * fr;
            if (fh > 1.0f) rrect(dev, bx + 1.0f * S, byy + bh - 1.0f * S - fh, bw - 2.0f * S, fh, (bw - 2.0f * S) * 0.5f, LT_CO[i], LT_CO[i], 1.0f);
            char vb[8]; sprintf(vb, "%d", lights[i]);
            fB->begin(dev); fB->draw_c(dev, lcx, cy + lightH + zB * 0.5f - 1.0f * S, vb, zB * 0.86f, white, strk, oB);
        }
    }
}

// Live / edit path : the Hud draws the box at its configured screen position (lazy-loads the Odyssey weapon icons).
void Hud::draw_zonetracker(const Frame& f, bool preview, float ovX, float ovY, float ovS) {
    if (!weaponIconsTried_) { weaponIcons_ = load_raw_texture(f.dev, WEAPON_ICONS_PATH(), 96, 32); weaponIconsTried_ = true; }
    zonetracker_draw(f, preview, ovX, ovY, ovS, (float)screenW_, (float)screenH_, weaponIcons_);
}

// The Help sample owns its own copy of the Odyssey weapon-icons sheet (lazy) so it can draw without a Hud.
static u32 zonetracker_help_weapons(u32 dev) {
    static u32 tex = 0; static bool tried = false;
    if (!tried) { tex = load_raw_texture(dev, WEAPON_ICONS_PATH(), 96, 32); tried = true; }
    return tex;
}

// Help sample : the REAL Zone Tracker box in preview mode for a FORCED content `variant` (config-aware), centred.
void zonetracker_help_box(const Frame& f, float cx, float cy, float s, int variant) {
    zonetracker_draw(f, true, cx, cy, s, 0.0f, 0.0f, zonetracker_help_weapons(f.dev), false, 0, 0, variant);
}

// Help : box dimensions of one variant at scale 1 (linear in S -> caller multiplies by its scale). For grid layout.
void zonetracker_help_measure(const Frame& f, int variant, float& outW, float& outH) {
    outW = 0.0f; outH = 0.0f;
    zonetracker_draw(f, true, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, zonetracker_help_weapons(f.dev), true, &outW, &outH, variant);
}

// Help scale-to-fit for one variant : measure at scale 1 (linear in S), pick the largest scale that fits availW.
void zonetracker_help_fit(const Frame& f, float availW, float maxScale, int variant, float& outScale, float& outH) {
    float bw = 0.0f, bh = 0.0f;
    zonetracker_draw(f, true, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, zonetracker_help_weapons(f.dev), true, &bw, &bh, variant);
    float s = (bw > 1.0f) ? (availW / bw) : maxScale;
    if (s > maxScale) s = maxScale; if (s < 0.6f) s = 0.6f;
    outScale = s; outH = bh * s;
}

} // namespace aio
