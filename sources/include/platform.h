/* platform.h -- the operating-system services the game runs on. Plain C types cross this header, and SDL's
   events (the game's event pump, runtime/burgerkbwin.cpp, reads them).
   On SDL (platform/platform_sdl.cpp, every platform): start-up, the window and its frame, the events, the cursor,
   the audio outputs and voices, the clocks, the debug log. Per system (platform/platform_win.cpp, platform_unix.cpp):
   opening files and changing folder by a UTF-8 path, the program's file type, the installed fonts. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <stdio.h>

/* -- start-up ------------------------------------------------------------------------------------- */
extern int PlatInit(void);                                 /* 0 when the layer is up */
extern void PlatQuit(void);

/* -- video: one window showing the game's frame: the interface's layer over the space view -------- */
/* The frame is stretched to the window with its aspect kept (black bars), linear filter unless
   PlatVideoSetScaling says otherwise. Alt+Enter switches between a borderless full screen and a resizable
   window; the layer handles it itself. The renderer is SDL's GPU renderer where it can be made, else another
   of SDL's (SDL_RENDER_DRIVER chooses, as in SDL); only the GPU renderer shows a brightness and kPlatDrawOr. */
extern int PlatVideoCreateWindow(const char *title);      /* hidden until PlatVideoOpen; 0 = made */
extern int PlatVideoOpen(int width, int height, int fullScreen);   /* the frame's size; 0 = open */
extern void PlatVideoClose(void);                          /* drops the frame, keeps the window */
/* The frame made anew at another size, the mode kept (a window fitted to it as PlatVideoOpen fits it); 0 = done,
   else the frame is unchanged. Nothing is shown from it until the next present. PlatVideoCanResize: 1 when
   PlatVideoResize to that size will succeed. */
extern int PlatVideoResize(int width, int height);
extern int PlatVideoCanResize(int width, int height);
typedef struct PlatRect {
    int x;
    int y;
    int w;
    int h;
} PlatRect;
/* An image, from x1r5g5b5 pixels and an alpha per pixel (0 = clear, else opaque); without pixels black,
   without alpha every pixel opaque. The layer keeps them and makes their textures when first drawn. An image
   destroyed while the view shown still draws it is freed once another view replaces that one. */
typedef struct PlatImage PlatImage;
extern PlatImage *PlatImageCreate(int width, int height, const uint16_t *pixels, const uint8_t *alpha);
/* The pixels of rect (it lies on the image) from `pixels', pitch bytes a row, which starts at the rect's first
   pixel; the alpha stays. Only what differs from the image's own is taken and uploaded: 1 when something did. */
extern int PlatImageUpdatePart(PlatImage *image, const PlatRect *rect, const uint16_t *pixels, int pitch);
extern void PlatImageDestroy(PlatImage *image);
/* An image from premultiplied ARGB pixels, pitch bytes a row, kept and drawn like the others; its shape is
   white times its alpha. PlatImageCreate32 without argb makes it clear. PlatImageUpdate32 replaces the pixels
   of rect (NULL: all of them; it lies on the image) from argb, which starts at the rect's first pixel; without
   argb it changes nothing. A part drawn through a linear filter takes in the texels next to it: the parts of
   an atlas lie a clear pixel apart. */
extern PlatImage *PlatImageCreate32(int width, int height, const uint32_t *argb, int pitch);
extern void PlatImageUpdate32(PlatImage *image, const PlatRect *rect, const uint32_t *argb, int pitch);

/* A canvas of the interface: width x height premultiplied ARGB pixels on the GPU, clear when made, drawn into
   at once (PlatTargetDraw). NULL for a zero size, and (logged) for one larger than PlatMaxTextureSize, out of
   memory or whose texture the renderer cannot make. One made before the renderer gets its texture when first
   used; while it cannot, the target is not drawn into, from or read. One destroyed while the view shown still
   draws it is freed once another view replaces that one, as an image. A new target is no layer until drawn
   into or cleared. Targets keep their pixels: a renderer that loses them sends the game a render reset event
   (PlatPollEvent). */
