/* burgerfatal2.cpp -- burgerfatal, exe run 2. Notes: Research/notes/burgerfatal.md */

#include <stdarg.h>
#include "burger.h"
#include "macport.h"
#include "platform.h"
#include "crashlog.h"

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
        ShutdownWinGame();
        PlatQuit();
        exit(kFatalExitCode);
    }
}

/* NonFatal: the message kept in ErrorMsg and written to the debug log as one line. */
// FUNCTION: EVNOVA 0x00503e80
void NonFatal(const char *fmt, ...)
{
    va_list args;
    size_t length;

    if (fmt != 0) {
        va_start(args, fmt);
        vsprintf(ErrorMsg, fmt, args);
        va_end(args);
        length = strlen(ErrorMsg);
        crashlog_print(ErrorMsg);
        if (length == 0 || ErrorMsg[length - 1] != '\n')
            crashlog_print("\n");
    }
}
