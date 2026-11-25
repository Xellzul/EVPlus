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

#define kFocusNotes     16              /* focus losses waiting in SDL's queue, a power of 2 */
#define kBytesPerPixel  2
#define kDefaultRefresh 60              /* Hz, when the display does not tell */
/* two frames are never held apart longer than this, whatever the display: the game's own frames in flight
   come 21 ms apart and must each be shown as they come */
#define kMaxShowInterval (10 * SDL_NS_PER_MS)
/* the game came back with another frame within this of the last one: it is not pacing itself (in flight it
   fills every frame up to 20 ms) but presenting in a burst, as at start-up */
#define kBurstGap       (2 * SDL_NS_PER_MS)
/* frame pixels past the view's right edge its last column is drawn over */
#define kViewEdge       2
/* unchanged rows between two changed ones that are uploaded with them, for fewer texture locks */
#define kRunGap         8
/* in a burst the game gets this many times as long as a show took before the next show */
#define kBurstRest      3
/* a window that is not full screen opens at the frame's size, or at this share of the display when the frame is larger */
#define kWindowShareNum 9
#define kWindowShareDen 10

static SDL_Window *gWindow;
static SDL_Renderer *gRenderer;
static SDL_Texture *gTexture;
static int gFrameWidth;
static int gFrameHeight;
static unsigned char *gFrame;           /* a frame waiting to be shown, gFrameWidth * kBytesPerPixel bytes a row */
static bool gFrameFresh;                /* gFrame holds one */
static bool gFrameDrawn;                /* the texture has a frame to show */
static const void *gGamePixels;         /* the game's last frame, where the game keeps it, */
static int gGamePitch;                  /* and its bytes a row */
static Uint64 gShowInterval;            /* ns between the starts of two shows */
static Uint64 gShowStart;               /* when the last show began */
static Uint64 gShowEnd;                 /* and when it was done */
static Uint64 gShowRest;                /* ns after gShowEnd in which nothing more is shown; 0 outside a burst */
static Uint64 gPresentEnd;              /* when the game's last present returned */
static SDL_ScaleMode gScaleMode = SDL_SCALEMODE_LINEAR;
static bool gWholePixel;                /* PlatVideoSetScaling's wholePixel */
static PlatVideoTimes gTimes;           /* for PlatVideoTakeTimes */
static SDL_Texture *gViewTexture;       /* the view under the frame, at the view's size */
static int gViewWidth;
static int gViewHeight;
static SDL_FRect gViewRect;             /* its place on the frame */
static bool gViewOn;                    /* a view was given: it is drawn under the frame, which is blended */
static bool gViewFresh;                 /* the draws are not yet rendered into gViewTexture */
static Uint16 *gShown;                  /* with a view: the frame as the texture holds it */
static bool gShownValid;
static std::vector<PlatDraw> gDraws;    /* the GPU view's, as last given */
static std::vector<PlatVertex> gVertices;
static std::vector<PlatDraw> gOverlays;

struct PlatImage {
    int width;
    int height;
    Uint16 *pixels;
    Uint8 *alpha;
    SDL_Texture *texture;               /* NULL: to be made (again) */
    SDL_Texture *silhouette;            /* white where opaque, made when first drawn */
    bool dead;                          /* destroyed by the game: freed once no kept view draws it */
    PlatImage *prev;
    PlatImage *next;
};
static PlatImage *gImages;              /* every image, for a brightness change or a lost device */
static int gDeadImages;
static void DropImageTextures(PlatImage *image);

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

