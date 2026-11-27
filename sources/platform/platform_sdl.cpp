/* platform_sdl.cpp -- platform.h on SDL3, the same on every platform: start-up, the window and its frame, the
   events, the cursor, the audio outputs and voices, the clocks, the debug log. Built without any game header. */
#include <stdlib.h>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>          /* makes `main' below the program's entry on every system */
#ifdef SDL_PLATFORM_WIN32
#include <windows.h>                    /* FocusStaysInProgram */
#endif
#include "platform.h"
#include "scratch.h"

#define kFocusNotes     16              /* focus losses waiting in SDL's queue, a power of 2 */
#define kDefaultRefresh 60              /* Hz, when the display does not tell */
/* two frames are never held apart longer than this, whatever the display: the game's own frames in flight
   come 21 ms apart and must each be shown as they come */
#define kMaxShowInterval (10 * SDL_NS_PER_MS)
/* the game came back with another frame within this of the last one: it is not pacing itself (in flight it
   fills every frame up to 20 ms) but presenting in a burst, as at start-up */
#define kBurstGap       (2 * SDL_NS_PER_MS)
/* frame pixels past the view's right edge its last column is drawn over */
#define kViewEdge       2
/* in a burst the game gets this many times as long as a show took before the next show */
#define kBurstRest      3
/* a window that is not full screen opens at the frame's size, or at this share of the display when the frame is larger */
#define kWindowShareNum 9
#define kWindowShareDen 10

static SDL_Window *gWindow;
static SDL_Renderer *gRenderer;
static int gFrameWidth;                 /* the frame's size; 0 while no frame is open */
static int gFrameHeight;
static bool gFrameDrawn;                /* gLayerTexture has a frame to show */
static Uint64 gShowInterval;            /* ns between the starts of two shows */
static Uint64 gShowStart;               /* when the last show began */
static Uint64 gShowEnd;                 /* and when it was done */
static Uint64 gShowRest;                /* ns after gShowEnd in which nothing more is shown; 0 outside a burst */
static Uint64 gPresentEnd;              /* when the game's last present returned */
static SDL_ScaleMode gScaleMode = SDL_SCALEMODE_LINEAR;
static PlatVideoTimes gTimes;           /* for PlatVideoTakeTimes */
static SDL_Texture *gViewTexture;       /* the view under the frame, at the view's size */
static int gViewWidth;
static int gViewHeight;
static SDL_FRect gViewRect;             /* its place on the frame */
static bool gViewOn;                    /* a view was given: it is drawn under the frame */
static bool gViewFresh;                 /* the draws are not yet rendered into gViewTexture */
static float gViewZoom = 1.0f;          /* the view shown magnified about its centre */
static std::vector<PlatDraw> gDraws;    /* the GPU view's, as last given */
static std::vector<PlatVertex> gVertices;
static std::vector<PlatDraw> gOverlays;
/* kPlatDrawOr, on SDL's GPU renderer only: the shaders (NULL: not drawn), and with a view the state that reads
   gViewCopy -- the view as copied last, brought up to date before a draw that reads the view where it changed
   since (gChanged). With a brightness the view is drawn in 5-bit codes into gViewCodes, then shown into
   gViewTexture through it. */
static bool gOrAsked;
static SDL_GPUShader *gOrShader;
static SDL_GPUShader *gBrightShader;
static SDL_GPUSampler *gNearest;
static SDL_GPURenderState *gBrightState;
static SDL_GPURenderState *gOrState;
static SDL_Texture *gViewCopy;
static SDL_Texture *gViewCodes;
static std::vector<SDL_Rect> gChanged;
/* the frame: a copy of the layer last given (PlatVideoPresentLayer), shown over the view */
static SDL_Texture *gLayerTexture;
static int gLayerWidth;
static int gLayerHeight;
static bool gLayerWaiting;              /* gLayerTexture holds a frame not yet shown */

/* an image's textures: as shown (through the brightness), in 5-bit codes (for kPlatDrawOr), white where opaque */
enum Texels { kTexelsShown, kTexelsCodes, kTexelsSilhouette, kTexelKinds };

struct PlatImage {
    int width;
    int height;
    Uint16 *pixels;                     /* 5-5-5 with an alpha each (no alpha: all opaque), or */
    Uint8 *alpha;
    Uint32 *argb;                       /* premultiplied ARGB, the other two NULL */
    SDL_Texture *textures[kTexelKinds]; /* NULL: to be made (again), when first drawn */
    bool dead;                          /* destroyed by the game: freed once no kept view draws it */
    PlatImage *prev;
    PlatImage *next;
};
static PlatImage *gImages;              /* every image, for a brightness change or a lost device */
static int gDeadImages;
static void DropImageTextures(PlatImage *image);
static void DropOrBlend(void);

struct PlatTarget {
    int width;
    int height;
    SDL_Texture *texture;               /* NULL: to be made (again), clear, when first used */
    bool drawn;                         /* holds the game's drawing: not when new or after its pixels' loss */
    bool dead;                          /* destroyed by the game: freed once no kept view draws it */
    PlatTarget *prev;
    PlatTarget *next;
};
static PlatTarget *gTargets;            /* every target, for a lost device */
static int gDeadTargets;
static SDL_BlendMode gMaxBlend = SDL_BLENDMODE_INVALID;    /* kPlatBlendMax's; invalid: made by hand */
static PlatTarget *gMaxScratch;         /* where a hand-made kPlatBlendMax draw is drawn first */
static unsigned gUIEpoch;               /* PlatUIEpoch */
static void DropTargetTextures(void);
static void FreeDeadTargets(void);
static void FreeTarget(PlatTarget *target);

static int gButtons;                    /* mouse buttons held, a bit each */
static SDL_Scancode gSwallowedKey;      /* the Enter of Alt+Enter: the game sees neither its press nor its release */
static bool gCapsLockPressed;           /* a Caps Lock key-down not yet asked for */
static bool gActive;                    /* the game was last told the window has the keyboard */

#define kPullFrames     1024            /* asked of a pull function at a time */
#define kBytesPerFrame  4               /* 16-bit stereo */
#define kSeconds1904To1970 2082844800   /* the Mac clock starts in 1904, SDL's in 1970 */

struct PlatAudio {
    SDL_AudioStream *stream;            /* NULL once the layer has shut down */
    PlatAudioPull pull;
    void *ctx;
    SDL_AtomicInt ended;                /* the pull returned 0 */
    PlatAudio *next;
};

struct PlatVoices {
    SDL_AudioDeviceID device;           /* 0 offline, or once the layer has shut down */
    int count;
    float gain;
    SDL_AudioStream **streams;          /* NULL entries once the layer has shut down */
    Uint64 *deadlines;                  /* device: when each voice counts as ended even if SDL still holds frames */
    float *work;                        /* offline: one voice's frames */
    int workFrames;
    PlatVoices *next;
};

static bool gAudioUp;
static PlatAudio *gAudioOutputs;        /* the open ones, for PlatQuit */
static PlatVoices *gVoicePools;         /* likewise */
/* for each focus loss still in SDL's queue, in order: it went to another window of this program */
static bool gFocusNote[kFocusNotes];
static unsigned gFocusNoteHead;
static unsigned gFocusNoteTail;

/* -- start-up ------------------------------------------------------------------------------------- */

int PlatInit(void)
{
    /* the click that brings the window to the front is a click in the game too, as a Win32 window gets it */
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return -1;
    gAudioUp = SDL_InitSubSystem(SDL_INIT_AUDIO);       /* the game runs without sound, too */
    return 0;
}

static void DropVoices(PlatVoices *v)
{
    int i;

    for (i = 0; i < v->count; i++) {
        SDL_DestroyAudioStream(v->streams[i]);
        v->streams[i] = NULL;
    }
    if (v->device != 0)
        SDL_CloseAudioDevice(v->device);
    v->device = 0;
}

void PlatQuit(void)
{
    PlatAudio *s;
    PlatVoices *v;

    /* outputs and voices the game still holds lose their streams here: closing them later only frees the record */
    for (s = gAudioOutputs; s != NULL; s = s->next) {
        SDL_DestroyAudioStream(s->stream);
        s->stream = NULL;
    }
    for (v = gVoicePools; v != NULL; v = v->next)
        DropVoices(v);
    gAudioUp = false;
    PlatVideoClose();
    for (PlatImage *image = gImages; image != NULL; image = image->next)
        DropImageTextures(image);
    if (gMaxScratch != NULL)
        FreeTarget(gMaxScratch);
    gMaxScratch = NULL;
    DropTargetTextures();
    DropOrBlend();
    if (gRenderer != NULL)
        SDL_DestroyRenderer(gRenderer);
    if (gWindow != NULL)
        SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;
    SDL_Quit();
}

/* -- video ---------------------------------------------------------------------------------------- */

static void DrawOverlays(void);

static Uint32 WhiteAlpha(void);

/* the frame last shown, as the window is now: the view, its overlays, the layer, the white-out */
static void Compose(void)
{
    SDL_Texture *frame = gLayerTexture;
    SDL_FRect all = { 0, 0, (float)gFrameWidth, (float)gFrameHeight };
    SDL_FRect edgeFrom;
    SDL_FRect edgeTo;

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);
    if (frame != NULL && gFrameDrawn) {
        /* over black alone the layer's colours are the frame's as they are */
        SDL_SetTextureBlendMode(frame, SDL_BLENDMODE_NONE);
        if (gViewOn && gViewTexture != NULL) {
            SDL_FRect shown;
            shown.w = gViewWidth / gViewZoom;
            shown.h = gViewHeight / gViewZoom;
            shown.x = (gViewWidth - shown.w) / 2;
            shown.y = (gViewHeight - shown.h) / 2;
            SDL_RenderTexture(gRenderer, gViewTexture, &shown, &gViewRect);
            /* the view's last column a little past its edge: where the window scales the frame, the frame's
               edge beside the view is blended over this, not over black */
            edgeFrom.x = shown.x + shown.w - 1;
            edgeFrom.y = shown.y;
            edgeFrom.w = 1;
            edgeFrom.h = shown.h;
            edgeTo.x = gViewRect.x + gViewRect.w;
            edgeTo.y = gViewRect.y;
            edgeTo.w = kViewEdge;
            edgeTo.h = gViewRect.h;
            SDL_RenderTexture(gRenderer, gViewTexture, &edgeFrom, &edgeTo);
            DrawOverlays();
            SDL_SetTextureBlendMode(frame, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        }
        SDL_RenderTexture(gRenderer, frame, NULL, NULL);
        if (WhiteAlpha() > 0) {
            SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, (Uint8)WhiteAlpha());
            SDL_RenderFillRect(gRenderer, &all);
        }
    }
}

static void Redraw(void)
{
    Uint64 start;

    if (gRenderer == NULL)
        return;
    start = SDL_GetTicksNS();
    Compose();
    SDL_RenderPresent(gRenderer);
    gTimes.renderNs += SDL_GetTicksNS() - start;
}

/* ns until another frame may be shown; 0 = now */
static Uint64 FrameDueIn(void)
{
    Uint64 now;
    Uint64 due;

    if (!gFrameDrawn)
        return 0;
    now = SDL_GetTicksNS();
    due = gShowStart + gShowInterval;
    if (due < gShowEnd + gShowRest)
        due = gShowEnd + gShowRest;
    return due > now ? due - now : 0;
}

/* the brightness: a channel shown as its value to the power gExponent */
#define kChannelSteps   32
#define kChannelTop     31

static bool gChannelsMade;
static double gExponent = 1.0;
static Uint32 gPlainChannel[kChannelSteps];     /* 5-bit channels through the brightness, as 8 bits */
static Uint8 gDeepChannel[256];         /* likewise 8-bit channels */
static Uint8 gDrawnChannel[256];        /* and back: the channel drawn that is shown nearest to a value */
static unsigned gWhite;

/* a 5-bit channel value as 8 bits, the top bits repeated, as a GPU widens 5 bits */
static Uint32 CodeOf(Uint32 v)
{
    return (v << 3) | (v >> 2);
}

/* a 5-bit channel value as 8 bits through the brightness */
static Uint32 ChannelOf(Uint32 v)
{
    if (gExponent == 1.0)
        return CodeOf(v);
    return (Uint32)(SDL_pow((double)v / kChannelTop, gExponent) * 255.0 + 0.5);
}

/* the white-out as the alpha of white laid over everything, 8 bits */
static Uint32 WhiteAlpha(void)
{
    return gWhite >= 256 ? 255 : gWhite;
}

static void MakeChannels(void)
{
    Uint32 c;
    Uint32 drawn = 0;

    for (c = 0; c < kChannelSteps; c++)
        gPlainChannel[c] = ChannelOf(c);
    for (c = 0; c < 256; c++)
        gDeepChannel[c] = gExponent == 1.0 ? (Uint8)c : (Uint8)(SDL_pow(c / 255.0, gExponent) * 255.0 + 0.5);
    /* the table rises: for each shown value the drawn one whose own is nearest */
    for (c = 0; c < 256; c++) {
        while (drawn < 255
               && SDL_abs((int)gDeepChannel[drawn + 1] - (int)c) <= SDL_abs((int)gDeepChannel[drawn] - (int)c))
            drawn++;
        gDrawnChannel[c] = (Uint8)drawn;
    }
    gChannelsMade = true;
    /* what the view's texture holds is in the old brightness */
    gViewFresh = gViewOn;
}

static void DropImageTextures(PlatImage *image)
{
    for (SDL_Texture *&texture : image->textures) {
        if (texture != NULL)
            SDL_DestroyTexture(texture);
        texture = NULL;
    }
}

static void SetBrightRamp(void);

void PlatVideoSetBrightness(double exponent)
{
    PlatImage *image;

    if (exponent != gExponent)
        gUIEpoch++;                     /* the targets hold another brightness */
    gExponent = exponent;
    MakeChannels();
    for (image = gImages; image != NULL; image = image->next)
        DropImageTextures(image);
    SetBrightRamp();
}

void PlatVideoSetWhite(unsigned level)
{
    gWhite = level > 256 ? 256 : level;
}

/* -- the GPU view: images, and the draws rendered into gViewTexture ---------------------------------- */

/* a 5-5-5 colour as ARGB through the brightness alone: the white-out is laid over the view whole */
static Uint32 PlainColor(Uint16 color)
{
    return 0xff000000u | gPlainChannel[(color >> 10) & 0x1f] << 16 | gPlainChannel[(color >> 5) & 0x1f] << 8
           | gPlainChannel[color & 0x1f];
}

static Uint32 CodeColor(Uint16 color)
{
    return 0xff000000u | CodeOf((color >> 10) & 0x1f) << 16 | CodeOf((color >> 5) & 0x1f) << 8 | CodeOf(color & 0x1f);
}

/* The view is drawn in 5-bit codes and shown through the brightness after it (kPlatDrawOr reads codes); at
   the default brightness the two are the same. */
static bool ViewInCodes(void)
{
    return gOrShader != NULL && gExponent != 1.0;
}

/* a premultiplied texel through a channel table: its colour at its alpha, what is over the alpha as added light */
static Uint32 MapTexel(Uint32 texel, const Uint8 *channel)
{
    Uint32 a = texel >> 24;
    Uint32 out = texel & 0xff000000u;

    for (int shift = 0; shift <= 16; shift += 8) {
        Uint32 c = texel >> shift & 0xff;
        if (c >= a)
            c = SDL_min(a + channel[c - a], 255u);
        else
            c = (channel[(c * 255 + a / 2) / a] * a + 127) / 255;
        out |= c << shift;
    }
    return out;
}

/* a premultiplied texel through the brightness */
static Uint32 DeepTexel(Uint32 texel)
{
    return gExponent == 1.0 ? texel : MapTexel(texel, gDeepChannel);
}

/* an image's texels of one kind are its kept pixels as they are */
static bool TexelsAsKept(const PlatImage *image, Texels kind)
{
    return image->argb != NULL && (kind == kTexelsCodes || (kind == kTexelsShown && gExponent == 1.0));
}

