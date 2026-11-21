/* uiscale.cpp -- UI scaling: logical to device pixels and back. */
#include <math.h>
#include "uiscale.h"

namespace UI {

namespace {

double gScale = 1.0;
double gMenuScale = 1.0;
bool gMenuActive;
short gScreenWidth;
short gScreenHeight;
short gMenuWidth;
short gMenuHeight;
int gDeviceWidth;
int gDeviceHeight;

/* a * b / c rounded to nearest, for any sign of a (b, c > 0) */
int MulDivRound(int a, int b, int c)
{
    long long n = (long long)a * b * 2 + c;
    long long d = (long long)c * 2;

    return (int)(n >= 0 ? n / d : -((-n + d - 1) / d));
}

/* a * b / c rounded down (b, c > 0): the logical pixel a device pixel lies in */
int MulDivFloor(int a, int b, int c)
{
    long long n = (long long)a * b;

    return (int)(n >= 0 ? n / c : -((-n + c - 1) / c));
}

} // namespace

Mapping::Mapping(const PixMap &pix)
    : left(pix.bounds.left),
      top(pix.bounds.top),
      logicalWidth(pix.bounds.right - pix.bounds.left),
      logicalHeight(pix.bounds.bottom - pix.bounds.top),
      deviceWidth(pix.width),
      deviceHeight(pix.height)
{
    if (logicalWidth <= 0 || deviceWidth <= 0)
        logicalWidth = deviceWidth = 1;
    if (logicalHeight <= 0 || deviceHeight <= 0)
        logicalHeight = deviceHeight = 1;
    deviceX = deviceWidth;
    logicalX = logicalWidth;
    deviceY = deviceHeight;
    logicalY = logicalHeight;
    if (Scaled() && gScreenWidth > 0 && gScreenHeight > 0 && deviceWidth == ScreenDeviceX(logicalWidth)
            && deviceHeight == ScreenDeviceY(logicalHeight)) {
        deviceX = gDeviceWidth;
        logicalX = gScreenWidth;
        deviceY = gDeviceHeight;
        logicalY = gScreenHeight;
    }
}

int Mapping::X(int logical) const
{
    return MulDivRound(logical - left, deviceX, logicalX);
}

int Mapping::Y(int logical) const
{
    return MulDivRound(logical - top, deviceY, logicalY);
}

Rect Mapping::DeviceRect(const Rect &logical) const
{
    Rect device;

    device.left = (short)X(logical.left);
    device.top = (short)Y(logical.top);
    device.right = (short)X(logical.right);
    device.bottom = (short)Y(logical.bottom);
    return device;
}

int Mapping::LogicalLength(int device) const
{
    return MulDivRound(device, logicalX, deviceX);
}

int Mapping::DeviceLengthY(int logical) const
{
    return MulDivRound(logical, deviceY, logicalY);
}

int Mapping::LogicalX(int device) const
{
    return left + MulDivFloor(device, logicalX, deviceX);
}

int Mapping::LogicalY(int device) const
{
    return top + MulDivFloor(device, logicalY, deviceY);
}

namespace {

/* an offscreen port's pixels made anew at a device size; its bounds stay logical */
void ResizePixels(CGrafPtr port, int width, int height)
{
    constexpr int kRowAlignBits = 32;
    int depth = port->pix.pixelSize;

    if (port->offscreen == 0)
        return;
    free(port->pixels);
    port->pix.width = width;
    port->pix.height = height;
    port->pnDeviceOffset = 0;                  /* the pen's device remainder belonged to the old size */
    port->pixels = NewPortPixels(width, height + 1, depth);
    port->pix.baseAddr = port->pixels;
    port->pix.rowBytes = (width * depth + kRowAlignBits - 1) / kRowAlignBits * (kRowAlignBits / 8);
}

} // namespace

void ScalePortPixels(CGrafPtr port, double scale)
{
    if (scale == 1.0)
        return;
    ResizePixels(port, (int)floor((port->pix.bounds.right - port->pix.bounds.left) * scale + 0.5),
                 (int)floor((port->pix.bounds.bottom - port->pix.bounds.top) * scale + 0.5));
}

void ScaleUIPort(CGrafPtr port)
{
    if (!Enabled())
        return;
    ResizePixels(port, ScreenDeviceX(port->pix.bounds.right - port->pix.bounds.left),
                 ScreenDeviceY(port->pix.bounds.bottom - port->pix.bounds.top));
}

short ScreenWidth()
{
    return gScreenWidth;
}

short ScreenHeight()
{
    return gScreenHeight;
}

void SetScreenSize(short width, short height, int deviceWidth, int deviceHeight)
{
    gScreenWidth = width;
    gScreenHeight = height;
    gDeviceWidth = deviceWidth;
    gDeviceHeight = deviceHeight;
}

int ScreenDeviceX(int logical)
{
    return gScreenWidth > 0 ? MulDivRound(logical, gDeviceWidth, gScreenWidth) : logical;
}

int ScreenDeviceY(int logical)
{
    return gScreenHeight > 0 ? MulDivRound(logical, gDeviceHeight, gScreenHeight) : logical;
}

double Scale()
{
    return gScale;
}

void SetScale(double scale)
{
    gScale = scale > 0 ? scale : 1.0;
}

void SetMenuScale(double scale)
{
    gMenuScale = scale > 0 ? scale : 1.0;
    gMenuWidth = (short)(gDeviceWidth / gMenuScale + 0.5);
    gMenuHeight = (short)(gDeviceHeight / gMenuScale + 0.5);
}

bool InMenuScale()
{
    return gMenuActive;
}

namespace {

void SetLogicalSize(Rect &bounds, short width, short height)
{
    bounds.right = (short)(bounds.left + width);
    bounds.bottom = (short)(bounds.top + height);
}

} // namespace

MenuScale::MenuScale() : switched(false)
{
    if (gMenuActive || gMenuScale == gScale || gMenuWidth <= 0 || gMenuHeight <= 0)
        return;
    switched = true;
    scale = gScale;
    screenWidth = gScreenWidth;
    screenHeight = gScreenHeight;
    windowBounds = gGamePort.pix.bounds;
    loadWorld = loadPort.world;
    gScale = gMenuScale;
    gScreenWidth = gMenuWidth;
    gScreenHeight = gMenuHeight;
    gMenuActive = true;
    SetLogicalSize(gGamePort.pix.bounds, gMenuWidth, gMenuHeight);
    gGamePort.pnDeviceOffset = 0;
    if (loadWorld != NULL) {
        loadBounds = loadPort.bounds;
        loadPixBounds = loadWorld->pix.bounds;
        SetLogicalSize(loadPort.bounds, gMenuWidth, gMenuHeight);
        SetLogicalSize(loadWorld->pix.bounds, gMenuWidth, gMenuHeight);
        loadWorld->pnDeviceOffset = 0;
    }
}

MenuScale::~MenuScale()
{
    if (!switched)
        return;
    gScale = scale;
    gScreenWidth = screenWidth;
    gScreenHeight = screenHeight;
    gMenuActive = false;
    gGamePort.pix.bounds = windowBounds;
    gGamePort.pnDeviceOffset = 0;
    if (loadWorld != NULL && loadPort.world == loadWorld) {
        loadPort.bounds = loadBounds;
        loadWorld->pix.bounds = loadPixBounds;
        loadWorld->pnDeviceOffset = 0;
    }
}

} // namespace UI
