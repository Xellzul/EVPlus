/* cestartup.cpp -- EV Nova CE: startup, windowing. CE left the window to cnc-ddraw (its ddraw.dll); the
   platform layer has it now. Notes: Research/cefix-plan.md */

#include "burger.h"
#include "cefix.h"
#include "platform.h"

#define kGameIconGroup      202
#define kNplayIconGroup     203                 /* CE's .nplay icon */

unsigned char ceRunInAWindow;

/* CE's entry wrapper and its MSL-entry hook, run as the game's first statement: the program and its .nplay
   files made known to the system (on Windows the registry: the App Paths key and the file type, written on
   every start), then the first argument becomes the current directory. */
void CE_Startup(void)
{
    PlatRegisterFileType("EV Nova.exe", ".nplay", "EVNova.Play.1", "Play EV Nova", kNplayIconGroup,
                         kGameIconGroup);

    /* like CE: a .nplay FILE as the argument fails here and changes nothing */
    if (PlatArgCount() > 1)
        PlatSetCurrentFolder(PlatArg(1));
}

/* The windowed checkbox shows the window's state as it is, also after Alt+Enter. */
unsigned char CE_IsWindowed(void)
{
    return PlatVideoIsFullScreen() == 0;
}

void CE_ToggleFullscreen(void)
{
    PlatVideoSetFullScreen(ceRunInAWindow == 0);
}
