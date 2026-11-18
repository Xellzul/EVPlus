/* quicktime.c -- the QuickTime 6 functions the game calls (formerly qtmlClient.lib + QuickTime.qts), same names
 * and signatures as the declarations in macport.h, burger.h, render.h, sound.h, spriteworld.h and
 * burgerpict.c. Media decoding is media.c (FFmpeg); audio output, clocks and logging are platform.h.
 * Movie and picture GWorlds are the port's own CGrafPort records, so the port's DisposeGWorld frees them.
 *
 * Behaviour follows QuickTime */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "macport.h"
#include "burger.h"
#include "media.h"
#include "platform.h"

#define kNoErr          0
#define kFnfErr         (-43)
#define kMemFullErr     (-108)
#define kNoMovieFound   (-2048)
#define kCodecErr       (-8960)

#define kGestaltQuickTime 0x7174696d                /* 'qtim' */
#define kQuickTimeVersion 0x06408000                /* QuickTime 6.4, the CE install's QuickTime.qts */
#define kVideoMediaType   0x76696465                /* 'vide' */
#define kFormat16BE555    0x00000010                /* k16BE555PixelFormat */
#define kFormat16LE555    0x4c353535                /* k16LE555PixelFormat 'L555' */
#define kTimeScale        600                       /* QuickTime's movie time units per second */
#define kLoopTimeBase     1                         /* loopTimeBase */
#define kDeviceToken      0x51544744                /* any non-NULL "GDHandle"; nothing reads it */

static short gMemErr;

/* ---- Memory Manager ---------------------------------------------------------------------------------- */

typedef struct HandleBlock {
    void *master;                                   /* *handle */
    int32_t size;
} HandleBlock;

Handle NewHandle(int32_t size)
{
    HandleBlock *b = (HandleBlock *)malloc(sizeof(HandleBlock) + (size_t)(size > 0 ? size : 0));

    if (b == NULL) {
        gMemErr = kMemFullErr;
        return NULL;
    }
    b->master = b + 1;
    b->size = size;
    gMemErr = kNoErr;
    return (Handle)&b->master;
}

void DisposeHandle(Handle h)
{
    if (h != NULL)
        free(h);                                    /* &block->master == block */
    gMemErr = kNoErr;
}

/* applied by loadbar.c to the port's own GetResource handles: must not touch them */
void HLock(Handle h) { (void)h; }
void HUnlock(Handle h) { (void)h; }
void HPurge(Handle h) { (void)h; }

void *NewPtr(int32_t size)
{
    void *p = malloc((size_t)(size > 0 ? size : 1));

    gMemErr = p != NULL ? kNoErr : kMemFullErr;
    return p;
}

void DisposePtr(void *p)
{
    free(p);
    gMemErr = kNoErr;
}

void BlockMove(const void *src, void *dst, uint32_t len)
{
    memmove(dst, src, len);
}

/* QuickTime's own last Memory Manager error, never the port's: 0 until a QuickTime allocation fails,
   -109 after NewMovieFromFile (measured) */
short MemError(void)
{
    return gMemErr;
}

/* ---- small Toolbox pieces ---------------------------------------------------------------------------- */

short InitializeQTML(int32_t flags) { (void)flags; return kNoErr; }
void TerminateQTML(void) { }
short EnterMovies(void) { return kNoErr; }

void Gestalt(int32_t selector, int32_t *result)
{
    if ((uint32_t)selector == kGestaltQuickTime && result != NULL)
        *result = kQuickTimeVersion;
}

/* the game only asks whether the Animation decompressor exists (initevnova.c); FFmpeg's qtrle is built in */
void *FindNextComponent(void *prev, ComponentDescription *desc)
{
    (void)desc;
    return prev == NULL ? (void *)(intptr_t)kDeviceToken : NULL;
}

int32_t GetMainDevice(void) { return kDeviceToken; }

