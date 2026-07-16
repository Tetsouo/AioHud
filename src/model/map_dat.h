// map_dat.h -- extract a zone's map image from the FFXI ROM DATs (reversed from FFXIDB, see
// docs/game-data/map-system.md section 4). Resolves a map file-id via the VTABLE/FTABLE volume scheme
// off the FFXI install (registry PlayOnline InstallFolder), reads the DAT, and decodes the 8-bit-palette
// graphic chunk to an A8R8G8B8 buffer. The stored image is horizontally MIRRORED -> flip U at draw.
#pragma once
#include "windower.h"   // u32 (windower::u32) -- model must not depend on gfx/
using windower::u32;

namespace aio {

// Load the map image for `fileId` (MapRecord::fileId). On success allocates a W*H A8R8G8B8 buffer
// (0xAARRGGBB, alpha already scaled 0..0x80 -> 0..255) and returns it via outPixels (free with
// free_map_image), with dims in outW/outH. Returns false if the install/tables/DAT/chunk aren't found.
bool load_zone_map(unsigned fileId, u32*& outPixels, int& outW, int& outH);
void free_map_image(u32* pixels);

} // namespace aio
