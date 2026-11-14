// COMPILE: -lang c++ -bool off
/* analysis.c -- libvorbis 1.0rc3 lib/analysis.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. Stock in BurgerLib's copy; altered: comments, the ANALYSIS branch (not built: the exe's _analysis_output is a bare ret) and what the exe does not link (vorbis_analysis, _analysis_output_always) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "scales.h"
#include "os.h"
#include "misc.h"

int analysis_noisy=1;

/* _analysis_output. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055bd00
void _analysis_output(char *base,int i,float *v,int n,int bark,int dB){
}
