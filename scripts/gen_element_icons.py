#!/usr/bin/env python3
# gen_element_icons.py -- build the elemental-day icon strip for the Minimap clock header.
#
# Source : design/icons/elements/_000N_<Name>.png  (8 full-colour RGBA emblems, ~325x360 each,
#          real transparency in the alpha channel).
# Output : assets/element_icons.raw  -- a 256x32 horizontal strip of 8 cells (32x32 each),
#          straight-alpha BGRA blob (B,G,R,A per pixel, row-major, tightly packed), ready for
#          load_raw_texture(). 256*32*4 = 32768 bytes.
#
# Cell order MUST match VanaClock.dayIdx (src/model/vana_clock.h):
#   0=Fire 1=Earth 2=Water 3=Wind 4=Ice 5=Lightning 6=Light 7=Dark
#
# Pipeline (mirrors gen_gil_icon.ps1 -- PIL's premultiplied resize so leftover background RGB
# never bleeds a coloured fringe into the anti-aliased edges):
#   1. Use each source's OWN alpha (do NOT key/flood-fill).
#   2. Trim to the emblem bounding box (alpha > 8).
#   3. Fit CENTERED into 32x32 with a small transparent margin, aspect preserved, LANCZOS in
#      premultiplied space.
#   4. Paste into the strip at (cell*32, 0) and write straight-alpha BGRA.
#
# Keep the emblems' own colours (drawn as-is, not tinted). Pure asset tooling -- no game/RE.
# Re-run if the source element art changes.

import os
import sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = os.path.join(ROOT, 'design', 'icons', 'elements')
OUT = os.path.join(ROOT, 'assets', 'element_icons.raw')

CELL, MARGIN = 32, 1                       # cell px, transparent margin px
CELLS = 8
W, H = CELL * CELLS, CELL

# dayIdx -> source file (Lightning == Thunder art)
ORDER = [
    '_0007_Fire.png',       # 0 Fire
    '_0002_Earth.png',      # 1 Earth
    '_0001_Water.png',      # 2 Water
    '_0003_Wind.png',       # 3 Wind
    '_0006_Ice.png',        # 4 Ice
    '_0000_Thunder.png',    # 5 Lightning
    '_0005_Light.png',      # 6 Light
    '_0004_Dark.png',       # 7 Dark
]


def fit_cell(path):
    """Return a 32x32x4 uint8 RGBA canvas holding the emblem, aspect-preserved & centered."""
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
        print('[element-icons] cell %d = %-16s center-alpha=%d' % (i, name, ca))

    bgra = strip[..., [2, 1, 0, 3]].tobytes()       # straight-alpha BGRA (B,G,R,A)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'wb') as f:
        f.write(bgra)
    print('[element-icons] OK -> %s  (%d bytes)  %dx%d  %d cells of %dpx  straight-alpha BGRA'
          % (OUT, len(bgra), W, H, CELLS, CELL))
    assert len(bgra) == W * H * 4, 'unexpected size'


if __name__ == '__main__':
    main()
