// party_state_pointwatch.cpp -- PointWatch module (XP / CP / ML + Merits progression),
// split out of party_state.cpp. PURE MOVE : the three packet handlers that feed PointWatch --
// on_char_stats (0x061), on_set_update (0x063 Order 2/5/9), on_exp_msg (0x029/0x02D live gains).
// The RateReg X/h ring + ffxi_now_tick stay in party_state.cpp (shared / used by other modules).
#include "model/party_state.h"
#include "model/party_state_internal.h"   // pkt_u16 / pkt_u32 (shared packet readers)

namespace aio {

void PartyState::on_char_stats(const unsigned char* p) {   // 0x061 Char Stats
    pwMainJob_    = p[0x0C];
    pw_.jobLevel  = p[0x0D];
    pw_.xpCur     = pkt_u16(p, 0x10);
    pw_.xpTnl     = pkt_u16(p, 0x12);
    pw_.masterLevel = p[0x65];
    pw_.epCur     = pkt_u32(p, 0x68);
    pw_.epTnml    = pkt_u32(p, 0x6C);
    pw_.valid     = true;
}
void PartyState::on_set_update(const unsigned char* p) {   // 0x063 Set Update
    const unsigned order = pkt_u16(p, 0x04);
    if (order == 2) {                                       // Order 2 : merits
        pw_.lpCur     = pkt_u16(p, 0x08);
        pw_.merits    = p[0x0A] & 0x7F;                     // bit[7]
        pw_.maxMerits = p[0x0C];
    } else if (order == 5 && pwMainJob_ >= 1 && pwMainJob_ <= 23) {   // Order 5 : per-job Capacity / Job Points
        const int e = 0x0C + (int)pwMainJob_ * 6;           // job_point_info[jobId] (6 bytes each)
        pw_.cpCur = pkt_u16(p, e);
        pw_.cpJp  = (int)pkt_u16(p, e + 2);
    } else if (order == 9) {                                // Order 9 : SELF BUFF TIMERS -- Buffs u16[32] @0x08, expiry
        buffTimerN_ = 0;                                    //   Time u32[32] @0x48 (absolute FFXI 1/60s ticks). Full refresh.
        for (int i = 0; i < 32; ++i) {
            const unsigned bid = pkt_u16(p, 0x08 + i * 2);
            if (bid == 0xFFFF || bid == 0xFF || bid == 0) continue;   // empty slot
            buffTimers_[buffTimerN_].id = (unsigned short)bid;
            buffTimers_[buffTimerN_].expiry = pkt_u32(p, 0x48 + i * 4);
            ++buffTimerN_;
        }
    }
}
void PartyState::on_exp_msg(const unsigned char* p, unsigned id) {   // 0x029 (Param1 @0x0C) / 0x02D (Param1 @0x10)
    const unsigned val = pkt_u32(p, id == 0x029 ? 0x0C : 0x10);
    const unsigned msg = pkt_u16(p, 0x18);
    if (!val) return;
    if (msg == 8 || msg == 105) {                           // XP gained
        pw_.xpReg.add((int)val);
        pw_.xpCur += val; if (pw_.xpCur > 55999u) pw_.xpCur = 55999u;
        if (pw_.xpTnl && pw_.xpCur > pw_.xpTnl) pw_.xpCur -= pw_.xpTnl;
    } else if (msg == 718 || msg == 735) {                  // Capacity Points gained
        pw_.cpReg.add((int)val);
        pw_.cpCur += val;
        if (pw_.cpCur > 30000u && pw_.cpJp != 500) { pw_.cpJp += (int)(pw_.cpCur / 30000u); if (pw_.cpJp > 500) pw_.cpJp = 500; pw_.cpCur %= 30000u; }
    } else if (msg == 371 || msg == 372) {                  // Limit Points gained (merits)
        pw_.lpCur += val;
        if (pw_.lpCur >= 10000u && pw_.merits != pw_.maxMerits) { pw_.merits += (int)(pw_.lpCur / 10000u); if (pw_.maxMerits && pw_.merits > pw_.maxMerits) pw_.merits = pw_.maxMerits; pw_.lpCur %= 10000u; }
        else if (pw_.lpCur > 9999u) pw_.lpCur = 9999u;
    } else if (msg == 809 || msg == 810) {                  // Exemplar Points gained (Master Level)
        pw_.epReg.add((int)val);
        pw_.epCur += val;
        if (pw_.epTnml && pw_.epCur >= pw_.epTnml) pw_.epCur -= pw_.epTnml;
    }
}

} // namespace aio
