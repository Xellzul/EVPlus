/* platform.h -- the OS services the game runs on: SDL (platform_sdl.cpp), per system (platform_win.cpp,
   platform_unix.cpp) */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <stdio.h>

/* -- start-up -- */
extern int PlatInit(void);                                 /* 0 = up */
extern void PlatQuit(void);

/* -- video: one window, the interface's layer over the space view (GPU renderer or a fallback) -- */
extern int PlatVideoCreateWindow(const char *title);      /* hidden until PlatVideoOpen; 0 = made */
extern int PlatVideoOpen(int width, int height, int fullScreen);   /* the frame's size; 0 = open */
extern void PlatVideoClose(void);                          /* drops the frame, keeps the window */
extern int PlatVideoResize(int width, int height);         /* 0 = done, else unchanged */
extern int PlatVideoCanResize(int width, int height);
typedef struct PlatRect {
    int x;
    int y;
    int w;
    int h;
} PlatRect;
/* images: x1r5g5b5 pixels with a 0/opaque alpha, or premultiplied ARGB (the 32 calls). pixels NULL = black, alpha NULL
   = opaque, argb NULL = clear. A destroyed image still shown is freed once another view replaces it. A linear filter
   samples neighbours: atlas parts need a clear 1-px gap. */
typedef struct PlatImage PlatImage;
extern PlatImage *PlatImageCreate(int width, int height, const uint16_t *pixels, const uint8_t *alpha);
/* pixels start at the rect's first pixel, pitch in bytes; the alpha stays; 1 when something differed */
extern int PlatImageUpdatePart(PlatImage *image, const PlatRect *rect, const uint16_t *pixels, int pitch);
extern void PlatImageDestroy(PlatImage *image);
extern PlatImage *PlatImageCreate32(int width, int height, const uint32_t *argb, int pitch);
extern void PlatImageUpdate32(PlatImage *image, const PlatRect *rect, const uint32_t *argb, int pitch);
                                                           /* rect NULL = all */

/* targets: GPU canvases of premultiplied ARGB; NULL when they cannot be made. Pixels are lost on a render reset event;
   a destroyed target still shown is freed once another view replaces it. */
typedef struct PlatTarget PlatTarget;
extern PlatTarget *PlatTargetCreate(int width, int height);
extern void PlatTargetDestroy(PlatTarget *target);
#if EVPLUS_TEST_PAGES
extern void PlatTargetClear(PlatTarget *target);
#endif
extern const char *PlatRendererName(void);         /* "none" before one */

/* a draw's kind, blend and filter */
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
/* kPlatDrawOr's 16-bit blend, on 5-bit channels; OR only on the GPU renderer, mod, alpha, part and source ignored */
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

/* the space view: drawn at width x height, scaled into the frame's rect, shown where the layer is clear; contents are
   copied */
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
extern PlatDraw PlatDrawOf(int kind, int blend);    /* opaque, colours whole, the rest 0 */
extern void PlatTargetDraw(PlatTarget *target, const PlatDraw *draws, int drawCount, const PlatVertex *vertices,
                           int vertexCount, const PlatRect *clip);
                                                           /* clip NULL: all; source is not the target */
extern int PlatTargetRead(PlatTarget *target, const PlatRect *rect, uint32_t *out);
                                                           /* out rows are the rect's width; 0 = not read */
extern void PlatVideoPresentLayer(PlatTarget *layer, const PlatVideoView *view);
                                                  /* view NULL: the last; may show at the next present or poll */
extern void PlatVideoDropView(void);
extern void PlatVideoSetFullScreen(int on);
extern void PlatVideoSetScaling(int filter);            /* kPlatFilter* */
extern void PlatVideoSetBrightness(double exponent);    /* 1 = as drawn */
extern int PlatVideoBrightness(void);                   /* 1 while it can be shown */
extern void PlatVideoAskOrBlend(int on);                /* before the first PlatVideoOpen */
extern int PlatVideoOrBlend(void);                      /* 1 while kPlatDrawOr is drawn */
extern void PlatVideoSetWhite(unsigned level);          /* toward white by level of 256 */
extern int PlatVideoIsFullScreen(void);
extern int PlatMaxTextureSize(void);                     /* 0 = not known yet */
typedef struct PlatVideoTimes {
    uint64_t uploadNs;
    uint64_t renderNs;
} PlatVideoTimes;
extern void PlatVideoTakeTimes(PlatVideoTimes *times);  /* since the last call */
extern void PlatDesktopSize(int *width, int *height);
/* the game's entry (port/winmain.cpp) and its UTF-8 arguments */
extern int GameMain(void);
extern int PlatArgCount(void);
extern const char *PlatArg(int index);                     /* "" past the last */

extern void PlatRegisterProgram(const char *programName);  /* Windows: App Paths */
extern void PlatAlert(const char *title, const char *message);   /* blocks */

