/* keycodes.h -- the keys, by their place on the keyboard: USB HID usage numbers, the scan codes SDL reports
   (SDL_scancode.h), the same on every system. KeyIsDown and the key bindings use them.
   In C++ a key is a ScanCode, a type of its own: a plain number is not taken where a key is asked for, so a
   key can only be named by its kScan* constant (or made from a number on purpose, with ScanCode(n)). */
#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdint.h>

#define kScanCount          512         /* every scan code lies below this */

/* X(name, scan code) for every key the game names */
#define KEYCODES(X) \
    X(kScanNone,           0) /* no key */ \
    X(kScanA,              4) \
    X(kScanB,              5) \
    X(kScanC,              6) \
    X(kScanD,              7) \
    X(kScanE,              8) \
    X(kScanF,              9) \
    X(kScanG,              10) \
    X(kScanH,              11) \
    X(kScanI,              12) \
    X(kScanJ,              13) \
    X(kScanK,              14) \
    X(kScanL,              15) \
    X(kScanM,              16) \
    X(kScanN,              17) \
    X(kScanO,              18) \
    X(kScanP,              19) \
    X(kScanQ,              20) \
    X(kScanR,              21) \
    X(kScanS,              22) \
    X(kScanT,              23) \
    X(kScanU,              24) \
    X(kScanV,              25) \
    X(kScanW,              26) \
    X(kScanX,              27) \
    X(kScanY,              28) \
    X(kScanZ,              29) \
    X(kScan1,              30) \
    X(kScan2,              31) \
    X(kScan3,              32) \
    X(kScan4,              33) \
    X(kScan5,              34) \
    X(kScan6,              35) \
    X(kScan7,              36) \
    X(kScan8,              37) \
    X(kScan9,              38) \
    X(kScan0,              39) \
    X(kScanF1,             58) \
    X(kScanF2,             59) \
    X(kScanF3,             60) \
    X(kScanF4,             61) \
    X(kScanF5,             62) \
    X(kScanF6,             63) \
    X(kScanF7,             64) \
    X(kScanF8,             65) \
    X(kScanF9,             66) \
    X(kScanF10,            67) \
    X(kScanF11,            68) \
    X(kScanF12,            69) \
    X(kScanF13,            104) \
    X(kScanF14,            105) \
    X(kScanF15,            106) \
    X(kScanF16,            107) \
    X(kScanF17,            108) \
    X(kScanF18,            109) \
    X(kScanF19,            110) \
    X(kScanF20,            111) \
    X(kScanF21,            112) \
    X(kScanF22,            113) \
    X(kScanF23,            114) \
    X(kScanF24,            115) \
    X(kScanEscape,         41) \
    X(kScanReturn,         40) \
    X(kScanBackspace,      42) \
    X(kScanTab,            43) \
    X(kScanSpace,          44) \
    X(kScanMinus,          45) \
    X(kScanEquals,         46) \
    X(kScanLeftBracket,    47) \
    X(kScanRightBracket,   48) \
    X(kScanBackslash,      49) \
    X(kScanNonUSHash,      50) \
    X(kScanSemicolon,      51) \
    X(kScanApostrophe,     52) \
    X(kScanGrave,          53) \
    X(kScanComma,          54) \
    X(kScanPeriod,         55) \
    X(kScanSlash,          56) \
    X(kScanCapsLock,       57) \
    X(kScanPrintScreen,    70) \
    X(kScanScrollLock,     71) \
    X(kScanPause,          72) \
    X(kScanInsert,         73) \
    X(kScanHome,           74) \
    X(kScanPageUp,         75) \
    X(kScanDelete,         76) \
    X(kScanEnd,            77) \
    X(kScanPageDown,       78) \
    X(kScanRight,          79) \
    X(kScanLeft,           80) \
    X(kScanDown,           81) \
    X(kScanUp,             82) \
    X(kScanNumLock,        83) \
    X(kScanPadDivide,      84) \
    X(kScanPadMultiply,    85) \
    X(kScanPadMinus,       86) \
    X(kScanPadPlus,        87) \
    X(kScanPadEnter,       88) \
    X(kScanPad1,           89) \
    X(kScanPad2,           90) \
    X(kScanPad3,           91) \
    X(kScanPad4,           92) \
    X(kScanPad5,           93) \
    X(kScanPad6,           94) \
    X(kScanPad7,           95) \
    X(kScanPad8,           96) \
    X(kScanPad9,           97) \
    X(kScanPad0,           98) \
    X(kScanPadPeriod,      99) \
    X(kScanNonUSBackslash, 100) \
    X(kScanApplication,    101) \
    X(kScanPadEquals,      103) \
    X(kScanPadComma,       133) \
    X(kScanInternational1, 135) \
    X(kScanInternational2, 136) \
    X(kScanInternational3, 137) \
    X(kScanInternational4, 138) \
    X(kScanInternational5, 139) \
    X(kScanLang1,          144) \
    X(kScanLang2,          145) \
    X(kScanLCtrl,          224) \
    X(kScanLShift,         225) \
    X(kScanLAlt,           226) \
    X(kScanLGui,           227) \
    X(kScanRCtrl,          228) \
    X(kScanRShift,         229) \
    X(kScanRAlt,           230) \
    X(kScanRGui,           231)

#ifdef __cplusplus
enum class ScanCode : int16_t {};
#define KEYCODE_CONSTANT(name, value) constexpr ScanCode name = ScanCode(value);
KEYCODES(KEYCODE_CONSTANT)
#undef KEYCODE_CONSTANT
#else
typedef int16_t ScanCode;
#define KEYCODE_CONSTANT(name, value) name = value,
enum { KEYCODES(KEYCODE_CONSTANT) };
#undef KEYCODE_CONSTANT
#endif

/* The key characters the key queue gives: ASCII control characters, and codes of their own for the keys
   that type nothing (they share 0x80..0x97 with code page 1252's typed characters). */
#define kBackspaceChar      0x08
#define kTabChar            0x09
#define kDownArrowChar      0x0a
#define kUpArrowChar        0x0b
#define kReturnChar         0x0d
#define kRightArrowChar     0x15
#define kEscapeChar         0x1b
#define kPauseChar          0x80
#define kF1Char             0x81        /* F1..F12 are 0x81..0x8c */
#define kScrollLockChar     0x8d
#define kHomeChar           0x91
#define kPageUpChar         0x92
#define kEndChar            0x93
#define kPageDownChar       0x94
#define kInsertChar         0x95
#define kLeftArrowChar      0x96
#define kDeleteChar         0x97
#define IsReturnChar(c) ((c) == kReturnChar)

#endif
