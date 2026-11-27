/* screen.h -- the screen the game draws on, and the frame shown from it */
#pragma once

#include <stdint.h>

/* the white-out: blend from..to of 256 over 60 Hz ticks */
#define kWhiteNone          0
#define kWhiteSolid         256
#define kWhiteFadeTicks     90                      /* 1.1.1's CGDisplayFade duration, 1.5 s */

namespace Screen {

/* the screen in device pixels; all 0 while no display mode is set */
struct Page {
    int width;
    int height;
};

const Page &Get();

/* the game port's GPU target of that size, clear, is the interface's layer: see-through where nothing is drawn */
bool Open(int width, int height);               /* the screen of a display mode, after Close; false = no layer */
bool Resize(int width, int height);             /* a screen of another size; false = the old kept (layer and frame) */
void Close();

/* the canvas drew into the layer, within these device pixels */
void Drawn(int left, int top, int right, int bottom);

/* the frame: the space view the GPU draws under the layer, its place there see-through, the overlays over it */
void ComposeView();
void DropView();                    /* flight is over: the layer alone from now on */

/* shown only at a frame's end (EndFrame, Idle) */
void Changed();                     /* the screen has something new to show */
void EndFrame();                    /* a frame is finished: shown, if anything changed */
void Idle();                        /* a wait loop's turn: EndFrame, then a sleep until an event or 1 ms */
void FadeWhite(unsigned from, unsigned to, uint32_t ticks);

} // namespace Screen
