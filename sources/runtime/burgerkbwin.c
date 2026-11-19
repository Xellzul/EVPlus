// COMPILE: -lang c++ -bool off
/* burgerkbwin.c -- BurgerLib InKeyboardWin95: the key queue, and the event pump (platform.h's events) that fills it and the mouse state. Notes: Research/notes/burgerkbwin.md */

#include "burgertypes.h"
#include "burger.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void WinGameEvent(const PlatEvent *ev);   /* port/dialogmgr.c: the window's own events */

static int32_t BurgerKeyStart;                      /* 0x008658a4 */
static int32_t BurgerKeyEnd;                        /* 0x008658a8 */
static Word32 BurgerKeyBuffer[128];              /* 0x008658ac */
Word (*KeyboardGetchCallBack)(Word key);         /* 0x0087070c */
Word Win95NoProcess;                             /* 0x00870de0 */
static Word Recurse;                             /* 0x00865aac */
Word32 Win95MouseButton;                         /* 0x008701a0 */
int32_t Win95MouseWheel;                            /* 0x008700a8 */
int32_t Win95LastMouseX;                            /* 0x008700b0 */
int32_t Win95LastMouseY;                            /* 0x008700b4 */
int32_t Win95MouseXDelta;                           /* 0x00870190 */
int32_t Win95MouseYDelta;                           /* 0x00870180 */

#define kKeyQueueMask   0x7f
#define kKeyAsciiMask   0xff
#define kKeyHeld        1                        /* KeyArray: held now */
#define kKeyPressed     2                        /* KeyArray: pressed since last read */

/* Queue one key; a full queue drops it. */
static void BurgerPostKey(Word32 key)
{
    int32_t end;
    int32_t next;

    if (key != 0) {
        end = BurgerKeyEnd;
        BurgerKeyBuffer[end] = key;
        next = (end + 1) & kKeyQueueMask;
        if (next != BurgerKeyStart)
            BurgerKeyEnd = next;
    }
}

/* A queue entry: the scan code << 8, then the character (or the special key's code below). */
#define QueueEntry(scan, ch)    (((Word32)(scan) << 8) | (ch))

/* The keys that post a code of their own rather than a character, by PLAT_KEY_*. */
static const Word32 SpecialKeys[PLAT_KEY_F1 + 12] = {
    0,
    QueueEntry(kScanHome, 0x91),                 /* PLAT_KEY_HOME */
    QueueEntry(kScanUp, 0x0b),                   /* PLAT_KEY_UP */
    QueueEntry(kScanPageUp, 0x92),               /* PLAT_KEY_PAGEUP */
    QueueEntry(kScanLeft, 0x96),                 /* PLAT_KEY_LEFT */
    QueueEntry(kScanRight, 0x15),                /* PLAT_KEY_RIGHT */
    QueueEntry(kScanEnd, 0x93),                  /* PLAT_KEY_END */
    QueueEntry(kScanDown, 0x0a),                 /* PLAT_KEY_DOWN */
    QueueEntry(kScanPageDown, 0x94),             /* PLAT_KEY_PAGEDOWN */
    QueueEntry(kScanInsert, 0x95),               /* PLAT_KEY_INSERT */
    QueueEntry(kScanDelete, 0x97),               /* PLAT_KEY_DELETE */
    QueueEntry(kScanPause, 0x80),                /* PLAT_KEY_PAUSE */
    QueueEntry(kScanScrollLock, 0x8d),           /* PLAT_KEY_SCROLLLOCK */
    QueueEntry(kScanLCtrl, 0),                   /* PLAT_KEY_LCTRL */
    QueueEntry(kScanRCtrl, 0),                   /* PLAT_KEY_RCTRL */
    QueueEntry(kScanLShift, 0),                  /* PLAT_KEY_LSHIFT */
    QueueEntry(kScanRShift, 0),                  /* PLAT_KEY_RSHIFT */
    QueueEntry(kScanLAlt, 0),                    /* PLAT_KEY_LALT */
    QueueEntry(kScanRAlt, 0),                    /* PLAT_KEY_RALT */
    QueueEntry(kScanF1, 0x81), QueueEntry(kScanF2, 0x82), QueueEntry(kScanF3, 0x83),   /* PLAT_KEY_F1.. */
    QueueEntry(kScanF4, 0x84), QueueEntry(kScanF5, 0x85), QueueEntry(kScanF6, 0x86),
    QueueEntry(kScanF7, 0x87), QueueEntry(kScanF8, 0x88), QueueEntry(kScanF9, 0x89),
    QueueEntry(kScanF10, 0x8a), QueueEntry(kScanF11, 0x8b), QueueEntry(kScanF12, 0x8c)  /* ..F12 */
};

static inline Word32 SpecialKey(int key)
{
    if (key <= 0 || key >= (int)(sizeof(SpecialKeys) / sizeof(SpecialKeys[0])))
        return 0;
    return SpecialKeys[key];
}

