/* platform.h -- the operating-system services the game runs on. Plain C types cross this header, and SDL's
   events (the game's event pump, runtime/burgerkbwin.cpp, reads them).
   On SDL (platform/platform_sdl.cpp, every platform): start-up, the window and its frame, the events, the cursor,
   the audio outputs, the clocks, the debug log. Per system (platform/platform_win.cpp, platform_unix.cpp):
   opening files and changing folder by a UTF-8 path, the program's file type, the installed fonts. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -- start-up ------------------------------------------------------------------------------------- */
extern int PlatInit(void);                                 /* 0 when the layer is up */
extern void PlatQuit(void);

/* -- video: one window showing the game's frame, 16-bit x1r5g5b5 pixels in host byte order ---------- */
/* The frame is stretched to the window with its aspect kept (black bars), linear filter unless
   PlatVideoSetScaling says otherwise. Alt+Enter switches between a borderless full screen and a resizable
   window; the layer handles it itself. */
extern int PlatVideoCreateWindow(const char *title);      /* hidden until PlatVideoOpen; 0 = made */
extern int PlatVideoOpen(int width, int height, int fullScreen);   /* the frame's size; 0 = open */
extern void PlatVideoClose(void);                          /* drops the frame, keeps the window */
/* A complete frame: the size given to PlatVideoOpen, pitch bytes a row. It is shown at once, or -- when the
   last one was shown too short a time ago for a display to tell them apart -- copied, and shown by the next
   present or event poll after that time; a newer frame replaces one still waiting. The pixels must stay
   readable until the next PlatVideoPresent or PlatVideoClose. */
extern void PlatVideoPresent(const void *pixels, int pitch);
extern void PlatVideoSetFullScreen(int on);
/* How the frame is stretched, kept for every later frame and window: the filter (kPlatFilter*), and
   wholePixel = each frame pixel the same whole number of window pixels (the frame smaller in a window too
   small for that). Can be called before PlatVideoOpen. */
#define kPlatFilterNearest      0
#define kPlatFilterLinear       1
#define kPlatFilterPixelArt     2
extern void PlatVideoSetScaling(int filter, int wholePixel);
/* The frame's brightness: a colour channel c (0..1) is shown as c to the power `exponent'; 1 = as drawn.
   Can be called before PlatVideoOpen. */
extern void PlatVideoSetBrightness(double exponent);
extern int PlatVideoIsFullScreen(void);
extern void PlatDesktopSize(int *width, int *height);      /* the main display, in pixels */
/* The program starts in the platform layer, which calls the game (port/winmain.cpp) with the arguments
   kept for it: PlatArg(0) is the program, the rest what it was started with, UTF-8. */
extern int GameMain(void);
extern int PlatArgCount(void);
extern const char *PlatArg(int index);                     /* "" past the last */

/* Makes the program and a kind of file it opens known to the system: where the program lies, under the
   name it is asked for by (`programName'), and that a file with this extension is opened by starting the
   program with the file as its argument. `typeId' names the kind to the system, `description' to the
   user; the kind's icon is the program's icon resource `icon', or `fallbackIcon' when it has no such one.
   Windows: written to the user's registry. */
extern void PlatRegisterFileType(const char *programName, const char *extension, const char *typeId,
                                 const char *description, int icon, int fallbackIcon);

/* a message with an OK button, in front of the window; returns when the user has closed it. UTF-8 */
extern void PlatAlert(const char *title, const char *message);

/* -- events --------------------------------------------------------------------------------------- */
union SDL_Event;
/* The next SDL event for the game, after the platform's own part: 0 when none waits. Some events never
   reach the game: Alt+Enter (full screen; neither the Enter's press nor its release), redraws and render
   resets, a second focus gain or loss in a row, and on Windows a focus loss to another window of this
   program (a Win32 dialog the game opened) with the gain after it. Mouse motion arrives in frame pixels
   (outside 0..size-1 when over the bars); an ISO keyboard's key left of Enter as the backslash key. Also
   shows a frame that is waiting. */
extern int PlatPollEvent(union SDL_Event *e);
extern void PlatWaitEvent(int ms);                         /* returns once an event waits, or after ms */
/* 1 while the key with this scan code (kScan*) is held, as of the events read so far */
extern int PlatKeyIsDown(int scan);
/* the toggle state of a key named by its Windows virtual-key code (ddraw.ini's key_x2mode): the lock
   keys answer their lock state, a letter, digit or function key flips at each press */
