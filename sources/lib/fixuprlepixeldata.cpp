/* fixuprlepixeldata.cpp -- FixupRLEPixelData (win 0x00472000), the RLE byte-swapper. Notes: Research/notes/blitpixierleclipped.md */
#include "spriteworld.h"
#include "gameutil.h"

/* The 'RLE#' resource header; the layout of record is
   sources/lib/swcreaterledata.cpp. */

// FUNCTION: EVNOVA 0x00472000
void FixupRLEPixelData(unsigned char *tokens, RLEPixelData *header)
{
    uint32_t token, opcode, count;
    uint32_t depth;
    uint32_t frames;

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
                    *(unsigned short *)tokens = (unsigned short)((((token >> 8) & 0xff) | ((token & 0xff) << 8)) & 0x7fff);

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
