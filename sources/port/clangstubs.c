/* clangstubs.c -- what the Metrowerks toolchain supplied implicitly, for other compilers (llvm-mingw).
 *
 * 1. MW's linker drops every function nothing live calls before it checks for missing symbols. The tree keeps
 *    a few such dead functions (the Vorbis encoder's envelope and bitrate code, vorbis_comment_add_tag, zlib's
 *    deflateBound) whose callees were never in the exe, so they are not in the tree. lld wants them defined:
 *    these bodies abort, so a call that ever became live fails loudly instead of jumping to address 0.
 * 2. MSL's float limits, which BurgerLib's AsciiToFloat/AsciiToDouble read.
 *
 * MW builds compile this file empty.
 */
#ifndef __MWERKS__
#include <stdlib.h>
#include <float.h>
#include "int32.h"
#include "zlib.h"
#include "ogg.h"
#include "codec.h"
#include "iir.h"

/* 1. dead in the exe (see above) */
void IIRStateInit(IIR_state *s, int stages, float gain, float *A, float *B) { abort(); }
float IIRStateFilter(IIR_state *s, float in) { abort(); return 0; }
float IIRStateFilterBand(IIR_state *s, float in) { abort(); return 0; }
void IIRStateReset(IIR_state *s) { abort(); }
int32_t oggpack_bytes(oggpack_buffer *b) { abort(); return 0; }
unsigned char *oggpack_get_buffer(oggpack_buffer *b) { abort(); return 0; }
void vorbis_comment_add(vorbis_comment *vc, char *comment) { abort(); }
uLong ZEXPORT compressBound(uLong sourceLen) { abort(); return 0; }

/* 2. MSL's float limits */
const float XFloat_Inf = __builtin_inff();
const float XFloat_Nan = __builtin_nanf("");
const float XFloat_Min = FLT_MIN;
const float XFloat_Max = FLT_MAX;
const double XDouble_Inf = __builtin_inf();
const double XDouble_Nan = __builtin_nan("");
const double XDouble_Min = DBL_MIN;
const double XDouble_Max = DBL_MAX;
#endif
