/* burgerkbwin.cpp -- BurgerLib's keyboard and mouse (InKeyboard, InKeyboardWin95, InMouseWin95) on SDL's
   events: the event pump, the key queue, the held keys, the mouse buttons and position. */

#include <SDL3/SDL.h>
#include "burger.h"
#include "platform.h"
#include "screen.h"

/* port/dialogmgr.cpp: the window's own events */
extern void WinGameActivate(int on);
extern void WinGameMinimized(void);
extern void WinGameWindowMode(int windowed);
extern void WinGameClose(void);
extern void WinGameInputSeen(void);

static int32_t BurgerKeyStart;                   /* 0x008658a4 */
static int32_t BurgerKeyEnd;                     /* 0x008658a8 */
static uint8_t BurgerKeyBuffer[128];             /* 0x008658ac: key characters; 0 = Shift, Ctrl or Alt */
static Word Recurse;                             /* 0x00865aac */
static Word32 Win95MouseButton;                  /* 0x008701a0 */
static int32_t Win95MouseX;                      /* 0x0087001c */
static int32_t Win95MouseY;                      /* 0x00870020 */
static int32_t WheelNotches;                     /* whole wheel notches not taken yet; > 0 = up */
int32_t MouseClicked;                            /* 0x0087017c */

#define kKeyQueueMask   0x7f
#define kWheelBacklog   3                /* wheel notches kept for later dialog passes */
#define kMouseButtons   (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK | SDL_BUTTON_RMASK)   /* the side buttons are not used */

/* Queue one key character; a full queue drops it. */
static void BurgerPostKey(int ch)
{
    int32_t end;
    int32_t next;

    end = BurgerKeyEnd;
    BurgerKeyBuffer[end] = (uint8_t)ch;
    next = (end + 1) & kKeyQueueMask;
    if (next != BurgerKeyStart)
        BurgerKeyEnd = next;
}

/* The code a key posts of its own rather than a character, -1 for the other keys. Shift, Ctrl and Alt post 0:
   every queued key reaches a dialog as a key press, so a binding on a key that types nothing (an F key, Ctrl)
   works in the dialogs that check the bindings on a key press. The keypad's keys are the navigation keys
   with Num Lock off. */
static int SpecialKey(SDL_Scancode code, SDL_Keymod mod)
{
    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F12)
        return kF1Char + (code - SDL_SCANCODE_F1);
    if ((mod & SDL_KMOD_NUM) == 0) {
        switch (code) {
        case SDL_SCANCODE_KP_7:         code = SDL_SCANCODE_HOME; break;
        case SDL_SCANCODE_KP_8:         code = SDL_SCANCODE_UP; break;
        case SDL_SCANCODE_KP_9:         code = SDL_SCANCODE_PAGEUP; break;
        case SDL_SCANCODE_KP_4:         code = SDL_SCANCODE_LEFT; break;
        case SDL_SCANCODE_KP_6:         code = SDL_SCANCODE_RIGHT; break;
        case SDL_SCANCODE_KP_1:         code = SDL_SCANCODE_END; break;
        case SDL_SCANCODE_KP_2:         code = SDL_SCANCODE_DOWN; break;
        case SDL_SCANCODE_KP_3:         code = SDL_SCANCODE_PAGEDOWN; break;
        case SDL_SCANCODE_KP_0:         code = SDL_SCANCODE_INSERT; break;
        case SDL_SCANCODE_KP_PERIOD:    code = SDL_SCANCODE_DELETE; break;
        default:                        break;
        }
    }
    switch (code) {
    case SDL_SCANCODE_HOME:         return kHomeChar;
    case SDL_SCANCODE_UP:           return kUpArrowChar;
    case SDL_SCANCODE_PAGEUP:       return kPageUpChar;
    case SDL_SCANCODE_LEFT:         return kLeftArrowChar;
    case SDL_SCANCODE_RIGHT:        return kRightArrowChar;
    case SDL_SCANCODE_END:          return kEndChar;
    case SDL_SCANCODE_DOWN:         return kDownArrowChar;
    case SDL_SCANCODE_PAGEDOWN:     return kPageDownChar;
    case SDL_SCANCODE_INSERT:       return kInsertChar;
    case SDL_SCANCODE_DELETE:       return kDeleteChar;
    case SDL_SCANCODE_PAUSE:        return kPauseChar;
    case SDL_SCANCODE_SCROLLLOCK:   return kScrollLockChar;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:         return 0;
    default:                        return -1;
    }
}

