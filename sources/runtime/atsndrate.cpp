/* atsndrate.cpp -- the sound driver's 16.16 rate helpers: to whole hertz, and the saturating multiply and divide. Notes: Research/notes/atsnddrv.md */

#include "burger.h"



/* A 16.16 rate to whole hertz, rounded. */
// FUNCTION: EVNOVA 0x00507d0b
ULong UFix32_To_UInt32(ULong rate)
{
    return ((rate >> 15) + 1) >> 1;
}

/* 16.16 multiply and divide, saturating; 0 if either side is 0. */
// FUNCTION: EVNOVA 0x00507d16
ULong UFix32_Multiply(ULong ratio, ULong rate)
{
    unsigned long long a;
    unsigned long long b;
    unsigned long long product;
    ULong result;

    if (ratio != 0 && rate != 0) {
        a = ratio;
        b = rate;
        product = (a * b) >> kFixedShift;
        if (product > 0xffffffffUL)
            result = 0xffffffffUL;
        else
            result = product;
    } else
        result = 0;
    return result;
}

// FUNCTION: EVNOVA 0x00507d96
ULong UFix32_Divide(ULong device, ULong rate)
{
    unsigned long long a;
    unsigned long long b;
    unsigned long long quotient;
    ULong result;

    if (device != 0 && rate != 0) {
        a = device;
        b = rate;
        quotient = (a << kFixedShift) / b;
        if (quotient > 0xffffffffUL)
            result = 0xffffffffUL;
        else
            result = quotient;
    } else
        result = 0;
    return result;
}
