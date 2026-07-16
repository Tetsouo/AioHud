// resistances.cpp -- the Sheol resistance lookup + compute (ported from SheolHelper compute_res).
#include "model/resistances.h"
#include "model/resistances_gen.h"   // RES_ROWS[] / RES_ROWS_N (generated ; values are x1000)
#include <string.h>

namespace aio {

// binary search RES_ROWS (sorted by family) by name.
static const ResRow* res_lookup(const char* family) {
    int lo = 0, hi = RES_ROWS_N - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) >> 1;
        const int c = strcmp(RES_ROWS[mid].family, family);
        if (c == 0) return &RES_ROWS[mid];
        if (c < 0) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

// x1000 resistance -> integer percent (round to nearest, matching the addon's math.round(v*100)).
static inline int to_pct(int v1000) { return (v1000 >= 0) ? (v1000 + 5) / 10 : -(((-v1000) + 5) / 10); }

bool compute_resistances(const char* mobName, int sheolzone, ResData& out) {
    out.valid = false;
    if (!mobName || !mobName[0]) return false;

    // family = the name with a leading "Nostos "/"Agon " stripped (Odyssey mobs), else the name itself.
    const char* family = mobName;
    if (strstr(mobName, "Nostos") || strstr(mobName, "Agon")) {
        const char* sp = strchr(mobName, ' ');
        if (sp && sp[1]) family = sp + 1;
    }
    const ResRow* row = res_lookup(family);
    if (!row) return false;

    // max over ALL 12 raw values (the addon's table.max) -- the "greenest" element matches this.
    int maxv = row->v[0];
    for (int i = 1; i < 12; ++i) if (row->v[i] > maxv) maxv = row->v[i];

    out.wtype = row->wtype;
    // weapon cells : the family's own weapon type gets the -0.5 (=-500) Odyssey resistance bonus.
    for (int i = 0; i < 3; ++i) {
        int r = row->v[i]; if (i == row->wtype) r -= 500;
        out.weapon[i].pct = to_pct(r);
        out.weapon[i].color = (r > 1000) ? 1 : (r < 1000) ? 2 : 0;
    }
    // element cells : Magic-typed mobs get the -0.5 on every element. Green = equals the family max.
    for (int i = 0; i < 8; ++i) {
        const int raw = row->v[4 + i];
        const int val = (row->wtype == 3) ? raw - 500 : raw;
        out.elem[i].pct = to_pct(val);
        out.elem[i].color = (val == maxv) ? 1 : (val < 1000) ? 2 : 0;
    }
    // Cruel Joke : per-Sheol override (B=2 / C=3) if the family has one, else the default (joke slot v[3] != 0).
    signed char jz = -1;
    if (sheolzone == 2) jz = row->jokeB; else if (sheolzone == 3) jz = row->jokeC;
    if (jz < 0) jz = (row->v[3] != 0) ? 1 : 0;
    out.jokeVuln = jz;

    int k = 0; for (; family[k] && k < 23; ++k) out.family[k] = family[k]; out.family[k] = 0;
    out.valid = true;
    return true;
}

} // namespace aio
