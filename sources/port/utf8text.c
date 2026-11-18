/* utf8text.c -- the game's text as UTF-8 and back. The game's paths and what the platform layer shows are
   UTF-8; a name the player typed (a pilot's) and an alert's text are in the Western ANSI code page, 1252,
   as the exe's window received and showed them. The exe handed such text to Windows as it was, which took
   it in the system's code page. */

#include <string.h>
#include "macport.h"

#define kNoSuchChar     '_'             /* for a character the other side does not have */
#define kAnsiTop        0x100
#define kReplacement    0xfffd

/* one character off a UTF-8 string; a broken sequence gives kReplacement and moves on a byte */
static unsigned NextChar(const unsigned char **text)
{
    const unsigned char *p = *text;
    unsigned c = *p++;
    unsigned least;
    int more;
    int i;

    if (c < 0x80) {
        *text = p;
        return c;
    }
    if (c >= 0xc2 && c < 0xe0) {
        more = 1;
        c &= 0x1f;
        least = 0x80;
    } else if (c >= 0xe0 && c < 0xf0) {
        more = 2;
        c &= 0x0f;
        least = 0x800;
    } else if (c >= 0xf0 && c < 0xf5) {
        more = 3;
        c &= 0x07;
        least = 0x10000;
    } else {
        *text = p;
        return kReplacement;
    }
    for (i = 0; i < more; i++) {
        if ((p[i] & 0xc0) != 0x80) {
            *text = p;
            return kReplacement;
        }
        c = c << 6 | (p[i] & 0x3f);
    }
    *text = p + more;
    return c >= least ? c : kReplacement;
}

void TextToUtf8(char *out, const char *text, int outSize)
{
    const unsigned char *src = (const unsigned char *)text;
    unsigned c;
    int n;

    n = 0;
    while (*src != 0 && n + 3 < outSize) {
        c = AnsiToUnicode(*src++);
        if (c < 0x80) {
            out[n++] = (char)c;
        } else if (c < 0x800) {
            out[n++] = (char)(0xc0 | c >> 6);
            out[n++] = (char)(0x80 | (c & 0x3f));
        } else {
            out[n++] = (char)(0xe0 | c >> 12);
            out[n++] = (char)(0x80 | (c >> 6 & 0x3f));
            out[n++] = (char)(0x80 | (c & 0x3f));
        }
    }
    out[n] = 0;
}

void Utf8ToText(char *out, const char *name, int outSize)
{
    const unsigned char *src = (const unsigned char *)name;
    unsigned c;
    unsigned ansi;
    int n;

    n = 0;
    while (*src != 0 && n + 1 < outSize) {
        c = NextChar(&src);
        for (ansi = 0; ansi < kAnsiTop && AnsiToUnicode((unsigned char)ansi) != c; ansi++)
            ;
        out[n++] = ansi < kAnsiTop ? (char)ansi : kNoSuchChar;
    }
    out[n] = 0;
}
