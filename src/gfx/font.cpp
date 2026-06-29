// font.cpp -- see font.h.
#include "gfx/font.h"
#include "gfx/draw.h"
#include "gfx/texture.h"
#include <windows.h>

namespace aio {

static const int   AW = 512, AH = 512;     // atlas dims (pow2) -- 512 tall so even WIDE faces (Verdana)
                                           // fit all 96 glyphs ; a 256-tall atlas overflowed mid-bake
                                           // (later glyphs unbaked -> tofu boxes for long strings).
static const int   FIRST = 32, LAST = 127; // printable ASCII (+ DEL slot)
static const float BASE_EM = 32.0f;        // bake size -> downscaled at draw

void Font::build(u32 dev) {
    HDC hdc = CreateCompatibleDC(0);
    if (!hdc) return;

    BITMAPINFO bmi; ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = AW;
    bmi.bmiHeader.biHeight = -AH;           // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = 0;
    HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, 0, 0);
    if (!hbm) { DeleteDC(hdc); return; }
    HGDIOBJ oldbm = SelectObject(hdc, hbm);
    memset(bits, 0, AW * AH * 4);

    HFONT hf = CreateFontA((int)-BASE_EM, 0, 0, 0, weight_, 0, 0, 0,
                           DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                           ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face_);
    HGDIOBJ oldf = SelectObject(hdc, hf);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    TEXTMETRICA tm; GetTextMetricsA(hdc, &tm);
    int cellH = tm.tmHeight, pad = 4;          // pad keeps glyphs apart in the lower mips
    int penx = pad, peny = pad, rowH = cellH + pad;
    const u32* px = (const u32*)bits;
    int inkTop = cellH, inkBot = -1;                // ink box of caps/digits (for vertical centring)

    for (int c = FIRST; c <= LAST; ++c) {
        char ch = (char)c;
        int adv; ABC abc;
        if (GetCharABCWidthsA(hdc, c, c, &abc)) adv = abc.abcA + (int)abc.abcB + abc.abcC;
        else { SIZE sz; GetTextExtentPoint32A(hdc, &ch, 1, &sz); adv = sz.cx; }
        if (adv < 1) adv = (int)(BASE_EM * 0.3f);
        if (penx + adv + pad > AW) { penx = pad; peny += rowH; }
        if (peny + cellH > AH) break;
        if (c != ' ') TextOutA(hdc, penx, peny, &ch, 1);
        G& g = g_[c - FIRST];
        g.u0 = (float)penx / AW;          g.u1 = (float)(penx + adv) / AW;
        g.v0 = (float)peny / AH;          g.v1 = (float)(peny + cellH) / AH;
        g.w  = (float)adv; g.h = (float)cellH; g.adv = (float)adv;

        // per-glyph INK bbox (rel. to this cell's top-left) -> true visual centring (draw_cc)
        {
            int minx = adv, maxx = -1, miny = cellH, maxy = -1;
            for (int yy = 0; yy < cellH; ++yy) {
                const u32* row = px + (peny + yy) * AW;
                for (int xx = penx; xx < penx + adv && xx < AW; ++xx)
                    if (((row[xx] >> 16) & 0xFF) > 40) {
                        int rx = xx - penx;
                        if (rx < minx) minx = rx; if (rx > maxx) maxx = rx;
                        if (yy < miny) miny = yy; if (yy > maxy) maxy = yy;
                    }
            }
            if (maxx >= 0) { g.il = (float)minx; g.ir = (float)(maxx + 1); g.it = (float)miny; g.ib = (float)(maxy + 1); }
            else           { g.il = 0; g.ir = (float)adv; g.it = 0; g.ib = (float)cellH; }   // blank (space)
        }

        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {    // measure cap/digit ink extent
            for (int yy = 0; yy < cellH; ++yy) {
                const u32* row = px + (peny + yy) * AW;
                bool ink = false;
                for (int xx = penx; xx < penx + adv && xx < AW; ++xx) if (((row[xx] >> 16) & 0xFF) > 40) { ink = true; break; }
                if (ink) { if (yy < inkTop) inkTop = yy; if (yy > inkBot) inkBot = yy; }
            }
        }
        penx += adv + pad;
    }
    cap_top_ = (inkBot >= 0) ? (float)inkTop : cellH * 0.2f;
    cap_h_   = (inkBot >= 0) ? (float)(inkBot - inkTop + 1) : cellH * 0.66f;
    base_ = BASE_EM; aw_ = AW; ah_ = AH;