typedef struct PlatTarget PlatTarget;
extern PlatTarget *PlatTargetCreate(int width, int height);
extern void PlatTargetDestroy(PlatTarget *target);
extern void PlatTargetClear(PlatTarget *target);           /* every pixel clear */
extern const char *PlatRendererName(void);         /* SDL's name of the renderer; "none" before one */

/* What is drawn, back to front, in the pixels of what it is drawn into -- the GPU view, the frame (overlays), a
   target:
   - kPlatDrawImage: the image into the rect x, y, w, h, each channel times mod[c] / 255. Mix keeps what is
     under it times 1 - the image's alpha times alpha / 255, Add keeps it whole, Copy keeps nothing of it and
     writes the image times alpha / 255; Max keeps in each channel, alpha too, the greater of that and what is there
   - kPlatDrawSilhouette: the image's shape in the colour times mod[c] / 255, blended as an image
   - kPlatDrawFill: the rect in the colour: mixed by `alpha', added times `alpha', or copied or maxed at that alpha
   - kPlatDrawTriangles: `count' vertices from `first', each its own colour and alpha, blended as a fill
   - kPlatDrawOr: the image through the original renderer's 16-bit blend (PlatOrBlend); only in the view while
     PlatVideoOrBlend(), elsewhere the image mixed as it is. Always the whole image: mod, alpha, the part and
     `source' are not read
   - kPlatDrawTiled: as kPlatDrawImage, the image repeated over the rect: each tile tileScale times the
     image's size, one of them with its corner at tileX, tileY
   The colour is `color', x1r5g5b5, or with `deep' the 8-bit channels of `rgb' (a deep draw's vertices all take
   it). An image is drawn whole, or the part sx, sy, sw, sh of it when that is not empty, scaled by `filter'.
   With `source' that target is drawn in place of an image; it cannot be the target drawn into, and has no
   silhouette. Overlays and targets take everything, except a max where the renderer has none: only targets
   take that. */
#define kPlatDrawImage          0
#define kPlatDrawSilhouette     1
#define kPlatDrawFill           2
#define kPlatDrawTriangles      3
#define kPlatDrawOr             4
#define kPlatDrawTiled          5
#define kPlatBlendMix           0
#define kPlatBlendAdd           1
#define kPlatBlendCopy          2
#define kPlatBlendMax           3
#define kPlatFilterNearest      0
#define kPlatFilterLinear       1
#define kPlatFilterPixelArt     2
/* On 5-bit channels, each opaque pixel of the image (src) over the view (dst) becomes
       Scale(s, weight[c]) | over[c] | Scale(dst, keep)
   where s = Scale(src, own) | Scale(tint, 32 - own) when own < 32, else src, and Scale(v, k) = (v * k >> 5) & 31
   with only k's low 10 bits counting, as in the 32-bit pair math. Each pixel is blended as the low one of a
   pair; the high one differs only for factors that wrapped below 0. */
typedef struct PlatOrBlend {
    unsigned char weight[3];        /* 0..32 */
    unsigned char over[3];          /* 0..31 */
    unsigned char own;              /* 0..32 */
    unsigned short tint;            /* x1r5g5b5 */
    unsigned short keep;
} PlatOrBlend;
typedef struct PlatDraw {
    unsigned char kind;
    unsigned char blend;
    unsigned char alpha;
    unsigned char mod[3];
    unsigned short color;
    PlatOrBlend orBlend;
    PlatImage *image;
    float x;
    float y;
    float w;
    float h;
    int first;
    int count;
    unsigned char deep;
    unsigned char rgb[3];
    unsigned char filter;               /* kPlatFilter* */
    PlatTarget *source;
    float sx;
    float sy;
    float sw;
    float sh;
    float tileX;
    float tileY;
    float tileScale;                    /* 0 = 1 */
} PlatDraw;
typedef struct PlatVertex {
    float x;
    float y;
    unsigned short color;
    unsigned char alpha;
} PlatVertex;

