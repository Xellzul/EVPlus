// COMPILE: -lang c++ -bool off
/* burgerfatal.c -- BurgerLib's error reporters (DbDebugManager) and the key-name table. Notes: Research/notes/burgerfatal.md */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

char ErrorMsg[512];                              /* 0x0086e420 */
Word BombFlag;                                   /* 0x008701bc */

typedef struct KeyName {
    const char *name;
    int32_t code;
} KeyName;

/* 0x005776dc */
static KeyName kKeyNames[] = {
    { "Escape",     kScanEscape },
    { "Esc",        kScanEscape },
    { "0",          kScan0 },
    { "1",          kScan1 },
    { "2",          kScan2 },
    { "3",          kScan3 },
    { "4",          kScan4 },
    { "5",          kScan5 },
    { "6",          kScan6 },
    { "7",          kScan7 },
    { "8",          kScan8 },
    { "9",          kScan9 },
    { "A",          kScanA },
    { "B",          kScanB },
    { "C",          kScanC },
    { "D",          kScanD },
    { "E",          kScanE },
    { "F",          kScanF },
    { "G",          kScanG },
    { "H",          kScanH },
    { "I",          kScanI },
    { "J",          kScanJ },
    { "K",          kScanK },
    { "L",          kScanL },
    { "M",          kScanM },
    { "N",          kScanN },
    { "O",          kScanO },
    { "P",          kScanP },
    { "Q",          kScanQ },
    { "R",          kScanR },
    { "S",          kScanS },
    { "T",          kScanT },
    { "U",          kScanU },
    { "V",          kScanV },
    { "W",          kScanW },
    { "X",          kScanX },
    { "Y",          kScanY },
    { "Z",          kScanZ },
    { "F1",         kScanF1 },
    { "F2",         kScanF2 },
    { "F3",         kScanF3 },
    { "F4",         kScanF4 },
    { "F5",         kScanF5 },
    { "F6",         kScanF6 },
    { "F7",         kScanF7 },
    { "F8",         kScanF8 },
    { "F9",         kScanF9 },
    { "F10",        kScanF10 },
    { "F11",        kScanF11 },
    { "F12",        kScanF12 },
    { "-",          kScanMinus },
    { "=",          kScanEquals },
    { "BakSpc",     kScanBackspace },
    { "Tab",        kScanTab },
    { "[",          kScanLeftBracket },
    { "]",          kScanRightBracket },
    { ";",          kScanSemicolon },
    { "'",          kScanApostrophe },
    { "`",          kScanGrave },
    { "\\",         kScanBackslash },
    { ",",          kScanComma },
    { ".",          kScanPeriod },
    { "/",          kScanSlash },
    { "Enter",      kScanReturn },
    { "LCtrl",      kScanLCtrl },
    { "RCtrl",      kScanRCtrl },
    { "LShift",     kScanLShift },
    { "RShift",     kScanRShift },
    { "LAlt",       kScanLAlt },
    { "RAlt",       kScanRAlt },
    { "Space",      kScanSpace },
    { "CapLck",     kScanCapsLock },
    { "NumLck",     kScanNumLock },
    { "ScrLck",     kScanScrollLock },
    { "Pause",      kScanPause },
    { "Up",         kScanUp },
    { "Down",       kScanDown },
    { "Left",       kScanLeft },
    { "Right",      kScanRight },
    { "Insert",     kScanInsert },
    { "Delete",     kScanDelete },
    { "Home",       kScanHome },
    { "End",        kScanEnd },
    { "PgUp",       kScanPageUp },
    { "PgDn",       kScanPageDown },
    { "PrtScn",     kScanPrintScreen },
    { "KPad0",      kScanPad0 },
    { "KPad1",      kScanPad1 },
    { "KPad2",      kScanPad2 },
    { "KPad3",      kScanPad3 },
    { "KPad4",      kScanPad4 },
    { "KPad5",      kScanPad5 },
    { "KPad6",      kScanPad6 },
    { "KPad7",      kScanPad7 },
    { "KPad8",      kScanPad8 },
    { "KPad9",      kScanPad9 },
    { "KPad*",      kScanPadMultiply },
    { "KPad-",      kScanPadMinus },
    { "KPad+",      kScanPadPlus },
    { "KPad.",      kScanPadPeriod },
    { "KPad/",      kScanPadDivide },
    { "KPdEnt",     kScanPadEnter },
    { "KPadEnt",    kScanPadEnter },
    { "KPadEnter",  kScanPadEnter },
    { 0, -1 }
};

/* KeyboardScanCodeToString (BurgerLib KeyboardScanCodeToString). */
// FUNCTION: EVNOVA 0x004f1990
void KeyboardScanCodeToString(char *out, ULong size, ULong code)
{
    const KeyName *entry;

    if (size != 0) {
        entry = kKeyNames;
        do {
            if (entry->code == code) {
                code = strlen(entry->name);
                if (code >= size)
                    code = size - 1;
                memcpy(out, entry->name, code);
                out[code] = 0;
                return;
            }
            ++entry;
        } while (entry->name != 0);
        out[0] = 0;
    }
}

#ifdef __cplusplus
}
#endif
