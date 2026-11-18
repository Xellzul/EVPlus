// COMPILE: -lang c++ -bool off
/* computepixelvalues.c -- ComputePixelValues (win 0x00479210, donor SpriteWorldUtils.c line 900). Notes: Research/notes/computepixelvalues.md */
/* Names two SpriteWorldUtils.c globals outright: 0x007d0500 SWMaskedRGBValue and 0x007d0502
   SWTransparentRGBValue. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

unsigned short SWMaskedRGBValue;
unsigned short SWTransparentRGBValue;
RGBColor gSWTransparentColor = {0xFFFF, 0xFFFF, 0xFFFF};  /* white: SWCreatePixelMaskFromGWorld */
short gSWStickyError;


// FUNCTION: EVNOVA 0x00479210
void ComputePixelValues(short pixelSize)
{
    SWMaskedRGBValue = 0xFFFF;
    SWTransparentRGBValue = 0x0000;
}
