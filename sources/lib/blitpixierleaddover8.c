// COMPILE: -lang c++ -bool off
/* blitpixierleaddover8.c -- BlitPixieRLEAddOver8 (win 0x00472cf0), the 8-bit additive RLE walker and the direct-colour one's twin. Notes: Research/notes/blitpixierleaddover8.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00472cf0
// MAC: _BlitPixieRLEAddOver8 i386 0x000c21c1 ppc 0x000ce450
void BlitPixieRLEAddOver8(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes)
{
    uint32_t token, opcode, count;
    unsigned char *rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieAddOverCopy8(dstPixel, rleTokens, count);
            rleTokens += count;
            PtrAddBytes(rleTokens, (-(intptr_t)rleTokens) & 3);
            dstPixel += count;
            break;

        case 3:
            dstPixel += count;
            break;

        case 4:
            BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);
            rleTokens += 4;
            dstPixel += count;
            break;

        case 1:
            dstPixel = rowStart;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            return;
        }
    } while (opcode != 0);
}
