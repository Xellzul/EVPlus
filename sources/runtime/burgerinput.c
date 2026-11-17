// COMPILE: -lang c++ -bool off
/* burgerinput.c -- the alert box and the debug log. Notes: Research/notes/burgerinput.md */

#include "burgertypes.h"
#include "burger.h"
#include "macport.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif


int32_t Win95MouseX;                                /* 0x0087001c */
int32_t Win95MouseY;                                /* 0x00870020 */
int32_t MouseClicked;                               /* 0x0087017c */

#define kAlertStyle   0

static Word Tested;                              /* 0x00865ad8 DbWin95Code's .bss, in order */
static int (__stdcall *DebugTest)(void);
static char Semi[24];                            /* 0x00865ae0 critsec, the log file's lock */
static int32_t Inited;                              /* 0x00865af8 lock state */

#define kDebugLockNone  0
#define kDebugLockLive  1
#define kDebugLockGone  2

// FUNCTION: EVNOVA 0x004d9c90
void ShowAlert(const char *title, const char *message)
{
    unsigned int wasVisible;

    wasVisible = OSCursorIsVisible();
    OSCursorShow();
    SetForegroundWindow(GetDesktopWindow());
    MessageBoxA(Win95MainWindow, message, title, kAlertStyle);
    if (wasVisible == 0)
        OSCursorHide();
}

// FUNCTION: EVNOVA 0x004d9cf0
void Cleanup(void)
{
    DeleteCriticalSection(Semi);
    Inited = kDebugLockGone;
}

/* Halt (BurgerLib dbwin32): break into an attached debugger. Not linked. */
__declspec(dllimport) void __stdcall DebugBreak(void);

void Halt(void)
{
    void *kernel;

    if (!Tested) {
        kernel = LoadLibraryA("kernel32.dll");
        if (kernel)
            DebugTest = (int (__stdcall *)(void))GetProcAddress(kernel, "IsDebuggerPresent");
        Tested = 1;
    }
    if (DebugTest && DebugTest())
        DebugBreak();
}

// FUNCTION: EVNOVA 0x004d9d10
void DebugXString(const char *string)
{
    FILE *log;
    unsigned int length;

    if (Inited == kDebugLockNone) {
        InitializeCriticalSection(Semi);
        Inited = kDebugLockLive;
        atexit(Cleanup);
    }
    if (Inited == kDebugLockLive)
        EnterCriticalSection(Semi);
    length = strlen(string);
    if (length != 0) {
        log = FOpenExpandedPath("LogFile.Txt", "a");
        if (log != 0) {
            fwrite(string, 1, length, log);
            fclose(log);
        }
    }
    OutputDebugStringA(string);
    if (Inited == kDebugLockLive)
        LeaveCriticalSection(Semi);
}

#ifdef __cplusplus
}
#endif