/* the frame as it was last shown, stretched into the window as it is now */
static void Redraw(void)
{
    SDL_FRect edgeFrom;
    SDL_FRect edgeTo;
    Uint64 start;

    if (gRenderer == NULL)
        return;
    start = SDL_GetTicksNS();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);
    if (gTexture != NULL && gFrameDrawn) {
        if (gViewOn && gViewTexture != NULL) {
            SDL_RenderTexture(gRenderer, gViewTexture, NULL, &gViewRect);
            /* the view's last column a little past its edge: where the window scales the frame, the frame's
               edge beside the view is blended over this, not over black */
            edgeFrom.x = (float)(gViewWidth - 1);
            edgeFrom.y = 0;
            edgeFrom.w = 1;
            edgeFrom.h = (float)gViewHeight;
            edgeTo.x = gViewRect.x + gViewRect.w;
            edgeTo.y = gViewRect.y;
            edgeTo.w = kViewEdge;
            edgeTo.h = gViewRect.h;
            SDL_RenderTexture(gRenderer, gViewTexture, &edgeFrom, &edgeTo);
            DrawOverlays();
            if (!SDL_SetTextureBlendMode(gTexture, SDL_BLENDMODE_BLEND_PREMULTIPLIED))
                SDL_SetTextureBlendMode(gTexture, SDL_BLENDMODE_BLEND);
        } else {
            SDL_SetTextureBlendMode(gTexture, SDL_BLENDMODE_NONE);
        }
        SDL_RenderTexture(gRenderer, gTexture, NULL, NULL);
    }
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

/* every 16-bit pixel as the texture's ARGB8888, with the brightness and the white-out: x1r5g5b5, the top bit
   ignored but for kPlatSeeThrough, transparent -- which only a frame with a view under it is read with */
#define kColorCount     0x10000
#define kChannelSteps   32
#define kChannelTop     31

static Uint32 gColors[kColorCount];
static bool gColorsMade;
static double gExponent = 1.0;
static Uint32 gPlainChannel[32];        /* the brightness alone, for the GPU view's images and colours */
#define kKeepSeeThrough 0xffff
#define kNoSeeThrough   0x7fff
static unsigned gWhite;

/* a 5-bit channel value as 8 bits through the brightness */
static Uint32 ChannelOf(Uint32 v)
{
    if (gExponent == 1.0)
        return (v << 3) | (v >> 2);                     /* the top bits repeated, as a GPU widens 5 bits */
    return (Uint32)(SDL_pow((double)v / kChannelTop, gExponent) * 255.0 + 0.5);
}

static void MakeColors(void)
{
    Uint64 start = SDL_GetTicksNS();
    Uint32 channel[kChannelSteps];
    Uint32 c;

    for (c = 0; c < kChannelSteps; c++) {
        channel[c] = ChannelOf(c + (((kChannelTop - c) * gWhite + 128) >> 8));
        gPlainChannel[c] = ChannelOf(c);
    }
    for (c = 0; c < kColorCount; c++)
        gColors[c] = 0xff000000u | channel[(c >> 10) & 0x1f] << 16 | channel[(c >> 5) & 0x1f] << 8 | channel[c & 0x1f];
    gColors[kPlatSeeThrough] = 0;
    gColorsMade = true;
    gTimes.uploadNs += SDL_GetTicksNS() - start;
    /* what the textures hold is in the old colours */
    gShownValid = false;
    gViewFresh = gViewOn;
}

static void DropImageTextures(PlatImage *image)
{
    if (image->texture != NULL)
        SDL_DestroyTexture(image->texture);
    if (image->silhouette != NULL)
        SDL_DestroyTexture(image->silhouette);
    image->texture = NULL;
    image->silhouette = NULL;
}

void PlatVideoSetBrightness(double exponent)
{
    PlatImage *image;

    gExponent = exponent;
    MakeColors();
    for (image = gImages; image != NULL; image = image->next)
        DropImageTextures(image);
}

void PlatVideoSetWhite(unsigned level)
{
    if (level > 256)
        level = 256;
    if (gColorsMade && level == gWhite)
        return;
    gWhite = level;
    MakeColors();
}

/* Rows of 16-bit pixels into the part of a 32-bit texture at rect, from the same place in pixels: only the
   software renderer takes 5-5-5 pixels as they are, and SDL's own conversion of a whole frame takes several
   times as long as this one pass. */
