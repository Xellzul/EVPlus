// COMPILE: -lang c++ -bool off
/* burgerunquote.c -- BurgerLib StStringManager: trimming, extensions, the quoted-value reader, number formatting, string copies. Notes: Research/notes/burgerunquote.md */

#include "burgertypes.h"
#include "burger.h"

/* The member's one byte of .bss; no live code reads it. */
namespace Burger {
class Str {
public:
    static const char Empty[1];
};
}

const char Burger::Str::Empty[1] = "";

#ifdef __cplusplus
extern "C" {
#endif

/* StripLeadingAndTrailingWhiteSpace (BurgerLib StStringManager): spaces and tabs off both
   ends, in place. */
// FUNCTION: EVNOVA 0x004fbec0
void StripLeadingAndTrailingWhiteSpace(char *text)
{
    char *end;
    Word c;
    char *out;

    end = text;
    c = (Word8)*text++;
    if (c == ' ' || c == '\t') {
        do {
            c = (Word8)*text++;
        } while (c == ' ' || c == '\t');
    }
    if (c != 0) {
        out = end;
        do {
            *out++ = c;
            if (c != ' ' && c != '\t')
                end = out;
            c = (Word8)*text++;
        } while (c != 0);
    }
    *end = c;
}

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

// FUNCTION: EVNOVA 0x004fbf50
char *ParseQuotedString(char *out, int32_t outSize, const char *src)
{
    int32_t room;
    uint32_t c;
    int32_t next;

    if (*src == '"') {
        room = outSize;
        if (room != 0)
            --room;
        for (;;) {
            src = src + 1;
            c = (unsigned char)*src;
            if (c == '\t') {
                c = ' ';
            } else if (c == '"') {
                src = src + 1;
                next = (unsigned char)*src;
                if (next != '"')
                    break;
            } else if (c == 0 || c == '\n') {
                break;
            } else if (c == '\r') {
                if (*src == '\n')
                    src = src + 1;
                break;
            }
            if (room != 0) {
                *out = c;
                out = out + 1;
                room = room - 1;
            }
        }
    }
    if (outSize != 0)
        *out = 0;
    return (char *)src;
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

extern const float XFloat_Inf;
extern const float XFloat_Nan;
extern const float XFloat_Min;
extern const float XFloat_Max;
extern const double XDouble_Inf;
extern const double XDouble_Nan;
extern const double XDouble_Min;
extern const double XDouble_Max;

static const float PowerOf10FltTable[6] = {
    1.0e1f, 1.0e2f, 1.0e4f, 1.0e8f, 1.0e16f, 1.0e32f
};

static const double PowerOf10DblTable[9] = {
    1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256
};

#define kPascalMaxLen   0xff

/* StrCopy (BurgerLib StStringManager): a C string copied into a new pointer. */
// FUNCTION: EVNOVA 0x004fc120
char *StrCopy(const char *text)
{
    Word length;
    char *copy;

    length = strlen(text) + 1;
    copy = (char *)AllocAPointer(length);
    if (copy != 0)
        memcpy(copy, text, length);
    return copy;
}

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

/* StrCopyHandle (BurgerLib StStringManager): a C string copied into a new handle. */
// FUNCTION: EVNOVA 0x004fc1f0
Handle StrCopyHandle(const char *text)
{
    Word length;
    Handle handle;

    if (text != 0) {
        length = strlen(text) + 1;
        handle = (Handle)AllocAHandle2(length, 0);
        if (handle != 0) {
            memcpy(*handle, text, length);
            return handle;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