/* an image's texels of one kind in rect, row after row */
static void ImageTexels(const PlatImage *image, Texels kind, const SDL_Rect &rect, Uint32 *out)
{
    Uint32 last = 0xffffffffu;
    Uint32 lastColor = 0;

    for (int y = 0; y < rect.h; y++) {
        size_t at = (size_t)(rect.y + y) * image->width + rect.x;
        for (int x = 0; x < rect.w; x++, at++, out++) {
            if (image->argb != NULL) {
                Uint32 texel = image->argb[at];
                *out = kind == kTexelsSilhouette ? (texel >> 24) * 0x01010101u
                       : kind == kTexelsCodes ? texel : DeepTexel(texel);
            } else if (image->alpha != NULL && image->alpha[at] == 0) {
                *out = 0;
            } else if (kind == kTexelsSilhouette) {
                *out = 0xffffffffu;
            } else {
                if (image->pixels[at] != last) {
                    last = image->pixels[at];
                    lastColor = kind == kTexelsCodes ? CodeColor((Uint16)last) : PlainColor((Uint16)last);
                }
                *out = lastColor;
            }
        }
    }
}

/* the image's texels of one kind in rect into its texture of that kind */
static void UploadTexels(const PlatImage *image, Texels kind, const SDL_Rect &rect)
{
    static std::vector<Uint32> texels;

    if (TexelsAsKept(image, kind)) {
        SDL_UpdateTexture(image->textures[kind], &rect, image->argb + (size_t)rect.y * image->width + rect.x,
                          image->width * 4);
        return;
    }
    texels.resize((size_t)rect.w * rect.h);
    ImageTexels(image, kind, rect, texels.data());
    SDL_UpdateTexture(image->textures[kind], &rect, texels.data(), rect.w * 4);
    SpareScratch(texels);
}

static SDL_Texture *ImageTexture(PlatImage *image, Texels kind)
{
    SDL_Texture **slot = &image->textures[kind];
    SDL_Rect all = { 0, 0, image->width, image->height };

    if (*slot != NULL)
        return *slot;
    *slot = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, image->width,
                              image->height);
    if (*slot == NULL)
        return NULL;
    UploadTexels(image, kind, all);
    SDL_SetTextureScaleMode(*slot, SDL_SCALEMODE_NEAREST);
    return *slot;
}

static void FreeImage(PlatImage *image);

/* an image of that size among gImages, its pixels yet to come */
static PlatImage *NewImage(int width, int height)
{
    PlatImage *image;

    if (width <= 0 || height <= 0)
        return NULL;
    image = (PlatImage *)SDL_calloc(1, sizeof(PlatImage));
    if (image == NULL)
        return NULL;
    image->width = width;
    image->height = height;
    image->next = gImages;
    if (gImages != NULL)
        gImages->prev = image;
    gImages = image;
    return image;
}

PlatImage *PlatImageCreate(int width, int height, const uint16_t *pixels, const uint8_t *alpha)
{
    PlatImage *image = NewImage(width, height);
    size_t count = (size_t)width * height;

    if (image == NULL)
        return NULL;
    image->pixels = (Uint16 *)SDL_calloc(count, sizeof(Uint16));
    image->alpha = alpha != NULL ? (Uint8 *)SDL_malloc(count) : NULL;
    if (image->pixels == NULL || (alpha != NULL && image->alpha == NULL)) {
        FreeImage(image);
        return NULL;
    }
    if (pixels != NULL)
        SDL_memcpy(image->pixels, pixels, count * sizeof(Uint16));
    if (alpha != NULL)
        SDL_memcpy(image->alpha, alpha, count);
    return image;
}

PlatImage *PlatImageCreate32(int width, int height, const uint32_t *argb, int pitch)
{
    PlatImage *image = NewImage(width, height);

    if (image == NULL)
        return NULL;
    image->argb = (Uint32 *)SDL_calloc((size_t)width * height, sizeof(Uint32));
    if (image->argb == NULL) {
        FreeImage(image);
        return NULL;
    }
    PlatImageUpdate32(image, NULL, argb, pitch);
    return image;
}

/* the kept pixels of the rect and that part of each texture made so far; nothing for a rect off the image */
void PlatImageUpdate32(PlatImage *image, const PlatRect *rect, const uint32_t *argb, int pitch)
{
    SDL_Rect box;

    if (image == NULL || image->argb == NULL || argb == NULL)
        return;
    box = { 0, 0, image->width, image->height };
    if (rect != NULL) {
        if (rect->x < 0 || rect->y < 0 || rect->w <= 0 || rect->h <= 0 || rect->w > image->width - rect->x
                || rect->h > image->height - rect->y)
            return;
        box = { rect->x, rect->y, rect->w, rect->h };
    }
    for (int y = 0; y < box.h; y++)
        SDL_memcpy(image->argb + (size_t)(box.y + y) * image->width + box.x, (const char *)argb + (size_t)y * pitch,
                   (size_t)box.w * sizeof(Uint32));
    for (int kind = 0; kind < kTexelKinds; kind++) {
        if (image->textures[kind] != NULL)
            UploadTexels(image, (Texels)kind, box);
    }
}

void PlatImageUpdate(PlatImage *image, const uint16_t *pixels, const uint8_t *alpha)
{
    size_t count;

    if (image == NULL || image->pixels == NULL || image->alpha == NULL)
        return;
    count = (size_t)image->width * image->height;
    bool made = false;
    for (SDL_Texture *texture : image->textures)
        made = made || texture != NULL;
    if (made && SDL_memcmp(image->pixels, pixels, count * sizeof(Uint16)) == 0
            && SDL_memcmp(image->alpha, alpha, count) == 0)
        return;
    SDL_memcpy(image->pixels, pixels, count * sizeof(Uint16));
    SDL_memcpy(image->alpha, alpha, count);
    DropImageTextures(image);
}

/* the rows of the rect that differ taken over, and their span uploaded into the textures that show colours */
int PlatImageUpdatePart(PlatImage *image, const PlatRect *rect, const uint16_t *pixels, int pitch)
{
    SDL_Rect changed = { 0, 0, 0, 0 };
    int left;
    int right;

    if (image == NULL || image->pixels == NULL || rect == NULL || pixels == NULL || rect->x < 0 || rect->y < 0
            || rect->w <= 0 || rect->h <= 0 || rect->w > image->width - rect->x || rect->h > image->height - rect->y)
        return 0;
    left = rect->w;
    right = 0;
    for (int y = 0; y < rect->h; y++) {
        const Uint16 *now = (const Uint16 *)((const char *)pixels + (size_t)y * pitch);
        Uint16 *kept = image->pixels + (size_t)(rect->y + y) * image->width + rect->x;
        int l = 0;
        int r = rect->w;
        if (SDL_memcmp(now, kept, (size_t)rect->w * sizeof(Uint16)) == 0)
            continue;
        while (now[l] == kept[l])
            l++;
        while (now[r - 1] == kept[r - 1])
            r--;
        SDL_memcpy(kept + l, now + l, (size_t)(r - l) * sizeof(Uint16));
        left = SDL_min(left, l);
        right = SDL_max(right, r);
        if (changed.h == 0)
            changed.y = rect->y + y;
        changed.h = rect->y + y + 1 - changed.y;
    }
    if (changed.h == 0)
        return 0;
    changed.x = rect->x + left;
    changed.w = right - left;
    for (int kind = 0; kind < kTexelKinds; kind++) {
        if (image->textures[kind] != NULL && kind != kTexelsSilhouette)
            UploadTexels(image, (Texels)kind, changed);
    }
    return 1;
}

static bool ImageInUse(const PlatImage *image)
{
    for (const PlatDraw &d : gDraws)
        if (d.image == image)
            return true;
    for (const PlatDraw &d : gOverlays)
        if (d.image == image)
            return true;
    return false;
}

static void FreeImage(PlatImage *image)
{
    if (image->prev != NULL)
        image->prev->next = image->next;
    else
        gImages = image->next;
    if (image->next != NULL)
        image->next->prev = image->prev;
    if (image->dead)
        gDeadImages--;
    DropImageTextures(image);
    SDL_free(image->pixels);
    SDL_free(image->alpha);
    SDL_free(image->argb);
    SDL_free(image);
}

void PlatImageDestroy(PlatImage *image)
{
    if (image == NULL)
        return;
    if (ImageInUse(image)) {
        image->dead = true;
        gDeadImages++;
    } else {
        FreeImage(image);
    }
}

/* images the game destroyed that no kept view draws any more */
static void FreeDeadImages(void)
{
    PlatImage *image;
    PlatImage *next;

    if (gDeadImages == 0)
        return;
    for (image = gImages; image != NULL; image = next) {
        next = image->next;
        if (image->dead && !ImageInUse(image))
            FreeImage(image);
    }
}

/* -- the GPU view's OR blend (kPlatDrawOr), on SDL's GPU renderer ---------------------------------------- */

/* a shader in one of the formats shipped (shaders/build-shaders.sh) */
struct ShaderBlob {
    SDL_GPUShaderFormat format;
    const unsigned char *code;
    size_t size;
    const char *entry;
};
#include "shaders/viewshaders.h"

#define kOrKeepBits     0x3ff           /* the low 10 bits of a factor: all the pair math sees of it */
#define kOrNoTint       32

/* PlatOrBlend on one pixel, as platform.h gives it */
static Uint16 OrPixel(Uint16 src, Uint16 dst, const PlatOrBlend &blend)
{
    auto scale = [](Uint32 v, Uint32 k) { return (v * (k & kOrKeepBits) >> 5) & 0x1f; };
    Uint32 out = 0;

    for (int c = 0; c < 3; c++) {
        int shift = 10 - 5 * c;
        Uint32 s = src >> shift & 0x1f;
        if (blend.own < kOrNoTint)
            s = scale(s, blend.own) | scale(blend.tint >> shift & 0x1f, kOrNoTint - blend.own);
        out |= (scale(s, blend.weight[c]) | (blend.over[c] & 0x1fu) | scale(dst >> shift & 0x1f, blend.keep)) << shift;
    }
    return (Uint16)out;
}

/* the draw reads the view under it: a keep of 0 or 1 scales every channel to 0 */
static bool ReadsView(const PlatDraw &d)
{
    return d.kind == kPlatDrawOr && (d.orBlend.keep & kOrKeepBits) > 1;
}

/* PlatOrBlend as whole numbers in the vertex colour, as orblend.frag reads them */
static void SetOrBlend(SDL_Texture *texture, const PlatOrBlend &blend)
{
    Uint32 own = blend.own < kOrNoTint ? blend.own : kOrNoTint;

    SDL_SetTextureColorModFloat(texture,
                                (float)((blend.weight[0] & 0x3f) | (blend.weight[1] & 0x3f) << 6 | (blend.weight[2] & 0x3f) << 12),
                                (float)((blend.over[0] & 0x1f) << 10 | (blend.over[1] & 0x1f) << 5 | (blend.over[2] & 0x1f)),
                                (float)(blend.tint & 0x7fff));
    SDL_SetTextureAlphaModFloat(texture, (float)((blend.keep & kOrKeepBits) | own << 10));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
}

/* the brightness's 8-bit levels into the brightness shader */
static void SetBrightRamp(void)
{
    float level[kChannelSteps];

    if (gBrightState == NULL)
        return;
    for (Uint32 c = 0; c < kChannelSteps; c++)
        level[c] = ChannelOf(c) / 255.0f;
    SDL_SetGPURenderStateFragmentUniforms(gBrightState, 0, level, sizeof level);
}

static SDL_Texture *MakeTarget(int width, int height, SDL_ScaleMode mode)
{
    SDL_Texture *texture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width,
                                             height);

    if (texture != NULL) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        if (!SDL_SetTextureScaleMode(texture, mode))
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }
    return texture;
}

/* the OR shader's state, reading the view from `copy' */
static SDL_GPURenderState *MakeOrState(SDL_Texture *copy)
{
    SDL_GPUTextureSamplerBinding binding;
    SDL_GPURenderStateCreateInfo info;

    binding.texture = (SDL_GPUTexture *)SDL_GetPointerProperty(SDL_GetTextureProperties(copy),
                                                               SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL);
    binding.sampler = gNearest;
    if (binding.texture == NULL) {
        SDL_SetError("the view's copy has no GPU texture");
        return NULL;
    }
    SDL_zero(info);
    info.fragment_shader = gOrShader;
    info.num_sampler_bindings = 1;
    info.sampler_bindings = &binding;
    return SDL_CreateGPURenderState(gRenderer, &info);
}

/* the view's copy and the codes target dropped, made again by the next RenderView */
static void DropOrTargets(void)
{
    if (gOrState != NULL)
        SDL_DestroyGPURenderState(gOrState);
    gOrState = NULL;
    if (gViewCopy != NULL)
        SDL_DestroyTexture(gViewCopy);
    gViewCopy = NULL;
    if (gViewCodes != NULL)
        SDL_DestroyTexture(gViewCodes);
    gViewCodes = NULL;
}

/* kPlatDrawOr off: what it was drawn with released */
static void DropOrBlend(void)
{
    SDL_GPUDevice *device = gRenderer != NULL ? SDL_GetGPURendererDevice(gRenderer) : NULL;

    DropOrTargets();
    if (gBrightState != NULL)
        SDL_DestroyGPURenderState(gBrightState);
    gBrightState = NULL;
    if (device != NULL) {
        if (gOrShader != NULL)
            SDL_ReleaseGPUShader(device, gOrShader);
        if (gBrightShader != NULL)
            SDL_ReleaseGPUShader(device, gBrightShader);
        if (gNearest != NULL)
            SDL_ReleaseGPUSampler(device, gNearest);
    }
    gOrShader = NULL;
    gBrightShader = NULL;
    gNearest = NULL;
}

/* the shader from its first blob the device takes */
static SDL_GPUShader *MakeShader(SDL_GPUDevice *device, const ShaderBlob *blobs, size_t count, Uint32 samplers,
                                 Uint32 uniforms)
{
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderCreateInfo info;

    for (size_t i = 0; i < count; i++) {
        if ((blobs[i].format & formats) == 0)
            continue;
        SDL_zero(info);
        info.code = blobs[i].code;
        info.code_size = blobs[i].size;
        info.entrypoint = blobs[i].entry;
        info.format = blobs[i].format;
        info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        info.num_samplers = samplers;
        info.num_uniform_buffers = uniforms;
        return SDL_CreateGPUShader(device, &info);
    }
    SDL_SetError("no shader in a format the GPU device takes");
    return NULL;
}

/* One pixel through both shaders and read back: false when it is not OrPixel's (a draw whose pipeline cannot
   be made is left out without an error). */
