// texture.cpp -- see texture.h.
#include "texture.h"
#include "noise.h"
#include <windows.h>
#include <math.h>

namespace aio {

// small struct matching D3DLOCKED_RECT's first two fields (Pitch, pBits).
struct LR { int Pitch; void* pBits; };

// create an A8R8G8B8 MANAGED texture and lock its top mip for writing.
// returns the texture (0 on failure); fills lr on success.
static u32 create_locked(u32 dev, int W, int H, LR* lr)
{
    lr->Pitch = 0; lr->pBits = 0;
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    u32 tex = 0;
    if (!fCreate || fCreate(dev, W, H, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) < 0 || !valid_ptr(tex))
        return 0;
    auto fLock = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
    if (!fLock || fLock(tex, 0, lr, 0, 0) < 0 || !lr->pBits) return tex;   // created but unlocked
    return tex;
}

static void unlock(u32 tex)
{
    auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);
    if (fUnlock) fUnlock(tex, 0);
}

u32 make_liquid_texture(u32 dev, int variant)
{
    const int W = 512, H = 64;   // ~8:1, MATCHES the bar's aspect -> shown 1x (no tiling, no repeat)
    LR lr; u32 tex = create_locked(dev, W, H, &lr);
    if (tex && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float u = (float)x / W, v = (float)y / H;
                // SMOOTH liquid: broad, soft, LOW-CONTRAST light variation -- no thin
                // filaments, no busy detail. The "liquid" feel comes from the MOTION
                // (flow + traveling specular + meniscus), not from a detailed texture.
                float bright;
                if (variant == 1) {                                  // MP : slow arcane swirl (gentle, low freq)
                    float wx = fbm(1 * u, 1 * v + 0.30f) - 0.5f;
                    float n  = fbm(3 * u + 0.6f * wx, 1 * v);
                    bright = 0.45f + 0.48f * n;
                } else {                                             // HP / TP : soft broad liquid (8:1 texture)
                    int fu = (variant == 2) ? 5 : 3;                 // horizontal cycles (texture is 8:1 -> gentle streak)
                    int fv = 1;                                      // 1 -> large soft vertical variation (no tiny detail)
                    float n  = fbm(fu * u, fv * v);
                    float c  = (variant == 2) ? 0.50f : 0.42f;       // TP a touch more contrast (energetic)
                    bright = (0.5f - 0.5f * c) + c * n;
                }
                bright = 0.62f + 0.38f * bright;                     // HIGH floor -> the grey texture barely dims the vivid colour
                if (bright > 1.0f) bright = 1.0f; if (bright < 0.0f) bright = 0.0f;
                u32 b = (u32)(bright * 255.0f);
                // mostly opaque -> colour reads saturated over the dark game background.
                float al = 0.74f + 0.24f * bright;
                u32 av = (u32)(al * 255.0f);
                row[x] = (av << 24) | (b << 16) | (b << 8) | b;
            }
        }
        unlock(tex);
    }
    return tex;
}

u32 make_glow(u32 dev)
{
    const int W = 128, H = 64;
    LR lr; u32 tex = create_locked(dev, W, H, &lr);
    if (tex && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float nx = ((x + 0.5f) / W) * 2.0f - 1.0f;
                float ny = ((y + 0.5f) / H) * 2.0f - 1.0f;
                // CAPSULE / stadium falloff: flat bright core with SEMICIRCULAR rounded
                // ends. cx = straight half-length; kx compresses the end caps in
                // texture-x so they look round once the quad is stretched (~3.6x) wide.
                const float cx = 0.72f, kx = 3.6f;
                float dx = fabsf(nx) - cx; if (dx < 0) dx = 0;
                float d  = sqrtf((dx * kx) * (dx * kx) + ny * ny);   // 0 core .. 1 edges, rounded ends
                float a  = 1.0f - d; if (a < 0) a = 0; if (a > 1) a = 1;
                a = a * (2.0f - a);                          // ease-out -> SOFT tail, melts to 0
                u32 av = (u32)(a * 255.0f);
                row[x] = 0xFF000000 | (av << 16) | (av << 8) | av;   // shape in RGB, alpha opaque
            }
        }
        unlock(tex);
    }
    return tex;
}

