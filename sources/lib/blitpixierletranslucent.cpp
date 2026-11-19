/* blitpixierletranslucent.cpp -- BlitPixieRLETranslucent (win 0x00472900), the direct-colour partial-translucency RLE walker. Notes: Research/notes/blitpixierletranslucent.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* This body is the only writer of the three tint shorts. */

// FUNCTION: EVNOVA 0x00472900
// MAC: _BlitPixieRLETranslucent i386 0x000c1568 ppc 0x000cd4fc
void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes)
{
    uint32_t token, opcode, count;
    unsigned char *rowStart;
    uint32_t redScale, greenScale, blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
    biasColor = ((biasColor << 16) & 0xffff0000) |
            (biasColor & 0xffff);

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;

    overR = overG = overB = 0;

    redScale = (scaleR + 32 - alpha)
            ? (scaleR + 32 - alpha) : 0;
    alphaR = redScale;
    greenScale = (scaleG + 32 - alpha)
            ? (scaleG + 32 - alpha) : 0;
    alphaG = greenScale;
    blueScale = (scaleB + 32 - alpha)
            ? (scaleB + 32 - alpha) : 0;
    alphaB = blueScale;

    if (redScale > 32) {
        overR = ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }
    if (greenScale > 32) {
        overG = ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }
    if (blueScale > 32) {
        overB = (((blueScale - 32) * 0x1f001f) >> 5) & 0x1f001f;
        alphaB = 32;
    }

    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieTranslucentCopy(dstPixel, rleTokens, count);
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