static bool OrSelfTest(void)
{
    const Uint16 dst = 20 << 10 | 9 << 5 | 31;
    const Uint16 src = 7 << 10 | 30 << 5 | 1;
    const PlatOrBlend blend = { { 32, 20, 32 }, { 0, 0, 5 }, 20, 3 << 10 | 17 << 5 | 25, 24 };
    Uint16 want = OrPixel(src, dst, blend);
    SDL_Texture *view = MakeTarget(1, 1, SDL_SCALEMODE_NEAREST);
    SDL_Texture *copy = MakeTarget(1, 1, SDL_SCALEMODE_NEAREST);
    SDL_Texture *shown = MakeTarget(1, 1, SDL_SCALEMODE_NEAREST);
    SDL_Texture *image = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
    SDL_GPURenderState *state = copy != NULL ? MakeOrState(copy) : NULL;
    bool passed = false;

    if (view != NULL && shown != NULL && image != NULL && state != NULL) {
        Uint32 texel = CodeColor(src);
        Uint32 under = CodeColor(dst);
        float level[kChannelSteps];
        SDL_Surface *pixel;
        Uint8 r = 0, g = 0, b = 0, a = 0;

        SDL_UpdateTexture(image, NULL, &texel, 4);
        SDL_SetTextureScaleMode(image, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderTarget(gRenderer, view);
        SDL_SetRenderDrawColor(gRenderer, (Uint8)(under >> 16), (Uint8)(under >> 8), (Uint8)under, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gRenderer);
        SDL_SetRenderTarget(gRenderer, copy);
        SDL_RenderTexture(gRenderer, view, NULL, NULL);
        SDL_SetRenderTarget(gRenderer, view);
        SetOrBlend(image, blend);
        SDL_SetGPURenderState(gRenderer, state);
        SDL_RenderTexture(gRenderer, image, NULL, NULL);
        SDL_SetGPURenderState(gRenderer, NULL);
        /* through the brightness shader with each level at 8 times its value */
        for (Uint32 c = 0; c < kChannelSteps; c++)
            level[c] = c * 8 / 255.0f;
        SDL_SetGPURenderStateFragmentUniforms(gBrightState, 0, level, sizeof level);
        SDL_SetRenderTarget(gRenderer, shown);
        SDL_SetGPURenderState(gRenderer, gBrightState);
        SDL_RenderTexture(gRenderer, view, NULL, NULL);
        SDL_SetGPURenderState(gRenderer, NULL);
        pixel = SDL_RenderReadPixels(gRenderer, NULL);
        passed = pixel != NULL && SDL_ReadSurfacePixel(pixel, 0, 0, &r, &g, &b, &a)
                 && r == (want >> 10 & 0x1f) * 8 && g == (want >> 5 & 0x1f) * 8 && b == (want & 0x1f) * 8;
        if (pixel != NULL && !passed)
            SDL_SetError("the shaders' test pixel came out %02x%02x%02x", r, g, b);
        SDL_DestroySurface(pixel);
        SDL_SetRenderTarget(gRenderer, NULL);
    }
    if (state != NULL)
        SDL_DestroyGPURenderState(state);
    for (SDL_Texture *texture : { view, copy, shown, image })
        if (texture != NULL)
            SDL_DestroyTexture(texture);
    return passed;
}

/* SDL's GPU renderer for the window, its device asked only for the shader formats shipped, with the shaders
   and a passed self-test; NULL when any of it fails, the reason in SDL_GetError */
static SDL_Renderer *MakeOrRenderer(void)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_GPURenderStateCreateInfo info;
    SDL_GPUSamplerCreateInfo nearest;
    SDL_GPUDevice *device;
    char why[256];

    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, gWindow);
    SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, "gpu");
    for (const ShaderBlob &blob : kOrBlendShader) {
        if (blob.format == SDL_GPU_SHADERFORMAT_SPIRV)
            SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
        else if (blob.format == SDL_GPU_SHADERFORMAT_DXIL)
            SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);
        else if (blob.format == SDL_GPU_SHADERFORMAT_MSL)
            SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN, true);
    }
    gRenderer = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);
    if (gRenderer == NULL)
        return NULL;
    device = SDL_GetGPURendererDevice(gRenderer);
    SDL_zero(nearest);
    nearest.min_filter = nearest.mag_filter = SDL_GPU_FILTER_NEAREST;
    nearest.address_mode_u = nearest.address_mode_v = nearest.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    gNearest = device != NULL ? SDL_CreateGPUSampler(device, &nearest) : NULL;
    gOrShader = gNearest != NULL ? MakeShader(device, kOrBlendShader, SDL_arraysize(kOrBlendShader), 2, 0) : NULL;
    gBrightShader = gOrShader != NULL ? MakeShader(device, kBrightnessShader, SDL_arraysize(kBrightnessShader), 1, 1)
                                      : NULL;
    if (gBrightShader != NULL) {
        SDL_zero(info);
        info.fragment_shader = gBrightShader;
        gBrightState = SDL_CreateGPURenderState(gRenderer, &info);
    }
    if (gBrightState != NULL && OrSelfTest()) {
        SetBrightRamp();
        return gRenderer;
    }
    SDL_strlcpy(why, SDL_GetError(), sizeof why);
    DropOrBlend();
    SDL_DestroyRenderer(gRenderer);
    gRenderer = NULL;
    SDL_SetError("%s", why);
    return NULL;
}

/* the renderer: for kPlatDrawOr SDL's GPU renderer, else (or when that cannot be made) SDL's default */
static SDL_Renderer *MakeRenderer(void)
{
    if (gOrAsked) {
        if (MakeOrRenderer() != NULL) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "original blending on: SDL's GPU renderer, %s",
                        SDL_GetGPUDeviceDriver(SDL_GetGPURendererDevice(gRenderer)));
            return gRenderer;
        }
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "original blending not available, sprites added instead: %s",
                    SDL_GetError());
    }
    return SDL_CreateRenderer(gWindow, NULL);
}

/* kPlatBlendMax as the renderer's per-channel max, or made by hand where it has none (software, GLES2 w/o minmax) */
static void FindMaxBlend(void)
{
    const SDL_BlendFactor one = SDL_BLENDFACTOR_ONE;
    SDL_BlendMode max = SDL_ComposeCustomBlendMode(one, one, SDL_BLENDOPERATION_MAXIMUM, one, one,
                                                   SDL_BLENDOPERATION_MAXIMUM);

    gMaxBlend = SDL_SetRenderDrawBlendMode(gRenderer, max) ? max : SDL_BLENDMODE_INVALID;
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "max blend: %s",
                gMaxBlend != SDL_BLENDMODE_INVALID ? "the renderer's" : "made by hand");
}

void PlatVideoAskOrBlend(int on)
{
    gOrAsked = on != 0;
}

int PlatVideoOrBlend(void)
{
    return gOrShader != NULL;
}

/* the view's render targets dropped, made again by the next RenderView */
static void DropViewTextures(void)
{
    if (gViewTexture != NULL)
        SDL_DestroyTexture(gViewTexture);
    gViewTexture = NULL;
    DropOrTargets();
}

/* The view's render targets that are missing; false when the view's own cannot be made. When the OR blend's
   cannot, kPlatDrawOr stops (logged once) and the view is drawn without it. */
static bool MakeViewTextures(void)
{
    if (gViewTexture == NULL && (gViewTexture = MakeTarget(gViewWidth, gViewHeight, gScaleMode)) == NULL)
        return false;
    if (gOrShader == NULL)
        return true;
    if (gViewCopy == NULL)
        gViewCopy = MakeTarget(gViewWidth, gViewHeight, SDL_SCALEMODE_NEAREST);
    if (gOrState == NULL && gViewCopy != NULL)
        gOrState = MakeOrState(gViewCopy);
    if (ViewInCodes() && gViewCodes == NULL)
        gViewCodes = MakeTarget(gViewWidth, gViewHeight, SDL_SCALEMODE_NEAREST);
    if (gOrState == NULL || (ViewInCodes() && gViewCodes == NULL)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "original blending stopped, sprites added instead: %s",
                    SDL_GetError());
        DropOrBlend();
    }
    return true;
}

/* a draw's place in whole pixels within width x height, its vertices from `vertices' (empty for none) */
static SDL_Rect DrawBoxWithin(const PlatDraw &d, const PlatVertex *vertices, size_t vertexCount, int width,
                              int height)
{
    SDL_FRect place = { d.x, d.y, d.w, d.h };
    SDL_FRect within = { 0, 0, (float)width, (float)height };
    SDL_Rect box = { 0, 0, 0, 0 };

    if (d.kind == kPlatDrawTriangles) {
        if (d.first < 0 || d.count <= 0 || (size_t)d.first + d.count > vertexCount)
            return box;
        float left = vertices[(size_t)d.first].x;
        float top = vertices[(size_t)d.first].y;
        float right = left;
        float bottom = top;
        for (int i = 1; i < d.count; i++) {
            const PlatVertex &v = vertices[(size_t)d.first + i];
            left = SDL_min(left, v.x);
            top = SDL_min(top, v.y);
            right = SDL_max(right, v.x);
            bottom = SDL_max(bottom, v.y);
        }
        place = { left, top, right - left, bottom - top };
    }
    if (!SDL_GetRectIntersectionFloat(&place, &within, &place))
        return box;
    box.x = (int)SDL_floorf(place.x);
    box.y = (int)SDL_floorf(place.y);
    box.w = (int)SDL_ceilf(place.x + place.w) - box.x;
    box.h = (int)SDL_ceilf(place.y + place.h) - box.y;
    return box;
}

/* a draw's place in the view, in whole pixels, within the view (empty for none) */
static SDL_Rect DrawBox(const PlatDraw &d)
{
    return DrawBoxWithin(d, gVertices.data(), gVertices.size(), gViewWidth, gViewHeight);
}

/* Before a draw that reads the view at `box': the copy brought up to date where the view changed since it was
   last, when the box lies on such a part. Draws that do not overlap any change since read the same copy. */
static void CopyViewFor(const SDL_Rect &box, SDL_Texture *view)
{
    bool stale = false;

    for (const SDL_Rect &r : gChanged)
        stale = stale || SDL_HasRectIntersection(&r, &box);
    if (!stale)
        return;
    SDL_SetRenderTarget(gRenderer, gViewCopy);
    for (const SDL_Rect &r : gChanged) {
        SDL_FRect part = { (float)r.x, (float)r.y, (float)r.w, (float)r.h };
        SDL_RenderTexture(gRenderer, view, &part, &part);
    }
    SDL_SetRenderTarget(gRenderer, view);
    gChanged.clear();
}

/* -- targets: the interface's canvases ------------------------------------------------------------------- */

/* a target's texture with every pixel clear */
static void ClearTexture(SDL_Texture *texture)
{
    SDL_Texture *drawnInto = SDL_GetRenderTarget(gRenderer);

    SDL_SetRenderTarget(gRenderer, texture);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
    SDL_RenderClear(gRenderer);
    SDL_SetRenderTarget(gRenderer, drawnInto);
}

/* the target's texture, made clear when it is not there; NULL when it cannot be made */
static SDL_Texture *TargetTexture(PlatTarget *target)
{
    if (target->texture != NULL || gRenderer == NULL)
        return target->texture;
    target->texture = MakeTarget(target->width, target->height, SDL_SCALEMODE_NEAREST);
    if (target->texture != NULL)
        ClearTexture(target->texture);
    return target->texture;
}

/* every target's texture dropped, made again clear when the target is next used */
static void DropTargetTextures(void)
{
    for (PlatTarget *target = gTargets; target != NULL; target = target->next) {
        if (target->texture != NULL)
            SDL_DestroyTexture(target->texture);
        target->texture = NULL;
        target->drawn = false;
    }
}

/* the render targets lost their pixels: every target clear, as after a lost device */
static void ClearTargetTextures(void)
{
    for (PlatTarget *target = gTargets; target != NULL; target = target->next) {
        if (target->texture != NULL)
            ClearTexture(target->texture);
        target->drawn = false;
    }
}

static bool TargetInUse(const PlatTarget *target)
{
    for (const PlatDraw &d : gDraws)
        if (d.source == target)
            return true;
    for (const PlatDraw &d : gOverlays)
        if (d.source == target)
            return true;
    return false;
}

static void FreeTarget(PlatTarget *target)
{
    if (target->prev != NULL)
        target->prev->next = target->next;
    else
        gTargets = target->next;
    if (target->next != NULL)
        target->next->prev = target->prev;
    if (target->dead)
        gDeadTargets--;
    if (target->texture != NULL)
        SDL_DestroyTexture(target->texture);
    SDL_free(target);
}

PlatTarget *PlatTargetCreate(int width, int height)
{
    PlatTarget *target;
    int largest = PlatMaxTextureSize();

    if (width <= 0 || height <= 0)
        return NULL;
    if (largest > 0 && (width > largest || height > largest)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "no %d x %d target: the renderer's textures end at %d", width,
                     height, largest);
        return NULL;
    }
    target = (PlatTarget *)SDL_calloc(1, sizeof(PlatTarget));
    if (target == NULL)
        return NULL;
    target->width = width;
    target->height = height;
    target->next = gTargets;
    if (gTargets != NULL)
        gTargets->prev = target;
    gTargets = target;
    if (gRenderer != NULL && TargetTexture(target) == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "no %d x %d target: %s", width, height, SDL_GetError());
        FreeTarget(target);
        return NULL;
    }
    return target;
}

/* a target the kept view still draws lives on, as an image does, until another view replaces that one */
void PlatTargetDestroy(PlatTarget *target)
{
    if (target == NULL || target->dead)
        return;
    if (TargetInUse(target)) {
        target->dead = true;
        gDeadTargets++;
    } else {
        FreeTarget(target);
    }
}

/* targets the game destroyed that no kept view draws any more */
static void FreeDeadTargets(void)
{
    PlatTarget *target;
    PlatTarget *next;

    if (gDeadTargets == 0)
        return;
    for (target = gTargets; target != NULL; target = next) {
        next = target->next;
        if (target->dead && !TargetInUse(target))
            FreeTarget(target);
    }
}

void PlatTargetClear(PlatTarget *target)
{
    SDL_Texture *texture = target != NULL ? TargetTexture(target) : NULL;

    if (texture == NULL)
        return;
    ClearTexture(texture);
    target->drawn = true;
}

unsigned PlatUIEpoch(void)
{
    return gUIEpoch;
}

/* -- draws ------------------------------------------------------------------------------------------------ */

PlatDraw PlatDrawOf(int kind, int blend)
{
    PlatDraw d = {};

    d.kind = (unsigned char)kind;
    d.blend = (unsigned char)blend;
    d.alpha = 255;
    d.mod[0] = d.mod[1] = d.mod[2] = 255;
    return d;
}

/* where draws go: the vertices they index, how colours and images are shown there, what they are kept to */
struct DrawSink {
    const PlatVertex *vertices;
    size_t vertexCount;
    bool codes;                         /* in 5-bit codes, the brightness after them: the view under the OR blend */
    SDL_GPURenderState *orState;        /* kPlatDrawOr's; NULL: such a draw is a plain image */
    const PlatTarget *target;           /* the target drawn into; NULL: the view or the window */
    const SDL_Rect *clip;               /* NULL: none */
};

static Uint32 SinkColor(Uint16 color, const DrawSink &sink)
{
    return sink.codes ? CodeColor(color) : PlainColor(color);
}

/* a draw's colour as ARGB */
static Uint32 DrawColor(const PlatDraw &d, const DrawSink &sink)
{
    if (!d.deep)
        return SinkColor(d.color, sink);
    if (sink.codes)
        return 0xff000000u | (Uint32)d.rgb[0] << 16 | (Uint32)d.rgb[1] << 8 | d.rgb[2];
    return 0xff000000u | (Uint32)gDeepChannel[d.rgb[0]] << 16 | (Uint32)gDeepChannel[d.rgb[1]] << 8
           | gDeepChannel[d.rgb[2]];
}

static SDL_ScaleMode ScaleModeOf(int filter)
{
    if (filter == kPlatFilterNearest)
        return SDL_SCALEMODE_NEAREST;
    return filter == kPlatFilterPixelArt ? SDL_SCALEMODE_PIXELART : SDL_SCALEMODE_LINEAR;
}

/* a draw's texture -- its source target's, else its image's of that kind -- with its blend and filter, or NULL */
static SDL_Texture *DrawTexture(const PlatDraw &d, const DrawSink &sink, Texels kind)
{
    SDL_Texture *texture;
    bool deep;

    if (d.source != NULL) {
        SDL_assert(d.source != sink.target);
        /* a target holds the brightness already: not for the view in codes, which is shown through it */
        if (d.source == sink.target || kind == kTexelsSilhouette || sink.codes)
            return NULL;
        texture = TargetTexture(d.source);
        deep = true;
    } else {
        texture = d.image != NULL ? ImageTexture(d.image, kind) : NULL;
        deep = d.image != NULL && d.image->argb != NULL;
    }
    if (texture == NULL)
        return NULL;
    if (!SDL_SetTextureScaleMode(texture, ScaleModeOf(d.filter)))
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    if (d.blend == kPlatBlendAdd) {
        SDL_SetTextureAlphaMod(texture, 255);
        SDL_SetTextureBlendMode(texture, deep ? SDL_BLENDMODE_ADD_PREMULTIPLIED : SDL_BLENDMODE_ADD);
    } else {
        SDL_SetTextureAlphaMod(texture, d.alpha);
        if (d.blend == kPlatBlendCopy)
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        else if (d.blend == kPlatBlendMax)
            SDL_SetTextureBlendMode(texture, gMaxBlend);
        else
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
    }
    return texture;
}