u32 make_bubble(u32 dev)
{
    const int W = 32, H = 32;
    LR lr; u32 tex = create_locked(dev, W, H, &lr);
    if (tex && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* row = (u32*)((char*)lr.pBits + y * lr.Pitch);
            for (int x = 0; x < W; x++) {
                float nx = ((x + 0.5f) / W) * 2.0f - 1.0f;
                float ny = ((y + 0.5f) / H) * 2.0f - 1.0f;
                float d = sqrtf(nx * nx + ny * ny);
                float body = 1.0f - d; if (body < 0) body = 0; body = body * body * 0.45f; // faint fill
                float rim = 1.0f - fabsf(d - 0.72f) * 4.5f; if (rim < 0) rim = 0;           // bright thin ring
                float a = body + rim * 0.95f; if (a > 1) a = 1; if (a < 0) a = 0;
                u32 av = (u32)(a * 255.0f);
                row[x] = 0xFF000000 | (av << 16) | (av << 8) | av;   // shape in RGB, alpha opaque
            }
        }
        unlock(tex);
    }
    return tex;
}

u32 load_raw_texture(u32 dev, const char* path, int W, int H)
{
    HANDLE hf = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hf == INVALID_HANDLE_VALUE) return 0;
    DWORD need = (DWORD)(W * H * 4), got = 0;
    char* buf = (char*)HeapAlloc(GetProcessHeap(), 0, need);
    if (!buf) { CloseHandle(hf); return 0; }
    BOOL ok = ReadFile(hf, buf, need, &got, 0);
    CloseHandle(hf);
    if (!ok || got != need) { HeapFree(GetProcessHeap(), 0, buf); return 0; }

    LR lr; u32 tex = create_locked(dev, W, H, &lr);
    if (tex && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* dpix = (u32*)((char*)lr.pBits + y * lr.Pitch);
            u32* spix = (u32*)(buf + y * W * 4);
            for (int xx = 0; xx < W; xx++) dpix[xx] = spix[xx];
        }
        unlock(tex);
    }
    HeapFree(GetProcessHeap(), 0, buf);
    return tex;
}

u32 make_texture_argb(u32 dev, int W, int H, const u32* pixels)
{
    LR lr; u32 tex = create_locked(dev, W, H, &lr);
    if (tex && lr.pBits) {
        for (int y = 0; y < H; y++) {
            u32* dpix = (u32*)((char*)lr.pBits + y * lr.Pitch);
            const u32* spix = pixels + y * W;
            for (int x = 0; x < W; x++) dpix[x] = spix[x];
        }
        unlock(tex);
    }
    return tex;
}

static int mip_count(int w, int h) {
    int n = 1; while (w > 1 || h > 1) { w >>= 1; if (!w) w = 1; h >>= 1; if (!h) h = 1; n++; } return n;
}

u32 make_texture_argb_mip(u32 dev, int W, int H, const u32* pixels)
{
    int levels = mip_count(W, H);
    auto fCreate = vmethod<long(__stdcall*)(u32,u32,u32,u32,u32,u32,u32,u32*)>(dev, 20);
    u32 tex = 0;
    if (!fCreate || fCreate(dev, W, H, levels, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex) < 0 || !valid_ptr(tex))
        return 0;
    auto fLock   = vmethod<long(__stdcall*)(u32,u32,void*,void*,u32)>(tex, 16);
    auto fUnlock = vmethod<long(__stdcall*)(u32,u32)>(tex, 17);

    u32* cur = (u32*)HeapAlloc(GetProcessHeap(), 0, W * H * 4);
    if (!cur) return tex;
    for (int i = 0; i < W * H; ++i) cur[i] = pixels[i];

    int lw = W, lh = H;
    for (int lvl = 0; lvl < levels; ++lvl) {
        LR lr; lr.Pitch = 0; lr.pBits = 0;
        if (fLock && fLock(tex, lvl, &lr, 0, 0) >= 0 && lr.pBits) {
            for (int y = 0; y < lh; ++y) {
                u32* d = (u32*)((char*)lr.pBits + y * lr.Pitch);
                for (int x = 0; x < lw; ++x) d[x] = cur[y * lw + x];
            }
            if (fUnlock) fUnlock(tex, lvl);
        }
        if (lvl + 1 >= levels) break;
        int nw = lw > 1 ? lw / 2 : 1, nh = lh > 1 ? lh / 2 : 1;
        u32* nxt = (u32*)HeapAlloc(GetProcessHeap(), 0, nw * nh * 4);
        if (!nxt) break;
        for (int y = 0; y < nh; ++y) for (int x = 0; x < nw; ++x) {
            int x0 = x * 2, y0 = y * 2, x1 = (x0 + 1 < lw) ? x0 + 1 : x0, y1 = (y0 + 1 < lh) ? y0 + 1 : y0;
            u32 a = cur[y0 * lw + x0], b = cur[y0 * lw + x1], c = cur[y1 * lw + x0], e = cur[y1 * lw + x1];
            u32 A = (((a>>24)&0xFF)+((b>>24)&0xFF)+((c>>24)&0xFF)+((e>>24)&0xFF)) / 4;
            u32 R = (((a>>16)&0xFF)+((b>>16)&0xFF)+((c>>16)&0xFF)+((e>>16)&0xFF)) / 4;
            u32 G = (((a>>8) &0xFF)+((b>>8) &0xFF)+((c>>8) &0xFF)+((e>>8) &0xFF)) / 4;
            u32 Bl = ((a&0xFF)+(b&0xFF)+(c&0xFF)+(e&0xFF)) / 4;
            nxt[y * nw + x] = (A << 24) | (R << 16) | (G << 8) | Bl;
        }
        HeapFree(GetProcessHeap(), 0, cur); cur = nxt; lw = nw; lh = nh;
    }
    HeapFree(GetProcessHeap(), 0, cur);
    return tex;
}

