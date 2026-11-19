/* uiscale.cpp -- UI scaling: logical to device pixels and back. */
#include <math.h>
#include <stdlib.h>
#include "uiscale.h"

namespace UI {

namespace {

double gScale = 1.0;
short gScreenWidth;
short gScreenHeight;
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
}

int Mapping::X(int logical) const
{
    return MulDivRound(logical - left, deviceWidth, logicalWidth);
}

int Mapping::Y(int logical) const
{
    return MulDivRound(logical - top, deviceHeight, logicalHeight);
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

int Mapping::LogicalX(int device) const
{
    return left + MulDivFloor(device, logicalWidth, deviceWidth);
}

int Mapping::LogicalY(int device) const
{
    return top + MulDivFloor(device, logicalHeight, deviceHeight);
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

} // namespace UI
