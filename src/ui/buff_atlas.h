// buff_atlas.h -- the shared status-icon atlas geometry (assets/buff_atlas.raw, built by
// scripts/gen_buff_atlas.ps1 from XivParty's icon set). A fixed 32-col grid of 32px cells ; a status id maps
// to cell (id % COLS, id / COLS). party / player / target / timers all read the SAME atlas, so the dimensions,
// the runtime path, and the id->UV math live here once (each widget keeps its own per-icon layout/draw).
#pragma once
#include "gfx/d3d.h"          // u32
#include "model/paths.h"      // plugin_path (runtime-derived asset path)

namespace aio {

static const int BUFF_ATLAS_W = 1024, BUFF_ATLAS_H = 640, BUFF_CELL = 32, BUFF_COLS = 32;
static const int BUFF_ATLAS_ROWS = BUFF_ATLAS_H / BUFF_CELL;         // 20 -> highest mappable id = COLS*ROWS - 1 (639)

inline const char* buff_atlas_path() { static char b[260]; if (!b[0]) plugin_path(b, 260, "assets\\buff_atlas.raw"); return b; }

// UV of status-icon `id`'s cell : cell size (au,av) + top-left (u0,v0). id must be in [0, COLS*ROWS).
inline void buff_cell_uv(int id, float& au, float& av, float& u0, float& v0) {
    au = (float)BUFF_CELL / (float)BUFF_ATLAS_W; av = (float)BUFF_CELL / (float)BUFF_ATLAS_H;
    u0 = (float)(id % BUFF_COLS) * au; v0 = (float)(id / BUFF_COLS) * av;
}

} // namespace aio
