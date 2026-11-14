// COMPILE: -lang c++ -bool off
/* iir.c -- libvorbis 1.0rc3 lib/iir.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (IIR_clear renamed IIRStateDestroy, FastMemSet); altered: comments, the self-test and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "iir.h"

/* IIRStateDestroy (rc3 IIR_clear). STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055bd10
void IIRStateDestroy(IIR_state *s){
  if(s){
    _ogg_free(s->coeff_A);
    _ogg_free(s->coeff_B);
    _ogg_free(s->z_A);
    FastMemSet(s,0,sizeof(*s));
  }
}
