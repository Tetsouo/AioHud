# gen_gil_icon.ps1 -- build the GIL coin icon from a full-colour illustration.
#
# Source : an RGBA gil-coin illustration whose background is ALREADY transparent (alpha 0).
#          Default: assets/icon_gil_src/gil.png (the source of record); override with -Src.
# Output : assets\icon_gil.raw -- 64x64 straight-alpha BGRA blob for load_raw_texture().
#
# Pipeline (System.Drawing's premultiplied resize is lossy, so this shells out to Python + PIL/NumPy):
#   1. Use the source's OWN alpha channel (do NOT key/flood-fill -- the background is real transparency ;
#      transparent pixels merely carry leftover RGB, which must be ignored).
#   2. Trim to the coin bounding box (alpha > 8).
#   3. Fit CENTERED into 64x64 with a ~2px transparent margin, aspect preserved. The source is small
#      (23x23) so it UPSCALES to fill -- resize runs in PREMULTIPLIED-alpha space with LANCZOS so the
#      leftover background RGB never bleeds a coloured fringe into the anti-aliased edge.
#   4. Write straight-alpha BGRA (B,G,R,A per pixel; byte order LockBits Format32bppArgb produces),
#      row-major, tightly packed = 64*64*4 = 16384 bytes.
#
# Pure asset tooling -- no game/RE involved. Re-run if the source gil art changes.

param(
    [string]$Src = (Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\icon_gil_src\gil.png'),
    [string]$Out = (Join-Path (Split-Path $PSScriptRoot -Parent) 'assets\icon_gil.raw')
)

$ErrorActionPreference = 'Stop'

$python = (Get-Command python -ErrorAction SilentlyContinue)
if (-not $python) { throw "python not found on PATH (needs Python 3 with Pillow + NumPy)." }
if (-not (Test-Path $Src)) { throw "source image not found: $Src" }

$py = @'
import sys
import numpy as np
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
SIZE, MARGIN = 64, 2   # canvas px, transparent margin px

img = Image.open(src).convert('RGBA')
a = np.asarray(img).astype(np.float64)          # H,W,4  -- keep the REAL alpha
H, W = a.shape[:2]
alpha = a[..., 3]

# --- 2. trim to the coin bounding box (by the source's own alpha) ---------------------------
ys, xs = np.where(alpha > 8)
y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
cw, ch = x1 - x0, y1 - y0

# --- 3. fit centered into SIZE x SIZE with MARGIN, aspect preserved (premultiplied resize) ---
pm = a[..., 0:3] * (alpha[..., None] / 255.0)   # premultiply so transparent RGB can't bleed
crop_pm = pm[y0:y1, x0:x1]
crop_a  = alpha[y0:y1, x0:x1]

avail = SIZE - 2 * MARGIN
scale = min(avail / cw, avail / ch)
tw, th = max(1, round(cw * scale)), max(1, round(ch * scale))

pm_s = np.stack([np.asarray(Image.fromarray(crop_pm[..., c].astype(np.uint8)).resize((tw, th), Image.LANCZOS))
                 for c in range(3)], axis=2).astype(np.float64)
a_s = np.asarray(Image.fromarray(crop_a.astype(np.uint8)).resize((tw, th), Image.LANCZOS)).astype(np.float64)
with np.errstate(divide='ignore', invalid='ignore'):
    rgb_s = np.where(a_s[..., None] > 0, pm_s / (a_s[..., None] / 255.0), 0)
rgb_s = np.clip(rgb_s, 0, 255)

canvas = np.zeros((SIZE, SIZE, 4), np.uint8)
ox, oy = (SIZE - tw) // 2, (SIZE - th) // 2
canvas[oy:oy + th, ox:ox + tw, 0:3] = rgb_s.astype(np.uint8)
canvas[oy:oy + th, ox:ox + tw, 3]   = np.clip(a_s, 0, 255).astype(np.uint8)

# --- 4. straight-alpha BGRA raw ------------------------------------------------------------
bgra = canvas[..., [2, 1, 0, 3]].tobytes()
with open(out, 'wb') as f:
    f.write(bgra)

def A(y, x):
    return int(canvas[y, x, 3])
print("[gil-icon] coin bbox %dx%d @(%d,%d)  fit %dx%d @(%d,%d)  bytes=%d"
      % (cw, ch, x0, y0, tw, th, ox, oy, len(bgra)))
print("[gil-icon] corner alpha TL,TR,BL,BR = %d,%d,%d,%d   center = %d"
      % (A(0, 0), A(0, SIZE - 1), A(SIZE - 1, 0), A(SIZE - 1, SIZE - 1), A(SIZE // 2, SIZE // 2)))
'@

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("gen_gil_icon_{0}.py" -f ([guid]::NewGuid().ToString('N')))
try {
    Set-Content -LiteralPath $tmp -Value $py -Encoding UTF8
    & $python.Source $tmp $Src $Out
    if ($LASTEXITCODE -ne 0) { throw "python pipeline failed (exit $LASTEXITCODE)." }
} finally {
    Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue
}

$len = (Get-Item -LiteralPath $Out).Length
Write-Host "[gil-icon] OK -> $Out  ($len bytes)  64x64 straight-alpha BGRA"
if ($len -ne 16384) { throw "unexpected output size $len (expected 16384)." }
