// party_state_internal.h -- shared inline helpers used by party_state.cpp and its split module TUs. Not public.
#pragma once
#include "windower.h"
#include "model/party_state.h"   // HateEntry (for record_hate below)
namespace aio {
inline unsigned pkt_u16(const unsigned char* p, int o) { return (unsigned)p[o] | ((unsigned)p[o + 1] << 8); }
inline unsigned pkt_u32(const unsigned char* p, int o) { return (unsigned)p[o] | ((unsigned)p[o + 1] << 8) | ((unsigned)p[o + 2] << 16) | ((unsigned)p[o + 3] << 24); }
// Declared packet length in bytes (FFXI header : u16@0 = id(9 bits) | size-in-dwords(7 bits)). A truncated
// packet still lands in the decode buffer, so the SEH guard alone won't catch it reading garbage past the
// real end -> each handler floors on its highest field offset before parsing (0x029 already did).
inline int pkt_bytes(const unsigned char* p) {
    return (int)(((((unsigned)p[0] | ((unsigned)p[1] << 8)) >> 9) & 0x7F) * 4);
}
// HATE LIST enmity tracker : record/refresh a mob->PC pair. Defined in party_state_hate.cpp ; called by
// on_pet_status (party_state_hate.cpp) AND the hate block in on_action (party_state.cpp).
void record_hate(HateEntry* h, unsigned mob, unsigned pc);
}