void Microseconds(UnsignedWide *now)
{
    uint64_t us = PlatMicroseconds();

    now->lo = (uint32_t)us;
    now->hi = (uint32_t)(us >> 32);
}

void GetDateTime(uint32_t *secs)
{
    *secs = PlatLocalSeconds1904();
}

/* measured: optional sign, then decimal digits up to the first other character (" 12" is 0, "12a3" is 12),
   wrapping on overflow */
void StringToNum(const unsigned char *str, int32_t *value)
{
    uint32_t v = 0;
    int i = 1, n = str[0], neg = 0;

    if (i <= n && (str[i] == '-' || str[i] == '+')) {
        neg = str[i] == '-';
        i++;
    }
    for (; i <= n && str[i] >= '0' && str[i] <= '9'; i++)
        v = v * 10u + (uint32_t)(str[i] - '0');
    *value = (int32_t)(neg ? 0u - v : v);
}

/* QuickDraw's generator; randSeed starts at 1 (measured: 16807, 15089, -21287, ...) */
static uint32_t gRandSeed = 1;

short Random(void)
{
    short r;

    gRandSeed = (uint32_t)(((uint64_t)gRandSeed * 16807u) % 0x7fffffffu);
    r = (short)(gRandSeed & 0xffff);
    if (r == -32768)
        r = 0;
    return r;
}

/* QuickTime's ForeColor sets the colour of QuickTime's own current port, which is never one of the port
   layer's current port (QuickTime's SetGWorld keeps its own, below), so it has no visible
   effect on the game's drawing.
   BUG: the credits scroller (loadbar.c) calls it to pick black and white; those calls do nothing and the
   text keeps the port's previous colour, as with QuickTime */
void (ForeColor)(int32_t color) { (void)color; }

void MacSetRect(Rect *r, short left, short top, short right, short bottom)
{
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
}

/* only called by engineutils.c VectorToAngle, which the exe does not contain; Inside Macintosh semantics
   (degrees clockwise from 12 o'clock, the rect's corners at the 45-degree lines) */
void PtToAngle(const Rect *r, Point pt, short *angle)
{
    double w = r->right - r->left, h = r->bottom - r->top;
    double dx = pt.h - (r->left + r->right) / 2.0, dy = pt.v - (r->top + r->bottom) / 2.0;
    double a;

    if (w <= 0 || h <= 0) {
        *angle = 0;
        return;
    }
    a = atan2(dx / w, -dy / h) * 180.0 / 3.14159265358979323846;
    if (a < 0)
        a += 360.0;
    *angle = (short)(a + 0.5) % 360;
}

void DebugStr(const char *message)
{
    char buf[260];
    int n = (unsigned char)message[0];

    memcpy(buf, message + 1, (size_t)n);
    buf[n] = '\0';
    PlatDebugLog(buf);
}

/* ---- GWorlds: the port's CGrafPort (macport.h) ---------------------------------------------------------- */

/* a port record over pixels it does not own (offscreen 0: DisposeGWorld frees only the record) */
static void InitGWorld(CGrafPtr g, const Rect *bounds, int pixelSize, void *baseAddr, int32_t rowBytes)
{
    memset(g, 0, sizeof *g);
    g->pix.bounds = *bounds;
    g->pix.pixelSize = pixelSize;
    g->pix.baseAddr = baseAddr;
    g->pix.width = bounds->right - bounds->left;
    g->pix.height = bounds->bottom - bounds->top;
    g->pix.rowBytes = rowBytes;
    g->depth = pixelSize == 16 ? 15 : pixelSize;
    g->backColor.red = g->backColor.green = g->backColor.blue = -1;
    g->pnSize.h = g->pnSize.v = 1;
    g->txSize = 9;
}