// ---- party marker icons (real PNGs from game-icons.net) -------------------------
// White silhouettes converted to 128x128 BGRA raw under assets/ (PowerShell pre-step,
// no PNG decoder in the plugin). Loaded into an A8R8G8B8 + mip texture; the party
// widget draws them TINTED (COLOROP=MODULATE with a diffuse colour). To change an
// icon: drop a new PNG, re-run the convert step -> assets/icon_*.raw.
u32 make_dot(u32 dev) {                              // solid white disc with a smooth (AA) edge
    const int S = 32; u32 buf[32 * 32];
    const float cx = 16.0f, cy = 16.0f, R = 12.0f, fth = 1.5f;   // 4px transparent margin -> edge clip/wrap never touches the disc
    for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) {
        float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
        float d = sqrtf(dx * dx + dy * dy);
        float a = (R - d) / fth + 0.5f; if (a < 0) a = 0; if (a > 1) a = 1;   // 1 inside -> 0 just past R
        u32 av = (u32)(a * 255.0f + 0.5f);
        buf[y * S + x] = (av << 24) | 0x00FFFFFF;                            // white, alpha = coverage
    }
    return make_texture_argb_mip(dev, S, S, buf);
}

static u32 load_icon_raw(u32 dev, const char* path) {
    const int N = 128;
    HANDLE hf = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hf == INVALID_HANDLE_VALUE) return 0;
    DWORD need = (DWORD)(N * N * 4), got = 0;
    u32* buf = (u32*)HeapAlloc(GetProcessHeap(), 0, need);
    if (!buf) { CloseHandle(hf); return 0; }
    BOOL ok = ReadFile(hf, buf, need, &got, 0);
    CloseHandle(hf);
    u32 tex = (ok && got == need) ? make_texture_argb_mip(dev, N, N, buf) : 0;
    HeapFree(GetProcessHeap(), 0, buf);
    return tex;
}
u32 make_icon_party_lead(u32 dev)    { return load_icon_raw(dev, "D:\\Windower Tetsouo\\plugins\\_aiohud_re\\assets\\icon_crown.raw"); }
u32 make_icon_alliance_lead(u32 dev) { return load_icon_raw(dev, "D:\\Windower Tetsouo\\plugins\\_aiohud_re\\assets\\icon_laurel.raw"); }
u32 make_icon_qm(u32 dev)            { return load_icon_raw(dev, "D:\\Windower Tetsouo\\plugins\\_aiohud_re\\assets\\icon_chest.raw"); }

void release_texture(u32 tex)
{
    if (!tex) return;
    auto fRelease = vmethod<long(__stdcall*)(u32)>(tex, 2);
    if (fRelease) __try { fRelease(tex); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void preload_texture(u32 tex)
{
    if (!tex) return;
    auto fPre = vmethod<void(__stdcall*)(u32)>(tex, 9);
    if (fPre) __try { fPre(tex); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

} // namespace aio
