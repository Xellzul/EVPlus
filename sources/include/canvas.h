/* canvas.h -- the Painter's drawing on a port with a GPU target: the same device pixels as the port's QuickDraw
   draws, as draws gathered per target and sent in one go when the last Painter on it is done. */
#pragma once

#include <vector>
#include "painter.h"
#include "platform.h"
#include "textlayout.h"

namespace UI {

class Canvas {
public:
    /* the canvas of the port's target, for one more Painter; NULL for a port without a target */
    static Canvas *Open(CGrafPtr port);
    void Close();                       /* that Painter is done: the last one's sends the draws */
    void Flush();                       /* the draws gathered so far sent now */
    static void FlushAll();             /* every canvas': before a target is read or an image freed */
    /* before a frame is shown: every canvas' sent; ports' images no frame copied from for long are freed */
    static void FrameDone();

    /* a port's target made at the size of its pixels, clear; false = not made, the target it had is kept */
    static bool MakeTarget(CGrafPtr port);
    /* a Painter still open on the port then draws nothing; a port disposed drops its own */
    static void DropTarget(CGrafPtr port);
    /* what is kept between draws -- the images of ports' pixels, scratch images and targets -- freed */
    static void Release();
    /* a port's target as drawn, in 16-bit pixels: `into' is the port's pixel map over them; false = not read */
    static bool ReadBack(CGrafPtr port, PixMap &into, std::vector<unsigned short> &pixels);

    void Fill(const Rect &r, const RGBColor &colour);
    void Clear(const Rect &r);          /* see-through: what lies under the target shows there */
    void Frame(const Rect &r, const RGBColor &colour);
    void Line(short h0, short v0, short h1, short v1, const RGBColor &colour, short width);
    void Oval(const Rect &r, const RGBColor &colour, short width);     /* width < 0: filled */
    void Pattern(const Rect &r, const Image_t *pattern);
    void Picture(const Image_t *pic, const Rect &dst);
    void Icon(const Image_t *icon, const Rect &dst);
    /* from a port with a target (also this one) or with 16-bit pixels */
    void Copy(CGrafPtr from, const Rect &src, const Rect &dst, Blit mode);
    /* a target's own alpha is its mask; pixels are copied where the mask port's are black */
    void CopyMasked(CGrafPtr from, CGrafPtr mask, const Rect &src, const Rect &maskRect, const Rect &dst);
    void Sprite(DrawProcPtr draw, const SWFrameRec *sprite, const Rect &src, const Rect &dst);
    void OwnPixels();                   /* the port's 16-bit pixels, whole and opaque, onto its target */

    /* between the two, on a scaled port: pictures, patterns and copies into area put together, stretched as one */
    bool BeginComposition(const Rect &area);    /* false = nothing to end: one is open, or the port is not scaled */
    void EndComposition();

    void Text(const TextLayout::Setup &setup, TextLayout::Pen &pen, const void *text, uint32_t length,
              const RGBColor &colour);
    /* wrapped into box, scrolled up by scroll rows, spans in their colours; returns all lines' height */
    int TextWrapped(const TextLayout::Setup &setup, const Rect &box, const void *text, uint32_t length,
                    const RGBColor &colour, int scroll, const TextSpan *spans, int spanCount);

    PlatDraw &Add(unsigned char kind, unsigned char blend);            /* a draw, in the target's device pixels */

private:
    static bool Waiting(const void *used);
    static void SendUsing(const void *used);
    static void PortGone(CGrafPtr port);
    PlatImage *Mirror(const PixMap &pix, Rect &part);
    static PlatImage *ScratchImage(int width, int height);
    static PlatTarget *ScratchTarget(int width, int height);
    static PlatTarget *CopyOfPart(PlatTarget *from, const Rect &part, int pad);

    void Touch();
    void Send(const PlatDraw *list, int count, const PlatRect *clip);
    void AddFill(int left, int top, int right, int bottom, const RGBColor &colour, unsigned char alpha);
    void AddImage(PlatImage *image, PlatTarget *source, const Rect *part, const Rect &place, unsigned char blend,
                  bool smooth);
    void AddPixels(const uint32_t *pixels, const Rect &place, unsigned char blend);
    void Compose(const uint32_t *pixels, int stride, const Rect &place);
    void ComposePixels(const PixMap &from, const PixMap *mask, const Rect &src, const Rect &maskRect,
                       const Rect &dst);
    void ComposeTarget(CGrafPtr from, const Rect &src, const Rect &dst);
    void CopyPixels(const PixMap &from, const PixMap *mask, const Rect &src, const Rect &maskRect, const Rect &dst,
                    unsigned char blend);
    void CopyTarget(CGrafPtr from, const Rect &src, const Rect &dst, unsigned char blend);

    CGrafPtr port = nullptr;
    PlatTarget *target = nullptr;
    int users = 0;
    std::vector<PlatDraw> draws;
    std::vector<const void *> used;             /* kept images and targets the draws use: changed after a flush */
    bool composing = false;
    Rect composed;                              /* the composition's logical area */
    std::vector<uint32_t> composition;          /* its pixels at the logical size; alpha 0 = not drawn */
};

} // namespace UI