/* QuickTime's QTNewGWorldFromPtr (Carbon's NewGWorldFromPtr): the game's movie destinations, 16-bit 555 */
short NewGWorldFromPtr(CGrafPtr *gw, OSType pixelFormat, const Rect *bounds, void *cTable, void *aGDevice,
                       int32_t flags, void *baseAddr, int32_t rowBytes)
{
    CGrafPtr g = (CGrafPtr)AllocAPointer(sizeof(CGrafPort));

    (void)cTable;
    (void)aGDevice;
    (void)flags;
    *gw = g;
    if (g == NULL)
        return kMemFullErr;
    InitGWorld(g, bounds, 16, baseAddr, rowBytes);
    g->bigEndian = pixelFormat != kFormat16LE555;
    return kNoErr;
}

Boolean LockPixels(PixMapPtr pm) { (void)pm; return 1; }

/* QuickTime's own current port, apart from the port layer's CurrentGrafPort as it was: only burgerpict.c
   sets it, around DecompressImage, and nothing draws through it */
static CGrafPtr gCurrentGWorld;

void GetGWorld(CGrafPtr *port, void **device)
{
    *port = gCurrentGWorld;
    *device = NULL;
}

void SetGWorld(CGrafPtr port, void *device)
{
    (void)device;
    gCurrentGWorld = port;
}

/* ImageDescription fields (host order: burgerpict.c swapped them) */
#define DescType(d)     (*(const uint32_t *)((const char *)(d) + 0x04))
#define DescWidth(d)    (*(const short *)((const char *)(d) + 0x20))
#define DescHeight(d)   (*(const short *)((const char *)(d) + 0x22))
#define DescDataSize(d) (*(const int32_t *)((const char *)(d) + 0x2c))
#define DescDepth(d)    (*(const short *)((const char *)(d) + 0x52))

static short PixelSizeForDepth(short depth)
{
    if (depth == 16)
        return 16;
    if (depth == 24 || depth == 32)
        return 32;
    return 8;                                       /* 1/2/4/8 colour, 33-40 gray */
}

/* the record and its pixels in one block, so the port's DisposeGWorld frees both */
short NewImageGWorld(CGrafPtr *gw, void **desc, uint32_t flags)
{
    const void *d = *desc;
    short w = DescWidth(d), h = DescHeight(d), size = PixelSizeForDepth(DescDepth(d));
    int32_t rowBytes = ((int32_t)w * (size / 8) + 3) & ~3;
    /* burgerpict.c copies rowBytes x the PICT's own height, which may exceed the description's: slack rows */
    size_t pixels = (size_t)rowBytes * ((size_t)h * 2 + 64);
    Rect bounds;
    CGrafPtr g;

    (void)flags;
    *gw = NULL;
    if (w <= 0 || h <= 0)
        return kCodecErr;
    g = (CGrafPtr)AllocAPointer((Word32)(sizeof(CGrafPort) + pixels));
    if (g == NULL)
        return kMemFullErr;
    bounds.top = bounds.left = 0;
    bounds.bottom = h;
    bounds.right = w;
    InitGWorld(g, &bounds, size, g + 1, rowBytes);
    memset(g + 1, 0, pixels);
    g->bigEndian = 1;
    *gw = g;
    return kNoErr;
}

short DecompressImage(char *data, void **desc, PixMapPtr pm, const Rect *srcRect, const Rect *dstRect,
                      short mode, void *mask)
{
    const void *d = *desc;

    (void)srcRect;
    (void)dstRect;
    (void)mode;
    (void)mask;
    if (MediaDecodeImage(DescType(d), DescWidth(d), DescHeight(d), DescDepth(d), data, DescDataSize(d),
                      pm->baseAddr, pm->rowBytes, pm->pixelSize) != 0)
        return kCodecErr;
    return kNoErr;
}

/* ---- files ------------------------------------------------------------------------------------------- */

/* the spec is opaque to the game: QuickTime's FSSpec layout (vRefNum, parID, name at +6, 262 bytes), which
   fits all three callers' buffers (music.c FSSpec, tradedlg.c FileSpec and a 0x108-byte scratch) */