/* The space view under a frame: drawn by the GPU at width x height from draws/vertices, scaled into the frame's
   rect x, y, w, h (magnified zoom times about its centre) and shown wherever the frame's layer is clear.
   Overlays are draws over the view in the frame's pixels. Everything is copied; the view is shown under every
   later frame until another view, PlatVideoDropView or PlatVideoClose. */
typedef struct PlatVideoView {
    int width;
    int height;
    int x;
    int y;
    int w;
    int h;
    float zoom;                         /* 1 = the whole view */
    const PlatDraw *draws;
    int drawCount;
    const PlatVertex *vertices;
    int vertexCount;
    const PlatDraw *overlays;
    int overlayCount;
} PlatVideoView;
/* a draw of that kind and blend, opaque, its image's colours whole, everything else 0 */
extern PlatDraw PlatDrawOf(int kind, int blend);
/* The draws done at once into a target, in its pixels, kept to `clip' (NULL: the whole target). */
extern void PlatTargetDraw(PlatTarget *target, const PlatDraw *draws, int drawCount, const PlatVertex *vertices,
                           int vertexCount, const PlatRect *clip);
/* The target's pixels in rect (NULL: all of them) as premultiplied ARGB, the rect's width a row; 0 = not read. */
extern int PlatTargetRead(PlatTarget *target, const PlatRect *rect, uint32_t *out);
/* A frame: the interface's layer as it is at the call, stretched over the frame and mixed over the view and
   its overlays (`view' with it, NULL = the view of the frame before), all of it through the brightness, the
   white-out over that. It is shown at once, or -- when the last one was shown too short a time ago for a
   display to tell them apart -- by the next present or event poll after that time; a newer frame replaces
   one still waiting. */
extern void PlatVideoPresentLayer(PlatTarget *layer, const PlatVideoView *view);
extern void PlatVideoDropView(void);            /* later frames alone: black where the layer is clear */
extern void PlatVideoSetFullScreen(int on);
/* The filter (kPlatFilter*) the frame is stretched with, kept for every later frame. Can be called before
   PlatVideoOpen. */
extern void PlatVideoSetScaling(int filter);
/* The frame's brightness: a colour channel c (0..1) of the finished frame is shown as c to the power
   `exponent'; 1 = as drawn. Can be called before PlatVideoOpen. PlatVideoBrightness: 1 while the renderer
   can show it (SDL's GPU renderer), 0 when frames are shown as drawn whatever it is. */
extern void PlatVideoSetBrightness(double exponent);
extern int PlatVideoBrightness(void);
/* kPlatDrawOr asked for, before the first PlatVideoOpen, which makes the renderer. PlatVideoOrBlend: 1 while
   kPlatDrawOr is drawn, 0 when it cannot be (not SDL's GPU renderer, or its shader fails). */
extern void PlatVideoAskOrBlend(int on);
extern int PlatVideoOrBlend(void);
/* The frame blended toward white by level of 256, as CGDisplayFade's kCGDisplayBlendSolidColor; 0 = as
   drawn. Shown from the next frame on. */
extern void PlatVideoSetWhite(unsigned level);
extern int PlatVideoIsFullScreen(void);
extern int PlatMaxTextureSize(void);                     /* the largest texture side; 0 = not known yet */
/* Time spent showing frames since the last call, then zeroed: upload = the view's draws and the layer into
   their textures, render = those to the window and SDL's present. */
typedef struct PlatVideoTimes {
    uint64_t uploadNs;
    uint64_t renderNs;
} PlatVideoTimes;
extern void PlatVideoTakeTimes(PlatVideoTimes *times);
extern void PlatDesktopSize(int *width, int *height);      /* the main display, in pixels */
/* The program starts in the platform layer, which calls the game (port/winmain.cpp) with the arguments
   kept for it: PlatArg(0) is the program, the rest what it was started with, UTF-8. */
extern int GameMain(void);
extern int PlatArgCount(void);
extern const char *PlatArg(int index);                     /* "" past the last */

