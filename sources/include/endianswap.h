/* endianswap.h -- the big-endian field readers the win port uses on Mac-authored resources.
   Notes: Research/notes/endianswap.md */
#ifndef ENDIANSWAP_H
#define ENDIANSWAP_H

/* BurgerLib's SwapEndian written out; `static __inline' is the only spelling that inlines an
   asm block in C */
static __inline short SwapEndianShort(const unsigned short *p)
{
#ifdef __MWERKS__
    register unsigned short t;
#else
    unsigned short t;
#endif

    t = *p;
#ifdef __MWERKS__
    asm { ror t, 8 }
#else
    t = __builtin_bswap16(t);
#endif
    return t;
}

static __inline int32_t SwapEndianLong(const uint32_t *p)
{
#ifdef __MWERKS__
    register uint32_t t;
#else
    uint32_t t;
#endif

    t = *p;
#ifdef __MWERKS__
    asm { bswap t }
#else
    t = __builtin_bswap32(t);
#endif
    return t;
}

#endif
