/* movies.cpp -- full-screen and looping movies */

#include "dialogs.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "uiscale.h"

Movie loopingMovie = NULL;

// FUNCTION: EVNOVA 0x0049d7c0
// MAC: _StartLoopingMovie i386 0x00060266 ppc 0x00067fb8
void StartLoopingMovie(const unsigned char *name, void *window, Rect *box)
{
    CGrafPtr port;
    char cname[0x100];
    char path[kMoviePathMax];
    const char *names[1];
    CGrafPtr *windowPort = (CGrafPtr *)window;

    if (settings.noMovies != 0)
        return;

    if (loopingMovie != NULL)
        EndLoopingMovie();

    port = *windowPort;
    PStr2CStr(cname, name);
    names[0] = cname;
    if (!MovieFindFile(names, 1, path, sizeof path))
        return;
    loopingMovie = MovieOpen(path);
    if (loopingMovie == NULL)
        return;

    Rect device = UI::Mapping(port->pix).DeviceRect(*box);
    MovieSetTarget(loopingMovie, port->pix.baseAddr, port->pix.rowBytes, device.right - device.left,
                   device.bottom - device.top, 1);
    MovieSetLoop(loopingMovie, 1);
    MovieRewind(loopingMovie);
    MovieStart(loopingMovie);
}

// FUNCTION: EVNOVA 0x0049da80
// MAC: _LoopingMovieIdle i386 0x000558d9 ppc 0x0005db04
void LoopingMovieIdle(void)
{
    if (loopingMovie != NULL) {
        MovieDraw(loopingMovie);
        nv_UpdateDialogWindow(0);
        ScreenEndScene();
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
    CGrafPtr port;
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
    port = GetCWindowGrafPort(gWind);
    Rect device = UI::Mapping(port->pix).DeviceRect(box);
    MovieSetTarget(movie, (char *)port->pix.baseAddr + device.top * port->pix.rowBytes +
                              (port->pix.pixelSize >> 3) * device.left,
                   port->pix.rowBytes, device.right - device.left, device.bottom - device.top, 0);
    InsetRect(&box, -1, -1);
    FrameRect(&box);
    MovieRewind(movie);
    if (fade != 0)
        FadeScreenIn();
    MovieStart(movie);
    /* only a new press skips: an input still down from before (the Return that confirmed a jump,
       the click on OK) counts once it has been let go */
    KeyboardKbhit();
    held = MovieSkipInputs();
    while (MovieDone(movie) == 0) {
        if (MovieDraw(movie))
            ScreenEndScene();
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
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
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