/* The characters a key press types that SDL's text input leaves out: it reports printable text only. */
static int ControlChar(SDL_Scancode code, SDL_Keymod mod)
{
    SDL_Keycode key;

    switch (code) {
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:     return kReturnChar;
    case SDL_SCANCODE_BACKSPACE:    return kBackspaceChar;
    case SDL_SCANCODE_TAB:          return kTabChar;
    case SDL_SCANCODE_ESCAPE:       return kEscapeChar;
    default:                        break;
    }
    if ((mod & SDL_KMOD_CTRL) != 0 && (mod & SDL_KMOD_ALT) == 0) {
        key = SDL_GetKeyFromScancode(code, SDL_KMOD_NONE, false);
        if (key >= 'a' && key <= 'z')
            return (int)(key - 'a' + 1);
    }
#ifdef SDL_PLATFORM_WIN32
    /* Alt+key: Windows types the key's character (WM_SYSCHAR), which SDL does not pass on. Left Alt
       only: the right one is AltGr on many layouts, and what that types does arrive as text. */
    if ((mod & SDL_KMOD_LALT) != 0 && (mod & (SDL_KMOD_RALT | SDL_KMOD_CTRL | SDL_KMOD_GUI)) == 0) {
        key = SDL_GetKeyFromScancode(code, (SDL_Keymod)(mod & (SDL_KMOD_SHIFT | SDL_KMOD_CAPS)), false);
        if (key >= 0x20 && key < 0x7f)
            return (int)key;
    }
#endif
    return 0;
}

/* code page 1252's 0x80..0x9f, the characters that are not their own Unicode code point */
static const Word16 Ansi80[32] = {
    0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
    0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178
};

/* A typed character as the byte the game's Win32 window received in its WM_CHAR: the Western ANSI code
   page, 1252. 0 for a character it does not have. */
static Word AnsiChar(Uint32 ch)
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

    if (BurgerKeyStart == BurgerKeyEnd) {
        while (KeyboardKbhit() == 0)
            ;
    }
    i = BurgerKeyStart;
    BurgerKeyStart = (i + 1) & kKeyQueueMask;
    return BurgerKeyBuffer[i];
}

/* KeyboardKbhit (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d72a0
unsigned int KeyboardKbhit(void)
{
    int32_t i;

    ProcessSystemEvents();
    for (i = BurgerKeyStart; i != BurgerKeyEnd; i = (i + 1) & kKeyQueueMask) {
        if (BurgerKeyBuffer[i] != 0)
            return BurgerKeyBuffer[i];
    }
    return 0;
}

/* Pop the next key character, 0 included; false when the queue is empty. */
/* KeyboardGet2 (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d72f0
bool KeyboardGet2(int *ch)
{
    int32_t i;

    ProcessSystemEvents();
    i = BurgerKeyStart;
    if (i != BurgerKeyEnd) {
        BurgerKeyStart = (i + 1) & kKeyQueueMask;
        *ch = BurgerKeyBuffer[i];
        return true;
    }
    return false;
}

/* KeyIsDown (BurgerLib KeyboardIsPressed). */
// FUNCTION: EVNOVA 0x004f1900
unsigned int KeyIsDown(ScanCode scanCode)
{
    if ((unsigned)scanCode >= kScanCount)
        return 0;
    return PlatKeyIsDown((int)scanCode);
}

/* KeyboardGet (BurgerLib InKeyboard). */
// FUNCTION: EVNOVA 0x004f1920
int KeyboardGet(void)
{
    if (KeyboardKbhit() == 0)
        return 0;
    return KeyboardGetch();
}

/* Drain whatever is queued and forget every click. */
/* KeyboardFlush (BurgerLib KeyboardFlush). */
// FUNCTION: EVNOVA 0x004f1940
void KeyboardFlush(void)
{
    // BUG (all versions): KeyboardKbhit skips entries without a character, so the drain stops before a
    // trailing run of them; they stay queued and the next dialog gets them as key presses.
    while (KeyboardKbhit() != 0)
        KeyboardGetch();
    MouseClicked = 0;
}

/* MouseReadButtons (BurgerLib InMouseWin95). */
// FUNCTION: EVNOVA 0x004d7b10
int MouseReadButtons(void)
{
    return Win95MouseButton;
}

