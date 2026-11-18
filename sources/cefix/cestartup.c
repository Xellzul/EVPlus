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
#define kGameIconGroup      202
#define kNplayIconGroup     203                 /* CE's .nplay icon */


#define kIniPathMax         0x800
#define kIniLineMax         512

static char ceIniPath[kIniPathMax];             /* beside the exe; UTF-8 */
int ceX2Hotkey;
unsigned char ceRunInAWindow;

/* CE's entry wrapper and its MSL-entry hook, run as the game's first statement: the ddraw.ini path, the
   program and its .nplay files made known to the system (on Windows the registry: the App Paths key and
   the file type, written on every start), then the first argument becomes the current directory. */
void CE_Startup(void)
{
    snprintf(ceIniPath, sizeof(ceIniPath), "%s%s", PlatExeFolder(), kIniFileName);

    PlatRegisterFileType("EV Nova.exe", ".nplay", "EVNova.Play.1", "Play EV Nova", kNplayIconGroup,
                         kGameIconGroup);

    /* like CE: a .nplay FILE as the argument fails here and changes nothing */
    if (PlatArgCount() > 1)
        PlatSetCurrentFolder(PlatArg(1));
}

/* -- ddraw.ini's [EV Nova] section, read as Windows' GetPrivateProfileString read it for CE: a line is
   `key=value' with blanks around either dropped, a line starting with `;' is a comment, section and key
   names are upper and lower case alike, the first key of that name counts, and a value in quotes loses
   them. --------------------------------------------------------------------------------------- */

static int IsBlank(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* the text between `from' and `to' without the blanks at its ends, as a string in place */
static char *Trimmed(char *from, char *to)
{
    while (from < to && IsBlank(*from))
        from++;
    while (to > from && IsBlank(to[-1]))
        to--;
    *to = 0;
    return from;
}

/* the key's value into `out', cut to fit; 0 = no such key (or no file): `out' gets `def' */
static int IniString(const char *key, const char *def, char *out, int outSize)
{
    char line[kIniLineMax];
    FILE *file;
    char *text;
    char *end;
    char *equals;
    const char *value;
    size_t length;
    int inSection;
    int found;
    int skipped;

    value = def;
    found = 0;
    inSection = 0;
    file = PlatFileOpen(ceIniPath, "r");
    while (file != NULL && !found && fgets(line, sizeof(line), file) != NULL) {
        /* a line too long for the buffer: its rest is not another line */
        if (strchr(line, '\n') == NULL) {
            while ((skipped = fgetc(file)) != EOF && skipped != '\n')
                ;
        }
        text = Trimmed(line, line + strlen(line));
        if (text[0] == ';' || text[0] == 0)
            continue;
        if (text[0] == '[') {
            end = strchr(text, ']');
            if (end != NULL)
                *end = 0;
            inSection = stricmp(text + 1, kCEIniSection) == 0;
            continue;
        }
        equals = strchr(text, '=');
        if (!inSection || equals == NULL)
            continue;
        if (stricmp(Trimmed(text, equals), key) != 0)
            continue;
        text = Trimmed(equals + 1, equals + 1 + strlen(equals + 1));
        length = strlen(text);
        if (length >= 2 && (text[0] == '"' || text[0] == '\'') && text[length - 1] == text[0]) {
            text[length - 1] = 0;
            text++;
        }
        value = text;
        found = 1;
    }
    strncpy(out, value, outSize - 1);
    out[outSize - 1] = 0;
    if (file != NULL)
        fclose(file);
    return found;
}

void CE_ReadX2Hotkey(void)
{
    char buf[8];

    IniString("key_x2mode", kDefaultX2Hotkey, buf, sizeof(buf));
    ceX2Hotkey = strtol(buf, NULL, 0);
}

/* as GetPrivateProfileInt: the digits the value starts with, after a `-' if there is one; a value that
   is no number is 0 */
int CE_IniInt(const char *key, int def)
{
    char buf[32];
    const char *p;
    int negative;
    int value;

    if (!IniString(key, "", buf, sizeof(buf)))
        return def;
    p = buf;
    negative = *p == '-';
    if (negative)
        p++;
    for (value = 0; *p >= '0' && *p <= '9'; p++)
        value = value * 10 + (*p - '0');
    return negative ? -value : value;
}

double CE_IniDouble(const char *key, const char *def)
{
    char buf[8];

    IniString(key, def, buf, sizeof(buf));
    return atof(buf);
}

/* Replaces the monitor setup: ddraw.ini's game_width and game_height, else the desktop. */
void CE_InitializeMonitor(void)
{
    int32_t width;
    int32_t height;
    int desktopWidth;
    int desktopHeight;

    width = CE_IniInt("game_width", 0);
    height = CE_IniInt("game_height", 0);
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
