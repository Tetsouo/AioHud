// AioTest.dll -- Windower 4 plugin, STEP 2b: identify the D3D device pointer.
// ----------------------------------------------------------------------------
// Counting (step 2a) showed slots 5 & 6 are the per-frame render hooks (no args),
// and the device is delivered once via a setup slot (candidates: slot 9 count=2,
// slot 2 count=1, both carry a pointer). To know WHICH pointer is the
// IDirect3DDevice8*, we resolve, for each slot's first pointer argument, the
// module that owns *(arg) (the COM vtable) -- the device's vtable lives in d3d8.dll.
// Dumped to plugins\AioTest_slots.txt.

#include <windows.h>
#include <cstdint>

typedef uint32_t u32;

static volatile long g_count[34];
static u32           g_firstArg[34];
static volatile long g_total;

// safely read a dword; returns false on access violation
static bool safeReadU32(u32 p, u32* out)
{
    __try { *out = *(volatile u32*)p; return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static bool looksLikePtr(u32 v) { return v >= 0x10000 && v < 0x80000000; }

// describe what a pointer points at: its vtable dword + owning module basename
static void describePtr(u32 p, char* out, int outsz)
{
    out[0] = 0;
    if (!looksLikePtr(p)) return;                  // small ints (key codes etc.) -> skip
    u32 vtbl;
    if (!safeReadU32(p, &vtbl)) { lstrcpynA(out, "  [badread]", outsz); return; }
    if (!looksLikePtr(vtbl)) { wsprintfA(out, "  *p=0x%08X (not a vtable)", vtbl); return; }
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)vtbl, &hm) && hm) {
        char path[MAX_PATH]; GetModuleFileNameA(hm, path, MAX_PATH);
        const char* b = path;
        for (const char* q = path; *q; q++) if (*q == '\\' || *q == '/') b = q + 1;
        wsprintfA(out, "  vtbl=0x%08X in %s", vtbl, b);
    } else {
        wsprintfA(out, "  vtbl=0x%08X (no module)", vtbl);
    }
}

// Host probe: FFXIDB Init does `this[2] = host->vtbl[3](host)`. We replicate that
// one safe call and record what it returns. Both host and that result are Windower
// interfaces (Hook.dll objects); dump their vtable method RVAs (relative to the
// Hook.dll base) so they can be decompiled in Ghidra to find the draw API.
static u32 g_host = 0, g_q3 = 0, g_txt = 0;
static char g_probe[2048];

// read the MSVC RTTI class name of a polymorphic object (obj->vtbl[-1]=COL ->
// TypeDescriptor+8 = mangled ".?AV<Class>@@").
static void rttiName(u32 obj, char* out, int sz)
{
    out[0] = 0;
    u32 vt, col, td;
    if (!safeReadU32(obj, &vt) || !looksLikePtr(vt)) return;
    if (!safeReadU32(vt - 4, &col) || !looksLikePtr(col)) return;
    if (!safeReadU32(col + 0x0c, &td) || !looksLikePtr(td)) return;
    __try { lstrcpynA(out, (const char*)(td + 8), sz); }
    __except (EXCEPTION_EXECUTE_HANDLER) { out[0] = 0; }
}

static u32 moduleBaseOf(u32 codeAddr)
{
    HMODULE hm = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)codeAddr, &hm) && hm) return (u32)hm;
    return 0;
}

