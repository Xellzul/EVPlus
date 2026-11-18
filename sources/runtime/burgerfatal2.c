// COMPILE: -lang c++ -bool off
/* burgerfatal2.c -- burgerfatal, exe run 2. Notes: Research/notes/burgerfatal.md */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFatalExitCode 0x37

Word IAmExiting;                                 /* 0x00870d98 */

/* Fatal (BurgerLib DbDebugManager). */
// FUNCTION: EVNOVA 0x00503df0
void Fatal(const char *fmt, ...)
{
    va_list args;

    if (fmt != 0 && fmt != ErrorMsg) {
        va_start(args, fmt);
        vsprintf(ErrorMsg, fmt, args);
        va_end(args);
    }

    if (IAmExiting == 0) {
        IAmExiting = 1;
        if (fmt != 0)
            DebugXString(ErrorMsg);
        exit(kFatalExitCode);
    }
}

/* NonFatal (BurgerLib DbDebugManager). */
// FUNCTION: EVNOVA 0x00503e80
void NonFatal(const char *fmt, ...)
{
    va_list args;

    if (fmt != 0) {
        va_start(args, fmt);
        vsprintf(ErrorMsg, fmt, args);
        va_end(args);
        if (BombFlag != 0)
            Fatal(ErrorMsg);
    }
}

/* DebugXLongWord (BurgerLib DbDebugManager). */
// FUNCTION: EVNOVA 0x00503ef0
void DebugXLongWord(Word32 value)
{
    char text[16];

    NumberToAscii(text, value, 0);
    DebugXString(text);
}

#ifdef __cplusplus
}
#endif
