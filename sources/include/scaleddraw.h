/* scaleddraw.h -- the drawing primitives (rects, lines, ovals, copies) in logical coordinates, drawn in
   device pixels (uiscale.h); at scale 1 the two are the same. QuickDraw's calls in the port layer use these. */
#pragma once

#include "macport.h"

namespace Scaled {

void FillRect(const Rect &r, const RGBColor &color);
void InvertRect(const Rect &r);
void FrameRect(const Rect &r, const RGBColor &color);
void LineTo(short h, short v);
void FrameOval(const Rect &r);
void PaintOval(const Rect &r);
void CopyBits(const PixMap &src, const PixMap &dst, const Rect &srcRect, const Rect &dstRect, int mode);
void CopyMask(const PixMap &src, const PixMap &mask, const PixMap &dst, const Rect &srcRect, const Rect &maskRect,
              const Rect &dstRect);

} // namespace Scaled

namespace UI {

/* Pictures drawn piece by piece onto a scaled port (a dialog background in three parts, a button's ends and
   middle), put together at their own size and stretched onto the port in one go when the composition ends, so
   the pieces meet without seams. While one exists, plain copies (CopyBits srcCopy, CopyMask) into its port go
   to it; other transfer modes draw on the port directly. Does nothing on a port that is not scaled. */
class ArtComposition {
public:
    ArtComposition(CGrafPtr port, const Rect &area);
    ~ArtComposition();

private:
    CGrafPtr target;
};

/* for CopyBits / CopyMask: true when the copy went into the composition */
bool ComposeCopy(PixMap *src, PixMap *dst, const Rect *srcRect, const Rect *dstRect, int mode);
bool ComposeMask(PixMap *src, PixMap *mask, PixMap *dst, const Rect *srcRect, const Rect *maskRect,
                 const Rect *dstRect);

} // namespace UI