// dump the first N vtable method RVAs of a Windower interface object
static int dumpIface(char* buf, int n, const char* name, u32 obj)
{
    if (!looksLikePtr(obj)) { return n + wsprintfA(buf + n, "%s: (not a ptr) 0x%08X\r\n", name, obj); }
    u32 vt;
    if (!safeReadU32(obj, &vt) || !looksLikePtr(vt))
        return n + wsprintfA(buf + n, "%s: bad vtable\r\n", name);
    u32 base = moduleBaseOf(vt);
    char mod[MAX_PATH] = "?";
    if (base) { HMODULE hm=(HMODULE)base; char p[MAX_PATH]; GetModuleFileNameA(hm,p,MAX_PATH);
                const char* b=p; for(const char*q=p;*q;q++) if(*q=='\\'||*q=='/') b=q+1; lstrcpynA(mod,b,MAX_PATH); }
    n += wsprintfA(buf + n, "%s = 0x%08X  vtbl=0x%08X in %s (base 0x%08X)\r\n", name, obj, vt, mod, base);
    for (int i = 0; i < 16; i++) {
        u32 fn;
        if (safeReadU32(vt + i * 4, &fn) && looksLikePtr(fn))
            n += wsprintfA(buf + n, "   [%2d] 0x%08X  = %s+0x%X\r\n", i, fn, mod, base ? fn - base : 0);
    }
    return n;
}

static void dumpCounts()
{
    char buf[16384]; int n = 0; char info[160];
    n += wsprintfA(buf + n, "=== AioTest device-id build ===\r\n");
    n = dumpIface(buf, n, "HOST", g_host);
    n = dumpIface(buf, n, "VT3 ", g_q3);
    n += wsprintfA(buf + n, "--- PluginManager getter RTTI probe ---\r\n%s", g_probe);
    // full hex dump of the created TextObject (504 bytes) to inspect actual field state
    if (looksLikePtr(g_txt)) {
        n += wsprintfA(buf + n, "--- TextObject @0x%08X (504 bytes) ---\r\n", g_txt);
        for (int off = 0; off < 0x1f8; off += 16) {
            n += wsprintfA(buf + n, "+0x%03X:", off);
            for (int j = 0; j < 16; j += 4) { u32 v = 0; safeReadU32(g_txt + off + j, &v); n += wsprintfA(buf + n, " %08X", v); }
            n += wsprintfA(buf + n, "\r\n");
        }
    }
    for (int i = 0; i < 34; i++) {
        describePtr(g_firstArg[i], info, sizeof(info));
        n += wsprintfA(buf + n, "slot %2d : count=%6ld  firstArg=0x%08X%s\r\n",
                       i, g_count[i], g_firstArg[i], info);
    }
    HANDLE h = CreateFileA("D:\\Windower Tetsouo\\plugins\\AioTest_slots.txt",
                           GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(h, buf, n, &w, NULL); CloseHandle(h); }
}

static void hit(int i, u32 firstArg)
{
    g_count[i]++;
    if (firstArg && !g_firstArg[i]) g_firstArg[i] = firstArg;
    if ((++g_total % 1000) == 0) dumpCounts();
}

struct AioPlugin { void** vtbl; };

