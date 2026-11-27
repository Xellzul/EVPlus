/* movies.cpp -- full-screen and looping movies */

#include "dialogs.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "uiscale.h"
#include "screen.h"
#include "painter.h"

#define kMovieDepth     16              /* a movie's frames are decoded as 16-bit pixels */

Movie loopingMovie = NULL;
static GWorldContainer loopingFrame;    /* the looping movie's frames */
static DialogPtr loopingDialog;         /* the dialog it plays in */

/* the movie's frames decoded into a world of the box's device pixels on `port', the world's bounds the box */
static bool DecodeInto(GWorldContainer &frame, Movie movie, CGrafPtr port, const Rect &box)
{
    Rect device = UI::Mapping(port->pix).DeviceRect(box);

    OffsetRect(&device, -device.left, -device.top);
    CreateGWorldContainer(&frame, 0, kMovieDepth, &device);
    if (frame.world == NULL)
        return false;
    frame.world->pix.bounds = frame.bounds = box;
    MovieSetTarget(movie, frame.world->pix.baseAddr, frame.world->pix.rowBytes, frame.world->pix.width,
                   frame.world->pix.height, 0);
    return true;
}

// FUNCTION: EVNOVA 0x0049d7c0
// MAC: _StartLoopingMovie i386 0x00060266 ppc 0x00067fb8
void StartLoopingMovie(const std::string &name, DialogPtr dlg, const Rect &box)
{
    char path[kMoviePathMax];
    const char *names[1];

    if (settings.noMovies != 0)
        return;

    if (loopingMovie != NULL)
        EndLoopingMovie();

    names[0] = name.c_str();
    if (!MovieFindFile(names, 1, path, sizeof path))
        return;
    loopingMovie = MovieOpen(path);
    if (loopingMovie == NULL)
        return;
    if (!DecodeInto(loopingFrame, loopingMovie, GetDialogPort(dlg), box)) {
        EndLoopingMovie();
        return;
    }
    loopingDialog = dlg;
    MovieSetLoop(loopingMovie, 1);
    MovieRewind(loopingMovie);
    MovieStart(loopingMovie);
}

void DrawLoopingMovie(UI::Painter &p)
{
    if (loopingMovie != NULL)
        p.Copy(UI::Surface(loopingFrame), loopingFrame.bounds, loopingFrame.bounds);
}

// FUNCTION: EVNOVA 0x0049da80
// MAC: _LoopingMovieIdle i386 0x000558d9 ppc 0x0005db04
void LoopingMovieIdle(void)
{
    if (loopingMovie != NULL) {
        if (MovieDraw(loopingMovie)) {
            UI::Painter p(UI::Surface::Dialog(loopingDialog));
            DrawLoopingMovie(p);
        }
        nv_UpdateDialogWindow(0);
    }
}

// FUNCTION: EVNOVA 0x0049dab0
// MAC: _EndLoopingMovie i386 0x00055934 ppc 0x0005db80
void EndLoopingMovie(void)
{
    if (loopingMovie != NULL) {
        MovieClose(loopingMovie);
        loopingMovie = NULL;
    }
    DisposeGWorldContainer(&loopingFrame);
    loopingDialog = NULL;
}

/* The inputs that skip a movie, a bit each: any mouse button, Space, Return, keypad Enter. */
static unsigned MovieSkipInputs(void)
{
    return (MouseReadButtons() != 0 ? 1u : 0u) | (KeyIsDown(kScanSpace) != 0 ? 2u : 0u)
        | (KeyIsDown(kScanReturn) != 0 ? 4u : 0u) | (KeyIsDown(kScanPadEnter) != 0 ? 8u : 0u);
}

// FUNCTION: EVNOVA 0x0049db00
// MAC: _PlayQTMovie i386 0x00055957 ppc 0x0005dbc4
void PlayQTMovie(unsigned char *name, unsigned char doubled, unsigned char fade)
{
    unsigned held;
    unsigned down;
    char nameBuf[256];
    char path[kMoviePathMax];
    const char *names[1];
    short dw, dh;
    short w, h;
    int movieW, movieH;
    Rect movieBox;
    Rect box;
    Movie movie;
    GWorldContainer frame;
    float ratio;

    if (settings.noMovies != 0)
        return;
    PStr2CStr(nameBuf, name);
    names[0] = nameBuf;
    if (!MovieFindFile(names, 1, path, sizeof path))
        return;
    movie = MovieOpen(path);
    if (movie == NULL)
        return;
    if (fade != 0) {
        FadeScreenOut();
        HideTheCursor();
    }
    MovieSize(movie, &movieW, &movieH);
    box.left = 0;
    box.top = 0;
    box.right = movieW;
    box.bottom = movieH;
    if (doubled != 0) {
        box.right = movieW * 2 + 1;
        box.bottom = movieH * 2 + 1;
    }
    /* shrunk to fit the screen if it is bigger, keeping its shape, and centred */
    w = UI::ScreenWidth();
    h = UI::ScreenHeight();
    dw = box.right - box.left;
    dh = box.bottom - box.top;
    if (dw > w) {
        ratio = w / (float)dw;
        dw = (short)(dw * ratio);
        dh = (short)(dh * ratio);
    }
    if (dh > h) {
        ratio = h / (float)dh;
        dw = (short)(dw * ratio);
        dh = (short)(dh * ratio);
    }
    SetRect(&box, 0, 0, dw, dh);
    SetRect(&movieBox, 0, 0, w, h);
    CenterRect(&box, &movieBox);
    /* each frame decoded is copied onto the screen */
    DecodeInto(frame, movie, GetCWindowGrafPort(gWind), box);
    InsetRect(&box, -1, -1);
    UI::Painter(UI::Surface::Screen()).Frame(box, UI::kBlack);
    MovieRewind(movie);
    if (fade != 0)
        FadeScreenIn();
    MovieStart(movie);
    /* only a new press skips: an input still down from before (the Return that confirmed a jump,
       the click on OK) counts once it has been let go */
    KeyboardKbhit();
    held = MovieSkipInputs();
    while (MovieDone(movie) == 0) {
        if (MovieDraw(movie)) {
            UI::Painter(UI::Surface::Screen()).Copy(UI::Surface(frame), frame.bounds, frame.bounds);
            Screen::Changed();
            Screen::EndFrame();
        }
        KeyboardKbhit();
        down = MovieSkipInputs();
        held &= down;
        if ((down & ~held) != 0) {
            FlushEvents(0xf, 0);
            break;
        }
        LastTick = ReadTick();
        WaitTicks(2);
    }
    MovieStop(movie);
    if (fade != 0) {
        FadeScreenOut();
        ShowTheCursor();
    }
    MovieClose(movie);
    DisposeGWorldContainer(&frame);
    RedrawDialogs(1);
}

// FUNCTION: EVNOVA 0x0049e3c0
// MAC: _PlayQTMovieFromDesc i386 0x00055f11 ppc 0x0005e194
void PlayQTMovieFromDesc(short id)
{

    strcpy(descTextPtr, "");
    LoadDescription(descTextPtr, id);
    if (descMovieName[0] != 0)
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));
}
