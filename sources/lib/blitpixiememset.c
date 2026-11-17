// COMPILE: -lang c++ -bool off
/* blitpixiememset.c -- BlitPixieMemSet (win 0x00474410). Notes: Research/notes/blitpixiememset.md */
#include "macport.h"
#include "shipobj.h"

EXTERN_C_BEGIN

/* BlitPixieMemSet (mac _BlitPixieMemSet). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474410
// MAC: _BlitPixieMemSet i386 0x000c0452 ppc 0x000cc158
void BlitPixieMemSet( unsigned char *dst, uint32_t value, int32_t bytes )
{
	int32_t	i;

	for ( i = 0; i < (bytes >> 2); i++, dst += 4 )
		*(uint32_t *)dst = value;
	if ( bytes & 2 ) {
		*(unsigned short *)dst = (unsigned short)value;
		dst += 2;
	}
	if ( bytes & 1 )
		*dst = (unsigned char)value;
}

EXTERN_C_END
