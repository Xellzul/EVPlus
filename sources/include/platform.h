/* platform.h -- the operating-system services the game runs on. Only plain C types cross this header: the
   files behind it see their library's own headers and are built with that library's enum size, the game with
   -fshort-enums.
   On SDL (platform/platform_sdl.c, every platform): start-up, the window and its frame, the events, the cursor,
   the audio outputs, the clocks, the debug log. Still Windows (platform/platform_win.c): path conversion;
   another platform supplies its own. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- start-up ------------------------------------------------------------------------------------- */
extern int PlatInit(void);                                 /* 0 when the layer is up */
extern void PlatQuit(void);

/* -- video: one window showing the game's frame, 16-bit x1r5g5b5 pixels in host byte order ---------- */
/* The frame is stretched to the window with its aspect kept (black bars), linear filter. Alt+Enter
   switches between a borderless full screen and a resizable window; the layer handles it itself. */
extern int PlatVideoCreateWindow(const char *title);      /* hidden until PlatVideoOpen; 0 = made */
extern int PlatVideoOpen(int width, int height, int fullScreen);   /* the frame's size; 0 = open */
extern void PlatVideoClose(void);                          /* drops the frame, keeps the window */
/* A complete frame: the size given to PlatVideoOpen, pitch bytes a row. It is shown at once, or -- when the
   last one was shown too short a time ago for a display to tell them apart -- copied, and shown by the next
   present or event poll after that time; a newer frame replaces one still waiting. The pixels must stay
   readable until the next PlatVideoPresent or PlatVideoClose. */
extern void PlatVideoPresent(const void *pixels, int pitch);
extern void PlatVideoSetFullScreen(int on);
extern int PlatVideoIsFullScreen(void);
extern void PlatDesktopSize(int *width, int *height);      /* the main display, in pixels */
/* the window as the OS knows it (Windows: the HWND). Only for the GDI text and the Win32 dialogs,
   until those are replaced. */
extern void *PlatNativeWindow(void);

/* -- events --------------------------------------------------------------------------------------- */
#define PLAT_EV_KEY_DOWN    1       /* scan, key, repeat */
#define PLAT_EV_KEY_UP      2       /* scan, key */
#define PLAT_EV_CHAR        3       /* ch, scan (the key that typed it) */
#define PLAT_EV_MOUSE_MOVE  4       /* x, y */
#define PLAT_EV_MOUSE_DOWN  5       /* button (where: the last PLAT_EV_MOUSE_MOVE) */
#define PLAT_EV_MOUSE_UP    6       /* button */
#define PLAT_EV_MOUSE_WHEEL 7       /* wheel */
/* on: the window gained (1) or lost (0) the keyboard. On Windows a loss to another window of this program
   (a Win32 dialog the game opened) is not reported, nor the gain after it. */
#define PLAT_EV_ACTIVATE    8
#define PLAT_EV_MINIMIZED   9
#define PLAT_EV_CLOSE       10      /* the user asked to close the window or quit */

#define PLAT_BUTTON_LEFT    1
#define PLAT_BUTTON_RIGHT   2
#define PLAT_BUTTON_MIDDLE  4

/* the keys with a meaning beyond their position (PlatEvent.key); 0 for every other key */
#define PLAT_KEY_HOME       1
#define PLAT_KEY_UP         2
#define PLAT_KEY_PAGEUP     3
#define PLAT_KEY_LEFT       4
#define PLAT_KEY_RIGHT      5
#define PLAT_KEY_END        6
#define PLAT_KEY_DOWN       7
#define PLAT_KEY_PAGEDOWN   8
#define PLAT_KEY_INSERT     9
#define PLAT_KEY_DELETE     10
#define PLAT_KEY_PAUSE      11
#define PLAT_KEY_SCROLLLOCK 12
#define PLAT_KEY_LCTRL      13
#define PLAT_KEY_RCTRL      14
#define PLAT_KEY_LSHIFT     15
#define PLAT_KEY_RSHIFT     16
#define PLAT_KEY_LALT       17
#define PLAT_KEY_RALT       18
#define PLAT_KEY_F1         19      /* F1..F12 = PLAT_KEY_F1 .. PLAT_KEY_F1 + 11 */

typedef struct PlatEvent {
    int type;                       /* PLAT_EV_* */
    /* a key's position: its PC (set 1) scan code, | 0x80 for the extended (E0) keys, as a PC keyboard
       sends it whatever the layout; 0 when the key has none */
    int scan;
    int key;                        /* PLAT_KEY_* or 0. The keypad's keys are the navigation keys with Num Lock off */
    int repeat;                     /* key down: sent by auto-repeat */
    /* a typed character as a Unicode code point. Also 8, 9, 13 and 27 for Backspace, Tab, Enter and
       Escape, and 1..26 for Ctrl+A..Z */
    int ch;
    int x, y;                       /* mouse move: in frame pixels; outside 0..size-1 when over the bars */
    int button;                     /* PLAT_BUTTON_* */
    int wheel;                      /* 120 a notch, positive away from the user */
    int on;
} PlatEvent;

extern int PlatPollEvent(PlatEvent *ev);                   /* 0 = no event waiting */
extern void PlatWaitEvent(int ms);                         /* returns once an event waits, or after ms */
/* the toggle state of a key named by its Windows virtual-key code (ddraw.ini's key_x2mode): the lock
   keys answer their lock state, a letter, digit or function key flips at each press */
extern int PlatKeyToggled(int winKey);

/* -- cursor --------------------------------------------------------------------------------------- */
extern void PlatCursorShow(int show);
extern void PlatCursorArrow(void);                         /* back to the system arrow */

/* -- audio: outputs of 16-bit stereo, each fed on the audio thread by a pull function. The Sound Tool's
   mix is one (runtime/atsnddrv.c), every playing movie's sound another (media/quicktime.c) -------------- */
typedef struct PlatAudio PlatAudio;
/* called on the audio thread with the output locked; fills `frames' stereo frames or fewer and returns how
   many, 0 at the end (it is not called again after that) */
typedef int (*PlatAudioPull)(void *ctx, int16_t *out, int frames);

/* starts playing at once; `rate' in frames a second. NULL when there is no audio device */
extern PlatAudio *PlatAudioOpen(int rate, PlatAudioPull pull, void *ctx);
extern void PlatAudioPause(PlatAudio *s);
extern void PlatAudioResume(PlatAudio *s);
extern void PlatAudioLock(PlatAudio *s);                   /* excludes the pull */
extern void PlatAudioUnlock(PlatAudio *s);
extern int PlatAudioDrained(PlatAudio *s);                 /* the pull ended and what it gave was played */
extern void PlatAudioClose(PlatAudio *s);                  /* synchronous: the pull is never called after */

/* -- clocks, log, paths ----------------------------------------------------------------------------- */
extern uint64_t PlatMicroseconds(void);                   /* monotonic */
extern uint32_t PlatLocalSeconds1904(void);               /* Mac clock: seconds since 1904-01-01, local time */
extern void PlatDebugLog(const char *msg);
/* a path in the ANSI code page (the game's) as UTF-8 (FFmpeg's file names on Windows) */
extern void PlatPathToUtf8(const char *path, char *out, int outSize);

#ifdef __cplusplus
}
#endif
#endif