/* Both axes are clamped to [0, extent-1]; a negative reads as 0. */
/* MouseReadPosition (BurgerLib MouseReadAbs). */
// FUNCTION: EVNOVA 0x004d7b20
void MouseReadPosition(int32_t *h, int32_t *v)
{
    int32_t p;

    if (h != 0) {
        p = Win95MouseX;
        if (p < 0)
            p = 0;
        if (p >= Screen::Get().width)
            p = Screen::Get().width - 1;
        *h = p;
    }
    if (v != 0) {
        p = Win95MouseY;
        if (p < 0)
            p = 0;
        if (p >= Screen::Get().height)
            p = Screen::Get().height - 1;
        *v = p;
    }
}

/* Take one wheel notch: 1 up, -1 down, 0 none. */
int MouseTakeWheel(void)
{
    if (WheelNotches > 0) {
        WheelNotches--;
        return 1;
    }
    if (WheelNotches < 0) {
        WheelNotches++;
        return -1;
    }
    return 0;
}

/* Forget the wheel notches not taken. */
void MouseFlushWheel(void)
{
    WheelNotches = 0;
}

/* Win95ProcessMessage (BurgerLib InKeyboardWin95, 0x004d7330) and the window procedure's part
   (WinGameWndProc, port/dialogmgr.cpp), on an SDL event in place of a Win32 message. */
static void Win95ProcessEvent(const SDL_Event *e)
{
    const char *text;
    Uint32 ch;
    int key;

    switch (e->type) {
    case SDL_EVENT_KEY_DOWN:
        key = SpecialKey(e->key.scancode, e->key.mod);
        if (key >= 0)
            BurgerPostKey(key);
        key = AnsiChar(ControlChar(e->key.scancode, e->key.mod));
        if (key != 0)
            BurgerPostKey(key);
        break;
    case SDL_EVENT_TEXT_INPUT:
        text = e->text.text;
        while ((ch = SDL_StepUTF8(&text, NULL)) != 0) {
            key = AnsiChar(ch);
            if (key != 0)
                BurgerPostKey(key);
        }
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        Win95MouseButton |= SDL_BUTTON_MASK(e->button.button) & kMouseButtons;
        MouseClicked |= SDL_BUTTON_MASK(e->button.button) & kMouseButtons;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        Win95MouseButton &= ~SDL_BUTTON_MASK(e->button.button);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        Win95MouseX = (int32_t)SDL_floorf(e->motion.x);
        Win95MouseY = (int32_t)SDL_floorf(e->motion.y);
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        WinGameActivate(1);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        WinGameActivate(0);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        WinGameMinimized();
        break;
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        WinGameWindowMode(0);
        break;
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        WinGameWindowMode(1);
        break;
    case SDL_EVENT_QUIT:
        WinGameClose();
        return;
    case SDL_EVENT_RENDER_TARGETS_RESET:
    case SDL_EVENT_RENDER_DEVICE_RESET:
    case SDL_EVENT_RENDER_DEVICE_LOST:
        PlatDebugLog(e->type == SDL_EVENT_RENDER_TARGETS_RESET ? "render targets lost"
                     : e->type == SDL_EVENT_RENDER_DEVICE_RESET ? "render device reset" : "render device lost");
        Fatal("The graphics device was reset (SDL's %s renderer).", PlatRendererName());
        return;
    case SDL_EVENT_MOUSE_WHEEL:
        /* integer_y: SDL's whole notches, trackpad fractions summed, in the system's scroll direction; a few
           kept at most, so a flicked wheel does not keep scrolling after it stops */
        WheelNotches += e->wheel.integer_y;
        if (WheelNotches > kWheelBacklog)
            WheelNotches = kWheelBacklog;
        if (WheelNotches < -kWheelBacklog)
            WheelNotches = -kWheelBacklog;
        break;
    case SDL_EVENT_KEY_UP:
        break;
    default:
        return;
    }
    WinGameInputSeen();
}

/* ProcessSystemEvents (BurgerLib InKeyboardWin95). */
// FUNCTION: EVNOVA 0x004d79a0
void ProcessSystemEvents(void)
{
    SDL_Event e;

    if (Recurse == 0) {
        Recurse = 1;
        while (PlatPollEvent(&e) != 0)
            Win95ProcessEvent(&e);
        Recurse = 0;
    }
}

/* Reset the input subsystem: empty the key queue, forget every click. */
/* KeyboardInit (BurgerLib KeyboardInit). */
// FUNCTION: EVNOVA 0x004d7a80
void KeyboardInit(void)
{
    BurgerKeyStart = 0;
    BurgerKeyEnd = 0;
    MouseClicked = 0;
}
