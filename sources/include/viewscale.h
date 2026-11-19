/* viewscale.h -- the space view drawn at the game size and enlarged onto the screen. */
#pragma once

#include "macport.h"

/* engine/gameloop.cpp: the comm line, floating map and escort menu laid over the screen after the view */
void DrawOverlays();

namespace View {

/* the sizes, set by SetupMonitor */
extern short gameWidth;             /* the game size (Resolution): what the game shows and plays at */
extern short width;                 /* the space view, in game pixels */
extern short height;

void SetSizes(short gameW, short gameH, short drawW, short drawH, double uiScale);
CWindowPtr Open();                  /* the view's buffer and port, for the sprite world */
void Flush();                       /* the view (enlarged) into its place on the screen, the overlays over it */
void ToView(Point *pt);             /* a screen point in view coordinates */
void SetPortToView();               /* the port of the space view itself */

/* The comm line, floating map and escort menu are drawn on the screen over the view, not inside it. Their
   frames get device pixels; Flush lays them over the screen. */
Rect OverlayFrameRect(const Rect &viewRect);    /* an overlay frame's logical rect for a rect in the view */
Rect OverlayLocalRect(const Rect &viewRect);    /* the same, from 0, 0: the frame's own coordinates */
Point ToOverlay(Point pt, const Rect &viewRect); /* a view point in the coordinates of the overlay at viewRect */
void ScaleOverlayFrame(SWFrameRec *frame);
Rect ScreenRectOfView(const Rect &viewRect);    /* a rect in the view, in device pixels on the screen */
int ScaleInView(int value);         /* a length of the interface in view pixels (the UI scale applied) */

/* Scratch frames of the screen's size for the main screen and dialogs. */
void CreateUIFrames(Rect bounds);
SWFrameRec *UIWork();
SWFrameRec *UIBack();
void SetPortToUIWork();
void SetPortToUIBack();

} // namespace View
