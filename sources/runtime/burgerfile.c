// COMPILE: -lang c++ -bool off
/* burgerfile.c -- BurgerLib's hand-written fixed-point multiply. Notes: Research/notes/burgerfile.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t BurgerSndKillProcs[3];                      /* 0x00870d9c */
ULong BurgerSndExitIn;                           /* 0x0086ffec */

#define kFixedMax       0x7fffffff
#define kFixedMin       0x80000000

/* FixedMultiply (BurgerLib IMFixMul, hand assembly in the library: FxMicrosoftVC5.obj).
   STATE: new. OPEN: none */
// FUNCTION: EVNOVA 0x004f1c70
NAKED int32_t FixedMultiply(int32_t a, int32_t b)
{
    __asm {
        mov     eax, dword ptr [esp + 4]
        mov     edx, dword ptr [esp + 8]
        imul    edx
        cmp     edx, 0x8000
        jge     overflow
        cmp     edx, 0xffff8000
        jl      underflow
        shrd    eax, edx, kFixedShift
        ret
        align   16
    overflow:
        mov     eax, kFixedMax
        ret
    underflow:
        mov     eax, kFixedMin
        ret
    }
}

#ifdef __cplusplus
}
#endif