    // GDI wrote white AA glyphs (rgb = coverage, alpha byte unused). Convert -> ARGB
    // with alpha = coverage, rgb = white : MODULATE by the vertex colour tints the text.
    u32* buf = (u32*)HeapAlloc(GetProcessHeap(), 0, AW * AH * 4);
    if (buf) {
        const u32* src = (const u32*)bits;
        for (int i = 0; i < AW * AH; ++i) {
            u32 cov = (src[i] >> 16) & 0xFF;      // R channel
            buf[i] = (cov << 24) | 0x00FFFFFF;
        }
        tex_ = make_texture_argb_mip(dev, AW, AH, buf);   // mip chain -> crisp minification
        HeapFree(GetProcessHeap(), 0, buf);
    }

    SelectObject(hdc, oldf); DeleteObject(hf);
    SelectObject(hdc, oldbm); DeleteObject(hbm);
    DeleteDC(hdc);
}

void Font::ensure(u32 dev) {
    if (!valid_ptr(dev)) return;
    if (dirty_) { if (tex_) release_texture(tex_); tex_ = 0; dirty_ = false; }   // face changed -> rebuild
    if (tex_) return;
    build(dev);
}

void Font::set_face(const char* face, int weight) {
    if (face && face[0] && lstrcmpA(face, face_) != 0) { lstrcpynA(face_, face, sizeof(face_)); dirty_ = true; }
    if (weight > 0 && weight != weight_) { weight_ = weight; dirty_ = true; }
}

// ---- FontManager ----
void FontManager::set_default(const char* face, int weight) {
    if (face && face[0]) lstrcpynA(defFace_, face, sizeof(defFace_));
    if (weight > 0) defWeight_ = weight;
}
Font* FontManager::get(const char* face, int weight) {
    const char* fc = (face && face[0]) ? face : defFace_;
    int w = weight > 0 ? weight : defWeight_;
    for (int i = 0; i < n_; ++i) if (wt_[i] == w && lstrcmpA(face_[i], fc) == 0) return &f_[i];
    if (n_ < MAXF) {
        lstrcpynA(face_[n_], fc, 64); wt_[n_] = w;
        f_[n_].set_face(fc, w);
        return &f_[n_++];
    }
    return &f_[0];   // pool full -> fall back to the default slot
}
void FontManager::ensure_all(u32 dev)   { for (int i = 0; i < n_; ++i) f_[i].ensure(dev); }
void FontManager::on_device_lost()      { for (int i = 0; i < n_; ++i) f_[i].on_device_lost(); }
void FontManager::dispose()             { for (int i = 0; i < n_; ++i) f_[i].dispose(); }

void Font::dispose() {
    if (tex_) { release_texture(tex_); tex_ = 0; }
}

