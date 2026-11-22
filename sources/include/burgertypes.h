/*
 * burgertypes.h -- BurgerLib's scalar typedefs, which every TU that used one
 * had been spelling for itself: `typedef unsigned long ULong;' alone appeared
 * 46 times. Types and pure constants only -- nothing with linkage -- so
 * including this exposes a TU to nothing that can move its codegen.
 */
#ifndef BURGERTYPES_H
#define BURGERTYPES_H
#include "int32.h"
/* the original MSL's stdio.h pulled these in behind the scenes; mingw's does not */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef char **Handle;
typedef unsigned char Word8;
typedef unsigned short Word16;
typedef unsigned int Word;
typedef uint32_t Word32;
typedef uint32_t ULong;
typedef unsigned long long ULongLong;

/* the decoded picture: BurgerLib's image record, and the Windows port's PicHandle */
typedef struct Image_t {
    Word8 *ImagePtr;                             /* +0x00 */
    Word8 *PalettePtr;                           /* +0x04 RGB triplets, 8 BPP only */
    Word8 *AlphaPtr;                             /* +0x08 */
    Word Width;                                  /* +0x0c */
    Word Height;                                 /* +0x10 */
    Word RowBytes;                               /* +0x14 */
    int DataType;                                /* +0x18 bits per pixel */
} Image_t;

typedef enum {IMAGE332=4,IMAGE8ALPHA=5,IMAGE8_PAL_ALPHA_PAL=6,IMAGE8_PAL_ALPHA=7,IMAGE8_PAL=8,
    IMAGE4444=13,IMAGE1555=14,IMAGE555=15,IMAGE565=16,IMAGE888=24,IMAGE8888=32} ImageTypes_e;


#endif
