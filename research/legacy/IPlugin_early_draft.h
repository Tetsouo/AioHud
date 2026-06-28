// IPlugin.h  -- reconstructed Windower 4 plugin ABI (hook 4.7.9.0)
// ------------------------------------------------------------------
// Reverse-engineered from FFXIDB.dll / Timers.dll with Ghidra.
// DO NOT trust the method NAMES yet (generic vfunNN until Hook.dll call
// sites are labelled). What IS locked and verified:
//   * 34 virtual methods, in this exact order.
//   * Each method's STACK-ARG count = (ret N)/4 measured from FFXIDB's
//     implementation (Ghidra getStackPurgeSize). The host pushes exactly
//     that many 4-byte args, so our overrides MUST pop the same amount or
//     the stack corrupts. MSVC compiles a C++ virtual member fn as
//     __thiscall (this in ECX, args on stack, callee cleans) -> declaring
//     N uint32_t params yields `ret N*4`, matching the contract.
//   * A plugin DLL must export:
//       extern "C" IPlugin* CreateInstance();          // returns new ConcretePlugin
//       extern "C" unsigned int GetInterfaceVersion();  // MUST return 0x04070300
//
// Args are typed as uint32_t placeholders except where Ghidra recovered a
// concrete type (char* on slot 7). Refine once Hook.dll is labelled.
#ifndef WINDOWER_IPLUGIN_H
#define WINDOWER_IPLUGIN_H

#include <cstdint>

#define WINDOWER_PLUGIN_INTERFACE_VERSION 0x04070300u

struct IPlugin
{
    // slot, stackargs (purge/4), ghidra-inferred return / notes
    virtual uint32_t vfun00(uint32_t a) = 0;                                    // [0]  1  ret int
    virtual uint32_t vfun01(uint32_t a) = 0;                                    // [1]  1  ret int
    virtual void     vfun02(uint32_t a, uint32_t b) = 0;                        // [2]  2
    virtual void     vfun03(uint32_t a) = 0;                                    // [3]  1  (lifecycle?)
    virtual void     vfun04(uint32_t a) = 0;                                    // [4]  1
    virtual void     vfun05(uint32_t a) = 0;                                    // [5]  1
    virtual void     vfun06(uint32_t a) = 0;                                    // [6]  1
    virtual void     vfun07(const char* cmd, uint32_t b) = 0;                   // [7]  2  *** likely HandleCommand(char*) ***
    virtual void     vfun08(uint32_t a, uint32_t b) = 0;                        // [8]  2
    virtual void     vfun09(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [9]  4
    virtual void     vfun10(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [10] 4
    virtual bool     vfun11(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = 0;            // [11] 5  ret bool  (packet/text?)
    virtual bool     vfun12(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = 0;            // [12] 5  ret bool
    virtual bool     vfun13(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) = 0;// [13] 6  ret bool
    virtual bool     vfun14(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;                        // [14] 4  ret bool
    virtual void     vfun15(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = 0;            // [15] 5
    virtual void     vfun16(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = 0;            // [16] 5
    virtual uint32_t vfun17(uint32_t a) = 0;                                    // [17] 1
    virtual void     vfun18(uint32_t a) = 0;                                    // [18] 1  (calls CoInitialize)
    virtual void     vfun19() = 0;                                             // [19] 0
    virtual bool     vfun20() = 0;                                             // [20] 0  ret bool
    virtual void     vfun21() = 0;                                             // [21] 0
    virtual void     vfun22() = 0;                                             // [22] 0
    virtual void     vfun23(uint32_t a) = 0;                                    // [23] 1
    virtual bool     vfun24(uint32_t a) = 0;                                    // [24] 1  ret bool
    virtual void     vfun25(uint32_t a, uint32_t b, uint32_t c) = 0;            // [25] 3
    virtual void     vfun26(uint32_t a, uint32_t b, uint32_t c) = 0;            // [26] 3
    virtual bool     vfun27(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [27] 4  ret bool
    virtual bool     vfun28(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [28] 4  ret bool
    virtual void     vfun29(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = 0;            // [29] 5
    virtual void     vfun30(uint32_t a, uint32_t b, uint32_t c) = 0;            // [30] 3
    virtual void     vfun31(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [31] 4
    virtual void     vfun32(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = 0;// [32] 4
    virtual void     vfun33() = 0;                                             // [33] 0
};

#endif // WINDOWER_IPLUGIN_H
