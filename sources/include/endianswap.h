/* endianswap.h -- the big-endian field readers the win port uses on Mac-authored resources.
   Notes: Research/notes/endianswap.md */
#ifndef ENDIANSWAP_H
#define ENDIANSWAP_H

/* BurgerLib's SwapEndian */
static __inline short SwapEndianShort(const unsigned short *p)
{
    unsigned short t;

    t = *p;
    t = __builtin_bswap16(t);
    return t;
}

static __inline int32_t SwapEndianLong(const uint32_t *p)
{
    uint32_t t;

    t = *p;
    t = __builtin_bswap32(t);
    return t;
}

#endif
