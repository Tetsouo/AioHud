// entity_color.h -- the shared claim/allegiance colour convention used by the Target name and the Minimap
// markers. One palette (mob unclaimed / your-claim red / other-claim magenta ; NPC green ; PC self+solo white
// / your-party cyan / another-party blue) so the two boxes can never drift. The caller normalises its entity
// (spawnType bits vs the minimap's precomputed type) into isMob/isPC and passes the "unclaimed mob" colour
// (Target its configurable TGT_NAME base ; the minimap gold).
#pragma once
#include "gfx/d3d.h"             // u32
#include "model/party_state.h"   // party() : selfId_ / find

namespace aio {

// id belongs to you or your party/alliance roster.
inline bool in_my_group(unsigned id) { return id != 0 && (id == party().selfId_ || party().find(id) >= 0); }

// the claim/allegiance colour. `mobBase` = colour for an unclaimed/idle mob.
inline u32 allegiance_color(bool isMob, bool isPC, unsigned id, unsigned claimId, unsigned status, unsigned pflags, u32 mobBase) {
    if (isMob) {                                                          // MOB -> by claim
        if (claimId != 0 && !in_my_group(claimId)) return 0xFFF67AF5u;    // another player's / alliance-mate's claim -> magenta
        if (claimId != 0 || status == 1)           return 0xFFF0787Au;    // your own / party claim OR engaged -> red
        return mobBase;                                                  // unclaimed / idle
    }
    if (!isPC)                        return 0xFFBCF4BCu;                 // not a PC and not a mob -> NPC / object -> soft green
    if (id == party().selfId_)        return 0xFFFFFFFFu;                 // PC : yourself -> white
    if (in_my_group(id))              return 0xFFB6EEF0u;                 //      your party -> cyan
    if (pflags & 0x00800000u)         return 0xFF7AB7F1u;                 //      another party -> blue
    return 0xFFFFFFFFu;                                                   //      solo -> white
}

} // namespace aio
