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
void Close();

/* The frame: the space view enlarged onto the page with the overlays over it (ComposeView, where the view
   is finished), then the page shown (Present, at the end of a scene). */
void ComposeView();
void Present();
void FadeWhite(unsigned from, unsigned to, uint32_t ticks);

} // namespace Screen
