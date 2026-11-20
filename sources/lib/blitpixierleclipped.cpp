/* blitpixierleclipped.cpp -- BlitPixieRLEClipped and the RLE byte-swapper FixupRLEPixelData (win 0x00471e90-0x00472190). Notes: Research/notes/blitpixierleclipped.md */
#include "spriteworld.h"

EXTERN_C_BEGIN


// FUNCTION: EVNOVA 0x00471e90
// MAC: _BlitPixieRLEClipped i386 0x000bde9c ppc 0x000c9938
void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    unsigned char *rowStart;
    uint32_t token;
    uint32_t opcode;
    int32_t count;
    int32_t x;
    int32_t rowIndex;
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
            // BUG: lastCol is inclusive but srcBlitRect->right is exclusive, so the right trim leaves one byte
            // past the clip edge (a partial pixel at 16bpp), and a span ending exactly on the left edge is
            // skipped; the translucent/add-over clipped blitters use x + count and clip exactly
            lastCol = x + count - 1;
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

                BlitPixieMemCopy(dstPixel, rleTokens, count);

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
            lastCol = x + count - 1;
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
            return;
        }
    } while (opcode != 0);
}


/* The 'RLE#' resource header; the layout of record is
   sources/lib/swcreaterledata.cpp. */

// FUNCTION: EVNOVA 0x00472000
void FixupRLEPixelData(unsigned char *tokens, RLEPixelData *header)
{
    uint32_t token, opcode, count;
    uint32_t depth;
    uint32_t frames;
    unsigned short pixel;

    depth = header->depth;
    frames = header->frames;

    if (frames == 0)
        return;

    do {
        token = *(uint32_t *)tokens;
        *(uint32_t *)tokens = ((token >> 24) & 0xff) | ((token >> 8) & 0xff00) |
                ((token & 0xff00) << 8) | ((token & 0xff) << 24);

        token = *(uint32_t *)tokens;
        opcode = token >> 24;
        tokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            switch (depth) {
            case 8:
                tokens += count;
                break;

            case 16:
                count >>= 1;
                do {
                    token = *(unsigned short *)tokens;
                    token = (((int32_t)token >> 8) & 0xff) | ((token & 0xff) << 8);
                    pixel = token;

                    *(unsigned short *)tokens =
                            (gGammaRamp[(pixel >> 10) & 0x1f] << 10) +
                            (gGammaRamp[(pixel >> 5) & 0x1f] << 5) +
                            gGammaRamp[pixel & 0x1f];

                    tokens += 2;
                } while (--count);
                break;

            case 32:
                count >>= 2;
                do {
                    token = *(uint32_t *)tokens;
                    *(uint32_t *)tokens = ((token >> 24) & 0xff) |
                            ((token >> 8) & 0xff00) | ((token & 0xff00) << 8) |
                            ((token & 0xff) << 24);

                    tokens += 4;
                } while (--count);
                break;
            }

            tokens += (-(intptr_t)tokens) & 3;
            break;

        case 4:
            token = *(uint32_t *)tokens;
            *(uint32_t *)tokens = ((token >> 24) & 0xff) |
                    ((token >> 8) & 0xff00) | ((token & 0xff00) << 8) |
                    ((token & 0xff) << 24);

            tokens += 4;
            break;

        case 0:
        case 1:
        case 3:
            break;

        default:
            return;
        }
    } while (opcode != 0 || --frames != 0);
}

EXTERN_C_END