static bool Convert(SDL_Texture *texture, const void *pixels, int pitch, const SDL_Rect *rect, Uint16 mask)
{
    const Uint16 *src;
    Uint32 *dst;
    void *texels;
    int texelPitch;
    int x;
    int y;

    if (!SDL_LockTexture(texture, rect, &texels, &texelPitch))
        return false;
    for (y = 0; y < rect->h; y++) {
        src = (const Uint16 *)((const char *)pixels + (size_t)(rect->y + y) * pitch) + rect->x;
        dst = (Uint32 *)((char *)texels + (size_t)y * texelPitch);
        for (x = 0; x < rect->w; x++)
            dst[x] = gColors[src[x] & mask];
    }
    SDL_UnlockTexture(texture);
    return true;
}

/* Row y of the frame against what the texture holds: false = the same, else the changed span */
static bool RowChanged(const void *pixels, int pitch, int y, int *left, int *right)
{
    const Uint16 *now;
    const Uint16 *was;
    int l;
    int r;

    if (!gShownValid) {
        *left = 0;
        *right = gFrameWidth;
        return true;
    }
    now = (const Uint16 *)((const char *)pixels + (size_t)y * pitch);
    was = gShown + (size_t)y * gFrameWidth;
    if (SDL_memcmp(now, was, (size_t)gFrameWidth * kBytesPerPixel) == 0)
        return false;
    l = 0;
    while (now[l] == was[l])
        l++;
    r = gFrameWidth;
    while (now[r - 1] == was[r - 1])
        r--;
    *left = l;
    *right = r;
    return true;
}

/* With a view under it the frame stays mostly the same (the view moves, not the page): only runs of rows
   that changed go into the texture, each from its leftmost to its rightmost changed pixel. */
static bool UploadChanged(const void *pixels, int pitch)
{
    SDL_Rect all = { 0, 0, gFrameWidth, gFrameHeight };
    SDL_Rect run;
    int y;
    int row;
    int left;
    int right;
    int runRight;
    int gap;

    if (gShown == NULL) {
        gShown = (Uint16 *)SDL_malloc((size_t)gFrameWidth * gFrameHeight * kBytesPerPixel);
        gShownValid = false;
        if (gShown == NULL)
            return Convert(gTexture, pixels, pitch, &all, kKeepSeeThrough);
    }
    y = 0;
    while (y < gFrameHeight) {
        if (!RowChanged(pixels, pitch, y, &left, &right)) {
            y++;
            continue;
        }
        run.x = left;
        run.y = y;
        runRight = right;
        gap = 0;
        for (y++; y < gFrameHeight && gap <= kRunGap; y++) {
            if (!RowChanged(pixels, pitch, y, &left, &right)) {
                gap++;
                continue;
            }
            gap = 0;
            if (left < run.x)
                run.x = left;
            if (right > runRight)
                runRight = right;
        }
        y -= gap;
        run.w = runRight - run.x;
        run.h = y - run.y;
        if (!Convert(gTexture, pixels, pitch, &run, kKeepSeeThrough)) {
            gShownValid = false;
            return false;
        }
        for (row = run.y; row < y; row++)
            SDL_memcpy(gShown + (size_t)row * gFrameWidth + run.x,
                       (const Uint16 *)((const char *)pixels + (size_t)row * pitch) + run.x,
                       (size_t)run.w * kBytesPerPixel);
    }
    gShownValid = true;
    return true;
}

/* -- the GPU view: images, and the draws rendered into gViewTexture ---------------------------------- */

/* a 5-5-5 colour as ARGB through the brightness alone: the white-out is laid over the view whole */
static Uint32 PlainColor(Uint16 color)
{
    return 0xff000000u | gPlainChannel[(color >> 10) & 0x1f] << 16 | gPlainChannel[(color >> 5) & 0x1f] << 8
           | gPlainChannel[color & 0x1f];
}

