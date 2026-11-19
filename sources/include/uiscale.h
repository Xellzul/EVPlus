/* uiscale.h -- UI scaling: the UI is laid out in logical pixels (the game's own coordinates) and drawn in
   device pixels. A scaled pixel map holds more device pixels than its logical bounds span; Mapping converts
   between the two, one axis at a time. A pixel map whose size equals its bounds is not scaled. */
#pragma once

#include "macport.h"

namespace UI {

class Mapping {
public:
    explicit Mapping(const PixMap &pix);

    bool Scaled() const { return logicalWidth != deviceWidth || logicalHeight != deviceHeight; }

    /* the device column / row where a logical edge or point falls, counted from the first pixel;
       neighbouring rects share their edges, so they meet without a gap */
    int X(int logical) const;
    int Y(int logical) const;
    Rect DeviceRect(const Rect &logical) const;

    /* back from device to logical, for the mouse */
    int LogicalX(int device) const;
    int LogicalY(int device) const;

    double ScaleX() const { return (double)deviceWidth / logicalWidth; }
    double ScaleY() const { return (double)deviceHeight / logicalHeight; }

private:
    int left;
    int top;
    int logicalWidth;
    int logicalHeight;
    int deviceWidth;
    int deviceHeight;
};

/* An offscreen port made at its logical size (NewGWorld) gets device pixels for the scale; its bounds stay
   logical. A scale of 1 leaves it as it is. */
void ScalePortPixels(CGrafPtr port, double scale);

/* The UI scale in use: device pixels per logical pixel on UI pixel maps. 1 = the interface is not scaled. */
double Scale();
void SetScale(double scale);
inline bool Enabled() { return Scale() != 1.0; }

/* A UI port (the screen's scratch pages, dialogs, popups, overlays) made at its logical size gets device
   pixels in the screen's own proportion, so UI ports of one logical size have one device size. */
void ScaleUIPort(CGrafPtr port);

/* the screen in logical pixels, and in device pixels */
short ScreenWidth();
short ScreenHeight();
void SetScreenSize(short width, short height, int deviceWidth, int deviceHeight);

/* a logical length or position on the screen, in device pixels */
int ScreenDeviceX(int logical);
int ScreenDeviceY(int logical);

} // namespace UI