/* a copy and a max are not blended by the alpha: their colour takes it in, staying premultiplied */
static bool AlphaInColor(int blend)
{
    return blend == kPlatBlendCopy || blend == kPlatBlendMax;
}

static Uint32 ColorShare(const PlatDraw &d)
{
    return AlphaInColor(d.blend) ? d.alpha : 255;
}

static void SetDrawColorMod(SDL_Texture *texture, const PlatDraw &d, Uint32 r, Uint32 g, Uint32 b)
{
    Uint32 share = ColorShare(d);

    SDL_SetTextureColorMod(texture, (Uint8)((r * share + 127) / 255), (Uint8)((g * share + 127) / 255),
                           (Uint8)((b * share + 127) / 255));
}

/* a fill's or a triangle's blend: its colour comes plain, and is premultiplied by the draw for a copy or a max */
static SDL_BlendMode PlainBlend(int blend)
{
    if (blend == kPlatBlendAdd)
        return SDL_BLENDMODE_ADD;
    if (blend == kPlatBlendMax)
        return gMaxBlend;
    return blend == kPlatBlendCopy ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND;
}

/* kPlatDrawTiled: whole tiles laid from the grid's lines left of and above the rect, cut to the rect by the clip */
static void RenderTiled(SDL_Texture *texture, const PlatDraw &d, const DrawSink &sink, const SDL_FRect *from)
{
    float scale = d.tileScale > 0 ? d.tileScale : 1.0f;
    SDL_FRect tile = { 0, 0, (float)texture->w, (float)texture->h };
    SDL_FRect laid;
    SDL_Rect area;
    float tileW;
    float tileH;

    /* the part within the texture, as SDL cuts it: the grid's period is that tile's size */
    if (from != NULL && !SDL_GetRectIntersectionFloat(from, &tile, &tile))
        return;
    tileW = tile.w * scale;
    tileH = tile.h * scale;
    area.x = (int)SDL_floorf(d.x);
    area.y = (int)SDL_floorf(d.y);
    area.w = (int)SDL_ceilf(d.x + d.w) - area.x;
    area.h = (int)SDL_ceilf(d.y + d.h) - area.y;
    if (sink.clip != NULL && !SDL_GetRectIntersection(&area, sink.clip, &area))
        return;
    if (area.w <= 0 || area.h <= 0 || tileW <= 0 || tileH <= 0)
        return;
    laid.x = d.tileX + SDL_floorf((d.x - d.tileX) / tileW) * tileW;
    laid.y = d.tileY + SDL_floorf((d.y - d.tileY) / tileH) * tileH;
    laid.w = SDL_ceilf((d.x + d.w - laid.x) / tileW) * tileW;
    laid.h = SDL_ceilf((d.y + d.h - laid.y) / tileH) * tileH;
    SDL_SetRenderClipRect(gRenderer, &area);
    SDL_RenderTextureTiled(gRenderer, texture, &tile, scale, &laid);
    SDL_SetRenderClipRect(gRenderer, sink.clip);
}

static void RenderMaxByHand(const PlatDraw &d, const DrawSink &sink, std::vector<SDL_Vertex> &scratch);

static void RenderDraw(const PlatDraw &d, const DrawSink &sink, std::vector<SDL_Vertex> &scratch)
{
    SDL_FRect rect = { d.x, d.y, d.w, d.h };
    SDL_FRect part = { d.sx, d.sy, d.sw, d.sh };
    const SDL_FRect *from = d.sw > 0 && d.sh > 0 ? &part : NULL;
    SDL_Texture *texture;
    Uint32 argb;
    Uint32 share;
    int i;

    if (d.blend == kPlatBlendMax && gMaxBlend == SDL_BLENDMODE_INVALID && d.kind != kPlatDrawOr) {
        RenderMaxByHand(d, sink, scratch);
        return;
    }
    switch (d.kind) {
    case kPlatDrawOr:
        texture = d.image != NULL ? ImageTexture(d.image, sink.orState != NULL ? kTexelsCodes : kTexelsShown) : NULL;
        if (texture == NULL)
            return;
        if (sink.orState == NULL) {
            /* no OR blend here: the image as it is */
            SDL_SetTextureColorMod(texture, 255, 255, 255);
            SDL_SetTextureAlphaMod(texture, 255);
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
            SDL_RenderTexture(gRenderer, texture, NULL, &rect);
            break;
        }
        SetOrBlend(texture, d.orBlend);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_SetGPURenderState(gRenderer, sink.orState);
        SDL_RenderTexture(gRenderer, texture, NULL, &rect);
        SDL_SetGPURenderState(gRenderer, NULL);
        break;
    case kPlatDrawImage:
    case kPlatDrawTiled:
        texture = DrawTexture(d, sink, sink.codes ? kTexelsCodes : kTexelsShown);
        if (texture == NULL)
            return;
        SetDrawColorMod(texture, d, d.mod[0], d.mod[1], d.mod[2]);
        if (d.kind == kPlatDrawTiled)
            RenderTiled(texture, d, sink, from);
        else
            SDL_RenderTexture(gRenderer, texture, from, &rect);
        break;
    case kPlatDrawSilhouette:
        texture = DrawTexture(d, sink, kTexelsSilhouette);
        if (texture == NULL)
            return;
        argb = DrawColor(d, sink);
        SetDrawColorMod(texture, d, ((argb >> 16) & 0xff) * d.mod[0] / 255, ((argb >> 8) & 0xff) * d.mod[1] / 255,
                        (argb & 0xff) * d.mod[2] / 255);
        SDL_RenderTexture(gRenderer, texture, from, &rect);
        break;
    case kPlatDrawFill:
        argb = DrawColor(d, sink);
        share = ColorShare(d);
        SDL_SetRenderDrawBlendMode(gRenderer, PlainBlend(d.blend));
        SDL_SetRenderDrawColor(gRenderer, (Uint8)((((argb >> 16) & 0xff) * share + 127) / 255),
                               (Uint8)((((argb >> 8) & 0xff) * share + 127) / 255),
                               (Uint8)(((argb & 0xff) * share + 127) / 255), d.alpha);
        SDL_RenderFillRect(gRenderer, &rect);
        break;
    case kPlatDrawTriangles:
        if (d.first < 0 || d.count <= 0 || (size_t)d.first + d.count > sink.vertexCount)
            return;
        scratch.resize((size_t)d.count);
        for (i = 0; i < d.count; i++) {
            const PlatVertex &v = sink.vertices[(size_t)d.first + i];
            float held = AlphaInColor(d.blend) ? v.alpha / 255.0f : 1.0f;
            argb = d.deep ? DrawColor(d, sink) : SinkColor(v.color, sink);
            scratch[i].position.x = v.x;
            scratch[i].position.y = v.y;
            scratch[i].color.r = ((argb >> 16) & 0xff) / 255.0f * held;
            scratch[i].color.g = ((argb >> 8) & 0xff) / 255.0f * held;
            scratch[i].color.b = (argb & 0xff) / 255.0f * held;
            scratch[i].color.a = v.alpha / 255.0f;
            scratch[i].tex_coord.x = 0;
            scratch[i].tex_coord.y = 0;
        }
        SDL_SetRenderDrawBlendMode(gRenderer, PlainBlend(d.blend));
        SDL_RenderGeometry(gRenderer, NULL, scratch.data(), d.count, NULL, 0);
        break;
    }
}

/* the render target's pixels in box as ARGB, or NULL */
static SDL_Surface *ReadRendered(const SDL_Rect &box)
{
    SDL_Surface *read = SDL_RenderReadPixels(gRenderer, &box);
    SDL_Surface *argb = read != NULL ? SDL_ConvertSurface(read, SDL_PIXELFORMAT_ARGB8888) : NULL;

    SDL_DestroySurface(read);
    if (argb != NULL && (argb->w != box.w || argb->h != box.h)) {
        SDL_DestroySurface(argb);
        argb = NULL;
    }
    return argb;
}

/* the hand-made max's scratch target, at least width x height */
static SDL_Texture *MaxScratch(int width, int height)
{
    if (gMaxScratch != NULL && (gMaxScratch->width < width || gMaxScratch->height < height)) {
        width = SDL_max(width, gMaxScratch->width);
        height = SDL_max(height, gMaxScratch->height);
        FreeTarget(gMaxScratch);
        gMaxScratch = NULL;
    }
    if (gMaxScratch == NULL)
        gMaxScratch = PlatTargetCreate(width, height);
    return gMaxScratch != NULL ? TargetTexture(gMaxScratch) : NULL;
}

/* kPlatBlendMax without the renderer's: the draw copied onto clear scratch, each channel's greater worked out here */
static void RenderMaxByHand(const PlatDraw &d, const DrawSink &sink, std::vector<SDL_Vertex> &scratch)
{
    static int said;
    static int saidWindow;
    SDL_Texture *into = SDL_GetRenderTarget(gRenderer);
    SDL_Texture *held;
    SDL_Surface *drawn;
    SDL_Surface *under;
    PlatDraw copy = d;
    DrawSink onto = sink;
    SDL_FRect area;
    SDL_Rect box;
    int width;
    int height;

    if (into == NULL) {
        PlatDebugLogOnce(&saidWindow, "platform: a max blend made by hand needs a target, it is not drawn");
        return;
    }
    if (!SDL_GetCurrentRenderOutputSize(gRenderer, &width, &height))
        return;
    box = DrawBoxWithin(d, sink.vertices, sink.vertexCount, width, height);
    if (sink.clip != NULL && !SDL_GetRectIntersection(&box, sink.clip, &box))
        return;
    if (box.w <= 0 || box.h <= 0)
        return;
    held = MaxScratch(width, height);
    if (held == NULL || !SDL_SetRenderTarget(gRenderer, held)) {
        PlatDebugLogOnce(&said, "platform: no scratch target for a max blend, it is not drawn");
        SDL_SetRenderTarget(gRenderer, into);
        return;
    }
    area = { (float)box.x, (float)box.y, (float)box.w, (float)box.h };
    SDL_SetRenderClipRect(gRenderer, &box);
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
    SDL_RenderFillRect(gRenderer, &area);
    copy.blend = kPlatBlendCopy;
    onto.clip = &box;
    RenderDraw(copy, onto, scratch);
    drawn = ReadRendered(box);
    SDL_SetRenderClipRect(gRenderer, NULL);
    SDL_SetRenderTarget(gRenderer, into);
    SDL_SetRenderClipRect(gRenderer, sink.clip);
    under = drawn != NULL ? ReadRendered(box) : NULL;
    if (under != NULL) {
        for (int y = 0; y < box.h; y++) {
            Uint8 *mine = (Uint8 *)drawn->pixels + (size_t)y * drawn->pitch;
            const Uint8 *there = (const Uint8 *)under->pixels + (size_t)y * under->pitch;
            for (int x = 0; x < box.w * 4; x++)
                mine[x] = SDL_max(mine[x], there[x]);
        }
        SDL_UpdateTexture(held, &box, drawn->pixels, drawn->pitch);
        SDL_SetTextureBlendMode(held, SDL_BLENDMODE_NONE);
        SDL_SetTextureColorMod(held, 255, 255, 255);
        SDL_SetTextureAlphaMod(held, 255);
        SDL_SetTextureScaleMode(held, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(gRenderer, held, &area, &area);
    }
    SDL_DestroySurface(drawn);
    SDL_DestroySurface(under);
}

void PlatTargetDraw(PlatTarget *target, const PlatDraw *draws, int drawCount, const PlatVertex *vertices,
                    int vertexCount, const PlatRect *clip)
{
    static std::vector<SDL_Vertex> scratch;
    SDL_Texture *texture = target != NULL ? TargetTexture(target) : NULL;
    SDL_Rect box = { 0, 0, 0, 0 };
    size_t given = vertices != NULL && vertexCount > 0 ? (size_t)vertexCount : 0;
    DrawSink sink = { vertices, given, false, NULL, target, NULL };

    if (texture == NULL || draws == NULL || !SDL_SetRenderTarget(gRenderer, texture))
        return;
    if (!gChannelsMade)
        MakeChannels();
    if (clip != NULL) {
        box = { clip->x, clip->y, SDL_max(clip->w, 0), SDL_max(clip->h, 0) };
        sink.clip = &box;
    }
    SDL_SetRenderClipRect(gRenderer, sink.clip);
    for (int i = 0; i < drawCount; i++)
        RenderDraw(draws[i], sink, scratch);
    SDL_SetRenderClipRect(gRenderer, NULL);
    SDL_SetRenderTarget(gRenderer, NULL);
    target->drawn = true;
}

int PlatTargetRead(PlatTarget *target, const PlatRect *rect, uint32_t *out)
{
    SDL_Texture *texture = target != NULL ? TargetTexture(target) : NULL;
    SDL_Surface *argb;
    SDL_Rect box;

    if (texture == NULL || out == NULL)
        return 0;
    box = { 0, 0, target->width, target->height };
    if (rect != NULL) {
        if (rect->x < 0 || rect->y < 0 || rect->w <= 0 || rect->h <= 0 || rect->w > target->width - rect->x
                || rect->h > target->height - rect->y)
            return 0;
        box = { rect->x, rect->y, rect->w, rect->h };
    }
    if (!SDL_SetRenderTarget(gRenderer, texture))
        return 0;
    argb = ReadRendered(box);
    SDL_SetRenderTarget(gRenderer, NULL);
    if (argb == NULL)
        return 0;
    for (int y = 0; y < box.h; y++)
        SDL_memcpy(out + (size_t)y * box.w, (const char *)argb->pixels + (size_t)y * argb->pitch,
                   (size_t)box.w * sizeof(Uint32));
    SDL_DestroySurface(argb);
    return 1;
}

int PlatTargetReadDrawn(PlatTarget *target, const PlatRect *rect, uint32_t *out)
{
    size_t count;

    if (PlatTargetRead(target, rect, out) == 0)
        return 0;
    if (gExponent == 1.0)
        return 1;
    if (!gChannelsMade)
        MakeChannels();
    count = rect != NULL ? (size_t)rect->w * rect->h : (size_t)target->width * target->height;
    for (size_t i = 0; i < count; i++)
        if (out[i] >> 24 != 0)
            out[i] = MapTexel(out[i], gDrawnChannel);
    return 1;
}

/* the GPU view's draws into its render target (in codes first when ViewInCodes) */
static bool RenderView(void)
{
    static std::vector<SDL_Vertex> scratch;
    SDL_Texture *view;
    bool levelled;

    if (!MakeViewTextures())
        return false;
    DrawSink sink = { gVertices.data(), gVertices.size(), ViewInCodes(), gOrState, NULL, NULL };
    view = ViewInCodes() ? gViewCodes : gViewTexture;
    levelled = gOrState != NULL;
    if (!SDL_SetRenderTarget(gRenderer, view))
        return false;
    if (levelled)
        SDL_SetTextureScaleMode(view, SDL_SCALEMODE_NEAREST);     /* the copies take it pixel for pixel */
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);
    gChanged.assign(1, SDL_Rect{ 0, 0, gViewWidth, gViewHeight });
    for (const PlatDraw &d : gDraws) {
        if (levelled) {
            SDL_Rect box = DrawBox(d);
            if (ReadsView(d))
                CopyViewFor(box, view);
            if (box.w > 0 && box.h > 0)
                gChanged.push_back(box);
        }
        RenderDraw(d, sink, scratch);
    }
    if (view != gViewTexture) {
        SDL_SetRenderTarget(gRenderer, gViewTexture);
        SDL_SetGPURenderState(gRenderer, gBrightState);
        SDL_RenderTexture(gRenderer, view, NULL, NULL);
        SDL_SetGPURenderState(gRenderer, NULL);
    } else if (levelled && !SDL_SetTextureScaleMode(gViewTexture, gScaleMode)) {
        SDL_SetTextureScaleMode(gViewTexture, SDL_SCALEMODE_LINEAR);
    }
    SDL_SetRenderTarget(gRenderer, NULL);
    gViewFresh = false;
    return true;
}

