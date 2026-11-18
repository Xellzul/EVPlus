// COMPILE: -lang c++ -bool off
/* blitpixierletranslucentclipped.c -- tu-map object O13: four of the six inner translucent RLE blitters of the un-anchored BlitPixie block. Notes: Research/notes/blitpixierletranslucentclipped.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


// FUNCTION: EVNOVA 0x00472d70
// MAC: _BlitPixieRLETranslucentClipped i386 0x000c18c0 ppc 0x000cd93c
void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    short x;
    short rowIndex;
    int32_t lastCol;
    int32_t trim;
    int32_t redScale;
    int32_t greenScale;
    int32_t blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
    biasColor = ((biasColor << 16) & 0xffff0000) | (biasColor & 0xffff);

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
    overR = overG =
            overB = 0;

    // BUG (all versions): `x != 0 ? x : 0' is no clamp -- when alpha > scale + 32 the value goes negative,
    // passes the unsigned `> 32' test below and gives garbage over/alpha values instead of 0
    alphaR = redScale =
            (scaleR + 32 - alpha != 0)
            ? scaleR + 32 - alpha : 0;

    alphaG = greenScale =
            (scaleG + 32 - alpha != 0)
            ? scaleG + 32 - alpha : 0;

    alphaB = blueScale =
            (scaleB + 32 - alpha != 0)
            ? scaleB + 32 - alpha : 0;

    if ((uint32_t)redScale > 32) {
        overR =
                ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }

    if ((uint32_t)greenScale > 32) {
        overG =
                ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }

    if ((uint32_t)blueScale > 32) {
        overB =
                ((uint32_t)(blueScale - 32) * 0x1f001f >> 5) & 0x1f001f;
        alphaB = 32;
    }

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                    rleTokens += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieTranslucentCopy(dstPixel, rleTokens, count);

                x += trim;
                dstPixel += trim;
                rleTokens += trim;
            }
            rleTokens += count;
            x += count;
            dstPixel += count;
            rleTokens += (-(intptr_t)rleTokens) & 3;
            break;

        case 3:
            x += count;
            dstPixel += count;
            break;

        case 4:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);

                x += trim;
                dstPixel += trim;
            }
            x += count;
            dstPixel += count;
            rleTokens += 4;
            break;

        case 1:
            if (rowIndex < srcBlitRect->top)
                rleTokens += count;
            else if (rowIndex >= srcBlitRect->bottom) {
                opcode = 0;
                break;
            }

            dstPixel = rowStart;
            x = 0;
            rowIndex++;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}

// FUNCTION: EVNOVA 0x004730d0
// MAC: _BlitPixieRLEAddOverClipped i386 0x000c25e7 ppc 0x000ce954
void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    short x;
    short rowIndex;
    int32_t lastCol;
    int32_t trim;

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                    rleTokens += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieAddOverCopy(dstPixel, rleTokens, count);

                x += trim;
                dstPixel += trim;
                rleTokens += trim;
            }
            rleTokens += count;
            x += count;
            dstPixel += count;
            rleTokens += (-(intptr_t)rleTokens) & 3;
            break;

        case 3:
            x += count;
            dstPixel += count;
            break;

        case 4:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);

                x += trim;
                dstPixel += trim;
            }
            x += count;
            dstPixel += count;
            rleTokens += 4;
            break;

        case 1:
            if (rowIndex < srcBlitRect->top)
                rleTokens += count;
            else if (rowIndex >= srcBlitRect->bottom) {
                opcode = 0;
                break;
            }

            dstPixel = rowStart;
            x = 0;
            rowIndex++;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}

// FUNCTION: EVNOVA 0x00473270
// MAC: _BlitPixieRLETranslucentClipped8 i386 0x000c1ee4 ppc 0x000ce0f4
void BlitPixieRLETranslucentClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    short x;
    short rowIndex;
    int32_t lastCol;
    int32_t trim;
    int32_t level;
    Boolean useTint;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = fix_power_of_two(alpha);
    alpha >>= 3;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;

    if (gSWCurrentSpriteBeingDrawn->tintMode >= 3) {
        useTint = 1;

        level = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
        level = (level < 31) ? level : 31;

        overBias = level >> 3;
        overBias = (overBias < 4) ? overBias : 4;
    } else {
        useTint = 0;
    }

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                    rleTokens += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                if (useTint)
                    BlitPixieTranslucentBiasCopy8(dstPixel, rleTokens, count);
                else
                    BlitPixieTranslucentCopy8(dstPixel, rleTokens, count);

                x += trim;
                dstPixel += trim;
                rleTokens += trim;
            }
            rleTokens += count;
            x += count;
            dstPixel += count;
            rleTokens += (-(intptr_t)rleTokens) & 3;
            break;

        case 3:
            x += count;
            dstPixel += count;
            break;

        case 4:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);

                x += trim;
                dstPixel += trim;
            }
            x += count;
            dstPixel += count;
            rleTokens += 4;
            break;

        case 1:
            if (rowIndex < srcBlitRect->top)
                rleTokens += count;
            else if (rowIndex >= srcBlitRect->bottom) {
                opcode = 0;
                break;
            }

            dstPixel = rowStart;
            x = 0;
            rowIndex++;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}

// FUNCTION: EVNOVA 0x00473500
// MAC: _BlitPixieRLEAddOverClipped8 i386 0x000c22af ppc 0x000ce594
void BlitPixieRLEAddOverClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    short x;
    short rowIndex;
    int32_t lastCol;
    int32_t trim;

    rowIndex = 0;
    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                    rleTokens += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieAddOverCopy8(dstPixel, rleTokens, count);

                x += trim;
                dstPixel += trim;
                rleTokens += trim;
            }
            rleTokens += count;
            x += count;
            dstPixel += count;
            rleTokens += (-(intptr_t)rleTokens) & 3;
            break;

        case 3:
            x += count;
            dstPixel += count;
            break;

        case 4:
            lastCol = x + count;
            if (x < srcBlitRect->right && lastCol > srcBlitRect->left) {
                if (x < srcBlitRect->left) {
                    trim = srcBlitRect->left - x;
                    count -= trim;
                    x += trim;
                    dstPixel += trim;
                }

                trim = 0;
                if (lastCol > srcBlitRect->right) {
                    trim = lastCol - srcBlitRect->right;
                    count -= trim;
                }

                BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);

                x += trim;
                dstPixel += trim;
            }
            x += count;
            dstPixel += count;
            rleTokens += 4;
            break;

        case 1:
            if (rowIndex < srcBlitRect->top)
                rleTokens += count;
            else if (rowIndex >= srcBlitRect->bottom) {
                opcode = 0;
                break;
            }

            dstPixel = rowStart;
            x = 0;
            rowIndex++;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            DebugStr("\runknown token");
            return;
        }
    } while (opcode != 0);
}
