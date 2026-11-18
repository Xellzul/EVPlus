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

/* PC scan code (bit 7 = extended, PlatEvent.scan) to BurgerLib scan code. */
Word8 NormalScan[256] = {                        /* 0x00576b50 */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x6e, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6a, 0x6b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x60, 0x61, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00, 0x65,
    0x66, 0x67, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 0x70, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define kKeyQueueMask   0x7f
#define kKeyRelease     0x8000                   /* the queue entry is a key-up */
#define kKeyAsciiMask   0xff
#define kKeyHeld        1                        /* KeyArray: held now */
#define kKeyPressed     2                        /* KeyArray: pressed since last read */
#define kScanCodeMask   0xff

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

/* The keys that post a code of their own rather than a character, by PLAT_KEY_*. (The exe also has one
   for VK_PRINT, 0x6d8f, a key PC keyboards do not have.) */
static const Word16 SpecialKeys[PLAT_KEY_F1 + 12] = {
    0,
    0x6091,                                      /* PLAT_KEY_HOME */
    0x610b,                                      /* PLAT_KEY_UP */
    0x6292,                                      /* PLAT_KEY_PAGEUP */
    0x6396,                                      /* PLAT_KEY_LEFT */
    0x6415,                                      /* PLAT_KEY_RIGHT */
    0x6593,                                      /* PLAT_KEY_END */
    0x660a,                                      /* PLAT_KEY_DOWN */
    0x6794,                                      /* PLAT_KEY_PAGEDOWN */
    0x6895,                                      /* PLAT_KEY_INSERT */
    0x6997,                                      /* PLAT_KEY_DELETE */
    0x6e80,                                      /* PLAT_KEY_PAUSE */
    0x468d,                                      /* PLAT_KEY_SCROLLLOCK */
    0x1d00,                                      /* PLAT_KEY_LCTRL */
    0x6b00,                                      /* PLAT_KEY_RCTRL */
    0x2a00,                                      /* PLAT_KEY_LSHIFT */
    0x3600,                                      /* PLAT_KEY_RSHIFT */
    0x3800,                                      /* PLAT_KEY_LALT */
    0x6f00,                                      /* PLAT_KEY_RALT */
    0x3b81, 0x3c82, 0x3d83, 0x3e84, 0x3f85, 0x4086, /* PLAT_KEY_F1.. */
    0x4187, 0x4288, 0x4389, 0x448a, 0x578b, 0x588c  /* ..F12 */
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

    do {
        if (BurgerKeyStart == BurgerKeyEnd) {
            while (KeyboardKbhit() == 0)
                ;
        }
        i = BurgerKeyStart;
        key = BurgerKeyBuffer[i];
        BurgerKeyStart = (i + 1) & kKeyQueueMask;
    } while (key & kKeyRelease);
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
        key = BurgerKeyBuffer[i];
        if ((key & kKeyRelease) == 0) {
            key &= kKeyAsciiMask;
            if (key != 0)
                return key;
        }
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

    scan = NormalScan[ev->scan & kScanCodeMask];
    switch (ev->type) {
    case PLAT_EV_KEY_DOWN:
        key = SpecialKey(ev->key);
        if (key != 0)
            BurgerPostKey(key);
        KeyArray[scan] |= kKeyHeld | kKeyPressed;
        break;
    case PLAT_EV_KEY_UP:
        key = SpecialKey(ev->key);
        if (key != 0)
            BurgerPostKey(key | kKeyRelease);
        else
            BurgerPostKey((scan << 8) + kKeyRelease);
        KeyArray[scan] &= ~kKeyHeld;
        break;
    case PLAT_EV_CHAR:
        key = AnsiChar(ev->ch);
        if (key != 0)
            BurgerPostKey((scan << 8) + key);
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
    FastMemSet(KeyArray, 0, kKeyTableSize);
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
