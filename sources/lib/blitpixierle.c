// COMPILE: -lang c++ -bool off
/* blitpixierle.c -- BlitPixieRLE (win 0x00471e10). Notes: Research/notes/blitpixierle.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00471e10
// MAC: _BlitPixieRLE i386 0x000bddd0 ppc 0x000c9820
void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
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
            BlitPixieMemCopy(dstPixel, rleTokens, count);
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

