/* blitpixiememcopy.cpp -- BlitPixieMemCopy (win 0x00474350). Notes: Research/notes/blitpixiememcopy.md */
#include "macport.h"
#include "shipobj.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00474350
// MAC: _BlitPixieMemCopy i386 0x000c03d0 ppc 0x000cc0cc
void BlitPixieMemCopy( unsigned char *dst, unsigned char *src, int32_t bytes)
{
	int32_t	i;

	for ( i = 0; i < (bytes >> 2); i++, dst += 4, src += 4 )
		*(uint32_t *)dst = *(uint32_t *)src;
	if ( bytes & 2 ) {
		*(unsigned short *)dst = *(unsigned short *)src;
		dst += 2;
		src += 2;
	}
	if ( bytes & 1 )
		*dst = *src;
}

EXTERN_C_END