/* -- events -- */
union SDL_Event;
extern int PlatPollEvent(union SDL_Event *e);              /* 0 = none; also shows a waiting frame, swallows Alt+Enter;
                                                              mouse motion in frame pixels, maybe outside it */
extern void PlatWaitEvent(int ms);
extern int PlatKeyIsDown(int scan);                        /* kScan* */
extern int PlatKeyLockState(int scan);                     /* 1, 0; -1 for a key with no lock */
extern int PlatCapsLockPressed(void);                      /* 1 once per Caps Lock press */

/* -- cursor -- */
extern void PlatCursorShow(int show);
extern void PlatCursorArrow(void);

/* -- audio: 16-bit stereo outputs fed by a pull on the audio thread -- */
typedef struct PlatAudio PlatAudio;
typedef int (*PlatAudioPull)(void *ctx, int16_t *out, int frames);   /* returns frames written; 0 = the end */
extern PlatAudio *PlatAudioOpen(int rate, PlatAudioPull pull, void *ctx);   /* NULL: no device */
extern void PlatAudioPause(PlatAudio *s);
extern void PlatAudioResume(PlatAudio *s);
extern void PlatAudioLock(PlatAudio *s);
extern void PlatAudioUnlock(PlatAudio *s);
extern int PlatAudioDrained(PlatAudio *s);
extern void PlatAudioClose(PlatAudio *s);               /* synchronous: no pull after it returns */

/* -- voices: sounds mixed into the default output -- */
typedef struct PlatVoices PlatVoices;
extern PlatVoices *PlatVoicesOpen(int count, float gain, int offline);   /* NULL: no device */
extern void PlatVoicesClose(PlatVoices *v);
extern int PlatVoicesStart(PlatVoices *v, int index, const int16_t *frames, int count, int rate, double speed);
                                                           /* copies frames; speed 0.01..100; 0 = started */
extern void PlatVoicesStop(PlatVoices *v, int index);
extern int PlatVoicesEnded(PlatVoices *v, int index);
#define kPlatVoicesRenderRate 48000
#if EVPLUS_TEST_PAGES
extern void PlatVoicesRender(PlatVoices *v, float *out, float *unclamped, int frames);   /* offline */
#endif

/* -- locks, recursive -- */
typedef struct PlatMutex PlatMutex;
extern PlatMutex *PlatMutexCreate(void);
extern void PlatMutexLock(PlatMutex *m);
extern void PlatMutexUnlock(PlatMutex *m);
extern void PlatMutexDestroy(PlatMutex *m);

/* -- files and fonts: UTF-8 paths, names matched in any case -- */
extern const char *PlatExeFolder(void);                    /* ends in a separator */
extern int PlatSetCurrentFolder(const char *path);         /* 0 = no folder */
#define kPlatPathMax     4096                              /* with the NUL */
extern FILE *PlatFileOpen(const char *path, const char *mode);
extern int PlatFindPath(const char *path, char *out, int outSize);   /* as on disk; 0 = too long */

#define kPlatNothing    0
#define kPlatFile       1
#define kPlatFolder     2
extern int PlatPathKind(const char *path, uint64_t *size); /* size may be NULL */

typedef void (*PlatFoundProc)(const char *name, int isFolder, uint64_t size, void *ctx);
extern int PlatListFolder(const char *folder, PlatFoundProc found, void *ctx);
                                                           /* by name, never . or ..; 0 = not read */
extern int PlatMakeFolder(const char *path);               /* with the ones above it; 1 when something is there after */
extern int PlatRemove(const char *path);                   /* a file or an empty folder; 0 = not removed */
extern int PlatEnv(const char *name, char *out, int outSize);     /* 0 = not set */
extern int PlatOpenFileDialog(const char *kindName, const char *extension, const char *startFolder, char *out,
                              int outSize);                /* 0 = cancelled */
extern int PlatListFiles(const char *folder, const char *pattern, void (*found)(const char *path, void *ctx),
                         void *ctx);                       /* pattern in any case, whole paths; returns the count */
extern void *PlatLoadFile(const char *path, int *size);    /* malloc'd; NULL = not read */
extern void *PlatSystemFont(const char *family, int bold, int italic, int lookAlike, int *size);
                                                           /* malloc'd; NULL if none; lookAlike: any font will do */

/* -- clocks, random numbers, log -- */
extern uint64_t PlatMicroseconds(void);                   /* monotonic */
extern uint64_t PlatCpuMicroseconds(void);                /* the process, all threads */
extern void PlatSleep(int ms);
extern uint32_t PlatLocalSeconds1904(void);               /* local time, Mac epoch */
extern int32_t PlatRand(uint64_t *state, int32_t n);      /* [0, n) */
extern void PlatDebugLog(const char *msg);
extern void PlatDebugLogOnce(int *said, const char *msg);   /* while *said is 0 */

#endif
