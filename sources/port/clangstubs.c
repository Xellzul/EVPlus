/* clangstubs.c -- what the original Metrowerks toolchain supplied implicitly: MSL's float limits, which
 * BurgerLib's AsciiToFloat/AsciiToDouble read.
 */
#include <float.h>

const float XFloat_Inf = __builtin_inff();
const float XFloat_Nan = __builtin_nanf("");
const float XFloat_Min = FLT_MIN;
const float XFloat_Max = FLT_MAX;
const double XDouble_Inf = __builtin_inf();
const double XDouble_Nan = __builtin_nan("");
const double XDouble_Min = DBL_MIN;
const double XDouble_Max = DBL_MAX;