/* the overlays over the view, in the frame's pixels, as they are shown: the brightness in them */
static void DrawOverlays(void)
{
    std::vector<SDL_Vertex> scratch;
    DrawSink sink = { gVertices.data(), gVertices.size(), false, NULL, NULL, NULL };

    for (const PlatDraw &d : gOverlays)
        RenderDraw(d, sink, scratch);
}

/* the view no longer shown: the frame alone, black where it is clear */
static void DropView(void)
{
    gDraws.clear();
    gVertices.clear();
    gOverlays.clear();
    FreeDeadImages();
    FreeDeadTargets();
    gViewOn = false;
    gViewFresh = false;
}

static void DropLayerTexture(void)
{
    if (gLayerTexture != NULL)
        SDL_DestroyTexture(gLayerTexture);
    gLayerTexture = NULL;
    gLayerWaiting = false;
}

/* a layer kept as the frame to show: a copy, as the game draws on in its own; false for one never drawn into */
static bool KeepLayer(PlatTarget *layer)
{
    SDL_Texture *from = layer->drawn ? layer->texture : NULL;
    Uint64 start = SDL_GetTicksNS();

    if (from == NULL)
        return false;
    if (gLayerTexture != NULL && (gLayerWidth != layer->width || gLayerHeight != layer->height))
        DropLayerTexture();
    if (gLayerTexture == NULL) {
        gLayerTexture = MakeTarget(layer->width, layer->height, gScaleMode);
        gLayerWidth = layer->width;
        gLayerHeight = layer->height;
    }
    if (gLayerTexture == NULL || !SDL_SetRenderTarget(gRenderer, gLayerTexture))
        return false;
    SDL_SetTextureColorMod(from, 255, 255, 255);
    SDL_SetTextureAlphaMod(from, 255);
    SDL_SetTextureBlendMode(from, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(from, SDL_SCALEMODE_NEAREST);
    SDL_RenderTexture(gRenderer, from, NULL, NULL);
    SDL_SetRenderTarget(gRenderer, NULL);
    gLayerWaiting = true;
    gTimes.uploadNs += SDL_GetTicksNS() - start;
    return true;
}

/* the layer kept last shown, over the view drawn from its draws when those are new */
static void ShowLayer(void)
{
    gShowStart = SDL_GetTicksNS();
    if (!gChannelsMade)
        MakeChannels();
    if (gViewFresh && !RenderView())
        DropView();
    gTimes.uploadNs += SDL_GetTicksNS() - gShowStart;
    gFrameDrawn = true;
    gLayerWaiting = false;
    Redraw();
    gShowEnd = SDL_GetTicksNS();
}

/* The game presents far more often than a display can show -- at start-up once per step of its progress
   bar, thousands of times -- and an upload and present takes milliseconds (CE had cnc-ddraw show the page
   from a thread of its own, so a flip cost the game nothing). A frame presented too soon after the last
   one waits in gLayerTexture: the next present or event poll shows it once it is due, unless a newer frame
   has replaced it by then.
   Where a show takes longer than the time two are held apart (a large frame, a fast display) that alone
   would still show nearly every frame of a burst, so in a burst the game also gets kBurstRest times as long
   as the show took. Not in flight: there the frames come 21 ms apart and each is shown as it comes. */
static void ShowWaitingFrame(void)
{
    if (gLayerWaiting && FrameDueIn() == 0)
        ShowLayer();
}

/* The window loses the keyboard right now: does it go to another window of this program -- a Win32 dialog
   the game opened, its file dialog -- rather than to another program? The game's window procedure took only
   the second for being switched out (WM_ACTIVATEAPP). */
static bool FocusStaysInProgram(void)
{
#ifdef SDL_PLATFORM_WIN32
    HWND front;
    DWORD process;

    front = GetForegroundWindow();
    process = 0;
    if (front != NULL && GetWindowThreadProcessId(front, &process) != 0)
        return process == GetCurrentProcessId();
#endif
    return false;
}

static bool gFitWindowPending;          /* the frame changed size while full screen */
static void FitWindow(int width, int height);

/* Called as SDL queues an event, on the main thread for those taken here.
   - While the user drags the window's edge the event loop does not run: SDL sends these exposes for the
     window to be redrawn at once.
   - Where a focus loss goes can only be told while it happens; by the time the event is polled a dialog
     may have come and gone. */
static bool SDLCALL EventWatch(void *userdata, SDL_Event *event)
{
    unsigned next;

    (void)userdata;
    if (event->type == SDL_EVENT_WINDOW_EXPOSED && event->window.data1 == 1) {
        Redraw();
    } else if (event->type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN && gFitWindowPending) {
        gFitWindowPending = false;
        FitWindow(gFrameWidth, gFrameHeight);
    } else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        next = (gFocusNoteTail + 1) & (kFocusNotes - 1);
        if (next != gFocusNoteHead) {
            gFocusNote[gFocusNoteTail] = FocusStaysInProgram();
            gFocusNoteTail = next;
        }
    }
    return true;
}

/* the renderer's textures are gone (a lost device): each is made again when next needed */
static void DropFrameTextures(void)
{
    DropViewTextures();
    DropLayerTexture();
    gViewFresh = gViewOn;
    for (PlatImage *image = gImages; image != NULL; image = image->next)
        DropImageTextures(image);
    gFrameDrawn = false;
}

int PlatVideoCreateWindow(const char *title)
{
    gWindow = SDL_CreateWindow(title, 640, 480,
                               SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (gWindow == NULL)
        return -1;
    SDL_AddEventWatch(EventWatch, NULL);
    return 0;
}

/* The window's size when it is not full screen, in the window system's units: the frame's, kept to a share of
   the display and centred on it. */
static void FitWindow(int width, int height)
{
    SDL_DisplayID display;
    SDL_Rect usable;
    float density;
    int w;
    int h;

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
}

/* a frame of width x height stretched to the window with its aspect kept, nothing shown from it yet */
static void MakeFrame(int width, int height)
{
    gFrameWidth = width;
    gFrameHeight = height;
    DropLayerTexture();
    gFrameDrawn = false;
    SDL_SetRenderLogicalPresentation(gRenderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

#if EVPLUS_TEST_PAGES
/* -- EVPLUS_UITEST: targets, deep images and draws, the layer's frame -- drawn, read back, the result logged -- */

static bool OwnPart(SDL_Event *e);

struct UITest {
    int checks;
    int failed;
};

static void Expect(UITest &test, const char *what, bool passed)
{
    test.checks++;
    if (passed)
        return;
    test.failed++;
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "uitest: %s", what);
}

/* two ARGB pixels alike, each channel within `slack' */
static bool Alike(Uint32 got, Uint32 want, int slack)
{
    for (int shift = 0; shift < 32; shift += 8)
        if (SDL_abs((int)(got >> shift & 0xff) - (int)(want >> shift & 0xff)) > slack)
            return false;
    return true;
}

static void ExpectPixel(UITest &test, const char *what, PlatTarget *target, int x, int y, Uint32 want, int slack)
{
    PlatRect at = { x, y, 1, 1 };
    Uint32 got = 0;

    test.checks++;
    if (PlatTargetRead(target, &at, &got) != 0 && Alike(got, want, slack))
        return;
    test.failed++;
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "uitest: %s at %d,%d is %08x, not %08x", what, x, y, got, want);
}

/* a pixel of the frame as Compose draws it -- into `shown', a texture of the frame's size -- alpha left out */
static void ExpectShown(UITest &test, SDL_Texture *shown, const char *what, int x, int y, Uint32 want)
{
    SDL_Rect at = { x, y, 1, 1 };
    SDL_Surface *read;
    Uint8 r = 0, g = 0, b = 0;
    Uint32 got;

    SDL_SetRenderTarget(gRenderer, shown);
    Compose();
    read = SDL_RenderReadPixels(gRenderer, &at);
    SDL_SetRenderTarget(gRenderer, NULL);
    if (read != NULL)
        SDL_ReadSurfacePixel(read, 0, 0, &r, &g, &b, NULL);
    got = 0xff000000u | (Uint32)r << 16 | (Uint32)g << 8 | b;
    test.checks++;
    if (read == NULL || !Alike(got, want | 0xff000000u, 1)) {
        test.failed++;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "uitest: %s at %d,%d is %08x, not %08x", what, x, y, got, want);
    }
    SDL_DestroySurface(read);
}

static PlatDraw TestDraw(int kind, int blend, float x, float y, float w, float h)
{
    PlatDraw d = PlatDrawOf(kind, blend);

    d.x = x;
    d.y = y;
    d.w = w;
    d.h = h;
    return d;
}

static PlatDraw TestFill(int blend, float x, float y, float w, float h, Uint8 r, Uint8 g, Uint8 b, Uint8 alpha)
{
    PlatDraw d = TestDraw(kPlatDrawFill, blend, x, y, w, h);

    d.deep = 1;
    d.rgb[0] = r;
    d.rgb[1] = g;
    d.rgb[2] = b;
    d.alpha = alpha;
    return d;
}

