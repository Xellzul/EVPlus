/* platform_sdl.c -- platform.h on SDL3, the same on every platform: start-up, the window and its frame, the
   events, the cursor. Built with SDL's enum size (-fno-short-enums, CMakeLists.txt) and without any game
   header. */
#define SDL_MAIN_HANDLED                /* the game has its own entry point (port/winmain.c) */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "platform.h"

#define kScanCount      256             /* SDL's keyboard scan codes all lie below this */
#define kExtended       0x80            /* PlatEvent.scan: an E0-prefixed key */
#define kQueueSize      64              /* events waiting, a power of 2 */
#define kWheelNotch     120
#define kBytesPerPixel  2
#define kDefaultRefresh 60              /* Hz, when the display does not tell */
/* a window that is not full screen opens at the frame's size, or at this share of the display when the frame is larger */
#define kWindowShareNum 9
#define kWindowShareDen 10

static SDL_Window *gWindow;
static SDL_Renderer *gRenderer;
static SDL_Texture *gTexture;
static int gFrameWidth;
static int gFrameHeight;
static unsigned char *gFrame;           /* the game's latest frame, gFrameWidth * kBytesPerPixel bytes a row */
static bool gFramePresented;            /* the game has presented a frame */
static bool gFrameFresh;                /* ... that the texture does not have yet */
static bool gFrameDrawn;                /* the texture has a frame to show */
static Uint64 gShowInterval;            /* ns between the starts of two shows */
static Uint64 gShowStart;               /* the last show: when it began, */
static Uint64 gShowEnd;                 /* when it was done */

static PlatEvent gQueue[kQueueSize];
static unsigned gQueueHead;
static unsigned gQueueTail;
static int gLastScan;                   /* the last key pressed: the key a typed character came from */
static int gButtons;                    /* PLAT_BUTTON_* held */
static SDL_Scancode gSwallowedKey;      /* its release is not reported: the Enter of Alt+Enter */
static unsigned char gToggled[kScanCount];

/* SDL scan code (USB usage) to PC set 1 scan code, | kExtended for the E0 keys. Pause and Num Lock as
   Windows reports them: Pause 45, Num Lock E0 45. */