static SDL_Texture *ImageTexture(PlatImage *image, bool silhouette)
{
    SDL_Texture **slot = silhouette ? &image->silhouette : &image->texture;
    std::vector<Uint32> texels;
    Uint32 last = 0xffffffffu;
    Uint32 lastColor = 0;
    size_t i;

    if (*slot != NULL)
        return *slot;
    texels.resize((size_t)image->width * image->height);
    for (i = 0; i < texels.size(); i++) {
        if (image->alpha[i] == 0) {
            texels[i] = 0;
        } else if (silhouette) {
            texels[i] = 0xffffffffu;
        } else {
            if (image->pixels[i] != last) {
                last = image->pixels[i];
                lastColor = PlainColor((Uint16)last);
            }
            texels[i] = lastColor;
        }
    }
    *slot = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, image->width,
                              image->height);
    if (*slot == NULL)
        return NULL;
    SDL_UpdateTexture(*slot, NULL, texels.data(), image->width * 4);
    SDL_SetTextureScaleMode(*slot, SDL_SCALEMODE_NEAREST);
    return *slot;
}

PlatImage *PlatImageCreate(int width, int height, const uint16_t *pixels, const uint8_t *alpha)
{
    PlatImage *image;
    size_t count;

    if (width <= 0 || height <= 0)
        return NULL;
    count = (size_t)width * height;
    image = (PlatImage *)SDL_calloc(1, sizeof(PlatImage));
    if (image == NULL)
        return NULL;
    image->pixels = (Uint16 *)SDL_malloc(count * sizeof(Uint16));
    image->alpha = (Uint8 *)SDL_malloc(count);
    if (image->pixels == NULL || image->alpha == NULL) {
        SDL_free(image->pixels);
        SDL_free(image->alpha);
        SDL_free(image);
        return NULL;
    }
    image->width = width;
    image->height = height;
    image->next = gImages;
    if (gImages != NULL)
        gImages->prev = image;
    gImages = image;
    PlatImageUpdate(image, pixels, alpha);
    return image;
}

void PlatImageUpdate(PlatImage *image, const uint16_t *pixels, const uint8_t *alpha)
{
    size_t count;

    if (image == NULL)
        return;
    count = (size_t)image->width * image->height;
    if (image->texture != NULL && SDL_memcmp(image->pixels, pixels, count * sizeof(Uint16)) == 0
            && SDL_memcmp(image->alpha, alpha, count) == 0)
        return;
    SDL_memcpy(image->pixels, pixels, count * sizeof(Uint16));
    SDL_memcpy(image->alpha, alpha, count);
    DropImageTextures(image);
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

static void SetDrawColor(Uint16 color, Uint8 alpha)
{
    Uint32 argb = PlainColor(color);

    SDL_SetRenderDrawColor(gRenderer, (Uint8)(argb >> 16), (Uint8)(argb >> 8), (Uint8)argb, alpha);
}

static void RenderDraw(const PlatDraw &d, std::vector<SDL_Vertex> &scratch)
{
    SDL_FRect rect = { d.x, d.y, d.w, d.h };
    SDL_Texture *texture;
    Uint32 argb;
    int i;

    switch (d.kind) {
    case kPlatDrawImage:
        texture = d.image != NULL ? ImageTexture(d.image, false) : NULL;
        if (texture == NULL)
            return;
        SDL_SetTextureColorMod(texture, d.mod[0], d.mod[1], d.mod[2]);
        if (d.blend == kPlatBlendAdd) {
            SDL_SetTextureAlphaMod(texture, 255);
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
        } else {
            SDL_SetTextureAlphaMod(texture, d.alpha);
            if (!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND_PREMULTIPLIED))
                SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        }
        SDL_RenderTexture(gRenderer, texture, NULL, &rect);
        break;
    case kPlatDrawSilhouette:
        texture = d.image != NULL ? ImageTexture(d.image, true) : NULL;
        if (texture == NULL)
            return;
        argb = PlainColor(d.color);
        SDL_SetTextureColorMod(texture, (Uint8)(((argb >> 16) & 0xff) * d.mod[0] / 255),
                               (Uint8)(((argb >> 8) & 0xff) * d.mod[1] / 255), (Uint8)((argb & 0xff) * d.mod[2] / 255));
        SDL_SetTextureAlphaMod(texture, 255);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
        SDL_RenderTexture(gRenderer, texture, NULL, &rect);
        break;
    case kPlatDrawFill:
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
        SetDrawColor(d.color, d.alpha);
        SDL_RenderFillRect(gRenderer, &rect);
        break;
    case kPlatDrawTriangles:
        if (d.first < 0 || d.count <= 0 || (size_t)d.first + d.count > gVertices.size())
            return;
        scratch.resize((size_t)d.count);
        for (i = 0; i < d.count; i++) {
            const PlatVertex &v = gVertices[(size_t)d.first + i];
            argb = PlainColor(v.color);
            scratch[i].position.x = v.x;
            scratch[i].position.y = v.y;
            scratch[i].color.r = ((argb >> 16) & 0xff) / 255.0f;
            scratch[i].color.g = ((argb >> 8) & 0xff) / 255.0f;
            scratch[i].color.b = (argb & 0xff) / 255.0f;
            scratch[i].color.a = v.alpha / 255.0f;
            scratch[i].tex_coord.x = 0;
            scratch[i].tex_coord.y = 0;
        }
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
        SDL_RenderGeometry(gRenderer, NULL, scratch.data(), d.count, NULL, 0);
        break;
    }
}