static void TestTargets(UITest &test)
{
    const Uint32 kBack = 0xff0a141eu;
    const Uint32 texels[4] = { 0xffff0000u, 0x80800000u, 0x00000000u, 0xff0000ffu };
    const Uint32 ramp[2] = { 0xff000000u, 0xffffffffu };
    const Uint32 green = 0xff00ff00u;
    const Uint32 dim[2] = { 0xff400000u, 0x80200000u };
    const Uint32 light = 0x00403020u;
    const Uint32 lit = 0x40501000u;
    Uint32 nine[9];
    const PlatRect clip = { 20, 20, 4, 4 };
    const PlatRect corner = { 1, 1, 1, 1 };
    PlatTarget *target = PlatTargetCreate(64, 48);
    PlatTarget *other = PlatTargetCreate(8, 8);
    PlatImage *image = PlatImageCreate32(2, 2, texels, 2 * sizeof(Uint32));
    PlatImage *steps = PlatImageCreate32(2, 1, ramp, 2 * sizeof(Uint32));
    PlatImage *dark = PlatImageCreate32(2, 1, dim, 2 * sizeof(Uint32));
    PlatImage *glow = PlatImageCreate32(1, 1, &light, sizeof(Uint32));
    PlatImage *over = PlatImageCreate32(1, 1, &lit, sizeof(Uint32));
    PlatImage *trio;
    PlatVertex vertices[6] = { { 0, 24, 0, 255 }, { 8, 24, 0, 255 }, { 8, 32, 0, 255 },
                               { 0, 24, 0, 255 }, { 8, 32, 0, 255 }, { 0, 32, 0, 255 } };
    std::vector<Uint32> all((size_t)64 * 48);
    unsigned epoch;
    SDL_Event lost;
    PlatDraw d;

    for (int i = 0; i < 9; i++)
        nine[i] = 0xff000000u | (Uint32)(i % 3 * 80 + 40) << 16 | (Uint32)(i / 3 * 80 + 40) << 8;
    trio = PlatImageCreate32(3, 3, nine, 3 * sizeof(Uint32));
    ExpectPixel(test, "a new target", target, 5, 5, 0, 0);

    /* fills */
    PlatDraw fills[] = { TestFill(kPlatBlendCopy, 0, 0, 64, 48, 10, 20, 30, 255),
                         TestFill(kPlatBlendMix, 0, 0, 8, 8, 255, 255, 255, 128),
                         TestFill(kPlatBlendCopy, 8, 0, 8, 8, 200, 100, 50, 128),
                         TestFill(kPlatBlendAdd, 16, 0, 8, 8, 100, 100, 100, 255),
                         TestDraw(kPlatDrawFill, kPlatBlendCopy, 24, 0, 8, 8) };
    fills[4].color = 0x7c1f;
    PlatTargetDraw(target, fills, 5, NULL, 0, NULL);
    ExpectPixel(test, "a copied fill", target, 40, 40, kBack, 0);
    ExpectPixel(test, "a mixed fill", target, 4, 4, 0xff858a8fu, 1);
    ExpectPixel(test, "a copied fill with alpha", target, 12, 4, 0x80643219u, 0);
    ExpectPixel(test, "an added fill", target, 20, 4, 0xff6e7882u, 1);
    ExpectPixel(test, "a 5-bit colour", target, 28, 4, 0xffff00ffu, 0);
    d = TestFill(kPlatBlendCopy, 0, 0, 64, 48, 255, 0, 0, 255);
    PlatTargetDraw(target, &d, 1, NULL, 0, &clip);
    ExpectPixel(test, "a fill in its clip", target, 21, 21, 0xffff0000u, 0);
    ExpectPixel(test, "a fill left of its clip", target, 19, 21, kBack, 0);
    ExpectPixel(test, "a fill under its clip", target, 21, 24, kBack, 0);

    /* an image: copied, mixed, added, a part of it, with a colour and an alpha */
    PlatDraw images[] = { TestDraw(kPlatDrawImage, kPlatBlendCopy, 30, 30, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendMix, 40, 30, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendAdd, 44, 30, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 48, 30, 4, 4),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 54, 30, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendMix, 58, 30, 2, 2),
                          TestDraw(kPlatDrawSilhouette, kPlatBlendMix, 40, 34, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 44, 34, 2, 2),
                          TestDraw(kPlatDrawSilhouette, kPlatBlendCopy, 52, 34, 2, 2),
                          TestDraw(kPlatDrawImage, kPlatBlendMix, 8, 16, 1, 1) };
    for (PlatDraw &each : images)
        each.image = image;
    images[3].sx = images[3].sy = images[3].sw = images[3].sh = 1;
    images[4].mod[0] = 128;
    images[5].alpha = 128;
    images[6].deep = 1;
    images[6].rgb[1] = 200;
    images[7].alpha = 128;
    images[8].alpha = 128;
    images[8].deep = 1;
    images[8].rgb[1] = 200;
    images[9].image = glow;
    PlatTargetDraw(target, images, 10, NULL, 0, NULL);
    ExpectPixel(test, "a copied image", target, 30, 30, texels[0], 0);
    ExpectPixel(test, "a copied image's alpha", target, 31, 30, texels[1], 0);
    ExpectPixel(test, "a copied image's clear pixel", target, 30, 31, texels[2], 0);
    ExpectPixel(test, "a mixed image", target, 40, 30, 0xffff0000u, 1);
    ExpectPixel(test, "a mixed image's alpha", target, 41, 30, 0xff850a0fu, 1);
    ExpectPixel(test, "a mixed image's clear pixel", target, 40, 31, kBack, 0);
    ExpectPixel(test, "an added image", target, 45, 30, 0xff8a141eu, 1);
    ExpectPixel(test, "an added image's clear pixel", target, 44, 31, kBack, 0);
    ExpectPixel(test, "a part of an image", target, 48, 30, texels[3], 0);
    ExpectPixel(test, "a part of an image, far corner", target, 51, 33, texels[3], 0);
    ExpectPixel(test, "an image with a colour", target, 54, 30, 0xff800000u, 1);
    ExpectPixel(test, "an image with an alpha", target, 58, 30, 0xffff0a0fu, 1);
    ExpectPixel(test, "a silhouette", target, 40, 34, 0xff00c800u, 1);
    ExpectPixel(test, "a silhouette's alpha", target, 41, 34, 0xff056e0fu, 1);
    ExpectPixel(test, "a copied image with an alpha", target, 44, 34, 0x80800000u, 1);
    ExpectPixel(test, "a copied silhouette with an alpha", target, 52, 34, 0x80006400u, 1);
    ExpectPixel(test, "a texel without alpha, mixed: added", target, 8, 16, 0xff4a443eu, 1);

    /* tiles from a grid whose lines do not start at the rect */
    d = TestDraw(kPlatDrawTiled, kPlatBlendCopy, 0, 40, 7, 5);
    d.image = image;
    d.tileX = 1;
    d.tileY = 40;
    PlatTargetDraw(target, &d, 1, NULL, 0, NULL);
    ExpectPixel(test, "a tile cut at the left", target, 0, 40, texels[1], 0);
    ExpectPixel(test, "a whole tile", target, 1, 40, texels[0], 0);
    ExpectPixel(test, "a tile's far corner", target, 2, 41, texels[3], 0);
    ExpectPixel(test, "the last tile", target, 6, 44, texels[1], 0);
    ExpectPixel(test, "right of the tiles", target, 7, 40, kBack, 0);
    ExpectPixel(test, "under the tiles", target, 0, 45, kBack, 0);

    /* tiles of 3 x 3 at twice their size, the grid's corner off the target */
    d = TestDraw(kPlatDrawTiled, kPlatBlendCopy, 10, 40, 13, 7);
    d.image = trio;
    d.tileX = -1;
    d.tileY = 39;
    d.tileScale = 2;
    PlatTargetDraw(target, &d, 1, NULL, 0, NULL);
    ExpectPixel(test, "a large tile cut at the left", target, 10, 40, nine[2], 0);
    ExpectPixel(test, "a large tile's first column", target, 11, 40, nine[0], 0);
    ExpectPixel(test, "a large tile's second pixel of a column", target, 12, 41, nine[3], 0);
    ExpectPixel(test, "a large tile's far corner", target, 16, 44, nine[8], 0);
    ExpectPixel(test, "the last large tile, cut at the right and bottom", target, 22, 46, nine[2], 0);
    ExpectPixel(test, "right of the large tiles", target, 23, 40, kBack, 0);
    ExpectPixel(test, "under the large tiles", target, 10, 47, kBack, 0);

    /* a target drawn into a target */
    d = TestFill(kPlatBlendCopy, 0, 0, 8, 8, 0, 0, 255, 128);
    PlatTargetDraw(other, &d, 1, NULL, 0, NULL);
    d = TestDraw(kPlatDrawImage, kPlatBlendMix, 56, 40, 8, 8);
    d.source = other;
    PlatTargetDraw(target, &d, 1, NULL, 0, NULL);
    ExpectPixel(test, "a target as the source", target, 60, 44, 0xff050a8fu, 1);

    /* triangles in a deep colour */
    d = TestDraw(kPlatDrawTriangles, kPlatBlendMix, 0, 0, 0, 0);
    d.deep = 1;
    d.rgb[0] = d.rgb[1] = 255;
    d.count = 6;
    PlatTargetDraw(target, &d, 1, vertices, 6, NULL);
    ExpectPixel(test, "triangles", target, 3, 27, 0xffffff00u, 1);

    /* a part of an image replaced, its texture there */
    PlatImageUpdate32(image, &corner, &green, sizeof(Uint32));
    d = TestDraw(kPlatDrawImage, kPlatBlendCopy, 30, 34, 2, 2);
    d.image = image;
    PlatTargetDraw(target, &d, 1, NULL, 0, NULL);
    ExpectPixel(test, "an image's replaced pixel", target, 31, 35, green, 0);
    ExpectPixel(test, "an image's kept pixel", target, 30, 34, texels[0], 0);

    /* the filters */
    PlatDraw scaled[] = { TestDraw(kPlatDrawImage, kPlatBlendCopy, 0, 12, 8, 1),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 0, 14, 8, 1) };
    scaled[0].image = scaled[1].image = steps;
    scaled[1].filter = kPlatFilterLinear;
    PlatTargetDraw(target, scaled, 2, NULL, 0, NULL);
    ExpectPixel(test, "an image scaled by the nearest pixel", target, 3, 12, ramp[0], 0);
    ExpectPixel(test, "an image scaled by the nearest pixel, right half", target, 4, 12, ramp[1], 0);
    ExpectPixel(test, "an image scaled smoothly", target, 3, 14, 0xff606060u, 48);
    ExpectPixel(test, "an image scaled smoothly, right half", target, 4, 14, 0xffa0a0a0u, 48);

    Expect(test, "a whole target read",
           PlatTargetRead(target, NULL, all.data()) != 0 && all[(size_t)40 * 64 + 40] == kBack
           && all[(size_t)4 * 64 + 12] == 0x80643219u);

    /* an image without pixels or alpha: black, opaque, and taking over the pixels of a part that differ */
    const Uint16 pair[2] = { 0x7c00, 0x001f };
    PlatRect both = { 0, 0, 2, 1 };
    PlatImage *plain = PlatImageCreate(2, 1, NULL, NULL);
    PlatDraw show = TestDraw(kPlatDrawImage, kPlatBlendCopy, 50, 44, 2, 1);
    show.image = plain;
    PlatTargetDraw(target, &show, 1, NULL, 0, NULL);
    ExpectPixel(test, "an image made without pixels", target, 51, 44, 0xff000000u, 0);
    Expect(test, "an image's part that differs", PlatImageUpdatePart(plain, &both, pair, sizeof pair) == 1);
    Expect(test, "an image's part that is the same", PlatImageUpdatePart(plain, &both, pair, sizeof pair) == 0);
    PlatTargetDraw(target, &show, 1, NULL, 0, NULL);
    ExpectPixel(test, "an image's updated part", target, 50, 44, 0xffff0000u, 0);
    ExpectPixel(test, "an image's updated part, the pixel beside", target, 51, 44, 0xff0000ffu, 0);
    PlatImageDestroy(plain);

    /* the brightness: in colours and images, and counted */
    epoch = PlatUIEpoch();
    PlatVideoSetBrightness(0.5);
    Expect(test, "the epoch after a brightness change", PlatUIEpoch() != epoch);
    PlatDraw bright[] = { TestFill(kPlatBlendCopy, 0, 16, 2, 2, 64, 64, 64, 255),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 2, 16, 2, 1),
                          TestDraw(kPlatDrawImage, kPlatBlendCopy, 10, 16, 1, 1) };
    bright[1].image = dark;
    bright[2].image = over;
    PlatTargetDraw(target, bright, 3, NULL, 0, NULL);
    ExpectPixel(test, "a bright colour", target, 0, 16, 0xff808080u, 0);
    PlatRect litAt = { 0, 16, 1, 1 };
    Uint32 drawn = 0;
    Expect(test, "a bright colour read as drawn",
           PlatTargetReadDrawn(target, &litAt, &drawn) != 0 && Alike(drawn, 0xff404040u, 1));
    ExpectPixel(test, "a bright image", target, 2, 16, 0xff800000u, 0);
    ExpectPixel(test, "a bright image's alpha", target, 3, 16, 0x80400000u, 0);
    ExpectPixel(test, "a bright texel with light over its alpha", target, 10, 16, 0x40802000u, 0);
    epoch = PlatUIEpoch();
    PlatVideoSetBrightness(1.0);
    Expect(test, "the epoch after the brightness is back", PlatUIEpoch() != epoch);
    PlatTargetDraw(target, bright, 2, NULL, 0, NULL);
    ExpectPixel(test, "a colour as it is", target, 0, 16, 0xff404040u, 0);
    ExpectPixel(test, "an image as it is", target, 3, 16, dim[1], 0);

    /* lost render targets, then a lost device: the target is there, clear */
    epoch = PlatUIEpoch();
    SDL_zero(lost);
    lost.type = SDL_EVENT_RENDER_TARGETS_RESET;
    OwnPart(&lost);
    Expect(test, "the epoch after lost render targets", PlatUIEpoch() == epoch + 1);
    ExpectPixel(test, "a target after lost render targets", target, 40, 40, 0, 0);
    Expect(test, "a target that lost its pixels is no frame", !KeepLayer(target));
    PlatTargetDraw(target, images, 1, NULL, 0, NULL);
    ExpectPixel(test, "an image after lost render targets", target, 30, 30, texels[0], 0);
    lost.type = SDL_EVENT_RENDER_DEVICE_RESET;
    OwnPart(&lost);
    Expect(test, "the epoch after a lost device", PlatUIEpoch() == epoch + 2);
    ExpectPixel(test, "a target after a lost device", target, 40, 40, 0, 0);
    PlatTargetDraw(target, images, 1, NULL, 0, NULL);
    ExpectPixel(test, "an image after a lost device", target, 30, 30, texels[0], 0);
    PlatTargetClear(target);
    ExpectPixel(test, "a cleared target", target, 30, 30, 0, 0);

    PlatImageDestroy(image);
    PlatImageDestroy(steps);
    PlatImageDestroy(dark);
    PlatImageDestroy(glow);
    PlatImageDestroy(over);
    PlatImageDestroy(trio);
    PlatTargetDestroy(other);
    PlatTargetDestroy(target);
}

/* frames from a layer over a view with an overlay */
static void TestLayerFrame(UITest &test)
{
    PlatTarget *layer = PlatTargetCreate(gFrameWidth, gFrameHeight);
    PlatTarget *lamp = PlatTargetCreate(4, 4);
    SDL_Texture *shown = MakeTarget(gFrameWidth, gFrameHeight, SDL_SCALEMODE_NEAREST);
    PlatDraw bars[] = { TestFill(kPlatBlendCopy, 0, 0, (float)gFrameWidth, 10, 0, 0, 200, 255),
                        TestFill(kPlatBlendCopy, 0, 10, (float)gFrameWidth, 10, 200, 0, 0, 128) };
    PlatDraw sky = TestDraw(kPlatDrawFill, kPlatBlendMix, 0, 0, 16, 16);
    PlatDraw over[] = { TestDraw(kPlatDrawFill, kPlatBlendAdd, 0, 30, (float)gFrameWidth, 10),
                        TestDraw(kPlatDrawImage, kPlatBlendCopy, 0, 40, (float)gFrameWidth, 10) };
    PlatDraw lit = TestFill(kPlatBlendCopy, 0, 0, 4, 4, 0, 0, 200, 255);
    PlatVideoView view = {};
    unsigned white = gWhite;

    sky.color = 0x03e0;
    over[0].color = 0x7c00;
    over[1].source = lamp;
    PlatTargetDraw(lamp, &lit, 1, NULL, 0, NULL);
    view.width = 16;
    view.height = 16;
    view.w = gFrameWidth;
    view.h = gFrameHeight;
    view.zoom = 1.0f;
    view.draws = &sky;
    view.drawCount = 1;
    view.overlays = over;
    view.overlayCount = 2;
    PlatTargetDraw(layer, bars, 2, NULL, 0, NULL);
    PlatVideoSetWhite(0);
    PlatVideoPresentLayer(layer, &view);
    if (gLayerWaiting)
        ShowLayer();
    /* the kept view draws the target on, until another view comes */
    PlatTargetDestroy(lamp);
    ExpectShown(test, shown, "a destroyed target the view still draws", 5, 45, 0x0000c8u);
    /* what is drawn into the layer after the present is not in the frame */
    PlatTargetClear(layer);
    ExpectShown(test, shown, "the layer", 5, 5, 0x0000c8u);
    ExpectShown(test, shown, "the layer mixed over the view", 5, 15, 0x647f00u);
    ExpectShown(test, shown, "the view under the layer", 5, 25, 0x00ff00u);
    ExpectShown(test, shown, "an overlay under the layer", 5, 35, 0xffff00u);
    PlatVideoSetWhite(128);
    PlatVideoPresentLayer(layer, NULL);
    if (gLayerWaiting)
        ShowLayer();
    ExpectShown(test, shown, "the white-out over the view", 5, 25, 0x80ff80u);
    ExpectShown(test, shown, "the white-out over an overlay", 5, 35, 0xffff80u);
    PlatTargetDraw(layer, bars, 1, NULL, 0, NULL);
    PlatVideoPresentLayer(layer, NULL);
    if (gLayerWaiting)
        ShowLayer();
    ExpectShown(test, shown, "the white-out over the layer", 5, 5, 0x8080e4u);
    PlatVideoDropView();
    ExpectShown(test, shown, "the layer alone", 5, 25, 0x808080u);
    /* the test's frame goes: nothing of it is shown again */
    gFrameDrawn = false;
    PlatVideoSetWhite(white);
    DropLayerTexture();
    PlatTargetDestroy(layer);
    if (shown != NULL)
        SDL_DestroyTexture(shown);
}

static void UISelfTest(void)
{
    UITest test = {};
    double exponent = gExponent;

    PlatVideoSetBrightness(1.0);
    TestTargets(test);
    TestLayerFrame(test);
    PlatVideoSetBrightness(exponent);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "uitest: %d checks, %d failed, on SDL's %s renderer", test.checks,
                test.failed, SDL_GetRendererName(gRenderer));
}
#endif

int PlatVideoOpen(int width, int height, int fullScreen)
{
    const SDL_DisplayMode *mode;
    float refresh;

    if (gWindow == NULL || width <= 0 || height <= 0)
        return -1;
    PlatVideoClose();
    if (gRenderer == NULL) {
        gRenderer = MakeRenderer();
        if (gRenderer == NULL)
            return -1;
        SDL_SetRenderVSync(gRenderer, 0);       /* the game paces itself */
        FindMaxBlend();
    }
    MakeFrame(width, height);
    FitWindow(width, height);
    gFitWindowPending = false;
    SDL_SetWindowFullscreen(gWindow, fullScreen != 0);     /* no mode chosen: the desktop's, borderless */
    SDL_ShowWindow(gWindow);
    SDL_SyncWindow(gWindow);
    SDL_StartTextInput(gWindow);                           /* typed characters arrive as events */
    mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(gWindow));
    refresh = mode != NULL ? mode->refresh_rate : 0;
    if (refresh < 1)
        refresh = kDefaultRefresh;
    gShowInterval = (Uint64)(SDL_NS_PER_SECOND / refresh) * 3 / 4;
    if (gShowInterval > kMaxShowInterval)
        gShowInterval = kMaxShowInterval;
    Redraw();
#if EVPLUS_TEST_PAGES
    if (SDL_getenv("EVPLUS_UITEST") != NULL)
        UISelfTest();
#endif
    return 0;
}

int PlatVideoCanResize(int width, int height)
{
    return gRenderer != NULL && gFrameWidth > 0 && width > 0 && height > 0;
}

int PlatVideoResize(int width, int height)
{
    if (!PlatVideoCanResize(width, height))
        return -1;
    MakeFrame(width, height);
    if (PlatVideoIsFullScreen())
        gFitWindowPending = true;           /* SDL sizes no full-screen window: on leaving full screen */
    else
        FitWindow(width, height);
    return 0;
}

void PlatVideoClose(void)
{
    gFrameWidth = 0;
    gFrameHeight = 0;
    gFrameDrawn = false;
    gShowRest = 0;
    DropViewTextures();
    DropLayerTexture();
    gDraws.clear();
    gVertices.clear();
    gOverlays.clear();
    FreeDeadImages();
    FreeDeadTargets();
    gViewOn = false;
    gViewFresh = false;
}

/* the view's draws copied, to be rendered into its texture with the frame they came with */
static void KeepView(const PlatVideoView *view)
{
    Uint64 start;

    if (view->width <= 0 || view->height <= 0)
        return;
    if (view->width != gViewWidth || view->height != gViewHeight) {
        DropViewTextures();
        gViewWidth = view->width;
        gViewHeight = view->height;
    }
    start = SDL_GetTicksNS();
    gDraws.assign(view->draws, view->draws + (view->draws != NULL ? view->drawCount : 0));
    gVertices.assign(view->vertices, view->vertices + (view->vertices != NULL ? view->vertexCount : 0));
    gOverlays.assign(view->overlays, view->overlays + (view->overlays != NULL ? view->overlayCount : 0));
    FreeDeadImages();
    FreeDeadTargets();
    gTimes.uploadNs += SDL_GetTicksNS() - start;
    gViewRect.x = (float)view->x;
    gViewRect.y = (float)view->y;
    gViewRect.w = (float)view->w;
    gViewRect.h = (float)view->h;
    gViewZoom = view->zoom >= 1.0f ? view->zoom : 1.0f;
    gViewOn = true;
    gViewFresh = true;
}

