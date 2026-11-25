/* burgerunquote.cpp -- BurgerLib StStringManager: file extensions, number formatting, Pascal string copies. Notes: Research/notes/burgerunquote.md */

#include "burger.h"

/* GetFileExtension (BurgerLib StStringManager): past the last '.', or the terminator when
   there is none. */
// FUNCTION: EVNOVA 0x004fbf20
char *GetFileExtension(const char *name)
{
    Word c;
    const char *p;

    c = (Word8)*name;
    if (c != 0) {
        p = name;
        name = 0;
        do {
            ++p;
            if (c == '.')
                name = p;
            c = (Word8)*p;
        } while (c != 0);
        if (name == 0)
            name = p;
    }
    return (char *)name;
}

#define kDigitMask     0xff
#define kSawDigit      0x8000
#define kMaxDigits     10

static const ULong kPow10[kMaxDigits] = {
    1UL, 10UL, 100UL, 1000UL, 10000UL,
    100000UL, 1000000UL, 10000000UL, 100000000UL, 1000000000UL
};

/* NumberToAscii (BurgerLib StStringManager). */
// FUNCTION: EVNOVA 0x004fbfe0
char *NumberToAscii(char *dst, ULong value, ULong flags)
{
    ULong digits;
    ULong place;
    ULong digit;

    digits = flags & kDigitMask;
    if (digits != 0 && digits < kMaxDigits)
        value = value % kPow10[digits];
    else
        digits = kMaxDigits;

    --digits;
    while (digits != 0) {
        place = kPow10[digits];
        digit = '0';
        if (value >= place) {
            flags = flags | kSawDigit;
            do {
                value = value - place;
                digit = digit + 1;
            } while (value >= place);
        }
        if ((flags & kSawDigit) != 0) {
            *dst = digit;
            dst = dst + 1;
        }
        --digits;
    }

    *dst = (value + '0');
    if ((flags & kNoTerminator) == 0)
        dst[1] = 0;
    return dst + 1;
}

#define kPascalMaxLen   0xff

/* C string to Pascal string, capped at 255 payload bytes. */
// FUNCTION: EVNOVA 0x004fc170
void CStr2PStr(char *dst, const char *src)
{
    ULong c;
    ULong len;
    ULong next;
    char *out;

    len = 0;
    out = dst + 1;
    c = (unsigned char)*src++;
    while (c != 0) {
        next = (unsigned char)*src++;
        *out++ = c;
        if (++len >= kPascalMaxLen)
            break;
        c = next;
    }
    dst[0] = len;
}

/* Pascal string to C string: the length byte leads, the terminator is ours. */
/* PStr2CStr (BurgerLib StStringManager). */
// FUNCTION: EVNOVA 0x004fc1c0
void PStr2CStr(char *dst, const unsigned char *src)
{
    uint32_t len;

    len = *src;
    src = src + 1;
    while (len != 0) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
        --len;
    }
    *dst = 0;
}
