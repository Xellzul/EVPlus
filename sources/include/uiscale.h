/* uiscale.h -- UI scaling: the UI is laid out in logical pixels (the game's own coordinates) and drawn in
   device pixels. A scaled pixel map holds more device pixels than its logical bounds span; Mapping converts
   between the two, one axis at a time. A pixel map whose size equals its bounds is not scaled. UI pixel maps
   (sized as ScaleUIPort sizes them) all map with the screen's own proportion: a logical edge falls on the
   same device pixel in each of them that starts where the screen does. */
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

    /* a horizontal device length in logical pixels, rounded */
    int LogicalLength(int device) const;

    /* a vertical logical length in device pixels, rounded */
    int DeviceLengthY(int logical) const;

    /* back from device to logical, for the mouse */
    int LogicalX(int device) const;
    int LogicalY(int device) const;

    double ScaleX() const { return (double)deviceX / logicalX; }
    double ScaleY() const { return (double)deviceY / logicalY; }

private:
    int left;
    int top;
    int logicalWidth;
    int logicalHeight;
    int deviceWidth;
    int deviceHeight;
    /* the proportion: device pixels per logical ones, as a fraction */
    int deviceX;
    int logicalX;
    int deviceY;
    int logicalY;
};

/* An offscreen port made at its logical size (NewGWorld) gets device pixels for the scale; its bounds stay
   logical. A scale of 1 leaves it as it is. */
void ScalePortPixels(CGrafPtr port, double scale);

/* The UI scale in use: device pixels per logical pixel on UI pixel maps; inside a MenuScale the menu scale.
   1 = the interface is not scaled. */
double Scale();
void SetScale(double scale);
inline bool Enabled() { return Scale() != 1.0; }
/* the interface (everything outside a MenuScale) is scaled */
bool InterfaceScaled();

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

/* The menu scale, for the main menu and the startup and intro pictures; set after SetScreenSize. */
void SetMenuScale(double scale);
/* a MenuScale is in effect and the menu scale differs from the interface's */
bool InMenuScale();

/* While it lives, the menu scale is in use: Scale(), the logical screen and the logical bounds of the window
   and loadPort are the menu's; device pixels stay. Nests; does nothing when both scales are the same. */
class MenuScale {
public:
    MenuScale();
    ~MenuScale();
    MenuScale(const MenuScale &) = delete;
    MenuScale &operator=(const MenuScale &) = delete;

private:
    bool switched;
    double scale;
    short screenWidth;
    short screenHeight;
    Rect windowBounds;
    CGrafPtr loadWorld;
    Rect loadBounds;
    Rect loadPixBounds;
};

} // namespace UI