/* the GPU view's draws into its render target, the white-out over them */
static bool RenderView(void)
{
    static std::vector<SDL_Vertex> scratch;
    SDL_FRect all = { 0, 0, (float)gViewWidth, (float)gViewHeight };

    if (gViewTexture == NULL) {
        gViewTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                         gViewWidth, gViewHeight);
        if (gViewTexture == NULL)
            return false;
        SDL_SetTextureBlendMode(gViewTexture, SDL_BLENDMODE_NONE);
        if (!SDL_SetTextureScaleMode(gViewTexture, gScaleMode))
            SDL_SetTextureScaleMode(gViewTexture, SDL_SCALEMODE_LINEAR);
    }
    if (!SDL_SetRenderTarget(gRenderer, gViewTexture))
        return false;
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(gRenderer);
    for (const PlatDraw &d : gDraws)
        RenderDraw(d, scratch);
    if (gWhite > 0) {
        SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, (Uint8)(gWhite >= 256 ? 255 : gWhite));
        SDL_RenderFillRect(gRenderer, &all);
    }
    SDL_SetRenderTarget(gRenderer, NULL);
    gViewFresh = false;
    return true;
}

/* the overlays over the view, in the frame's pixels */
static void DrawOverlays(void)
{
    SDL_Texture *texture;

    for (const PlatDraw &d : gOverlays) {
        SDL_FRect rect = { d.x, d.y, d.w, d.h };
        texture = d.image != NULL ? ImageTexture(d.image, false) : NULL;
        if (texture == NULL)
            continue;
        SDL_SetTextureColorMod(texture, d.mod[0], d.mod[1], d.mod[2]);
        SDL_SetTextureAlphaMod(texture, 255);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
        SDL_RenderTexture(gRenderer, texture, NULL, &rect);
    }
}

/* the view no longer shown: the frame alone, opaque */
static void DropView(void)
{
    gDraws.clear();
    gVertices.clear();
    gOverlays.clear();
    FreeDeadImages();
    gViewOn = false;
    gViewFresh = false;
    gShownValid = false;
}

/* the frame, and a view given since, into the textures; a view that cannot be is dropped */
static bool UploadFrame(const void *pixels, int pitch)
{
    SDL_Rect all = { 0, 0, gFrameWidth, gFrameHeight };
    Uint64 start;
    bool done;

    if (!gColorsMade)
        MakeColors();
    start = SDL_GetTicksNS();
    if (gViewFresh && !RenderView())
        DropView();
    if (gViewOn) {
        done = UploadChanged(pixels, pitch);
    } else {
        done = Convert(gTexture, pixels, pitch, &all, kNoSeeThrough);
        gShownValid = false;
    }
    gTimes.uploadNs += SDL_GetTicksNS() - start;
    return done;
}

/* false = the frame could not be put into the texture; a frame that was waiting still waits */
static bool ShowNow(const void *pixels, int pitch)
{
    bool shown;

    gShowStart = SDL_GetTicksNS();
    shown = UploadFrame(pixels, pitch);
    if (shown) {
        gFrameDrawn = true;
        gFrameFresh = false;
        Redraw();
    }
    gShowEnd = SDL_GetTicksNS();
    return shown;
}

