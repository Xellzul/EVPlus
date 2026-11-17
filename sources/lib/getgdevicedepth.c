// COMPILE: -lang c++ -bool off
/* getgdevicedepth.c -- GetGDeviceDepth (win 0x0047b7c0, donor SWGameUtils.c line 372). Notes: Research/notes/getgdevicedepth.md */
#include "macport.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x0047b7c0
// MAC: _GetGDeviceDepth i386 0x000c9520 ppc 0x000d54e4
short GetGDeviceDepth(void *theGDH)
{
    return 16;
}

EXTERN_C_END
