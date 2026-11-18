/* cestartup.c -- EV Nova CE: startup, ddraw.ini, video mode, windowing, x2 key. CE left the window to
   cnc-ddraw (its ddraw.dll); the platform layer has it now. Notes: Research/cefix-plan.md */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "macport.h"
#include "burger.h"
#include "player.h"
#include "cefix.h"
#include "platform.h"

#define kIniFileName        "ddraw.ini"
#define kDefaultX2Hotkey    "0x14"              /* VK_CAPITAL */
#define kVideoDepth         15
#define kVideoFullScreen    0x01                /* borderless, at the desktop's mode */
#define kVideoAlways        0x02
#define kRegOptionNonVolatile 0
#define kKeyWrite           0x20006
#define kRegSZ              1
#define kSHCNEAssocChanged  0x08000000
#define kSHCNFIDList        0
#define kGameIconGroup      202
#define kNplayIconGroup     203                 /* CE's .nplay icon */
#define kRTGroupIcon        ((const char *)14)  /* RT_GROUP_ICON */

__declspec(dllimport) unsigned int __stdcall GetPrivateProfileIntA(const char *app,
    const char *key, int def, const char *file);
__declspec(dllimport) uint32_t __stdcall GetPrivateProfileStringA(const char *app,
    const char *key, const char *def, char *out, uint32_t size, const char *file);
__declspec(dllimport) void __stdcall SHChangeNotify(int32_t eventId, unsigned int flags,
    const void *item1, const void *item2);

extern int __argc;                              /* MSL runtime */

char ceIniPath[kCEMaxPath];
int ceX2Hotkey;
unsigned char ceRunInAWindow;

static void SetRegValue(const char *key, const char *value)
{
    void *hkey;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, key, 0, NULL, kRegOptionNonVolatile, kKeyWrite,
                        NULL, &hkey, NULL) != 0)
        return;
    RegSetValueExA(hkey, NULL, 0, kRegSZ, (const unsigned char *)value, strlen(value) + 1);
    RegCloseKey(hkey);
}

/* CE's entry wrapper and its MSL-entry hook, run as WinMain's first statement: the
   ddraw.ini path, the App Paths key and the .nplay association, then the first argument
   becomes the current directory. */
void CE_Startup(void)
{
    char exePath[kCEMaxPath];
    char command[kCEMaxPath + 16];
    char *slash;
    int icon;

    GetModuleFileNameA(NULL, exePath, sizeof(exePath));

    strcpy(ceIniPath, exePath);
    slash = strrchr(ceIniPath, '\\');
    strcpy(slash != NULL ? slash + 1 : ceIniPath, kIniFileName);

    SetRegValue("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\EV Nova.exe",
                exePath);
    SetRegValue("Software\\Classes\\.nplay", "EVNova.Play.1");
    SetRegValue("Software\\Classes\\EVNova.Play.1", "Play EV Nova");
    sprintf(command, "\"%s\" \"%%1\"", exePath);
    SetRegValue("Software\\Classes\\EVNova.Play.1\\shell\\open\\command", command);
    icon = FindResourceA(NULL, (const char *)kNplayIconGroup, kRTGroupIcon) != NULL
               ? kNplayIconGroup : kGameIconGroup;
    sprintf(command, "\"%s\",-%d", exePath, icon);
    SetRegValue("Software\\Classes\\EVNova.Play.1\\DefaultIcon", command);
    SHChangeNotify(kSHCNEAssocChanged, kSHCNFIDList, NULL, NULL);

    /* like CE: a .nplay FILE as the argument fails here and changes nothing */
    if (__argc > 1)
        SetCurrentDirectoryA(__argv[1]);
}

void CE_ReadX2Hotkey(void)
{
    char buf[8];

    GetPrivateProfileStringA(kCEIniSection, "key_x2mode", kDefaultX2Hotkey, buf, sizeof(buf),
                             ceIniPath);
    ceX2Hotkey = strtol(buf, NULL, 0);
}

int CE_IniInt(const char *key, int def)
{
    return GetPrivateProfileIntA(kCEIniSection, key, def, ceIniPath);
}

double CE_IniDouble(const char *key, const char *def)
{
    char buf[8];

    GetPrivateProfileStringA(kCEIniSection, key, def, buf, sizeof(buf), ceIniPath);
    return atof(buf);
}

/* Replaces the monitor setup: ddraw.ini's game_width and game_height, else the desktop. */
void CE_InitializeMonitor(void)
{
    int32_t width;
    int32_t height;
    int desktopWidth;
    int desktopHeight;

    width = GetPrivateProfileIntA(kCEIniSection, "game_width", 0, ceIniPath);
    height = GetPrivateProfileIntA(kCEIniSection, "game_height", 0, ceIniPath);
    PlatDesktopSize(&desktopWidth, &desktopHeight);
    if (width == 0)
        width = desktopWidth;
    if (height == 0)
        height = desktopHeight;
    SetDisplayToSize(width, height, kVideoDepth, kVideoFullScreen | kVideoAlways);
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
