// COMPILE: -lang c++ -bool off
/* computepixelvalues.c -- ComputePixelValues (win 0x00479210, donor SpriteWorldUtils.c line 900). Notes: Research/notes/computepixelvalues.md */
/* Names two SpriteWorldUtils.c globals outright, because one function writes both in
   both arms with the values swapped: 0x007d0500 SWMaskedRGBValue (<= 8 bit 0x0000,
   > 8 bit 0xFFFF) and 0x007d0502 SWTransparentRGBValue (the reverse). */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

unsigned short SWMaskedRGBValue;
unsigned short SWTransparentRGBValue;
short gSWStickyError;


// FUNCTION: EVNOVA 0x00479210
void ComputePixelValues(short pixelSize)
{
    if (pixelSize <= 8) {
        SWMaskedRGBValue = 0x0000;
        SWTransparentRGBValue = 0xFFFF;
    } else {
        SWMaskedRGBValue = 0xFFFF;
        SWTransparentRGBValue = 0x0000;
    }
}