void Font::begin(u32 dev) {
    dSetVS(dev, FVF_XYZRHW_DIFFUSE_TEX1);
    dSetRS(dev, D3DRS_ZENABLE, 0);
    dSetRS(dev, D3DRS_CULLMODE, D3DCULL_NONE);
    dSetRS(dev, D3DRS_LIGHTING, 0);
    dSetRS(dev, D3DRS_ALPHATESTENABLE, 0);
    dSetRS(dev, D3DRS_FOGENABLE, 0);
    dSetRS(dev, D3DRS_ALPHABLENDENABLE, 1);
    dSetRS(dev, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dSetRS(dev, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dSetRS(dev, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    dSetTex(dev, 0, tex_);
    dSetTSS(dev, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    dSetTSS(dev, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dSetTSS(dev, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dSetTSS(dev, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
    dSetTSS(dev, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dSetTSS(dev, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dSetTSS(dev, 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
    dSetTSS(dev, 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
    dSetTSS(dev, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    dSetTSS(dev, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    dSetTSS(dev, 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);   // trilinear -> clean at any size
    dSetTSS(dev, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dSetTSS(dev, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

float Font::measure(const char* s, float size) const {
    if (!s) return 0.0f;
    float scale = size / base_, w = 0.0f;
    for (; *s; ++s) {
        int c = (unsigned char)*s; if (c < FIRST || c > LAST) c = '?';
        w += g_[c - FIRST].adv * scale;
    }
    return w;
}

void Font::glyphs(u32 dev, float x, float y, const char* s, float scale, u32 color) {
    float penx = x;
    for (; *s; ++s) {
        int c = (unsigned char)*s; if (c < FIRST || c > LAST) c = '?';
        const G& g = g_[c - FIRST];
        // half-pixel texel alignment is now applied inside tquad() (D3D8 XYZRHW rule) -> don't double it here.
        if (c != ' ') tquad(dev, penx, y, g.w * scale, g.h * scale, g.u0, g.u1, g.v0, g.v1, color, color);
        penx += g.adv * scale;
    }
}

float Font::draw(u32 dev, float x, float y, const char* s, float size, u32 color, u32 outline, float ow) {
    if (!s || !tex_) return 0.0f;
    float scale = size / base_;
    if ((outline >> 24) && ow > 0.0f) {                 // stroke : 8 offset passes behind
        static const float dx[8] = { -1, 1, 0, 0, -1, -1,  1, 1 };
        static const float dy[8] = {  0, 0, -1, 1, -1,  1, -1, 1 };
        for (int k = 0; k < 8; ++k) glyphs(dev, x + dx[k] * ow, y + dy[k] * ow, s, scale, outline);
    }
    glyphs(dev, x, y, s, scale, color);
    return measure(s, size);
}

// cell-top y so the cap/digit ink box is centred on `cy`.
float Font::draw_lv(u32 dev, float x, float cy, const char* s, float size, u32 color, u32 outline, float ow) {
    float y = cy - (cap_top_ + cap_h_ * 0.5f) * (size / base_);
    return draw(dev, x, y, s, size, color, outline, ow);
}
float Font::draw_c(u32 dev, float cx, float cy, const char* s, float size, u32 color, u32 outline, float ow) {
    float x = cx - measure(s, size) * 0.5f;
    return draw_lv(dev, x, cy, s, size, color, outline, ow);
}

// left-aligned at x, vertically centred on the REAL ink box -> aligns with badge/bars regardless of face.
float Font::draw_lc(u32 dev, float x, float cy, const char* s, float size, u32 color, u32 outline, float ow) {
    if (!s || !tex_) return 0.0f;
    float scale = size / base_, top = 1e9f, bot = -1e9f;
    for (const char* p = s; *p; ++p) {
        int c = (unsigned char)*p; if (c < FIRST || c > LAST) c = '?';
        const G& g = g_[c - FIRST];
        if (c != ' ') { float t = g.it * scale, b = g.ib * scale; if (t < top) top = t; if (b > bot) bot = b; }
    }
    if (bot < top) { top = 0; bot = cap_h_ * scale; }
    return draw(dev, x, cy - (top + bot) * 0.5f, s, size, color, outline, ow);
}

// centre on the REAL ink bbox of `s` (both axes) -> visually centred for ANY face/size.
float Font::draw_cc(u32 dev, float cx, float cy, const char* s, float size, u32 color, u32 outline, float ow) {
    if (!s || !tex_) return 0.0f;
    float scale = size / base_, penx = 0.0f;
    float left = 1e9f, right = -1e9f, top = 1e9f, bot = -1e9f;
    for (const char* p = s; *p; ++p) {
        int c = (unsigned char)*p; if (c < FIRST || c > LAST) c = '?';
        const G& g = g_[c - FIRST];
        if (c != ' ') {
            float l = penx + g.il * scale, r = penx + g.ir * scale, t = g.it * scale, b = g.ib * scale;
            if (l < left) left = l; if (r > right) right = r; if (t < top) top = t; if (b > bot) bot = b;
        }
        penx += g.adv * scale;
    }
    if (right < left) { left = 0; right = penx; top = 0; bot = cap_h_ * scale; }   // all-blank fallback
    float X = cx - (left + right) * 0.5f;
    float Y = cy - (top + bot) * 0.5f;
    return draw(dev, X, Y, s, size, color, outline, ow);
}

} // namespace aio