void PlatVideoPresentLayer(PlatTarget *layer, const PlatVideoView *view)
{
    if (gRenderer == NULL || gFrameWidth <= 0 || layer == NULL)
        return;
    if (view != NULL)
        KeepView(view);
    if (!KeepLayer(layer))
        return;
    gShowRest = SDL_GetTicksNS() - gPresentEnd < kBurstGap ? (gShowEnd - gShowStart) * kBurstRest : 0;
    if (FrameDueIn() == 0)
        ShowLayer();
    gPresentEnd = SDL_GetTicksNS();
}

void PlatVideoDropView(void)
{
    if (!gViewOn)
        return;
    /* a frame still waiting was made for the view: shown with it */
    if (gLayerWaiting)
        ShowLayer();
    DropView();
}

void PlatVideoTakeTimes(PlatVideoTimes *times)
{
    *times = gTimes;
    SDL_zero(gTimes);
}

void PlatVideoSetScaling(int filter)
{
    gScaleMode = ScaleModeOf(filter);
    if (gLayerTexture != NULL && !SDL_SetTextureScaleMode(gLayerTexture, gScaleMode))
        SDL_SetTextureScaleMode(gLayerTexture, SDL_SCALEMODE_LINEAR);
    if (gViewTexture != NULL && !SDL_SetTextureScaleMode(gViewTexture, gScaleMode))
        SDL_SetTextureScaleMode(gViewTexture, SDL_SCALEMODE_LINEAR);
    Redraw();
}

void PlatVideoSetFullScreen(int on)
{
    if (gWindow == NULL)
        return;
    SDL_SetWindowFullscreen(gWindow, on != 0);
    SDL_SyncWindow(gWindow);                /* the mode is in place (and reported) before this returns */
}

int PlatMaxTextureSize(void)
{
    if (gRenderer == NULL)
        return 0;
    return (int)SDL_GetNumberProperty(SDL_GetRendererProperties(gRenderer), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER,
                                      0);
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

/* no icon, as the game's box; what cannot be shown is at least logged */
void PlatAlert(const char *title, const char *message)
{
    if (!SDL_ShowSimpleMessageBox(0, title, message, gWindow))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", title, message);
}

/* -- events --------------------------------------------------------------------------------------- */

/* The keyboard's one key the game knows under two codes: an ISO keyboard's key left of Enter. */
static SDL_Scancode OneKey(SDL_Scancode code)
{
    return code == SDL_SCANCODE_NONUSHASH ? SDL_SCANCODE_BACKSLASH : code;
}

static int ButtonBit(Uint8 button)
{
    switch (button) {
    case SDL_BUTTON_LEFT:   return 1;
    case SDL_BUTTON_RIGHT:  return 2;
    case SDL_BUTTON_MIDDLE: return 4;
    default:                return 0;
    }
}

/* The platform's own part of an event; 0 when the event is the platform's alone. */
static bool OwnPart(SDL_Event *e)
{
    SDL_Scancode code;
    SDL_Keymod mod;
    bool staysInProgram;

    switch (e->type) {
    case SDL_EVENT_KEY_DOWN:
        code = OneKey(e->key.scancode);
        e->key.scancode = code;
        mod = e->key.mod;
        if (code == SDL_SCANCODE_CAPSLOCK && !e->key.repeat)
            gCapsLockPressed = true;
        if ((code == SDL_SCANCODE_RETURN || code == SDL_SCANCODE_KP_ENTER) && (mod & SDL_KMOD_ALT) != 0
            && (mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) == 0) {
            if (!e->key.repeat)
                PlatVideoSetFullScreen(!PlatVideoIsFullScreen());
            gSwallowedKey = code;
            return false;
        }
        return true;
    case SDL_EVENT_KEY_UP:
        code = OneKey(e->key.scancode);
        e->key.scancode = code;
        if (code == gSwallowedKey) {
            gSwallowedKey = SDL_SCANCODE_UNKNOWN;
            return false;
        }
        return true;
    case SDL_EVENT_MOUSE_MOTION:
        if (gRenderer == NULL)
            return false;
        SDL_ConvertEventToRenderCoordinates(gRenderer, e);
        return true;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.down)
            gButtons |= ButtonBit(e->button.button);
        else
            gButtons &= ~ButtonBit(e->button.button);
        SDL_CaptureMouse(gButtons != 0);        /* a drag goes on outside the window */
        return true;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        if (gActive)
            return false;
        gActive = true;
        return true;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        /* EventWatch's note for this loss; without one (more losses queued than notes) it counts */
        if (gFocusNoteHead != gFocusNoteTail) {
            staysInProgram = gFocusNote[gFocusNoteHead];
            gFocusNoteHead = (gFocusNoteHead + 1) & (kFocusNotes - 1);
            if (staysInProgram)
                return false;
        }
        if (!gActive)
            return false;
        gActive = false;
        return true;
    case SDL_EVENT_WINDOW_EXPOSED:
        Redraw();
        return false;
    case SDL_EVENT_RENDER_DEVICE_RESET:
        /* the textures are gone */
        if (gRenderer != NULL)
            DropFrameTextures();
        DropTargetTextures();
        gUIEpoch++;
        return false;
    case SDL_EVENT_RENDER_TARGETS_RESET:
        /* the render targets' contents are gone: the GPU view is drawn again, the interface by the game */
        gViewFresh = gViewOn;
        gLayerWaiting = false;
        gFrameDrawn = false;
        if (gRenderer != NULL)
            ClearTargetTextures();
        gUIEpoch++;
        return false;
    default:
        return true;
    }
}

int PlatPollEvent(SDL_Event *e)
{
    ShowWaitingFrame();
    while (SDL_PollEvent(e)) {
        if (OwnPart(e))
            return 1;
    }
    return 0;
}

void PlatWaitEvent(int ms)
{
    Uint64 due;

    if (SDL_HasEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST))
        return;
    ShowWaitingFrame();
    if (gLayerWaiting) {
        /* a frame is waiting for its time: that ends the wait too */
        due = FrameDueIn() / SDL_NS_PER_MS + 1;
        if (due < (Uint64)ms) {
            SDL_WaitEventTimeout(NULL, (Sint32)due);
            ShowWaitingFrame();
            return;
        }
    }
    SDL_WaitEventTimeout(NULL, ms);
}

int PlatKeyIsDown(int scan)
{
    int count;
    const bool *state;

    if (scan <= 0 || scan == gSwallowedKey)
        return 0;
    state = SDL_GetKeyboardState(&count);
    if (scan == SDL_SCANCODE_BACKSLASH && state[SDL_SCANCODE_NONUSHASH])
        return 1;
    return scan < count && state[scan];
}

int PlatCapsLockPressed(void)
{
    bool pressed = gCapsLockPressed;

    gCapsLockPressed = false;
    return pressed;
}

int PlatKeyLockState(int scan)
{
    SDL_Keymod mod;

    mod = SDL_GetModState();
    switch (scan) {
    case SDL_SCANCODE_CAPSLOCK:     return (mod & SDL_KMOD_CAPS) != 0;
    case SDL_SCANCODE_NUMLOCKCLEAR: return (mod & SDL_KMOD_NUM) != 0;
    case SDL_SCANCODE_SCROLLLOCK:   return (mod & SDL_KMOD_SCROLL) != 0;
    }
    return -1;
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

/* -- audio ---------------------------------------------------------------------------------------- */

/* SDL wants `additional' more bytes of the output's own format */
static void SDLCALL FeedOutput(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
    PlatAudio *s = (PlatAudio *)userdata;
    Sint16 buffer[kPullFrames * 2];
    int frames;
    int got;

    (void)total;
    frames = additional / kBytesPerFrame;
    while (frames > 0 && SDL_GetAtomicInt(&s->ended) == 0) {
        got = s->pull(s->ctx, buffer, frames < kPullFrames ? frames : kPullFrames);
        if (got <= 0) {
            SDL_SetAtomicInt(&s->ended, 1);
            /* without this SDL keeps the last frames back while it converts the rate, waiting for more,
               and the output never reads as drained */
            SDL_FlushAudioStream(stream);
            break;
        }
        SDL_PutAudioStreamData(stream, buffer, got * kBytesPerFrame);
        frames -= got;
    }
}

PlatAudio *PlatAudioOpen(int rate, PlatAudioPull pull, void *ctx)
{
    SDL_AudioSpec spec;
    PlatAudio *s;

    if (!gAudioUp)
        return NULL;
    s = (PlatAudio *)SDL_calloc(1, sizeof *s);
    if (s == NULL)
        return NULL;
    s->pull = pull;
    s->ctx = ctx;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = rate;
    s->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedOutput, s);
    if (s->stream == NULL) {
        SDL_free(s);
        return NULL;
    }
    s->next = gAudioOutputs;
    gAudioOutputs = s;
    SDL_ResumeAudioStreamDevice(s->stream);     /* a new output is paused */
    return s;
}

void PlatAudioPause(PlatAudio *s)
{
    if (s != NULL && s->stream != NULL)
        SDL_PauseAudioStreamDevice(s->stream);
}

void PlatAudioResume(PlatAudio *s)
{
    if (s != NULL && s->stream != NULL)
        SDL_ResumeAudioStreamDevice(s->stream);
}

void PlatAudioLock(PlatAudio *s)
{
    if (s != NULL && s->stream != NULL)
        SDL_LockAudioStream(s->stream);
}

void PlatAudioUnlock(PlatAudio *s)
{
    if (s != NULL && s->stream != NULL)
        SDL_UnlockAudioStream(s->stream);
}

int PlatAudioDrained(PlatAudio *s)
{
    if (s == NULL || s->stream == NULL)
        return 1;
    return SDL_GetAtomicInt(&s->ended) != 0 && SDL_GetAudioStreamQueued(s->stream) <= 0;
}

void PlatAudioClose(PlatAudio *s)
{
    PlatAudio **link;

    if (s == NULL)
        return;
    for (link = &gAudioOutputs; *link != NULL; link = &(*link)->next) {
        if (*link == s) {
            *link = s->next;
            break;
        }
    }
    if (s->stream != NULL)
        SDL_DestroyAudioStream(s->stream);      /* waits for a pull that is running */
    SDL_free(s);
}

/* -- voices --------------------------------------------------------------------------------------- */

#define kVoiceChannels      2
#define kVoiceMinSpeed      0.01                /* SDL's frequency ratio range */
#define kVoiceMaxSpeed      100.0
#define kVoiceLateNS        SDL_NS_PER_SECOND   /* how long past its length a voice may still be playing */

void PlatVoicesClose(PlatVoices *v)
{
    PlatVoices **link;

    if (v == NULL)
        return;
    for (link = &gVoicePools; *link != NULL; link = &(*link)->next) {
        if (*link == v) {
            *link = v->next;
            break;
        }
    }
    DropVoices(v);
    SDL_free(v->streams);
    SDL_free(v->deadlines);
    SDL_free(v->work);
    SDL_free(v);
}

PlatVoices *PlatVoicesOpen(int count, float gain, int offline)
{
    SDL_AudioSpec src;
    SDL_AudioSpec dst;
    PlatVoices *v;
    int i;

    if (!offline && !gAudioUp)
        return NULL;
    v = (PlatVoices *)SDL_calloc(1, sizeof *v);
    if (v == NULL)
        return NULL;
    v->gain = gain;
    v->streams = (SDL_AudioStream **)SDL_calloc(count, sizeof *v->streams);
    v->deadlines = (Uint64 *)SDL_calloc(count, sizeof *v->deadlines);
    if (v->streams == NULL || v->deadlines == NULL) {
        SDL_free(v->streams);
        SDL_free(v->deadlines);
        SDL_free(v);
        return NULL;
    }
    v->count = count;
    v->next = gVoicePools;
    gVoicePools = v;

    src.format = SDL_AUDIO_S16;
    src.channels = kVoiceChannels;
    src.freq = kPlatVoicesRenderRate;
    dst.format = SDL_AUDIO_F32;
    dst.channels = kVoiceChannels;
    dst.freq = kPlatVoicesRenderRate;
    for (i = 0; i < count; i++) {
        /* a bound stream takes the device's format */
        v->streams[i] = SDL_CreateAudioStream(&src, offline ? &dst : NULL);
        if (v->streams[i] == NULL) {
            PlatVoicesClose(v);
            return NULL;
        }
    }
    if (!offline) {
        v->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (v->device == 0 || !SDL_BindAudioStreams(v->device, v->streams, count)) {
            PlatVoicesClose(v);
            return NULL;
        }
        SDL_SetAudioDeviceGain(v->device, gain);
    }
    return v;
}

static SDL_AudioStream *VoiceStream(PlatVoices *v, int index)
{
    if (v == NULL || index < 0 || index >= v->count)
        return NULL;
    return v->streams[index];
}

int PlatVoicesStart(PlatVoices *v, int index, const int16_t *frames, int count, int rate, double speed)
{
    SDL_AudioStream *stream = VoiceStream(v, index);
    SDL_AudioSpec spec;
    bool ok;

    if (stream == NULL)
        return -1;
    if (rate <= 0 || count < 0 || count > SDL_MAX_SINT32 / (int)(kVoiceChannels * sizeof *frames)) {
        SDL_ClearAudioStream(stream);
        return -1;
    }
    if (speed < kVoiceMinSpeed)
        speed = kVoiceMinSpeed;
    if (speed > kVoiceMaxSpeed)
        speed = kVoiceMaxSpeed;
    spec.format = SDL_AUDIO_S16;
    spec.channels = kVoiceChannels;
    spec.freq = rate;
    /* the device must not pull from it half set up */
    SDL_LockAudioStream(stream);
    SDL_ClearAudioStream(stream);
    ok = SDL_SetAudioStreamFormat(stream, &spec, NULL)
         && SDL_SetAudioStreamFrequencyRatio(stream, (float)speed)
         && SDL_PutAudioStreamData(stream, frames, count * kVoiceChannels * (int)sizeof *frames)
         && SDL_FlushAudioStream(stream);
    if (!ok)
        SDL_ClearAudioStream(stream);
    SDL_UnlockAudioStream(stream);
    v->deadlines[index] = SDL_GetTicksNS() + (Uint64)((double)count / (rate * speed) * SDL_NS_PER_SECOND)
                          + kVoiceLateNS;
    return ok ? 0 : -1;
}

void PlatVoicesStop(PlatVoices *v, int index)
{
    SDL_AudioStream *stream = VoiceStream(v, index);

    if (stream != NULL)
        SDL_ClearAudioStream(stream);
}

int PlatVoicesEnded(PlatVoices *v, int index)
{
    SDL_AudioStream *stream = VoiceStream(v, index);

    if (stream == NULL || SDL_GetAudioStreamAvailable(stream) <= 0)
        return 1;
    /* a device that stopped pulling must not hold a voice forever */
    return v->device != 0 && SDL_GetTicksNS() >= v->deadlines[index];
}

void PlatVoicesRender(PlatVoices *v, float *out, float *unclamped, int frames)
{
    int bytes = frames * kVoiceChannels * (int)sizeof *out;
    int got;
    int i;
    int j;

    SDL_memset(out, 0, bytes);
    if (unclamped != NULL)
        SDL_memset(unclamped, 0, bytes);
    if (v == NULL)
        return;
    if (v->workFrames < frames) {
        float *work = (float *)SDL_realloc(v->work, bytes);
        if (work == NULL)
            return;
        v->work = work;
        v->workFrames = frames;
    }
    for (i = v->count - 1; i >= 0; i--) {
        if (v->streams[i] == NULL)
            continue;
        got = SDL_GetAudioStreamData(v->streams[i], v->work, bytes);
        if (got <= 0)
            continue;
        SDL_MixAudio((Uint8 *)out, (const Uint8 *)v->work, SDL_AUDIO_F32, (Uint32)got, v->gain);
        if (unclamped != NULL) {
            for (j = 0; j < got / (int)sizeof *out; j++)
                unclamped[j] += v->work[j] * v->gain;
        }
    }
}

/* -- the program's entry and its arguments, which SDL hands over as UTF-8 ------------------------- */

