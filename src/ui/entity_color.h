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
// `spawn` = the raw SpawnType byte (0 = caller has none). Bit 2 = in YOUR party, bit 3 = in your ALLIANCE --
// the client's own flags, read straight off the entity. They separate party from alliance, which the roster
// lookup below cannot: party().find() matches both, so alliance mates used to take the party colour.
inline u32 allegiance_color(bool isMob, bool isPC, unsigned id, unsigned claimId, unsigned status, unsigned pflags, u32 mobBase, unsigned spawn = 0) {
    if (isMob) {                                                          // MOB -> by claim
        if (claimId != 0 && !in_my_group(claimId)) return 0xFFF67AF5u;    // another player's / alliance-mate's claim -> magenta
        if (claimId != 0 || status == 1)           return 0xFFF0787Au;    // your own / party claim OR engaged -> red
        return mobBase;                                                  // unclaimed / idle
    }
    if (!isPC)                        return 0xFFBCF4BCu;                 // not a PC and not a mob -> NPC / object -> soft green
    if (id == party().selfId_)        return 0xFFFFFFFFu;                 // PC : yourself -> white
    if (spawn & 0x04)                 return 0xFFB6EEF0u;                 //      YOUR PARTY (client bit 2) -> cyan
    if (spawn & 0x08)                 return 0xFF8FE0B4u;                 //      your ALLIANCE (client bit 3) -> mint
    if (in_my_group(id))              return 0xFFB6EEF0u;                 //      roster fallback (no spawn byte) -> cyan
    if (pflags & 0x00800000u)         return 0xFF7AB7F1u;                 //      another party -> blue
    return 0xFFFFFFFFu;                                                   //      solo -> white
}

} // namespace aio