// All methods return u32 0 (never leave EAX garbage). Slot 0/1 = name/description
// strings. Slot 4 = IsCore() -> MUST return 0 or Windower refuses //unload.
#define SELF void* self
static u32 __stdcall m00(SELF){ hit(0,0); return (u32)(const void*)"AioTest"; }
static u32 __stdcall m01(SELF){ hit(1,0); return (u32)(const void*)"AioHUD hello-world test"; }
// Call Windower Console::print (RTTI .?AVConsole@@, vtbl[3] = print(self,char*)).
static void consolePrint(u32 console, const char* text)
{
    if (!looksLikePtr(console)) return;
    u32 vt, fn;
    if (!safeReadU32(console, &vt) || !looksLikePtr(vt)) return;
    if (!safeReadU32(vt + 3 * 4, &fn) || !looksLikePtr(fn)) return;   // vtbl[3]
    typedef void (__stdcall *PrintFn)(u32, const char*);
    __try { ((PrintFn)fn)(console, text); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// --- interface call helpers (all Windower methods = __stdcall, this on stack) ---
static u32 vfn(u32 obj, int idx) {                  // fetch obj->vtbl[idx]
    u32 vt, fn;
    if (!safeReadU32(obj, &vt) || !looksLikePtr(vt)) return 0;
    if (!safeReadU32(vt + idx * 4, &fn) || !looksLikePtr(fn)) return 0;
    return fn;
}
static u32 getIface(u32 host, int idx) {            // host->vtbl[idx](host) -> interface*
    u32 f = vfn(host, idx); if (!f) return 0;
    typedef u32 (__stdcall *F)(u32);
    u32 r = 0; __try { r = ((F)f)(host); } __except (EXCEPTION_EXECUTE_HANDLER) {} return r;
}
static u32 thCreate(u32 th, const char* name) {     // TextHandler::CreateText[0](name) -> TextObject*
    u32 f = vfn(th, 0); if (!f) return 0;
    typedef u32 (__stdcall *F)(u32, const char*);
    u32 r = 0; __try { r = ((F)f)(th, name); } __except (EXCEPTION_EXECUTE_HANDLER) {} return r;
}
static void txtSetText(u32 o, const char* s) {      // TextObject[0]
    u32 f = vfn(o, 0); if (!f) return;
    typedef void (__stdcall *F)(u32, const char*); __try { ((F)f)(o, s); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetVisible(u32 o, char v) {          // TextObject[1] (+0x30)
    u32 f = vfn(o, 1); if (!f) return;
    typedef void (__stdcall *F)(u32, char); __try { ((F)f)(o, v); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetColor(u32 o, char a, char r, char g, char b) {   // TextObject[6] (+0xa4)
    u32 f = vfn(o, 6); if (!f) return;
    typedef void (__stdcall *F)(u32, char, char, char, char); __try { ((F)f)(o, a, r, g, b); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetPos(u32 o, float x, float y) {    // TextObject[9] (+0x58/+0x5c)
    u32 f = vfn(o, 9); if (!f) return;
    typedef void (__stdcall *F)(u32, float, float); __try { ((F)f)(o, x, y); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetFont(u32 o, const char* name) {   // TextObject[3] = SetFont(char*) -> [2]
    u32 f = vfn(o, 3); if (!f) return;
    typedef void (__stdcall *F)(u32, const char*); __try { ((F)f)(o, name); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetSize(u32 o, float s) {            // TextObject[4] (+0x7c) = font size (default 0)
    u32 f = vfn(o, 4); if (!f) return;
    typedef void (__stdcall *F)(u32, float); __try { ((F)f)(o, s); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void txtSetPos2(u32 o, float x, float y) {   // TextObject[10] (+0x50/+0x54) alt position
    u32 f = vfn(o, 10); if (!f) return;
    typedef void (__stdcall *F)(u32, float, float); __try { ((F)f)(o, x, y); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static u32 __stdcall m02(SELF,u32 host){
    hit(2,host);
    g_host = host;
    if (!looksLikePtr(host)) return 0;
    u32 console = getIface(host, 3);                 // Console (for confirmation)
    consolePrint(console, ">>> AioTest: creating on-screen text via TextHandler <<<");
    u32 th = getIface(host, 4);                      // TextHandler
    g_q3 = th;
    u32 txt = looksLikePtr(th) ? thCreate(th, "aiotest_hello") : 0;
    if (looksLikePtr(txt)) {
        g_txt = txt;
        txtSetFont(txt, "Arial");                      // REQUIRED: no font -> no render
        txtSetSize(txt, 16.0f);                        // REQUIRED: size default 0
        txtSetText(txt, "Hello World - AioTest C++ plugin");
        txtSetPos(txt, 300.0f, 300.0f);                // primary pos (+0x58/+0x5c)
        txtSetPos2(txt, 300.0f, 300.0f);               // alt pos (+0x50/+0x54) too
        txtSetColor(txt, (char)0xFF, (char)0xFF, (char)0xFF, (char)0xFF);   // opaque white
        txtSetVisible(txt, 1);
        char m[96]; wsprintfA(m, ">>> AioTest: TextObject created @ 0x%08X <<<", txt);
        consolePrint(console, m);
    } else {
        consolePrint(console, ">>> AioTest: TextHandler.create FAILED <<<");
    }
    return 0;
}
static u32 __stdcall m03(SELF){ hit(3,0); return 0; }
static u32 __stdcall m04(SELF){ hit(4,0); return 0; }   // IsCore -> 0 = NOT core (unloadable)
static u32 __stdcall m05(SELF){ hit(5,0); return 0; }
static u32 __stdcall m06(SELF){ hit(6,0); return 0; }
static u32 __stdcall m07(SELF,char* c){ hit(7,(u32)c); return 0; }
static u32 __stdcall m08(SELF,u32 a){ hit(8,a); return 0; }
static u32 __stdcall m09(SELF,u32 a,u32,u32){ hit(9,a); return 0; }
static u32 __stdcall m10(SELF,u32 a,u32,u32){ hit(10,a); return 0; }
static u32 __stdcall m11(SELF,u32 a,u32,u32,u32){ hit(11,a); return 0; }
static u32 __stdcall m12(SELF,u32 a,u32,u32,u32){ hit(12,a); return 0; }
static u32 __stdcall m13(SELF,u32 a,u32,u32,u32,u32){ hit(13,a); return 0; }
static u32 __stdcall m14(SELF,u32 a,u32,u32){ hit(14,a); return 0; }
static u32 __stdcall m15(SELF,u32 a,u32,u32,u32){ hit(15,a); return 0; }
static u32 __stdcall m16(SELF,u32 a,u32,u32,u32){ hit(16,a); return 0; }
static u32 __stdcall m17(SELF){ hit(17,0); return 0; }
static u32 __stdcall m18(SELF){ hit(18,0); return 0; }
static u32 __stdcall m19(SELF){ hit(19,0); return 0; }
static u32 __stdcall m20(SELF){ hit(20,0); return 0; }
static u32 __stdcall m21(SELF){ hit(21,0); return 0; }
static u32 __stdcall m22(SELF){ hit(22,0); return 0; }
static u32 __stdcall m23(SELF){ hit(23,0); return 0; }
static u32 __stdcall m24(SELF){ hit(24,0); return 0; }
static u32 __stdcall m25(SELF,u32 a,u32){ hit(25,a); return 0; }
static u32 __stdcall m26(SELF,u32 a,u32){ hit(26,a); return 0; }
static u32 __stdcall m27(SELF,u32 a,u32,u32){ hit(27,a); return 0; }
static u32 __stdcall m28(SELF,u32 a,u32,u32){ hit(28,a); return 0; }
static u32 __stdcall m29(SELF,u32 a,u32,u32,u32){ hit(29,a); return 0; }
static u32 __stdcall m30(SELF,u32 a,u32){ hit(30,a); return 0; }
static u32 __stdcall m31(SELF,u32 a,u32,u32){ hit(31,a); return 0; }
static u32 __stdcall m32(SELF,u32 a,u32,u32){ hit(32,a); return 0; }
static u32 __stdcall m33(SELF){ hit(33,0); return 0; }

static void* g_vtbl[34] = {
    m00,m01,m02,m03,m04,m05,m06,m07,m08,m09,m10,m11,m12,m13,m14,m15,m16,
    m17,m18,m19,m20,m21,m22,m23,m24,m25,m26,m27,m28,m29,m30,m31,m32,m33
};
static AioPlugin g_plugin = { g_vtbl };

extern "C" {
__declspec(dllexport) void* CreateInstance()
{
    for (int i = 0; i < 34; i++) { g_count[i] = 0; g_firstArg[i] = 0; }
    g_total = 0;
    return &g_plugin;
}
__declspec(dllexport) unsigned int GetInterfaceVersion() { return 0x04070300u; }
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
