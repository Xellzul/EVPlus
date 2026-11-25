/* viewscale.h -- the space view: its sizes, its place on the screen, where space lies in it; the overlay and UI
   frames. */
#pragma once

#include "macport.h"

namespace Screen {
struct Page;
}

/* engine/gameloop.cpp: the comm line, floating map and escort menu laid over the screen after the view */
void DrawOverlays();

namespace View {

/* the sizes, set by SetupMonitor */
extern short gameWidth;             /* the game size (Resolution): what the game shows and plays at */
extern short width;                 /* the space view, in game pixels */
extern short height;

/* uiScale: the UI's device pixels per logical one */
void SetSizes(short gameW, short gameH, short drawW, short drawH, double uiScale);
Rect OnPage(const Screen::Page &page);      /* where the view shows on a page */
void ToView(Point *pt);             /* a screen point in view coordinates */

/* Where a point of space lies in the view: its distance from the player's ship -- truncated, as the original
   places sprites (X, Y) -- plus the view's centre; or unrounded (ExactX, ExactY), as it places beams and
   stellars, and as a renderer drawing between pixels would use it. */
int X(float spaceX);
int Y(float spaceY);
float ExactX(float spaceX);
float ExactY(float spaceY);

/* The comm line, floating map and escort menu are drawn on the screen over the view, not inside it. Their
   frames get device pixels; Screen::ComposeView lays them over the screen. */
Rect OverlayFrameRect(const Rect &viewRect);    /* an overlay frame's logical rect for a rect in the view */
Rect OverlayLocalRect(const Rect &viewRect);    /* the same, from 0, 0: the frame's own coordinates */
Point ToOverlay(Point pt, const Rect &viewRect); /* a view point in the coordinates of the overlay at viewRect */
void ScaleOverlayFrame(SWFrameRec *frame);
Rect ScreenRectOfView(const Rect &viewRect);    /* a rect in the view, in device pixels on the screen */
int ScaleInView(int value);         /* a length of the interface in view pixels (the UI scale applied) */

/* Scratch frames of the screen's size for the main screen and dialogs; inside a UI::MenuScale the menu's.
   CreateUIFrames replaces the ones made before. */
void CreateUIFrames(Rect bounds);
SWFrameRec *UIWork();
SWFrameRec *UIBack();
void SetPortToUIWork();
void SetPortToUIBack();

} // namespace View