#define kSpecName 6

static void PToC(char *dst, const unsigned char *p)
{
    memcpy(dst, p + 1, p[0]);
    dst[p[0]] = '\0';
}

static int FileExists(const char *path)
{
    FILE *f = PlatFileOpen(path, "rb");

    if (f == NULL)
        return 0;
    fclose(f);
    return 1;
}

short FSMakeFSSpec(int32_t vRefNum, int32_t dirID, const void *name, void *spec)
{
    const unsigned char *p = (const unsigned char *)name;
    unsigned char *s = (unsigned char *)spec;
    char path[256];

    (void)vRefNum;
    (void)dirID;
    memset(s, 0, kSpecName);
    memcpy(s + kSpecName, p, (size_t)p[0] + 1);
    PToC(path, p);
    return FileExists(path) ? kNoErr : kFnfErr;
}

#define kMaxRefs 16
#define kFirstRef 127
static char *gRefPath[kMaxRefs];

short OpenMovieFile(const void *spec, short *refNum, int32_t mode)
{
    char path[256];
    int i;

    (void)mode;
    PToC(path, (const unsigned char *)spec + kSpecName);
    if (!FileExists(path))
        return kFnfErr;
    for (i = 0; i < kMaxRefs; i++)
        if (gRefPath[i] == NULL)
            break;
    if (i == kMaxRefs)
        return kMemFullErr;
    gRefPath[i] = strdup(path);
    *refNum = (short)(kFirstRef + i);
    return kNoErr;
}

void CloseMovieFile(short refNum)
{
    int i = refNum - kFirstRef;

    if (i >= 0 && i < kMaxRefs) {
        free(gRefPath[i]);
        gRefPath[i] = NULL;
    }
}

/* ---- movies ------------------------------------------------------------------------------------------ */

typedef struct MovieRecord {
    struct MovieRecord *nextMovie;
    char *path;
    MediaFile *media;
    MediaAudio *audio;                 /* decoded on the output thread */
    PlatAudio *stream;
    Rect natural, box;
    CGrafPtr gw;
    short (*transferProc)(void);
    int32_t timeBaseFlags;
    int32_t hints;
    int playing;
    int64_t posMs;                  /* the movie time when the clock last started or stopped */
    uint64_t startUs;
    int64_t durationMs;
    volatile int volume;            /* 0x100 = unity */
    int audioFailed;                /* the output device would not open */
    int redraw;
} MovieRecord;

static MovieRecord *gMovies;

static int Looping(const MovieRecord *m) { return (m->timeBaseFlags & kLoopTimeBase) != 0; }

/* the movie time now, held at the end unless looping */
static int64_t MovieMs(const MovieRecord *m)
{
    int64_t t = m->posMs;

    if (m->playing)
        t += (int64_t)((PlatMicroseconds() - m->startUs) / 1000u);
    if (Looping(m)) {
        if (m->durationMs > 0)
            t %= m->durationMs;
    } else if (t > m->durationMs) {
        t = m->durationMs;
    }
    return t;
}

