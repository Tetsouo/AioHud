// geo_dur.h -- GEO Indicolure (Indi-) spell duration for a buff cast on an ally, reproducing the Windower Timers
// model. Geomancy duration is purely ADDITIVE (flat seconds), unlike enhancing (%) or songs (multipliers) :
//
//   dur = Base + JP(1362)*2 + SUM(equipped "Indicolure ... duration +N" seconds)
//
// - Base : res/spells.lua duration (tb_buff_gen, skill 44). - JP gift 1362 "Indicolure Spell Effect Dur." = +2 s
//   per rank (GEO main only). - gear : native "Indicolure effect duration +N" FLAT seconds (geo_dur_gen.h, from
//   the item text -> universal / future-proof). NO %, NO set bonus, NO merit (GEO's duration levers are only JP +
//   gear ; the group-1 merits are Indi/Geo POTENCY). Geo- (luopan) spells are NOT modelled -- they put no bearer
//   status on an ally, so they never reach the buffs-on-allies path.
#pragma once
#include "model/geo_dur_gen.h"   // GEO_DUR_LISTED : item id -> Indicolure duration seconds
namespace aio {

inline int geo_dur_gear_sec(const unsigned short ids[16]) {   // sum of equipped Indicolure-duration gear (flat seconds)
    int s = 0;
    for (int e = 0; e < 16; ++e) { if (!ids[e]) continue;
        for (int i = 0; i < GEO_DUR_LISTED_N; ++i) if (GEO_DUR_LISTED[i].id == ids[e]) { s += GEO_DUR_LISTED[i].sec; break; }
    }
    return s;
}

} // namespace aio
