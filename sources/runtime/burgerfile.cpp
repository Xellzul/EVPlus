/* burgerfile.cpp -- BurgerLib's fixed-point multiply. Notes: Research/notes/burgerfile.md */

#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFixedMax       0x7fffffff
#define kFixedMin       0x80000000

/* FixedMultiply (BurgerLib IMFixMul): two 16.16 fixed-point numbers multiplied, the answer held at the
   largest or smallest number when it does not fit. The library had this in hand assembly
   (FxMicrosoftVC5.obj): imul to 64 bits; a high half of 0x8000 or more answers kFixedMax, one below
   -0x8000 kFixedMin; else the product's bits 16..47 (shrd). The same steps here. */
// FUNCTION: EVNOVA 0x004f1c70
int32_t FixedMultiply(int32_t a, int32_t b)
{
    int64_t product;
    int32_t high;

    product = (int64_t)a * b;
    high = (int32_t)(product >> 32);
    if (high >= 0x8000)
        return kFixedMax;
    if (high < -0x8000)
        return (int32_t)kFixedMin;
    return (int32_t)(uint32_t)((uint64_t)product >> kFixedShift);
}

#ifdef __cplusplus
}
#endif
