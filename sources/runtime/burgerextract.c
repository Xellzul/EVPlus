// COMPILE: -lang c++ -bool off
/* burgerextract.c -- BurgerLib ClStdlib: one line out of a text buffer. Notes: Research/notes/burgerextract.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kExtractStrNoTranslate  2                /* keep CR, and CR LF, as they are */

/* ExtractAString2 (BurgerLib ClStdlib): copy text up to a NUL (or a line end) into dst,
   terminated and truncated to dstSize; *size is the buffer left in, the bytes used out.
   STATE: MATCH 100.0%, exe 61 / obj 61, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00530170
void ExtractAString2(const char *src, ULong *size, Word flags, char *dst, ULong dstSize)
{
    ULong remaining;
    const char *p;
    Word c;

    remaining = *size;
    if (remaining != 0) {
        if (dstSize != 0) {
            --dstSize;
            dst[dstSize] = 0;
        }
        p = src;
        do {
            c = (Word8)*p++;
            if (!(flags & kExtractStrNoTranslate) && c == '\r') {
                c = '\n';
                if (remaining >= 2 && *p == '\n') {
                    ++p;
                    --remaining;
                }
            }
            if ((flags & kExtractStrDelimitLF) && c == '\n')
                break;
            if (c == 0)
                break;
            if (dstSize != 0) {
                --dstSize;
                *dst++ = c;
            }
        } while (--remaining != 0);
        if (dstSize != 0)
            *dst = 0;
        *size = p - src;
    }
}

#ifdef __cplusplus
}
#endif