static int gArgCount;
static char **gArgs;

int main(int argc, char *argv[])
{
    gArgCount = argc;
    gArgs = argv;
    return GameMain();
}

int PlatArgCount(void)
{
    return gArgCount;
}

const char *PlatArg(int index)
{
    return (index >= 0 && index < gArgCount && gArgs[index] != NULL) ? gArgs[index] : "";
}

/* -- files ------------------------------------------------------------------------------------------ */

const char *PlatExeFolder(void)
{
    const char *base = SDL_GetBasePath();

    if (base == NULL)
        return "";
#ifdef __APPLE__
    /* in an app bundle the game's files sit beside the .app, as with the Mac versions */
    static char beside[kPlatPathMax];
    const char *bundle = SDL_strstr(base, ".app/Contents/");

    if (bundle != NULL) {
        size_t length = (size_t)(bundle - base);

        while (length > 0 && base[length - 1] != '/')
            length--;
        if (length > 0 && length < sizeof beside) {
            SDL_memcpy(beside, base, length);
            beside[length] = '\0';
            return beside;
        }
    }
#endif
    return base;
}

int PlatListFiles(const char *where, const char *pattern, void (*found)(const char *path, void *ctx), void *ctx)
{
    char folder[kPlatPathMax];
    char **names;
    char *path;
    size_t length;
    int count;
    int i;

    count = 0;
    if (!PlatFindPath(where, folder, sizeof folder))
        return 0;
    names = SDL_GlobDirectory(folder, pattern, SDL_GLOB_CASEINSENSITIVE, &count);
    if (names == NULL)
        return 0;
    for (i = 0; i < count; i++) {
        length = SDL_strlen(folder) + SDL_strlen(names[i]) + 2;
        path = (char *)SDL_malloc(length);
        if (path == NULL)
            continue;
        SDL_snprintf(path, length, "%s%s%s", folder,
                     (folder[0] != 0 && SDL_strchr("/\\", folder[SDL_strlen(folder) - 1]) == NULL) ? "/" : "", names[i]);
        found(path, ctx);
        SDL_free(path);
    }
    SDL_free(names);
    return count;
}

int PlatPathKind(const char *name, uint64_t *size)
{
    char path[kPlatPathMax];
    SDL_PathInfo info;

    if (size != NULL)
        *size = 0;
    if (!PlatFindPath(name, path, sizeof path) || !SDL_GetPathInfo(path, &info) || info.type == SDL_PATHTYPE_NONE)
        return kPlatNothing;
    if (info.type == SDL_PATHTYPE_DIRECTORY)
        return kPlatFolder;
    if (size != NULL)
        *size = info.size;
    return kPlatFile;
}

/* -- a folder's listing, in NTFS' order: names compared as UTF-16 after each character is made upper
   case. NTFS has a table for that; this is the table's part for Latin, Greek and Cyrillic letters, other
   characters compare as they are. ---------------------------------------------------------------- */

#define kFirstSurrogate     0xd800
#define kFirstAstral        0x10000
#define kAfterAstral        0x100000

static Uint32 UpperCase(Uint32 c)
{
    if (c < 0x80)
        return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
    if (c >= 0xe0 && c <= 0xfe && c != 0xf7)
        return c - 0x20;
    if (c == 0xff)
        return 0x178;
    if (c >= 0x100 && c <= 0x17f) {
        if (c == 0x131)
            return 'I';
        if (c == 0x17f)
            return 'S';
        if (c == 0x130 || c == 0x138 || c == 0x149 || c == 0x178)
            return c;
        if ((c >= 0x139 && c <= 0x148) || c >= 0x179)
            return (c & 1) == 0 ? c - 1 : c;        /* upper case on the odd codes */
        return (c & 1) != 0 ? c - 1 : c;            /* upper case on the even codes */
    }
    if (c == 0x3c2)
        return 0x3a3;                               /* final sigma */
    if ((c >= 0x3b1 && c <= 0x3c9) || (c >= 0x430 && c <= 0x44f))
        return c - 0x20;
    if (c >= 0x450 && c <= 0x45f)
        return c - 0x50;
    return c;
}

/* where a character stands in UTF-16's order: those past 0xffff are written with 0xd800..0xdfff */
static Uint32 Utf16Order(Uint32 c)
{
    if (c < kFirstSurrogate)
        return c;
    if (c >= kFirstAstral)
        return kFirstSurrogate + (c - kFirstAstral);
    return c + kAfterAstral;
}

typedef struct FolderEntry {
    char *name;
    int isFolder;
    Uint64 size;
} FolderEntry;

typedef struct FolderList {
    FolderEntry *items;
    int count;
    int room;
} FolderList;

static int SDLCALL CompareEntries(const void *a, const void *b)
{
    const char *left = ((const FolderEntry *)a)->name;
    const char *right = ((const FolderEntry *)b)->name;
    const char *l = left;
    const char *r = right;
    Uint32 cl;
    Uint32 cr;

    do {
        cl = Utf16Order(UpperCase(SDL_StepUTF8(&l, NULL)));
        cr = Utf16Order(UpperCase(SDL_StepUTF8(&r, NULL)));
        if (cl != cr)
            return cl < cr ? -1 : 1;
    } while (cl != 0);
    return SDL_strcmp(left, right);         /* two names Windows could not hold side by side */
}

static SDL_EnumerationResult SDLCALL AddEntry(void *userdata, const char *dirname, const char *fname)
{
    FolderList *list = (FolderList *)userdata;
    FolderEntry *grown;
    FolderEntry *entry;
    SDL_PathInfo info;
    char *path;

    if (list->count == list->room) {
        grown = (FolderEntry *)SDL_realloc(list->items, (size_t)(list->room * 2 + 16) * sizeof *grown);
        if (grown == NULL)
            return SDL_ENUM_FAILURE;
        list->items = grown;
        list->room = list->room * 2 + 16;
    }
    entry = &list->items[list->count];
    entry->name = SDL_strdup(fname);
    if (entry->name == NULL)
        return SDL_ENUM_FAILURE;
    entry->isFolder = 0;
    entry->size = 0;
    path = NULL;
    if (SDL_asprintf(&path, "%s%s", dirname, fname) >= 0 && SDL_GetPathInfo(path, &info)) {
        entry->isFolder = info.type == SDL_PATHTYPE_DIRECTORY;
        entry->size = entry->isFolder ? 0 : info.size;
    }
    SDL_free(path);
    list->count++;
    return SDL_ENUM_CONTINUE;
}

int PlatListFolder(const char *name, PlatFoundProc found, void *ctx)
{
    char folder[kPlatPathMax];
    FolderList list;
    bool read;
    int i;

    SDL_zero(list);
    read = PlatFindPath(name, folder, sizeof folder) && SDL_EnumerateDirectory(folder, AddEntry, &list);
    if (read) {
        SDL_qsort(list.items, (size_t)list.count, sizeof *list.items, CompareEntries);
        for (i = 0; i < list.count; i++)
            found(list.items[i].name, list.items[i].isFolder, list.items[i].size, ctx);
    }
    for (i = 0; i < list.count; i++)
        SDL_free(list.items[i].name);
    SDL_free(list.items);
    return read;
}

int PlatMakeFolder(const char *name)
{
    char path[kPlatPathMax];

    if (!PlatFindPath(name, path, sizeof path))
        return 0;
    SDL_CreateDirectory(path);
    return PlatPathKind(path, NULL) != kPlatNothing;
}

/* SDL counts removing what is not there as done */
int PlatRemove(const char *name)
{
    char path[kPlatPathMax];

    return PlatFindPath(name, path, sizeof path) && PlatPathKind(path, NULL) != kPlatNothing
           && SDL_RemovePath(path);
}

#ifdef SDL_PLATFORM_WINDOWS
int PlatFindPath(const char *path, char *out, int outSize)
{
    if (SDL_strlen(path) >= (size_t)outSize)
        return 0;
    SDL_strlcpy(out, path, (size_t)outSize);
    return 1;
}
#else
/* the first `length' bytes of `want' name `name', upper and lower case alike, in NTFS' way */
static bool SameName(const char *name, const char *want, size_t length)
{
    while (length != 0) {
        if (*name == 0 || UpperCase(SDL_StepUTF8(&name, NULL)) != UpperCase(SDL_StepUTF8(&want, &length)))
            return false;
    }
    return *name == 0;
}

typedef struct NameSearch {
    const char *want;
    size_t length;
    char *found;
} NameSearch;

static SDL_EnumerationResult SDLCALL MatchName(void *userdata, const char *dirname, const char *fname)
{
    NameSearch *search = (NameSearch *)userdata;

    (void)dirname;
    if (!SameName(fname, search->want, search->length))
        return SDL_ENUM_CONTINUE;
    search->found = SDL_strdup(fname);
    return SDL_ENUM_SUCCESS;
}

int PlatFindPath(const char *path, char *out, int outSize)
{
    SDL_PathInfo info;
    NameSearch search;
    size_t length;
    size_t start;
    size_t end;
    char first;
    char cut;

    length = SDL_strlen(path);
    if (length >= (size_t)outSize)
        return 0;
    SDL_memcpy(out, path, length + 1);
    if (SDL_GetPathInfo(out, &info))
        return 1;
    for (start = 0; start < length; start = end) {
        while (out[start] == '/')
            start++;
        for (end = start; end < length && out[end] != '/'; end++)
            ;
        if (end == start)
            break;
        cut = out[end];
        out[end] = 0;
        if (!SDL_GetPathInfo(out, &info)) {
            search.want = path + start;
            search.length = end - start;
            search.found = NULL;
            first = out[start];
            out[start] = 0;
            SDL_EnumerateDirectory(start == 0 ? "." : out, MatchName, &search);
            out[start] = first;
            /* a name whose other case is longer or shorter in UTF-8 would move the rest: not taken */
            if (search.found == NULL || SDL_strlen(search.found) != end - start) {
                SDL_free(search.found);
                out[end] = cut;
                break;
            }
            SDL_memcpy(out + start, search.found, end - start);
            SDL_free(search.found);
        }
        out[end] = cut;
    }
    return 1;
}
#endif

int PlatEnv(const char *name, char *out, int outSize)
{
    const char *value;

    out[0] = 0;
    value = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), name);
    if (value == NULL || value[0] == 0 || SDL_strlen(value) >= (size_t)outSize)
        return 0;
    SDL_strlcpy(out, value, (size_t)outSize);
    return 1;
}

/* -- the file dialog. SDL shows it without waiting and answers through a call that may come from another
   thread; the game wants the answer before it goes on, so the events are kept moving until it is there
   (they stay queued for the game) and the window is redrawn when the dialog uncovers it. --------------- */

#define kDialogPoll     10              /* ms */
#ifdef SDL_PLATFORM_WINDOWS
#define kSeparator      "\\"
#else
#define kSeparator      "/"
#endif

/* Windows' own dialog, which the game used, opens in the folder the program names only until the user
   has chosen a file somewhere: from then on it opens there. SDL's opens where it is told every time, so
   that folder is kept here, until the program ends. */
static char *gDialogFolder;

static bool IsSeparator(char c)
{
    return c == '/' || c == '\\';
}

/* a folder as SDL wants a place to start in: what follows the last separator is taken for a file's name */
static char *StartIn(const char *folder)
{
    char *start;
    size_t length;

    if (folder == NULL || PlatPathKind(folder, NULL) != kPlatFolder)
        return NULL;
    length = SDL_strlen(folder);
    start = NULL;
    SDL_asprintf(&start, "%s%s", folder, (length != 0 && IsSeparator(folder[length - 1])) ? "" : kSeparator);
    return start;
}

#define kDialogErrorMax 200

typedef struct DialogAnswer {
    SDL_AtomicInt done;
    char *path;                         /* NULL: cancelled, or the dialog could not be shown */
    bool failed;                        /* it could not be shown: `error' says why */
    char error[kDialogErrorMax];
} DialogAnswer;

static void SDLCALL DialogClosed(void *userdata, const char * const *files, int filter)
{
    DialogAnswer *answer = (DialogAnswer *)userdata;

    (void)filter;
    if (files == NULL) {
        answer->failed = true;
        SDL_strlcpy(answer->error, SDL_GetError(), sizeof answer->error);   /* this thread's error */
    } else if (files[0] != NULL) {
        answer->path = SDL_strdup(files[0]);
    }
    SDL_SetAtomicInt(&answer->done, 1);
}

int PlatOpenFileDialog(const char *kindName, const char *extension, const char *startFolder, char *out,
                       int outSize)
{
    SDL_DialogFileFilter filter;
    DialogAnswer answer;
    char *start;
    char *end;
    int chosen;

    out[0] = 0;
    filter.name = kindName;
    filter.pattern = extension;
    SDL_zero(answer);
    start = StartIn(gDialogFolder);
    if (start == NULL)
        start = StartIn(startFolder);
    SDL_ShowOpenFileDialog(DialogClosed, &answer, gWindow, &filter, 1, start, false);
    while (SDL_GetAtomicInt(&answer.done) == 0) {
        SDL_PumpEvents();
        ShowWaitingFrame();
        SDL_Delay(kDialogPoll);
    }
    SDL_free(start);
    if (answer.failed) {
        /* a system without a file dialog: the user must not take it for a cancel of their own */
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "file dialog: %s", answer.error);
        PlatAlert("", "The file dialog could not be opened.");
    }
    chosen = answer.path != NULL && SDL_strlen(answer.path) < (size_t)outSize;
    if (chosen) {
        SDL_strlcpy(out, answer.path, (size_t)outSize);
        for (end = answer.path + SDL_strlen(answer.path); end > answer.path && !IsSeparator(end[-1]); end--)
            ;
        *end = 0;
        SDL_free(gDialogFolder);
        gDialogFolder = answer.path;            /* the folder the file lies in */
    } else {
        SDL_free(answer.path);
    }
    return chosen;
}

void *PlatLoadFile(const char *name, int *size)
{
    char path[kPlatPathMax];
    size_t length;
    void *loaded;
    void *copy;

    *size = 0;
    if (!PlatFindPath(name, path, sizeof path))
        return NULL;
    loaded = SDL_LoadFile(path, &length);
    if (loaded == NULL)
        return NULL;
    copy = malloc(length != 0 ? length : 1);        /* the caller frees it with free() */
    if (copy != NULL) {
        SDL_memcpy(copy, loaded, length);
        *size = (int)length;
    }
    SDL_free(loaded);
    return copy;
}

/* -- locks ----------------------------------------------------------------------------------------- */

PlatMutex *PlatMutexCreate(void)
{
    return (PlatMutex *)SDL_CreateMutex();
}

void PlatMutexLock(PlatMutex *m)
{
    SDL_LockMutex((SDL_Mutex *)m);
}

void PlatMutexUnlock(PlatMutex *m)
{
    SDL_UnlockMutex((SDL_Mutex *)m);
}

void PlatMutexDestroy(PlatMutex *m)
{
    SDL_DestroyMutex((SDL_Mutex *)m);
}

/* -- clocks, log ----------------------------------------------------------------------------------- */

uint64_t PlatMicroseconds(void)
{
    return SDL_GetTicksNS() / SDL_NS_PER_US;
}

void PlatSleep(int ms)
{
    SDL_Delay((Uint32)ms);
}

int32_t PlatRand(uint64_t *state, int32_t n)
{
    return SDL_rand_r(state, n);
}

uint32_t PlatLocalSeconds1904(void)
{
    SDL_DateTime local;
    SDL_Time now;
    Sint64 seconds;

    if (!SDL_GetCurrentTime(&now))
        return 0;
    seconds = now / SDL_NS_PER_SECOND;
    if (SDL_TimeToDateTime(now, &local, true))
        seconds += local.utc_offset;
    return (uint32_t)(seconds + kSeconds1904To1970);
}

void PlatDebugLog(const char *msg)
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", msg);
}

void PlatDebugLogOnce(int *said, const char *msg)
{
    if (*said == 0)
        PlatDebugLog(msg);
    *said = 1;
}