static const unsigned char kPCScan[kScanCount] = {
    [SDL_SCANCODE_A] = 0x1e, [SDL_SCANCODE_B] = 0x30, [SDL_SCANCODE_C] = 0x2e, [SDL_SCANCODE_D] = 0x20,
    [SDL_SCANCODE_E] = 0x12, [SDL_SCANCODE_F] = 0x21, [SDL_SCANCODE_G] = 0x22, [SDL_SCANCODE_H] = 0x23,
    [SDL_SCANCODE_I] = 0x17, [SDL_SCANCODE_J] = 0x24, [SDL_SCANCODE_K] = 0x25, [SDL_SCANCODE_L] = 0x26,
    [SDL_SCANCODE_M] = 0x32, [SDL_SCANCODE_N] = 0x31, [SDL_SCANCODE_O] = 0x18, [SDL_SCANCODE_P] = 0x19,
    [SDL_SCANCODE_Q] = 0x10, [SDL_SCANCODE_R] = 0x13, [SDL_SCANCODE_S] = 0x1f, [SDL_SCANCODE_T] = 0x14,
    [SDL_SCANCODE_U] = 0x16, [SDL_SCANCODE_V] = 0x2f, [SDL_SCANCODE_W] = 0x11, [SDL_SCANCODE_X] = 0x2d,
    [SDL_SCANCODE_Y] = 0x15, [SDL_SCANCODE_Z] = 0x2c,
    [SDL_SCANCODE_1] = 0x02, [SDL_SCANCODE_2] = 0x03, [SDL_SCANCODE_3] = 0x04, [SDL_SCANCODE_4] = 0x05,
    [SDL_SCANCODE_5] = 0x06, [SDL_SCANCODE_6] = 0x07, [SDL_SCANCODE_7] = 0x08, [SDL_SCANCODE_8] = 0x09,
    [SDL_SCANCODE_9] = 0x0a, [SDL_SCANCODE_0] = 0x0b,
    [SDL_SCANCODE_RETURN] = 0x1c, [SDL_SCANCODE_ESCAPE] = 0x01, [SDL_SCANCODE_BACKSPACE] = 0x0e,
    [SDL_SCANCODE_TAB] = 0x0f, [SDL_SCANCODE_SPACE] = 0x39, [SDL_SCANCODE_MINUS] = 0x0c,
    [SDL_SCANCODE_EQUALS] = 0x0d, [SDL_SCANCODE_LEFTBRACKET] = 0x1a, [SDL_SCANCODE_RIGHTBRACKET] = 0x1b,
    [SDL_SCANCODE_BACKSLASH] = 0x2b, [SDL_SCANCODE_NONUSHASH] = 0x2b, [SDL_SCANCODE_SEMICOLON] = 0x27,
    [SDL_SCANCODE_APOSTROPHE] = 0x28, [SDL_SCANCODE_GRAVE] = 0x29, [SDL_SCANCODE_COMMA] = 0x33,
    [SDL_SCANCODE_PERIOD] = 0x34, [SDL_SCANCODE_SLASH] = 0x35, [SDL_SCANCODE_CAPSLOCK] = 0x3a,
    [SDL_SCANCODE_F1] = 0x3b, [SDL_SCANCODE_F2] = 0x3c, [SDL_SCANCODE_F3] = 0x3d, [SDL_SCANCODE_F4] = 0x3e,
    [SDL_SCANCODE_F5] = 0x3f, [SDL_SCANCODE_F6] = 0x40, [SDL_SCANCODE_F7] = 0x41, [SDL_SCANCODE_F8] = 0x42,
    [SDL_SCANCODE_F9] = 0x43, [SDL_SCANCODE_F10] = 0x44, [SDL_SCANCODE_F11] = 0x57, [SDL_SCANCODE_F12] = 0x58,
    [SDL_SCANCODE_PRINTSCREEN] = 0x37 | kExtended, [SDL_SCANCODE_SCROLLLOCK] = 0x46,
    [SDL_SCANCODE_PAUSE] = 0x45,
    [SDL_SCANCODE_INSERT] = 0x52 | kExtended, [SDL_SCANCODE_HOME] = 0x47 | kExtended,
    [SDL_SCANCODE_PAGEUP] = 0x49 | kExtended, [SDL_SCANCODE_DELETE] = 0x53 | kExtended,
    [SDL_SCANCODE_END] = 0x4f | kExtended, [SDL_SCANCODE_PAGEDOWN] = 0x51 | kExtended,
    [SDL_SCANCODE_RIGHT] = 0x4d | kExtended, [SDL_SCANCODE_LEFT] = 0x4b | kExtended,
    [SDL_SCANCODE_DOWN] = 0x50 | kExtended, [SDL_SCANCODE_UP] = 0x48 | kExtended,
    [SDL_SCANCODE_NUMLOCKCLEAR] = 0x45 | kExtended, [SDL_SCANCODE_KP_DIVIDE] = 0x35 | kExtended,
    [SDL_SCANCODE_KP_MULTIPLY] = 0x37, [SDL_SCANCODE_KP_MINUS] = 0x4a, [SDL_SCANCODE_KP_PLUS] = 0x4e,
    [SDL_SCANCODE_KP_ENTER] = 0x1c | kExtended,
    [SDL_SCANCODE_KP_1] = 0x4f, [SDL_SCANCODE_KP_2] = 0x50, [SDL_SCANCODE_KP_3] = 0x51,
    [SDL_SCANCODE_KP_4] = 0x4b, [SDL_SCANCODE_KP_5] = 0x4c, [SDL_SCANCODE_KP_6] = 0x4d,
    [SDL_SCANCODE_KP_7] = 0x47, [SDL_SCANCODE_KP_8] = 0x48, [SDL_SCANCODE_KP_9] = 0x49,
    [SDL_SCANCODE_KP_0] = 0x52, [SDL_SCANCODE_KP_PERIOD] = 0x53,
    [SDL_SCANCODE_NONUSBACKSLASH] = 0x56, [SDL_SCANCODE_APPLICATION] = 0x5d | kExtended,
    [SDL_SCANCODE_LCTRL] = 0x1d, [SDL_SCANCODE_LSHIFT] = 0x2a, [SDL_SCANCODE_LALT] = 0x38,
    [SDL_SCANCODE_LGUI] = 0x5b | kExtended, [SDL_SCANCODE_RCTRL] = 0x1d | kExtended,
    [SDL_SCANCODE_RSHIFT] = 0x36, [SDL_SCANCODE_RALT] = 0x38 | kExtended,
    [SDL_SCANCODE_RGUI] = 0x5c | kExtended
};

/* -- start-up ------------------------------------------------------------------------------------- */