/* the output thread's pull: decoded samples times the movie volume */
static int PullAudio(void *ctx, int16_t *out, int frames)
{
    MovieRecord *m = (MovieRecord *)ctx;
    int n = MediaAudioRead(m->audio, out, frames), i, gain = m->volume;

    if (n == 0 && Looping(m)) {
        MediaAudioRewind(m->audio);
        n = MediaAudioRead(m->audio, out, frames);
    }
    if (gain < 0)
        gain = 0;
    if (gain != 0x100) {
        for (i = 0; i < n * 2; i++) {
            int v = out[i] * gain / 0x100;
            out[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
    }
    return n;
}

int32_t NewMovieFromFile(void **movie, short refNum, short *resId, int32_t a4, int32_t a5, int32_t a6)
{
    int i = refNum - kFirstRef;
    MovieRecord *m;
    int w, h;

    (void)resId;
    (void)a4;
    (void)a5;
    (void)a6;
    *movie = NULL;
    if (i < 0 || i >= kMaxRefs || gRefPath[i] == NULL)
        return kNoMovieFound;
    m = (MovieRecord *)calloc(1, sizeof *m);
    if (m == NULL)
        return kMemFullErr;
    m->media = MediaOpen(gRefPath[i]);
    if (m->media == NULL) {
        free(m);
        return kNoMovieFound;
    }
    m->path = strdup(gRefPath[i]);
    MediaVideoSize(m->media, &w, &h);
    m->natural.right = (short)w;
    m->natural.bottom = (short)h;
    m->box = m->natural;
    m->durationMs = MediaDurationMs(m->media);
    m->volume = 0x100;
    m->nextMovie = gMovies;
    gMovies = m;
    *movie = m;
    return kNoErr;
}

void DisposeMovie(void *movie)
{
    MovieRecord *m = (MovieRecord *)movie, **p;

    if (m == NULL)
        return;
    for (p = &gMovies; *p != NULL; p = &(*p)->nextMovie)
        if (*p == m) {
            *p = m->nextMovie;
            break;
        }
    PlatAudioClose(m->stream);                        /* synchronous: no pull after this */
    MediaAudioClose(m->audio);
    MediaClose(m->media);
    free(m->path);
    free(m);
}

void StartMovie(void *movie)
{
    MovieRecord *m = (MovieRecord *)movie;

    if (m == NULL || m->playing)
        return;
    m->playing = 1;
    if (MediaHasAudio(m->media)) {
        if (m->stream != NULL) {
            /* audio-only: the sound decides when the movie is done, so it always resumes */
            PlatAudioResume(m->stream);
        } else if (m->audio == NULL && !m->audioFailed &&
                   (Looping(m) || !MediaHasVideo(m->media) || m->posMs < m->durationMs || m->durationMs <= 0)) {
            m->audio = MediaAudioOpen(m->path);
            if (m->audio != NULL) {
                m->stream = PlatAudioOpen(MEDIA_AUDIO_RATE, PullAudio, m);
                if (m->stream == NULL) {        /* no audio device: play on silently, open nothing again */
                    MediaAudioClose(m->audio);
                    m->audio = NULL;
                    m->audioFailed = 1;
                }
            }
        }
    }
    /* the clock starts once the sound is open, so picture and sound start together */
    m->startUs = PlatMicroseconds();
}

void StopMovie(void *movie)
{
    MovieRecord *m = (MovieRecord *)movie;

    if (m == NULL || !m->playing)
        return;
    m->posMs = MovieMs(m);
    m->playing = 0;
    PlatAudioPause(m->stream);
}

void SetMovieRate(void *movie, int32_t rate)
{
    if (rate == 0)
        StopMovie(movie);
    else
        StartMovie(movie);
}

void GoToBeginningOfMovie(void *movie)
{
    MovieRecord *m = (MovieRecord *)movie;

    if (m == NULL)
        return;
    m->posMs = 0;
    m->startUs = PlatMicroseconds();
    m->redraw = 1;
    if (m->stream != NULL) {
        PlatAudioLock(m->stream);
        MediaAudioRewind(m->audio);
        PlatAudioUnlock(m->stream);
    }
}

/* measured: NULL is not done (PlayQTMovie on a non-movie then waits for a click or key, as with QuickTime) */
Boolean IsMovieDone(void *movie)
{
    MovieRecord *m = (MovieRecord *)movie;

    if (m == NULL || Looping(m))
        return 0;
    if (!MediaHasVideo(m->media) && m->stream != NULL)
        return (Boolean)PlatAudioDrained(m->stream);   /* audio-only: the sound itself decides */
    return (Boolean)(MovieMs(m) >= m->durationMs);
}

void MoviesTask(void *movie, int32_t maxMs)
{
    MovieRecord *m = (MovieRecord *)movie;
    CGrafPtr g;
    char *dst;
    int w, h;

    (void)maxMs;
    if (m == NULL || m->gw == NULL || !MediaHasVideo(m->media))
        return;
    g = m->gw;
    w = m->box.right - m->box.left;
    h = m->box.bottom - m->box.top;
    /* QuickDraw: baseAddr is the pixel at bounds.topLeft; no clipping (QuickTime does none either) */
    dst = (char *)g->pix.baseAddr + (int32_t)(m->box.top - g->pix.bounds.top) * g->pix.rowBytes +
          (int32_t)(m->box.left - g->pix.bounds.left) * 2;
    if (MediaVideoDraw(m->media, MovieMs(m), dst, g->pix.rowBytes, w, h, g->bigEndian, m->redraw)) {
        m->redraw = 0;
        if (m->transferProc != NULL)
            m->transferProc();
    }
}

void SetMovieVolume(void *movie, short volume)
{
    if (movie != NULL)
        ((MovieRecord *)movie)->volume = volume;
}

void SetMovieGWorld(void *movie, void *gworld, int32_t gdh)
{
    (void)gdh;
    if (movie != NULL) {
        ((MovieRecord *)movie)->gw = (CGrafPtr)gworld;
        ((MovieRecord *)movie)->redraw = 1;
    }
}

void SetMovieBox(void *movie, Rect *box)
{
    if (movie != NULL) {
        ((MovieRecord *)movie)->box = *box;
        ((MovieRecord *)movie)->redraw = 1;
    }
}

/* measured: a NULL movie leaves the rect as it was */
void GetMovieBox(void *movie, Rect *box)
{
    if (movie != NULL)
        *box = ((MovieRecord *)movie)->box;
}

int32_t GetMovieDuration(void *movie)
{
    if (movie == NULL)
        return 0;
    return (int32_t)(((MovieRecord *)movie)->durationMs * kTimeScale / 1000);
}

void LoadMovieIntoRam(void *movie, int32_t time, int32_t duration, int32_t flags)
{
    (void)movie;
    (void)time;
    (void)duration;
    (void)flags;
}

void SetMoviePlayHints(void *movie, int32_t flags, int32_t mask)
{
    MovieRecord *m = (MovieRecord *)movie;

    if (m != NULL)
        m->hints = (m->hints & ~mask) | (flags & mask);
}

/* the movie is its own time base */
void *GetMovieTimeBase(void *movie) { return movie; }

int32_t GetTimeBaseFlags(void *timeBase)
{
    return timeBase != NULL ? ((MovieRecord *)timeBase)->timeBaseFlags : 0;
}

void SetTimeBaseFlags(void *timeBase, int32_t flags)
{
    if (timeBase != NULL)
        ((MovieRecord *)timeBase)->timeBaseFlags = flags;
}

/* the movie stands for its video track */
void *GetMovieIndTrackType(void *movie, int32_t index, int32_t type, int32_t flags)
{
    (void)flags;
    if (movie == NULL || index != 1 || (uint32_t)type != kVideoMediaType || !MediaHasVideo(((MovieRecord *)movie)->media))
        return NULL;
    return movie;
}

/* the game's transfer proc (tradedlg.c IntroMovieIdleProc) presents the screen; called once per drawn frame */
void SetTrackGWorld(void *track, void *gworld, int32_t gdh, short (*proc)(void), int32_t refCon)
{
    (void)gdh;
    (void)refCon;
    if (track != NULL) {
        ((MovieRecord *)track)->gw = (CGrafPtr)gworld;
        ((MovieRecord *)track)->transferProc = proc;
    }
}

/* quitting: silence every movie; the records stay (the game may still hold them) */
void ExitMovies(void)
{
    MovieRecord *m;

    for (m = gMovies; m != NULL; m = m->nextMovie)
        PlatAudioPause(m->stream);
}
