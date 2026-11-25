/* screen.h -- the screen page the game draws into, and the frame shown from it */
#pragma once

#include <stdint.h>

/* the white-out: blend from..to of 256 over 60 Hz ticks */
#define kWhiteNone          0
#define kWhiteSolid         256
#define kWhiteFadeTicks     90                      /* 1.1.1's CGDisplayFade duration, 1.5 s */

namespace Screen {

/* 5-5-5 pixels, row 0 at the top; pixels is NULL (and the rest 0) while no display mode is set */
struct Page {
    char *pixels;
    int pitch;                      /* bytes per row */
    int width;
    int height;
    int depth;                      /* 15 */
};

const Page &Get();

bool Open(int width, int height, int depth);    /* a cleared page, after Close; false when it can't be had */
bool Resize(int width, int height);             /* a cleared page and frame of another size; false = the old kept */
void Close();

/* The frame: the space view the GPU draws, under the page, with the overlays over it (ComposeView, where the
   view is finished); the page holds see-through pixels where the view shows. */
void ComposeView();
void DropView();                    /* flight is over: the page alone from now on */

/* The page is shown only where a frame is finished -- the end of a loop's turn (EndFrame) or a wait for input
   or time (Idle) -- never by the helpers that draw it, so a screen built in steps is not shown half done. */
void Changed();                     /* the page has something new to show */
void EndFrame();                    /* a frame is finished: shown, if anything changed */
void Idle();                        /* a wait loop's turn: EndFrame, then a sleep until an event or 1 ms */
void FadeWhite(unsigned from, unsigned to, uint32_t ticks);

} // namespace Screen