/* Makes the program known to the system under the name it is asked for by (`programName'): where it
   lies. Windows: written to the user's registry (App Paths). */
extern void PlatRegisterProgram(const char *programName);

/* a message with an OK button, in front of the window; returns when the user has closed it. UTF-8 */
extern void PlatAlert(const char *title, const char *message);

/* -- events --------------------------------------------------------------------------------------- */
union SDL_Event;
/* The next SDL event for the game, after the platform's own part: 0 when none waits. Some events never
   reach the game: Alt+Enter (full screen; neither the Enter's press nor its release), redraws, a second focus
   gain or loss in a row, and on Windows a focus loss to another window of this program (a Win32 dialog the
   game opened) with the gain after it. A render reset (SDL_EVENT_RENDER_TARGETS_RESET, _DEVICE_RESET,
   _DEVICE_LOST) does: what the renderer held, every target's pixels with it, is gone. Mouse motion arrives
   in frame pixels (outside 0..size-1 when over the bars); an ISO keyboard's key left of Enter as the
   backslash key. Also shows a frame that is waiting. */
extern int PlatPollEvent(union SDL_Event *e);
extern void PlatWaitEvent(int ms);                         /* returns once an event waits, or after ms */
/* 1 while the key with this scan code (kScan*) is held, as of the events read so far */
extern int PlatKeyIsDown(int scan);
/* a lock key's (Caps, Num, Scroll Lock) lock state, 1 or 0; -1 for any other key */
extern int PlatKeyLockState(int scan);
/* 1 once after a Caps Lock key-down event (some systems report it pressed and released at once), then 0 */
extern int PlatCapsLockPressed(void);

/* -- cursor --------------------------------------------------------------------------------------- */
extern void PlatCursorShow(int show);
extern void PlatCursorArrow(void);                         /* back to the system arrow */

/* -- audio: outputs of 16-bit stereo, each fed on the audio thread by a pull function: every playing movie's
   sound (media/movie.cpp) --------------------------------------------------------------------------------- */
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

/* -- voices: sounds played at once, resampled and summed by SDL into the default output (the Sound Tool's
   channels, runtime/atsndvoice.cpp). Each voice plays a copy of the 16-bit stereo frames it is given. ------ */
typedef struct PlatVoices PlatVoices;
/* `count' voices, the whole mix scaled by `gain'. Offline: played by nothing, only by PlatVoicesRender.
   NULL when there is no audio device (offline: when the streams cannot be made) */
extern PlatVoices *PlatVoicesOpen(int count, float gain, int offline);
extern void PlatVoicesClose(PlatVoices *v);
/* voice `index' drops what it played and plays `count' frames recorded at `rate' Hz, `speed' times as fast
   (0.01..100); 0 = started, else it is silent */
extern int PlatVoicesStart(PlatVoices *v, int index, const int16_t *frames, int count, int rate, double speed);
extern void PlatVoicesStop(PlatVoices *v, int index);
extern int PlatVoicesEnded(PlatVoices *v, int index);      /* nothing left to play, or a second past its length */
/* offline: the next `frames' frames of the mix, float stereo at kPlatVoicesRenderRate, summed as the device
   sums them (voice count-1 first, a clamp to -1..1 after each); `unclamped' (may be NULL) gets the plain sum */
#define kPlatVoicesRenderRate 48000
extern void PlatVoicesRender(PlatVoices *v, float *out, float *unclamped, int frames);

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
extern uint64_t PlatCpuMicroseconds(void);                /* CPU time of the whole process, all threads */
extern void PlatSleep(int ms);                            /* 0 may return at once */
extern uint32_t PlatLocalSeconds1904(void);               /* Mac clock: seconds since 1904-01-01, local time */

/* -- random numbers: a value in [0, n) for n > 0, the generator's state in *state -------------------- */
extern int32_t PlatRand(uint64_t *state, int32_t n);
extern void PlatDebugLog(const char *msg);
extern void PlatDebugLogOnce(int *said, const char *msg);   /* logged while *said is 0, which it then sets */

#endif
