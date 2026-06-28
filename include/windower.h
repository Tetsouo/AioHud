// windower.h -- Reverse-engineered Windower 4 plugin API (hook 4.7.9.0).
// Typed wrappers around the host interface objects a plugin receives at Init.
// All Windower interface methods are __stdcall with `this` passed as the first
// stack argument; we call them through their vtable by index. Every call is
// wrapped in SEH so a wrong index/pointer degrades to a no-op instead of a crash.
//
// Interfaces (RTTI names) and the indices we use:
//   PluginManager (host)  : vtbl[3]=Console vtbl[4]=TextHandler vtbl[5]=PrimitiveHandler vtbl[7]=ffxi
//   Console               : vtbl[3]=print(char*)
//   TextHandler           : vtbl[0]=create(char* name)->TextObject
//   TextObject            : 0=text 1=visible(+0x30) 3=font 4=size(+0x7c) 6=color(+0xa4) 9=pos(+0x58)
//                           NB: a TextObject needs BOTH font and size (size default 0) or it won't render.
#pragma once
#include <windows.h>
#include <cstdint>

namespace windower {

using u32 = uint32_t;
static const u32 PLUGIN_INTERFACE_VERSION = 0x04070300u;   // GetInterfaceVersion() must return this

inline bool valid_ptr(u32 v) { return v >= 0x10000 && v < 0x80000000; }

inline bool safe_read(u32 p, u32* out) {
    __try { *out = *(volatile u32*)p; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// fetch obj->vtbl[idx] (the function pointer), or 0 if anything is invalid
inline u32 vfn(u32 obj, int idx) {
    u32 vt, fn;
    if (!safe_read(obj, &vt) || !valid_ptr(vt)) return 0;
    if (!safe_read(vt + idx * 4, &fn) || !valid_ptr(fn)) return 0;
    return fn;
}
template <class Fn> inline Fn vmethod(u32 obj, int idx) { return reinterpret_cast<Fn>(vfn(obj, idx)); }

// ---------------------------------------------------------------- TextObject
class TextObject {
    u32 p_;
    void call_s(int i, const char* s) { auto f = vmethod<void(__stdcall*)(u32, const char*)>(p_, i); if (f) __try { f(p_, s); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
    void call_c(int i, char c)        { auto f = vmethod<void(__stdcall*)(u32, char)>(p_, i);        if (f) __try { f(p_, c); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
    void call_f(int i, float v)       { auto f = vmethod<void(__stdcall*)(u32, float)>(p_, i);       if (f) __try { f(p_, v); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
public:
    explicit TextObject(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    u32  raw() const { return p_; }

    void text(const char* s)  { call_s(0, s); }          // [0] SetText
    void visible(bool v)      { call_c(1, v ? 1 : 0); }  // [1] @+0x30 (default 1)
    void font(const char* n)  { call_s(3, n); }          // [3] REQUIRED (e.g. "Arial")
    void size(float pt)       { call_f(4, pt); }         // [4] @+0x7c REQUIRED (default 0)
    void color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {            // [6] @+0xa4
        auto f = vmethod<void(__stdcall*)(u32, char, char, char, char)>(p_, 6);
        if (f) __try { f(p_, (char)a, (char)r, (char)g, (char)b); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    void pos(float x, float y) {                                        // [10] @+0x50/+0x54 (screen pos)
        auto f = vmethod<void(__stdcall*)(u32, float, float)>(p_, 10);
        if (f) __try { f(p_, x, y); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    void pos58(float x, float y) {                                      // [9] @+0x58 (offset/anchor?)
        auto f = vmethod<void(__stdcall*)(u32, float, float)>(p_, 9);
        if (f) __try { f(p_, x, y); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    // measured pixel size of the current text (width,height) -- [12] GetExtents.
    // Valid the frame AFTER the text was set/rendered.
    void extents(float* w, float* h) {                                  // [12]
        *w = 0; *h = 0;
        auto f = vmethod<long(__stdcall*)(u32, float*, float*)>(p_, 12);
        if (f) __try { f(p_, w, h); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
};

// --------------------------------------------------------------- TextHandler
class TextHandler {
    u32 p_;
public:
    explicit TextHandler(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    // create-or-get a text object by name; it is added to Windower's render list.
    TextObject create(const char* name) {
        auto f = vmethod<u32(__stdcall*)(u32, const char*)>(p_, 0);
        u32 r = 0; if (f) __try { r = f(p_, name); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return TextObject(r);
    }
    // remove a text object from the render list ([3] clears its +0x1c "alive" flag).
    // REQUIRED on unload, else the object keeps rendering after the plugin is gone.
    void remove(TextObject t) {
        auto f = vmethod<void(__stdcall*)(u32, u32)>(p_, 3);
        if (f && t.valid()) __try { f(p_, t.raw()); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
};

// ------------------------------------------------------------------- Console
class Console {
    u32 p_;
public:
    explicit Console(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    void print(const char* s) {                                        // [3] print to Windower console
        auto f = vmethod<void(__stdcall*)(u32, const char*)>(p_, 3);
        if (f) __try { f(p_, s); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
};

// ----------------------------------------------------------- PrimitiveObject
// A primitive (rectangle / image quad). Fresh primitives default to invisible
// and transparent (color alpha 0) -- you MUST set color(alpha>0) + visible(true).
// A solid colour rect needs no texture; texture() tints an image instead.
class PrimitiveObject {
    u32 p_;
    void ff_(int i, float a, float b) { auto f = vmethod<void(__stdcall*)(u32, float, float)>(p_, i); if (f) __try { f(p_, a, b); } __except (EXCEPTION_EXECUTE_HANDLER) {} }
public:
    explicit PrimitiveObject(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    u32  raw() const { return p_; }
    void visible(bool v) { auto f = vmethod<void(__stdcall*)(u32, char)>(p_, 0); if (f) __try { f(p_, v ? 1 : 0); } __except (EXCEPTION_EXECUTE_HANDLER) {} }   // [0] @+0x28
    void color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) { auto f = vmethod<void(__stdcall*)(u32, char, char, char, char)>(p_, 1); if (f) __try { f(p_, (char)a, (char)r, (char)g, (char)b); } __except (EXCEPTION_EXECUTE_HANDLER) {} } // [1] @+0x29  order = A,R,G,B
    void texture(const char* path) { auto f = vmethod<void(__stdcall*)(u32, const char*)>(p_, 2); if (f) __try { f(p_, path); } __except (EXCEPTION_EXECUTE_HANDLER) {} } // [2]
    // GEOMETRY (calibrated): [7]@+0x30 = POSITION (top-left), [6]@+0x38 = SIZE (w,h),
    // [4]@+0x40 = SCALE (default 1.0). A box = position + size. (Earlier confusion:
    // [6] is the SIZE, not the bottom-right corner -- passing x+w,y+h made it giant.)
    void position(float x, float y) { ff_(7, x, y); }          // [7] @+0x30
    void size(float w, float h)     { ff_(6, w, h); }          // [6] @+0x38
    void scale(float sx, float sy)  { ff_(4, sx, sy); }        // [4] @+0x40
    void rect(float x, float y, float w, float h) { scale(1.0f, 1.0f); position(x, y); size(w, h); }
};

// ---------------------------------------------------------- PrimitiveHandler
class PrimitiveHandler {
    u32 p_;
public:
    explicit PrimitiveHandler(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    PrimitiveObject create(const char* name) {                          // [0]
        auto f = vmethod<u32(__stdcall*)(u32, const char*)>(p_, 0);
        u32 r = 0; if (f) __try { r = f(p_, name); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return PrimitiveObject(r);
    }
    void remove(PrimitiveObject o) {                                    // [3]
        auto f = vmethod<void(__stdcall*)(u32, u32)>(p_, 3);
        if (f && o.valid()) __try { f(p_, o.raw()); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
};

// ------------------------------------------------------------- PluginManager
class PluginManager {
    u32 p_;
    u32 get(int idx) {   // PluginManager getters (vtbl[idx]) return a service interface
        auto f = vmethod<u32(__stdcall*)(u32)>(p_, idx);
        u32 r = 0; if (f) __try { r = f(p_); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return r;
    }
public:
    explicit PluginManager(u32 p = 0) : p_(p) {}
    bool valid() const { return valid_ptr(p_); }
    u32  raw() const { return p_; }
    Console          console()           { return Console(get(3)); }
    TextHandler      text_handler()      { return TextHandler(get(4)); }
    PrimitiveHandler primitive_handler() { return PrimitiveHandler(get(5)); }
    // raw service pointers (for debug/exploration of not-yet-wrapped interfaces)
    u32 service_raw(int idx)    { return get(idx); }
    u32 console_raw()           { return get(3); }
    u32 text_handler_raw()      { return get(4); }
    u32 primitive_handler_raw() { return get(5); }
    u32 ffxi_raw()              { return get(7); }   // TODO: wrap ffxi (game data)
};

} // namespace windower