extern int PlatKeyToggled(int winKey);

/* -- cursor --------------------------------------------------------------------------------------- */
extern void PlatCursorShow(int show);
extern void PlatCursorArrow(void);                         /* back to the system arrow */

/* -- audio: outputs of 16-bit stereo, each fed on the audio thread by a pull function. The Sound Tool's
   mix is one (runtime/atsnddrv.cpp), every playing movie's sound another (media/movie.cpp) ------------------- */
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

/* -- locks: one thread at a time; the thread that holds one may take it again ----------------------- */
typedef struct PlatMutex PlatMutex;
extern PlatMutex *PlatMutexCreate(void);                   /* NULL when it cannot be made */
extern void PlatMutexLock(PlatMutex *m);
extern void PlatMutexUnlock(PlatMutex *m);
extern void PlatMutexDestroy(PlatMutex *m);

/* -- files and fonts ---------------------------------------------------------------------------------
   Every path is UTF-8, in the system's own form otherwise ("C:\dir\file" on Windows). Names are matched
   upper and lower case alike, as on Windows and the Mac, also where the file system tells them apart. */
extern const char *PlatExeFolder(void);                    /* where the program lies, ending in a separator */
/* the folder relative paths start from, ending in a separator; 0 = not known or too long for `out' */
extern int PlatCurrentFolder(char *out, int outSize);
extern int PlatSetCurrentFolder(const char *path);         /* 0 = it is no folder: nothing changed */
#define kPlatPathMax     4096                              /* bytes of a path, with its end */
extern FILE *PlatFileOpen(const char *path, const char *mode);    /* C's fopen */
/* The path as it lies on disk: each part that is not there under its own name is the one of that name in
   other case, when there is one; the parts after one found under no case stay as given. 0 = too long. */
extern int PlatFindPath(const char *path, char *out, int outSize);

#define kPlatNothing    0                                  /* there is nothing of that name */
#define kPlatFile       1
#define kPlatFolder     2
extern int PlatPathKind(const char *path, uint64_t *size); /* `size' may be NULL */

/* Everything in a folder, never "." or "..", in the order Windows lists an NTFS folder in, on every system:
   by name, upper and lower case alike. 0 = the folder could not be read. */
typedef void (*PlatFoundProc)(const char *name, int isFolder, uint64_t size, void *ctx);
extern int PlatListFolder(const char *folder, PlatFoundProc found, void *ctx);
/* makes the folder and the missing ones above it; nonzero when something of that name is there afterwards */
extern int PlatMakeFolder(const char *path);
extern int PlatRemove(const char *path);                   /* a file or an empty folder; 0 = nothing removed */
extern int PlatEnv(const char *name, char *out, int outSize);     /* an environment variable; 0 = not set */
/* The system's dialog for choosing one file to open, of the kind named ("EV Nova Pilot") with that
   extension ("plt"). It starts in `startFolder', and once a file has been chosen, in that file's folder.
   Returns when it is closed: nonzero with the path in `out', 0 when the user cancelled. */
extern int PlatOpenFileDialog(const char *kindName, const char *extension, const char *startFolder, char *out,
                              int outSize);
/* the files of a folder whose names match a pattern such as "*.ttf", upper or lower case; each is handed
   to `found' with its whole path. Returns how many. */
extern int PlatListFiles(const char *folder, const char *pattern, void (*found)(const char *path, void *ctx),
                         void *ctx);
extern void *PlatLoadFile(const char *path, int *size);    /* the whole file in a malloc'd block, or NULL */
/* An installed font by its family name, as the bytes of its font file in a malloc'd block; NULL when the
   system has no font of that name. With `lookAlike' the font the system draws that name with, whatever its
   own name. `bold' and `italic' ask for that member of the family; the system may answer with the plain
   one. */
extern void *PlatSystemFont(const char *family, int bold, int italic, int lookAlike, int *size);

/* -- clocks, log ------------------------------------------------------------------------------------- */
extern uint64_t PlatMicroseconds(void);                   /* monotonic, from any thread */
extern void PlatSleep(int ms);                            /* 0 may return at once */
extern uint32_t PlatLocalSeconds1904(void);               /* Mac clock: seconds since 1904-01-01, local time */

/* -- random numbers: a value in [0, n) for n > 0, the generator's state in *state -------------------- */
extern int32_t PlatRand(uint64_t *state, int32_t n);
extern void PlatDebugLog(const char *msg);

#ifdef __cplusplus
}
#endif
#endif
