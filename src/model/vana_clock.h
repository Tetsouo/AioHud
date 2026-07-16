// vana_clock.h -- Vana'diel time / elemental day / moon phase, computed PURELY from the real (UTC) system
// clock via FFXI's deterministic formula -- no game-memory read needed (server & client agree on it).
// vana_time = (unix_utc + 92514960) * 25 (Vana'diel runs 25x real time). Feeds the Minimap box's clock header.
#pragma once

namespace aio {

struct VanaClock {
    int  hh = 0, mm = 0;        // Vana'diel time (0..23 : 0..59)
    int  dayIdx = 0;            // 0=Fire 1=Earth 2=Water 3=Wind 4=Ice 5=Lightning 6=Light 7=Dark (day-of-week)
    int  moonPct = 0;           // moon illumination 0..100
    bool waning = false;        // true = the moon is waning (shadow on the right)
    int  moonPhase = 0;         // raw phase index 0..83 (0/84 = New, 42 = Full)
    int  moonPhaseId = 0;       // FFXI 12-phase id 0..11 (0 New, 3 First Qtr, 6 Full, 9 Last Qtr) -> the phase NAME
    int  daysToNew = 0;         // Vana'diel days until the next New moon (0 = now)
    int  daysToFull = 0;        // Vana'diel days until the next Full moon (0 = now)
};

// current Vana'diel clock from the system UTC time. Cheap ; call once/frame from the poller (snapshot into GameState).
VanaClock vana_clock_now();

} // namespace aio
