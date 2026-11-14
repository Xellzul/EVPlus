/* burgersqrt.c -- MSL's single-precision square root wrapper (sqrtf, math_x87.c) in the aligned run 0x004ee7a0-0x004eeff0. Notes: Research/notes/burgermisc.md */

#pragma inline_intrinsics off

extern double sqrt(double x);

/* FUN_004eed20 (MSL sqrtf). STATE: MATCH, byte-identical. OPEN: the name; see notes */
// FUNCTION: EVNOVA 0x004eed20
float FUN_004eed20(float x)
{
    return (sqrt)((x));
}