/* a frame kept for later: a copy, as the game draws on in its own */
static void KeepFrame(const void *pixels, int pitch)
{
    int y;

    for (y = 0; y < gFrameHeight; y++)
        SDL_memcpy(gFrame + (size_t)y * gFrameWidth * kBytesPerPixel, (const char *)pixels + (size_t)y * pitch,
                   (size_t)gFrameWidth * kBytesPerPixel);
    gFrameFresh = true;
}

/* The game presents far more often than a display can show -- at start-up once per step of its progress
   bar, thousands of times -- and an upload and present takes milliseconds (CE had cnc-ddraw show the page
   from a thread of its own, so a flip cost the game nothing). A frame presented too soon after the last
   one waits in gFrame: the next present or event poll shows it once it is due, unless a newer frame has
   replaced it by then.
   Where a show takes longer than the time two are held apart (a large frame, a fast display) that alone
   would still show nearly every frame of a burst, so in a burst the game also gets kBurstRest times as long
   as the show took. Not in flight: there the frames come 21 ms apart and each is shown as it comes. */
static void ShowWaitingFrame(void)
{
    if (!gFrameFresh || gTexture == NULL || FrameDueIn() != 0)
        return;
    ShowNow(gFrame, gFrameWidth * kBytesPerPixel);
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

/* The frame's fit in the window: whole-pixel scaling when asked for and the window holds the frame at
   least once (SDL would crop it otherwise), else stretched with its aspect kept. */
static void ApplyPresentation(void)
{
    SDL_RendererLogicalPresentation mode;
    int w;
    int h;

    if (gRenderer == NULL || gFrameWidth <= 0 || gFrameHeight <= 0)
        return;
    mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
    if (gWholePixel && SDL_GetCurrentRenderOutputSize(gRenderer, &w, &h) && w >= gFrameWidth
            && h >= gFrameHeight)
        mode = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
    SDL_SetRenderLogicalPresentation(gRenderer, gFrameWidth, gFrameHeight, mode);
}

/* Called as SDL queues an event, on the main thread for the three taken here.
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
    } else if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        ApplyPresentation();
    } else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        next = (gFocusNoteTail + 1) & (kFocusNotes - 1);
        if (next != gFocusNoteHead) {
            gFocusNote[gFocusNoteTail] = FocusStaysInProgram();
            gFocusNoteTail = next;
        }
    }
    return true;
}

static bool MakeFrameTexture(void)
{
    if (gTexture != NULL)
        SDL_DestroyTexture(gTexture);
    if (gViewTexture != NULL)
        SDL_DestroyTexture(gViewTexture);
    gViewTexture = NULL;
    gViewFresh = gViewOn;
    for (PlatImage *image = gImages; image != NULL; image = image->next)
        DropImageTextures(image);
    gShownValid = false;
    gFrameDrawn = false;
    gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 gFrameWidth, gFrameHeight);
    if (gTexture == NULL)
        return false;
    SDL_SetTextureBlendMode(gTexture, SDL_BLENDMODE_NONE);
    if (!SDL_SetTextureScaleMode(gTexture, gScaleMode))
        SDL_SetTextureScaleMode(gTexture, SDL_SCALEMODE_LINEAR);
    /* a frame the game gave before (the graphics device was reset) goes into the new texture */
    if (!gFrameFresh && gGamePixels != NULL)
        KeepFrame(gGamePixels, gGamePitch);
    return true;
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
    ApplyPresentation();
    SDL_StartTextInput(gWindow);                           /* typed characters arrive as events */
    mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(gWindow));
    refresh = mode != NULL ? mode->refresh_rate : 0;
    if (refresh < 1)
        refresh = kDefaultRefresh;
    gShowInterval = (Uint64)(SDL_NS_PER_SECOND / refresh) * 3 / 4;
    if (gShowInterval > kMaxShowInterval)
        gShowInterval = kMaxShowInterval;
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
    gGamePixels = NULL;
    gFrameFresh = false;
    gFrameDrawn = false;
    gShowRest = 0;
    if (gViewTexture != NULL)
        SDL_DestroyTexture(gViewTexture);
    gViewTexture = NULL;
    SDL_free(gShown);
    gShown = NULL;
    gDraws.clear();
    gVertices.clear();
    gOverlays.clear();
    FreeDeadImages();
    gViewOn = false;
    gViewFresh = false;
    gShownValid = false;
}