int PlatInit(void)
{
    SDL_SetMainReady();
    /* the click that brings the window to the front is a click in the game too, as a Win32 window gets it */
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return -1;
    return 0;
}

void PlatQuit(void)
{
    PlatVideoClose();
    if (gRenderer != NULL)
        SDL_DestroyRenderer(gRenderer);
    if (gWindow != NULL)
        SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;
    SDL_Quit();
}

/* -- video ---------------------------------------------------------------------------------------- */

/* the frame as it was last shown, stretched into the window as it is now */
static void Redraw(void)
{
    if (gRenderer == NULL)
        return;
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);
    if (gTexture != NULL && gFrameDrawn)
        SDL_RenderTexture(gRenderer, gTexture, NULL, NULL);
    SDL_RenderPresent(gRenderer);
}

/* ns until the fresh frame may be shown; 0 = now. Shows start no closer than 3/4 of a display refresh, and
   after a show the game gets at least as long as the show took. */
static Uint64 FrameDueIn(void)
{
    Uint64 now;
    Uint64 due;
    Uint64 rest;

    if (!gFrameDrawn)
        return 0;
    now = SDL_GetTicksNS();
    due = gShowStart + gShowInterval;
    rest = gShowEnd + (gShowEnd - gShowStart);
    if (rest > due)
        due = rest;
    return due > now ? due - now : 0;
}

/* The game's frame onto the screen, if it is fresh and due. The game presents far more often than a display
   can show -- at start-up once per step of its progress bar, thousands of times -- and an upload and present
   takes milliseconds (CE had cnc-ddraw show the page from a thread of its own, so a flip cost the game
   nothing). A frame presented before the last one has had its time waits here: the next present or event
   poll shows it once it is due, unless a newer frame has replaced it by then. */
static void ShowFrame(void)
{
    if (!gFrameFresh || gTexture == NULL || FrameDueIn() != 0)
        return;
    gShowStart = SDL_GetTicksNS();
    SDL_UpdateTexture(gTexture, NULL, gFrame, gFrameWidth * kBytesPerPixel);
    gFrameFresh = false;
    gFrameDrawn = true;
    Redraw();
    gShowEnd = SDL_GetTicksNS();
}

/* While the user drags the window's edge the event loop does not run: SDL sends these exposes here, on
   the main thread, for the window to be redrawn at once. */
static bool SDLCALL ExposeWatch(void *userdata, SDL_Event *event)
{
    (void)userdata;
    if (event->type == SDL_EVENT_WINDOW_EXPOSED && event->window.data1 == 1)
        Redraw();
    return true;
}

static bool MakeFrameTexture(void)
{
    if (gTexture != NULL)
        SDL_DestroyTexture(gTexture);
    gFrameDrawn = false;
    gFrameFresh = gFramePresented;          /* a frame the game gave before goes into the new texture */
    gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_XRGB1555, SDL_TEXTUREACCESS_STREAMING,
                                 gFrameWidth, gFrameHeight);
    if (gTexture == NULL)
        return false;
    SDL_SetTextureScaleMode(gTexture, SDL_SCALEMODE_LINEAR);
    return true;
}