/* code page 1252's 0x80..0x9f, the characters that are not their own Unicode code point */
static const Word16 Ansi80[32] = {
    0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
    0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178
};

/* A typed character as the byte the game's Win32 window received in its WM_CHAR: the Western ANSI code
   page, 1252. 0 for a character it does not have. */
static Word AnsiChar(int ch)
{
    Word i;

    if (ch < 0x80 || (ch >= 0xa0 && ch <= 0xff))
        return ch;
    for (i = 0; i < 32; i++) {
        if (Ansi80[i] == ch)
            return 0x80 + i;
    }
    return 0;
}

/* KeyboardGetch (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d7240
unsigned int KeyboardGetch(void)
{
    int32_t i;
    Word32 key;

    if (BurgerKeyStart == BurgerKeyEnd) {
        while (KeyboardKbhit() == 0)
            ;
    }
    i = BurgerKeyStart;
    key = BurgerKeyBuffer[i];
    BurgerKeyStart = (i + 1) & kKeyQueueMask;
    key &= kKeyAsciiMask;
    if (KeyboardGetchCallBack != 0)
        key = KeyboardGetchCallBack(key);
    return key;
}

/* KeyboardKbhit (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d72a0
unsigned int KeyboardKbhit(void)
{
    int32_t i;
    Word32 key;

    ProcessSystemEvents();
    KeyboardCallPollingProcs();
    for (i = BurgerKeyStart; i != BurgerKeyEnd; i = (i + 1) & kKeyQueueMask) {
        key = BurgerKeyBuffer[i] & kKeyAsciiMask;
        if (key != 0)
            return key;
    }
    return 0;
}

/* KeyboardGet2 (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d72f0
int32_t KeyboardGet2(void)
{
    int32_t i;

    ProcessSystemEvents();
    KeyboardCallPollingProcs();
    i = BurgerKeyStart;
    if (i != BurgerKeyEnd) {
        BurgerKeyStart = (i + 1) & kKeyQueueMask;
        return BurgerKeyBuffer[i];
    }
    return 0;
}

/* Win95ProcessMessage (BurgerLib InKeyboardWin95, 0x004d7330), on a platform event in place of a Win32
   message: the key queue, the key-down table and the mouse state. */
static void Win95ProcessEvent(const PlatEvent *ev)
{
    Word32 key;
    Word scan;

    scan = (unsigned)ev->scan < kScanCount ? ev->scan : kScanNone;
    switch (ev->type) {
    case PLAT_EV_KEY_DOWN:
        key = SpecialKey(ev->key);
        if (key != 0)
            BurgerPostKey(key);
        if (scan != kScanNone)
            KeyArray[scan] |= kKeyHeld | kKeyPressed;
        break;
    case PLAT_EV_KEY_UP:
        if (scan != kScanNone)
            KeyArray[scan] &= ~kKeyHeld;
        break;
    case PLAT_EV_CHAR:
        key = AnsiChar(ev->ch);
        if (key != 0)
            BurgerPostKey(QueueEntry(scan, key));
        break;
    case PLAT_EV_MOUSE_DOWN:
        Win95MouseButton |= ev->button;
        MouseClicked |= ev->button;
        break;
    case PLAT_EV_MOUSE_UP:
        Win95MouseButton &= ~ev->button;
        break;
    case PLAT_EV_MOUSE_WHEEL:
        Win95MouseWheel += ev->wheel;
        break;
    case PLAT_EV_MOUSE_MOVE:
        Win95MouseX = ev->x;
        Win95MouseY = ev->y;
        Win95MouseXDelta += Win95MouseX - Win95LastMouseX;
        Win95MouseYDelta += Win95MouseY - Win95LastMouseY;
        Win95LastMouseX = Win95MouseX;
        Win95LastMouseY = Win95MouseY;
        break;
    }
}

/* ProcessSystemEvents (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d79a0
void ProcessSystemEvents(void)
{
    PlatEvent ev;

    if (Win95NoProcess == 0 && Recurse == 0) {
        Recurse = 1;
        while (PlatPollEvent(&ev) != 0) {
            Win95ProcessEvent(&ev);
            WinGameEvent(&ev);
        }
        Recurse = 0;
    }
}

/* Reset the input subsystem: forget every key and click. */
/* KeyboardInit (BurgerLib KeyboardInit). */
// FUNCTION: EVNOVA 0x004d7a80
void KeyboardInit(void)
{
    memset(KeyArray, 0, sizeof(KeyArray));
    BurgerKeyStart = 0;
    BurgerKeyEnd = 0;
    MouseClicked = 0;
}

/* KeyboardDestroy (BurgerLib KeyboardDestroy). */
// FUNCTION: EVNOVA 0x004d7ae0
void KeyboardDestroy(void)
{
}

#ifdef __cplusplus
}
#endif
