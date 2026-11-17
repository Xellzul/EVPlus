// COMPILE: -lang c++ -bool off
/* simplecrypt.c -- the resource XOR cipher (win 0x0046f960). Notes: Research/notes/simplecrypt.md */
#include <stddef.h>
#include "int32.h"
#include "gametypes.h"

EXTERN_C_BEGIN

#define kCryptStep 0xdeadbeefUL

// FUNCTION: EVNOVA 0x0046f960
// MAC: _DoEncryption i386 0x000a51cb ppc 0x000b0f40
int32_t DoEncryption(void *buf, uint32_t len, uint32_t key)
{
    short rem;
    uint32_t count;
    uint32_t mask;
    unsigned char *p;
    uint32_t word;
    uint32_t curKey;

    if (buf != NULL && len != 0) {
        p = (unsigned char *)buf;
        count = len >> 2;
        rem = (len - count * 4);
        curKey = key;
        while (count--) {
            word = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
                   | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
            word ^= *(uint32_t *)p;
            *(uint32_t *)p = word;
            curKey += kCryptStep;
            p += 4;
            curKey ^= kCryptStep;
        }
        mask = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
               | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
        mask ^= *(uint32_t *)p;
        while (rem--) {
            *p = mask & 0xff;
            p += 1;
            mask >>= 8;
        }
    }
    return 0;
}

EXTERN_C_END
