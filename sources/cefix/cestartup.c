/* cestartup.c -- EV Nova CE: startup, ddraw.ini, windowing, x2 key. CE left the window to
   cnc-ddraw (its ddraw.dll); the platform layer has it now. Notes: Research/cefix-plan.md */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "macport.h"
#include "burger.h"
#include "cefix.h"
#include "platform.h"

#define kIniFileName        "ddraw.ini"
#define kDefaultX2Hotkey    "0x14"              /* VK_CAPITAL */
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

/* The windowed checkbox shows the window's state as it is, also after Alt+Enter. */
unsigned char CE_IsWindowed(void)
{
    return PlatVideoIsFullScreen() == 0;
}

void CE_ToggleFullscreen(void)
{
    PlatVideoSetFullScreen(ceRunInAWindow == 0);
}
