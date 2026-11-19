/* burgerinput.cpp -- the alert box and the debug log, on the platform layer (platform.h). Notes: Research/notes/burgerinput.md */

#include "burgertypes.h"
#include "burger.h"
#include "macport.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif


#define kAlertTitleMax      0x300
#define kAlertTextMax       0x1000               /* Alert's 1024 bytes, each up to three in UTF-8 */

static PlatMutex *Semi;                          /* 0x00865ae0 the log file's lock */
static int32_t Inited;                              /* 0x00865af8 lock state */

#define kDebugLockNone  0
#define kDebugLockLive  1
#define kDebugLockGone  2

/* The exe: a Windows message box with an OK button and no icon, the desktop brought to the front first.
   It took the text in the system's ANSI code page; here that is code page 1252. */
// FUNCTION: EVNOVA 0x004d9c90
void ShowAlert(const char *title, const char *message)
{
    char titleText[kAlertTitleMax];
    char messageText[kAlertTextMax];
    unsigned int wasVisible;

    wasVisible = OSCursorIsVisible();
    OSCursorShow();
    TextToUtf8(titleText, title, sizeof(titleText));
    TextToUtf8(messageText, message, sizeof(messageText));
    PlatAlert(titleText, messageText);
    if (wasVisible == 0)
        OSCursorHide();
}

// FUNCTION: EVNOVA 0x004d9cf0
void Cleanup(void)
{
    PlatMutexDestroy(Semi);
    Semi = 0;
    Inited = kDebugLockGone;
}

/* The text goes to LogFile.Txt and to the platform's debug output (the exe: OutputDebugString). */
// FUNCTION: EVNOVA 0x004d9d10
void DebugXString(const char *string)
{
    FILE *log;
    unsigned int length;

    if (Inited == kDebugLockNone) {
        Semi = PlatMutexCreate();
        Inited = kDebugLockLive;
        atexit(Cleanup);
    }
    if (Inited == kDebugLockLive)
        PlatMutexLock(Semi);
    length = strlen(string);
    if (length != 0) {
        log = FOpenExpandedPath("LogFile.Txt", "a");
        if (log != 0) {
            fwrite(string, 1, length, log);
            fclose(log);
        }
    }
    PlatDebugLog(string);
    if (Inited == kDebugLockLive)
        PlatMutexUnlock(Semi);
}

#ifdef __cplusplus
}
#endif
