// song_dur.h -- BRD song duration for a song cast on an ally, reproducing the Windower Timers model
// (RE'd from Timers.dll FUN_10007a40 ; see scratchpad/timers_song_phase2.txt + docs bard.md/fili).
//
//   dur = 120 (Base ; Miracle Cheer -> 900) x m1 x m2 x m3 + a3
//   m1 = 1 + SUM(per-item FLAT "song effect duration +%" gear) + 0.05 JP    (the +1<Song> gear is POTENCY, not here)
//   m2 = 1 + 1.0 if Troubadour (status 348)                                   -> x2
//   m3 = 1 + 0.5 if Soul Voice(52) or Marcato(231) AND song family in {11 Hymnus, 12 Mazurka, 14 Scherzo}
//   a3 = flat merit seconds : Clarion Call(499)/Tenuto(455) -> merit@+0x142 x2 ; Marcato(231) -> merit@+0x148 x1
//
// Song-duration gear is a QUALITATIVE res stat (no number), so the per-item %s are HARDCODED from Timers.dll
// (raw FFXI item ids, no name hash). Each item adds a FLAT % (all songs) and/or a FAMILY % (one song family).
#pragma once
#include "model/song_family_gen.h"   // song_family(spell) : spell id -> song family id
namespace aio {

struct SongDurItem { unsigned short id; unsigned char flat; unsigned char family; unsigned char familyPct; };
static const SongDurItem SONG_DUR[] = {
    // weapon (Carnwenhan ilvl stages, Kali/Legato) -- flat
    {0x4a38,10,0,0},{0x4a7d,20,0,0},{0x4a91,30,0,0},{0x4ca5,40,0,0},{0x4df5,50,0,0},{0x5051,50,0,0},{0x5052,50,0,0},{0x506a,50,0,0},{0x5077,5,0,0},{0x5095,5,0,0},
    // instruments (range) -- family-gated
    {0x43c0,0,4,10},{0x43c1,0,5,10},{0x43c2,0,5,10},{0x43c3,0,5,10},{0x43c4,0,5,10},{0x43c5,0,8,20},{0x43c6,0,6,10},{0x43c7,0,7,10},{0x43c8,0,5,10},{0x43c9,0,5,10},
    {0x43ca,0,3,10},{0x43cb,0,9,10},{0x43cc,0,5,10},{0x43cd,0,1,10},{0x43ce,0,1,30},{0x43cf,0,9,10},{0x43d0,0,8,30},{0x43d1,0,10,10},{0x43d2,0,5,10},{0x43d3,0,11,30},
    {0x43d4,0,5,10},{0x43d5,0,5,10},{0x43d6,0,5,10},{0x43d7,0,8,10},{0x43d8,0,5,10},{0x43d9,0,4,20},{0x43da,0,7,20},{0x43db,0,5,10},{0x43dc,0,5,10},{0x43dd,0,3,10},
    {0x43de,0,3,20},{0x43df,0,5,20},{0x43e0,0,9,20},{0x43e1,0,10,20},{0x43e2,0,6,20},{0x45a9,0,1,20},{0x45aa,0,9,20},{0x45ab,0,8,10},{0x45ac,0,8,10},{0x45ad,0,5,10},
    {0x45ae,0,12,20},{0x45af,0,5,10},{0x45b0,0,11,20},{0x45b1,0,5,10},{0x45b2,0,5,10},{0x45b3,0,5,10},{0x45b4,0,5,10},{0x45b5,0,5,20},{0x45b6,0,4,20},{0x45b7,0,10,20},
    {0x45b8,0,1,20},{0x45b9,0,7,10},{0x45ba,0,7,20},{0x45bb,0,5,10},{0x45bc,0,5,10},{0x45bd,0,8,20},{0x45be,0,5,10},{0x45bf,0,5,10},{0x45c0,0,3,30},{0x5722,30,0,0},{0x5723,40,0,0},
    // armour (Aoidos/Fili/Brioso/Mousai/Inyanga)
    {0x2b41,0,5,10},{0x2b55,10,4,10},{0x2b69,0,8,10},{0x2b7d,0,2,10},{0x2b91,0,14,10},{0x2d62,10,0,0},{0x5a09,0,1,10},{0x5a36,0,5,10},{0x5a79,13,4,10},{0x5abc,0,8,10},
    {0x5b15,13,0,0},{0x5b42,0,14,10},{0x5b58,0,1,20},{0x5b85,0,5,10},{0x5bc8,14,4,10},{0x5c0b,0,8,10},{0x5c4e,0,2,10},{0x5c64,15,0,0},{0x5c91,0,14,10},{0x5e14,15,0,0},
    {0x63d9,0,9,10},{0x63da,0,9,20},{0x6509,12,0,0},{0x650a,15,0,0},{0x651a,17,0,0},{0x652d,0,3,10},{0x652e,0,3,20},{0x6570,0,7,10},{0x6571,0,7,20},{0x6584,0,10,10},
    {0x6585,0,10,20},{0x65af,10,0,0},{0x65b1,30,0,0},{0x668f,0,5,10},{0x6886,0,5,10},{0x6887,0,5,10},{0x69be,0,8,10},{0x69bf,0,8,10},{0x6a77,0,2,10},{0x6a78,0,2,10},
    {0x6b25,0,14,10},{0x6b26,0,14,10},{0x6c18,0,1,10},{0x6c2d,0,1,10},{0x6e48,10,0,0},{0x6e5d,11,0,0},
};
static const int SONG_DUR_N = (int)(sizeof(SONG_DUR) / sizeof(SONG_DUR[0]));
static const unsigned short MIRACLE_CHEER_ID = 0x56E9;   // RANGE slot -> flat 900 s song

// m1 % = the FLAT "Song effect duration +N%" gear, summed over equipped slots. The per-item `family/familyPct`
// columns are the gear's "+1 <Song>" POTENCY bonus (Minuet=Attack, Ballad=MP/tick, ... -- see Song Potency doc),
// NOT duration ; the RE mis-read them into the timer's m1 register, so they are intentionally NOT counted here.
inline int song_dur_m1_pct(const unsigned short ids[16], int /*songFamily*/) {
    int p = 0;
    for (int s = 0; s < 16; ++s) { if (!ids[s]) continue;
        for (int i = 0; i < SONG_DUR_N; ++i) if (SONG_DUR[i].id == ids[s]) { p += SONG_DUR[i].flat; break; }
    }
    return p;
}
inline bool has_miracle_cheer(const unsigned short ids[16]) { return ids[2] == MIRACLE_CHEER_ID; }   // slot 2 = range

} // namespace aio
