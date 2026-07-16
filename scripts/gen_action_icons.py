#!/usr/bin/env python3
# gen_action_icons.py -- build the FFXI ACTION-icon atlas (spell / ability icons) for AioHUD.
#
# Source : D:/Windower Tetsouo/addons/AioHUD/assets/icons/000.png .. 640.png -- 641 full-colour
#          RGBA icons, zero-padded 3-digit filenames INDEXED BY FFXI icon_id (086.png == icon_id 86).
#          Each has real transparency in its own alpha channel.
# Output : assets/action_icons.raw -- a 1024x672 atlas: a 32-COLUMN grid of 32x32 cells
#          (32 cols x 21 rows; 32*21 = 672 >= 641). Straight-alpha BGRA blob (B,G,R,A per pixel,
#          row-major, tightly packed), ready for load_raw_texture(dev, path, 1024, 672).
#          1024*672*4 = 2752512 bytes.
#
# Cell placement (MUST match the C++ loader): icon_id N -> cell (col = N % 32, row = N // 32),
#   i.e. pixel origin (32 * (N % 32), 32 * (N // 32)). File NNN.png lands in that cell.
#   Missing files in 0..640 leave their cell fully transparent.
#
# Pipeline (mirrors gen_element_icons.py -- PIL's premultiplied resize so leftover background RGB
# never bleeds a coloured fringe into the anti-aliased edges):
#   1. Use each source's OWN alpha (do NOT key/flood-fill).
#   2. Trim to the icon bounding box (alpha > 8).
#   3. Fit CENTERED into 32x32 with a small transparent margin, aspect preserved, LANCZOS in
#      premultiplied space.
#   4. Paste into the atlas at (col*32, row*32) and write straight-alpha BGRA.
#
# Keep the icons' own colours (drawn as-is, not tinted). Pure asset tooling -- no game/RE.
# Re-run if the source action-icon art changes.

import os
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIR = r'D:\Windower Tetsouo\addons\AioHUD\assets\icons'
OUT = os.path.join(ROOT, 'assets', 'action_icons.raw')

CELL, MARGIN = 32, 1                        # cell px, transparent margin px
COLS, ROWS = 32, 21                         # 32 cols x 21 rows = 672 cells (>= 641 icons)
COUNT = 641                                 # icon_id 0..640
W, H = CELL * COLS, CELL * ROWS             # 1024 x 672


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
    atlas = np.zeros((H, W, 4), np.uint8)
    present = 0
    for n in range(COUNT):
        path = os.path.join(SRC_DIR, '%03d.png' % n)
        if not os.path.isfile(path):
            continue                                # missing id -> leave cell transparent
        cell = fit_cell(path)
        col, row = n % COLS, n // COLS
        px, py = col * CELL, row * CELL
        atlas[py:py + CELL, px:px + CELL, :] = cell
        present += 1

    bgra = atlas[..., [2, 1, 0, 3]].tobytes()       # straight-alpha BGRA (B,G,R,A)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'wb') as f:
        f.write(bgra)
    print('[action-icons] OK -> %s  (%d bytes)  %dx%d  %d/%d icons  cell=%dpx  grid=%dx%d  straight-alpha BGRA'
          % (OUT, len(bgra), W, H, present, COUNT, CELL, COLS, ROWS))
    assert len(bgra) == W * H * 4, 'unexpected size'


if __name__ == '__main__':
    main()
