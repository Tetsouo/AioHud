#!/usr/bin/env python3
# gen_weapon_icons.py -- build the physical-damage-type icon strip (Slashing / Piercing / Blunt).
#
# Source : addons/AioHUD/assets/bst/rdy_<type>.png  (3 full-colour RGBA icons with real
#          transparency in the alpha channel).
#            rdy_slashing.png  rdy_piercing.png  rdy_blunt.png
# Output : assets/weapon_icons.raw  -- a 96x32 horizontal strip of 3 cells (32x32 each),
#          straight-alpha BGRA blob (B,G,R,A per pixel, row-major, tightly packed), ready for
#          load_raw_texture(dev, path, 96, 32). 96*32*4 = 12288 bytes.
#
# Cell order MUST match the res weapon index (0/1/2):
#   0=Slashing 1=Piercing 2=Blunt
#
# Pipeline (mirrors gen_element_icons.py -- PIL's premultiplied resize so leftover background RGB
# never bleeds a coloured fringe into the anti-aliased edges):
#   1. Use each source's OWN alpha (do NOT key/flood-fill).
#   2. Trim to the icon bounding box (alpha > 8).
#   3. Fit CENTERED into 32x32 with a small transparent margin, aspect preserved, LANCZOS in
#      premultiplied space.
#   4. Paste into the strip at (cell*32, 0) and write straight-alpha BGRA.
#
# Keep the icons' own colours (drawn as-is, not tinted). Pure asset tooling -- no game/RE.
# Re-run if the source weapon art changes.

import os
import sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = os.path.join(ROOT, 'assets', 'weapon_icons_src')   # repo-local source PNGs (kept for regeneration)
OUT = os.path.join(ROOT, 'assets', 'weapon_icons.raw')

CELL, MARGIN = 32, 1                       # cell px, transparent margin px
CELLS = 3
W, H = CELL * CELLS, CELL

# weapon index -> source file
ORDER = [
    '_0001_Slashing.png',   # 0 Slashing
    '_0000_Piercing.png',   # 1 Piercing
    '_0005_Blunt.png',      # 2 Blunt
]


def fit_cell(path):
    """Return a 32x32x4 uint8 RGBA canvas holding the icon, aspect-preserved & centered."""
    img = Image.open(path).convert('RGBA')
    a = np.asarray(img).astype(np.float64)          # H,W,4  -- keep the REAL alpha
    alpha = a[..., 3]

    ys, xs = np.where(alpha > 8)
    if len(xs) == 0:                                # fully transparent -> empty cell
        return np.zeros((CELL, CELL, 4), np.uint8)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    cw, ch = x1 - x0, y1 - y0

    pm = a[..., 0:3] * (alpha[..., None] / 255.0)   # premultiply so transparent RGB can't bleed
    crop_pm = pm[y0:y1, x0:x1]
    crop_a = alpha[y0:y1, x0:x1]

    avail = CELL - 2 * MARGIN
    scale = min(avail / cw, avail / ch)
    tw, th = max(1, round(cw * scale)), max(1, round(ch * scale))

    pm_s = np.stack([np.asarray(Image.fromarray(crop_pm[..., c].astype(np.uint8)).resize((tw, th), Image.LANCZOS))
                     for c in range(3)], axis=2).astype(np.float64)
    a_s = np.asarray(Image.fromarray(crop_a.astype(np.uint8)).resize((tw, th), Image.LANCZOS)).astype(np.float64)
    with np.errstate(divide='ignore', invalid='ignore'):
        rgb_s = np.where(a_s[..., None] > 0, pm_s / (a_s[..., None] / 255.0), 0)
    rgb_s = np.clip(rgb_s, 0, 255)

    canvas = np.zeros((CELL, CELL, 4), np.uint8)
    ox, oy = (CELL - tw) // 2, (CELL - th) // 2
    canvas[oy:oy + th, ox:ox + tw, 0:3] = rgb_s.astype(np.uint8)
    canvas[oy:oy + th, ox:ox + tw, 3] = np.clip(a_s, 0, 255).astype(np.uint8)
    return canvas


def main():
    strip = np.zeros((H, W, 4), np.uint8)
    for i, name in enumerate(ORDER):
        path = os.path.join(SRC_DIR, name)
        if not os.path.isfile(path):
            sys.exit('source not found: %s' % path)
        cell = fit_cell(path)
        strip[:, i * CELL:(i + 1) * CELL, :] = cell
        ca = int(cell[CELL // 2, CELL // 2, 3])
        print('[weapon-icons] cell %d = %-16s center-alpha=%d' % (i, name, ca))

    bgra = strip[..., [2, 1, 0, 3]].tobytes()       # straight-alpha BGRA (B,G,R,A)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'wb') as f:
        f.write(bgra)
    print('[weapon-icons] OK -> %s  (%d bytes)  %dx%d  %d cells of %dpx  straight-alpha BGRA'
          % (OUT, len(bgra), W, H, CELLS, CELL))
    assert len(bgra) == W * H * 4, 'unexpected size'


if __name__ == '__main__':
    main()