void PlatVideoPresent(const void *pixels, int pitch)
{
    if (gTexture == NULL || gFrame == NULL)
        return;
    gGamePixels = pixels;
    gGamePitch = pitch;
    gShowRest = SDL_GetTicksNS() - gPresentEnd < kBurstGap ? (gShowEnd - gShowStart) * kBurstRest : 0;
    if (FrameDueIn() != 0 || !ShowNow(pixels, pitch))
        KeepFrame(pixels, pitch);
    gPresentEnd = SDL_GetTicksNS();
}

/* the view's draws copied, to be rendered into its texture with the frame they came with */
static void KeepView(const PlatVideoView *view)
{
    Uint64 start;

    if (view->width <= 0 || view->height <= 0)
        return;
    if (view->width != gViewWidth || view->height != gViewHeight) {
        if (gViewTexture != NULL)
            SDL_DestroyTexture(gViewTexture);
        gViewTexture = NULL;
        gViewWidth = view->width;
        gViewHeight = view->height;
    }
    start = SDL_GetTicksNS();
    gDraws.assign(view->draws, view->draws + (view->draws != NULL ? view->drawCount : 0));
    gVertices.assign(view->vertices, view->vertices + (view->vertices != NULL ? view->vertexCount : 0));
    gOverlays.assign(view->overlays, view->overlays + (view->overlays != NULL ? view->overlayCount : 0));
    FreeDeadImages();
    gTimes.uploadNs += SDL_GetTicksNS() - start;
    gViewRect.x = (float)view->x;
    gViewRect.y = (float)view->y;
    gViewRect.w = (float)view->w;
    gViewRect.h = (float)view->h;
    gViewOn = true;
    gViewFresh = true;
}

void PlatVideoPresentView(const void *pixels, int pitch, const PlatVideoView *view)
{
    if (gTexture == NULL || gFrame == NULL)
        return;
    if (view != NULL)
        KeepView(view);
    PlatVideoPresent(pixels, pitch);
}

void PlatVideoDropView(void)
{
    if (!gViewOn)
        return;
    /* a frame still waiting was made for the view: shown with it, or not at all */
    if (gFrameFresh && gTexture != NULL)
        ShowNow(gFrame, gFrameWidth * kBytesPerPixel);
    gFrameFresh = false;
    DropView();
}

void PlatVideoTakeTimes(PlatVideoTimes *times)
{
    *times = gTimes;
    SDL_zero(gTimes);
}

void PlatVideoSetScaling(int filter, int wholePixel)
{
    if (filter == kPlatFilterNearest)
        gScaleMode = SDL_SCALEMODE_NEAREST;
    else if (filter == kPlatFilterPixelArt)
        gScaleMode = SDL_SCALEMODE_PIXELART;
    else
        gScaleMode = SDL_SCALEMODE_LINEAR;
    gWholePixel = wholePixel != 0;
    if (gTexture != NULL && !SDL_SetTextureScaleMode(gTexture, gScaleMode))
        SDL_SetTextureScaleMode(gTexture, SDL_SCALEMODE_LINEAR);
    if (gViewTexture != NULL && !SDL_SetTextureScaleMode(gViewTexture, gScaleMode))
        SDL_SetTextureScaleMode(gViewTexture, SDL_SCALEMODE_LINEAR);
    ApplyPresentation();
    Redraw();
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
        if (gRenderer != NULL && gFrame != NULL)
            MakeFrameTexture();
        return false;
    case SDL_EVENT_RENDER_TARGETS_RESET:
        /* a render target's contents are gone: the GPU view is drawn again */
        gViewFresh = gViewOn;
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
    if (gFrameFresh) {
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
