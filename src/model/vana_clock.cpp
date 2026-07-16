// vana_clock.cpp -- see vana_clock.h. Deterministic FFXI clock from the UTC system time.
#include "model/vana_clock.h"
#include <time.h>

namespace aio {

VanaClock vana_clock_now() {
    VanaClock v;
    const long long unix = (long long)time(0);              // UTC seconds
    const long long vana = (unix + 92514960LL) * 25LL;      // Vana'diel seconds (25x real time)
    const long long days = vana / 86400LL;
    v.hh = (int)((vana / 3600LL) % 24LL);
    v.mm = (int)((vana / 60LL) % 60LL);
    v.dayIdx = (int)(days % 8LL);                           // 0=Fire .. 7=Dark
    // Moon : 84-day cycle. phase 0/84 = New, 42 = Full. Illumination ramps linearly to the quarter points.
    // Offset -16 calibrated live against the in-game moon (Darksday -> 64% Waxing Gibbous, 2026-07-05).
    const int mp = (int)(((days - 16LL) % 84LL + 84LL) % 84LL);
    v.moonPhase = mp;
    v.moonPhaseId = ((2 * mp + 7) / 14) % 12;               // 12-phase id (round(mp/7)) -> the phase NAME (waxing vs waning)
    v.waning = (mp > 42);
    v.moonPct = (mp <= 42) ? (mp * 100 + 21) / 42 : ((84 - mp) * 100 + 21) / 42;   // +21 = round
    if (v.moonPct > 100) v.moonPct = 100; if (v.moonPct < 0) v.moonPct = 0;
    v.daysToNew  = (84 - mp) % 84;                          // next New  (0/84)
    v.daysToFull = ((42 - mp) % 84 + 84) % 84;              // next Full (42)
    return v;
}

} // namespace aio
