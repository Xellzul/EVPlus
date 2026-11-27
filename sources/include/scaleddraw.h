/* scaleddraw.h -- the drawing primitives (rects, lines, ovals, copies) in logical coordinates, drawn in
   device pixels (uiscale.h); at scale 1 the two are the same. QuickDraw's calls in the port layer use these. */
#pragma once

#include "macport.h"

namespace Scaled {

void FillRect(const Rect &r, const RGBColor &color);
void InvertRect(const Rect &r);

/* a rect in the current port's device pixels */
void FillDeviceRect(const Rect &device, const RGBColor &color);
void InvertDeviceRect(Rect device);

void FrameRect(const Rect &r, const RGBColor &color);
/* a pen's logical size in device pixels at a scale, at least one */
int DevicePen(int logical, double scale);
void LineTo(short h, short v);
#if EVPLUS_TEST_PAGES
void FrameOval(const Rect &r);
void PaintOval(const Rect &r);
#endif
/* a copy's rects in the two pixel maps' device pixels */
void DeviceRects(const PixMap &src, const PixMap &dst, const Rect &srcRect, const Rect &dstRect, Rect &s, Rect &d);
void CopyBits(const PixMap &src, const PixMap &dst, const Rect &srcRect, const Rect &dstRect, int mode);
#if EVPLUS_TEST_PAGES
void CopyMask(const PixMap &src, const PixMap &mask, const PixMap &dst, const Rect &srcRect, const Rect &maskRect,
              const Rect &dstRect);
#endif

} // namespace Scaled