int PlatVideoCreateWindow(const char *title)
{
    gWindow = SDL_CreateWindow(title, 640, 480,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (gWindow == NULL)
        return -1;
    SDL_AddEventWatch(ExposeWatch, NULL);
    return 0;
}

int PlatVideoOpen(int width, int height, int fullScreen)
{
    const SDL_DisplayMode *mode;
    SDL_DisplayID display;
    SDL_Rect usable;
    float density;
    float refresh;
    int w;
    int h;

    if (gWindow == NULL || width <= 0 || height <= 0)
        return -1;
    PlatVideoClose();
    if (gRenderer == NULL) {
        gRenderer = SDL_CreateRenderer(gWindow, NULL);
        if (gRenderer == NULL)
            return -1;
        SDL_SetRenderVSync(gRenderer, 0);       /* the game paces itself */
    }
    gFrameWidth = width;
    gFrameHeight = height;
    gFrame = (unsigned char *)SDL_calloc((size_t)width * height, kBytesPerPixel);
    if (gFrame == NULL || !MakeFrameTexture())
        return -1;
    SDL_SetRenderLogicalPresentation(gRenderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    /* the window's size when it is not full screen, in the window system's units */
    density = SDL_GetWindowPixelDensity(gWindow);
    if (density <= 0)
        density = 1;
    w = (int)(width / density);
    h = (int)(height / density);
    display = SDL_GetDisplayForWindow(gWindow);
    if (SDL_GetDisplayUsableBounds(display, &usable)) {
        usable.w = usable.w * kWindowShareNum / kWindowShareDen;
        usable.h = usable.h * kWindowShareNum / kWindowShareDen;
        if (w > usable.w || h > usable.h) {
            if ((int64_t)w * usable.h > (int64_t)h * usable.w) {
                h = (int)((int64_t)usable.w * h / w);
                w = usable.w;
            } else {
                w = (int)((int64_t)usable.h * w / h);
                h = usable.h;
            }
        }
    }
    SDL_SetWindowSize(gWindow, w, h);
    SDL_SetWindowPosition(gWindow, SDL_WINDOWPOS_CENTERED_DISPLAY(display), SDL_WINDOWPOS_CENTERED_DISPLAY(display));
    SDL_SetWindowFullscreen(gWindow, fullScreen != 0);     /* no mode chosen: the desktop's, borderless */
    SDL_ShowWindow(gWindow);
    SDL_SyncWindow(gWindow);
    SDL_StartTextInput(gWindow);                           /* typed characters arrive as events */
    mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(gWindow));
    refresh = mode != NULL ? mode->refresh_rate : 0;
    if (refresh < 1)
        refresh = kDefaultRefresh;
    gShowInterval = (Uint64)(SDL_NS_PER_SECOND / refresh) * 3 / 4;
    Redraw();
    return 0;
}

void PlatVideoClose(void)
{
    if (gTexture != NULL)
        SDL_DestroyTexture(gTexture);
    gTexture = NULL;
    SDL_free(gFrame);
    gFrame = NULL;
    gFramePresented = false;
    gFrameFresh = false;
    gFrameDrawn = false;
}

void *PlatVideoLock(int *pitch)
{
    *pitch = gFrameWidth * kBytesPerPixel;
    return gFrame;
}

void PlatVideoPresent(void)
{
    if (gFrame == NULL)
        return;
    gFramePresented = true;
    gFrameFresh = true;
    ShowFrame();
}

void PlatVideoSetFullScreen(int on)
{
    if (gWindow != NULL)
        SDL_SetWindowFullscreen(gWindow, on != 0);
}

int PlatVideoIsFullScreen(void)
{
    return gWindow != NULL && (SDL_GetWindowFlags(gWindow) & SDL_WINDOW_FULLSCREEN) != 0;
}

void PlatDesktopSize(int *width, int *height)
{
    const SDL_DisplayMode *mode;

    mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (mode != NULL) {
        *width = (int)(mode->w * mode->pixel_density + 0.5f);
        *height = (int)(mode->h * mode->pixel_density + 0.5f);
    } else {
        *width = 640;
        *height = 480;
    }
}

void *PlatNativeWindow(void)
{
#ifdef SDL_PLATFORM_WIN32
    if (gWindow != NULL)
        return SDL_GetPointerProperty(SDL_GetWindowProperties(gWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    return NULL;
}

/* -- events --------------------------------------------------------------------------------------- */

static void Push(const PlatEvent *ev)
{
    unsigned next;

    next = (gQueueTail + 1) & (kQueueSize - 1);
    if (next == gQueueHead)
        return;                                 /* full: the event is lost */
    gQueue[gQueueTail] = *ev;
    gQueueTail = next;
}

static void PushType(int type, int on)
{
    PlatEvent ev;

    SDL_zero(ev);
    ev.type = type;
    ev.on = on;
    Push(&ev);
}

static int ScanOf(SDL_Scancode code)
{
    return (unsigned)code < kScanCount ? kPCScan[code] : 0;
}

static int SpecialKeyOf(SDL_Scancode code, SDL_Keymod mod)
{
    int numLock;

    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F12)
        return PLAT_KEY_F1 + (code - SDL_SCANCODE_F1);
    numLock = (mod & SDL_KMOD_NUM) != 0;
    switch (code) {
    case SDL_SCANCODE_HOME:         return PLAT_KEY_HOME;
    case SDL_SCANCODE_UP:           return PLAT_KEY_UP;
    case SDL_SCANCODE_PAGEUP:       return PLAT_KEY_PAGEUP;
    case SDL_SCANCODE_LEFT:         return PLAT_KEY_LEFT;
    case SDL_SCANCODE_RIGHT:        return PLAT_KEY_RIGHT;
    case SDL_SCANCODE_END:          return PLAT_KEY_END;
    case SDL_SCANCODE_DOWN:         return PLAT_KEY_DOWN;
    case SDL_SCANCODE_PAGEDOWN:     return PLAT_KEY_PAGEDOWN;
    case SDL_SCANCODE_INSERT:       return PLAT_KEY_INSERT;
    case SDL_SCANCODE_DELETE:       return PLAT_KEY_DELETE;
    case SDL_SCANCODE_KP_7:         return numLock ? 0 : PLAT_KEY_HOME;
    case SDL_SCANCODE_KP_8:         return numLock ? 0 : PLAT_KEY_UP;
    case SDL_SCANCODE_KP_9:         return numLock ? 0 : PLAT_KEY_PAGEUP;
    case SDL_SCANCODE_KP_4:         return numLock ? 0 : PLAT_KEY_LEFT;
    case SDL_SCANCODE_KP_6:         return numLock ? 0 : PLAT_KEY_RIGHT;
    case SDL_SCANCODE_KP_1:         return numLock ? 0 : PLAT_KEY_END;
    case SDL_SCANCODE_KP_2:         return numLock ? 0 : PLAT_KEY_DOWN;
    case SDL_SCANCODE_KP_3:         return numLock ? 0 : PLAT_KEY_PAGEDOWN;
    case SDL_SCANCODE_KP_0:         return numLock ? 0 : PLAT_KEY_INSERT;
    case SDL_SCANCODE_KP_PERIOD:    return numLock ? 0 : PLAT_KEY_DELETE;
    case SDL_SCANCODE_PAUSE:        return PLAT_KEY_PAUSE;
    case SDL_SCANCODE_SCROLLLOCK:   return PLAT_KEY_SCROLLLOCK;
    case SDL_SCANCODE_LCTRL:        return PLAT_KEY_LCTRL;
    case SDL_SCANCODE_RCTRL:        return PLAT_KEY_RCTRL;
    case SDL_SCANCODE_LSHIFT:       return PLAT_KEY_LSHIFT;
    case SDL_SCANCODE_RSHIFT:       return PLAT_KEY_RSHIFT;
    case SDL_SCANCODE_LALT:         return PLAT_KEY_LALT;
    case SDL_SCANCODE_RALT:         return PLAT_KEY_RALT;
    default:                        return 0;
    }
}

/* The characters a key press types that SDL's text input leaves out: it reports printable text only. */
static int ControlCharOf(SDL_Scancode code, SDL_Keymod mod)
{
    SDL_Keycode key;

    switch (code) {
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:     return '\r';
    case SDL_SCANCODE_BACKSPACE:    return '\b';
    case SDL_SCANCODE_TAB:          return '\t';
    case SDL_SCANCODE_ESCAPE:       return 0x1b;
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

static int ButtonOf(int sdlButton)
{
    switch (sdlButton) {
    case SDL_BUTTON_LEFT:   return PLAT_BUTTON_LEFT;
    case SDL_BUTTON_RIGHT:  return PLAT_BUTTON_RIGHT;
    case SDL_BUTTON_MIDDLE: return PLAT_BUTTON_MIDDLE;
    default:                return 0;
    }
}

/* one SDL event as the platform events it stands for (none, one or several) */
static void Translate(SDL_Event *e)
{
    PlatEvent ev;
    SDL_Scancode code;
    SDL_Keymod mod;
    const char *text;
    Uint32 ch;

    SDL_zero(ev);
    switch (e->type) {
    case SDL_EVENT_KEY_DOWN:
        code = e->key.scancode;
        mod = e->key.mod;
        if ((code == SDL_SCANCODE_RETURN || code == SDL_SCANCODE_KP_ENTER) && (mod & SDL_KMOD_ALT) != 0
            && (mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) == 0) {
            if (!e->key.repeat)
                PlatVideoSetFullScreen(!PlatVideoIsFullScreen());
            gSwallowedKey = code;
            break;
        }
        if (!e->key.repeat && (unsigned)code < kScanCount)
            gToggled[code] ^= 1;
        ev.type = PLAT_EV_KEY_DOWN;
        ev.scan = ScanOf(code);
        ev.key = SpecialKeyOf(code, mod);
        ev.repeat = e->key.repeat;
        Push(&ev);
        gLastScan = ev.scan;
        ev.ch = ControlCharOf(code, mod);
        if (ev.ch != 0) {
            ev.type = PLAT_EV_CHAR;
            ev.key = 0;
            ev.repeat = 0;
            Push(&ev);
        }
        break;
    case SDL_EVENT_KEY_UP:
        code = e->key.scancode;
        if (code == gSwallowedKey) {
            gSwallowedKey = SDL_SCANCODE_UNKNOWN;
            break;
        }
        ev.type = PLAT_EV_KEY_UP;
        ev.scan = ScanOf(code);
        ev.key = SpecialKeyOf(code, e->key.mod);
        Push(&ev);
        break;
    case SDL_EVENT_TEXT_INPUT:
        text = e->text.text;
        ev.type = PLAT_EV_CHAR;
        ev.scan = gLastScan;
        while ((ch = SDL_StepUTF8(&text, NULL)) != 0) {
            ev.ch = (int)ch;
            Push(&ev);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (gRenderer == NULL)
            break;
        SDL_ConvertEventToRenderCoordinates(gRenderer, e);
        ev.type = PLAT_EV_MOUSE_MOVE;
        ev.x = (int)SDL_floorf(e->motion.x);
        ev.y = (int)SDL_floorf(e->motion.y);
        Push(&ev);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        ev.button = ButtonOf(e->button.button);
        if (ev.button == 0)
            break;
        if (e->button.down) {
            ev.type = PLAT_EV_MOUSE_DOWN;
            gButtons |= ev.button;
        } else {
            ev.type = PLAT_EV_MOUSE_UP;
            gButtons &= ~ev.button;
        }
        SDL_CaptureMouse(gButtons != 0);        /* a drag goes on outside the window */
        Push(&ev);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        ev.type = PLAT_EV_MOUSE_WHEEL;
        ev.wheel = (int)(e->wheel.y * kWheelNotch);
        if (e->wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            ev.wheel = -ev.wheel;
        Push(&ev);
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        PushType(PLAT_EV_ACTIVATE, 1);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        PushType(PLAT_EV_ACTIVATE, 0);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        PushType(PLAT_EV_MINIMIZED, 0);
        break;
    case SDL_EVENT_QUIT:
        PushType(PLAT_EV_CLOSE, 0);
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
        Redraw();
        break;
    case SDL_EVENT_RENDER_DEVICE_RESET:
        /* the textures are gone */
        if (gRenderer != NULL && gFrame != NULL)
            MakeFrameTexture();
        break;
    default:
        break;
    }
}

int PlatPollEvent(PlatEvent *ev)
{
    SDL_Event e;

    ShowFrame();
    while (gQueueHead == gQueueTail) {
        if (!SDL_PollEvent(&e))
            return 0;
        Translate(&e);
    }
    *ev = gQueue[gQueueHead];
    gQueueHead = (gQueueHead + 1) & (kQueueSize - 1);
    return 1;
}

void PlatWaitEvent(int ms)
{
    Uint64 due;

    if (gQueueHead != gQueueTail)
        return;
    ShowFrame();
    if (gFrameFresh) {
        /* a frame is waiting for its time: that ends the wait too */
        due = FrameDueIn() / SDL_NS_PER_MS + 1;
        if (due < (Uint64)ms) {
            SDL_WaitEventTimeout(NULL, (Sint32)due);
            ShowFrame();
            return;
        }
    }
    SDL_WaitEventTimeout(NULL, ms);
}

int PlatKeyToggled(int winKey)
{
    SDL_Keymod mod;
    SDL_Scancode code;

    mod = SDL_GetModState();
    switch (winKey) {
    case 0x14:  return (mod & SDL_KMOD_CAPS) != 0;          /* VK_CAPITAL */
    case 0x90:  return (mod & SDL_KMOD_NUM) != 0;           /* VK_NUMLOCK */
    case 0x91:  return (mod & SDL_KMOD_SCROLL) != 0;        /* VK_SCROLL */
    }
    if (winKey >= '0' && winKey <= '9')
        code = SDL_GetScancodeFromKey((SDL_Keycode)winKey, NULL);
    else if (winKey >= 'A' && winKey <= 'Z')
        code = SDL_GetScancodeFromKey((SDL_Keycode)(winKey - 'A' + 'a'), NULL);
    else if (winKey >= 0x70 && winKey <= 0x7b)              /* VK_F1..VK_F12 */
        code = (SDL_Scancode)(SDL_SCANCODE_F1 + (winKey - 0x70));
    else
        return 0;
    return (unsigned)code < kScanCount ? gToggled[code] : 0;
}

/* -- cursor --------------------------------------------------------------------------------------- */

void PlatCursorShow(int show)
{
    if (show)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

void PlatCursorArrow(void)
{
    SDL_SetCursor(SDL_GetDefaultCursor());
}
