/* painter.cpp -- UI::Painter through the port's GPU canvas; a port of pixels alone takes fills, pictures, copies */
#include "painter.h"

#include <vector>
#include "canvas.h"
#include "dialogs.h"
#include "fontraster.h"
#include "game.h"
#include "gameconst.h"
#include "render.h"
#include "spriteworld.h"
#include "textlayout.h"
#include "uiscale.h"

namespace UI {

namespace {

constexpr short kBoldFace = 1;
constexpr short kTitleSize = 18;
constexpr short kSystemSize = 12;
constexpr short kVersionSize = 9;

struct FontChoice {
    short font;
    short size;
};

FontChoice Lookup(Style style)
{
    switch (style) {
    case Style::Small:
        break;
    case Style::Status:
        return { statusFontID, statusFontSize[0] };
    case Style::StatusLarge:
        return { statusFontID, statusFontSize[1] };
    case Style::Menu:
        return { menuFontID, menuTextSize };
    case Style::Button:
        return { buttonFontID, buttonFontSize };
    case Style::Title:
        return { gTimesFont, kTitleSize };
    case Style::System:
        return { 0, kSystemSize };
    case Style::Comm:
        return { 0, kCommTextSize };
    case Style::Version:
        return { smallTextFont, kVersionSize };
    }
    return { smallTextFont, smallTextSize };
}

/* the style's font and size, Small's when the font cache has no such font */
FontChoice Resolve(Style style, short face)
{
    FontChoice choice = Lookup(style);
    FontSpec spec;

    if (!GetFont(choice.font, choice.size, face, &spec))
        choice = Lookup(Style::Small);
    return choice;
}

/* the port of the list List or ListSelect is drawing */
CGrafPtr listPort;

/* the port has pixels to draw on: one without a GPU target */
bool OnPixels(CGrafPtr port)
{
    return port->pix.baseAddr != NULL;
}

/* a drawing the pixels of a port without a GPU target do not take */
void NotDrawn()
{
    static int said;

    PlatDebugLogOnce(&said, "painter: only fills, pictures and plain copies are drawn on a port without a GPU target");
}

} // namespace

/* -- Surface -- */

Surface::Surface(const GWorldContainer &world) : port(world.world) {}

Surface::Surface(SWFrameRec *frame) : port(frame != nullptr ? frame->framePort : nullptr) {}

Surface Surface::Screen()
{
    return Surface(gWind != NULL ? GetCWindowGrafPort(gWind) : nullptr);
}

Surface Surface::Load()
{
    return Surface(loadPort);
}

// FUNCTION: EVNOVA 0x004d17a0
Surface Surface::Dialog(DialogPtr dlg)
{
    return Surface(dlg != NULL ? GetDialogPort(dlg) : nullptr);
}

void Surface::Prepare() const
{
    CGrafPtr current;

    if (port == nullptr)
        return;
    GetPort(&current);
    SetPort(port);
    RGBForeColor(&kBlack);
    RGBBackColor(&kWhite);
    TextFont(smallTextFont);
    TextFace(0);
    TextSize(smallTextSize);
    SetPort(current);
}

/* -- Painter -- */

Painter::Painter(const Surface &target) : port(target.port), canvas(Canvas::Open(target.port))
{
    GetPort(&previous);
    pen.h = pen.v = 0;
    penOffset = 0;
    if (port != nullptr)
        SetPort(port);
}

Painter::~Painter()
{
    if (canvas != nullptr)
        canvas->Close();
    SetPort(previous);
}

bool Painter::Bind()
{
    if (port == nullptr)
        return false;
    SetPort(port);
    return true;
}

void Painter::Ink(const RGBColor &colour)
{
    RGBForeColor(&colour);
}

void Painter::Font(Style style, bool bold)
{
    short face = bold ? kBoldFace : 0;
    FontChoice choice = Resolve(style, face);

    TextFont(choice.font);
    TextSize(choice.size);
    TextFace(face);
}

void Painter::Apply(const TextSpec &spec)
{
    Font(spec.style, spec.bold);
    Ink(spec.colour);
}

void Painter::KeepPen()
{
    pen = port->pnLoc;
    penOffset = port->pnDeviceOffset;
    penSet = true;
}

/* -- shapes -- */

void Painter::Fill(const Rect &r, const RGBColor &colour)
{
    Rect box = r;

    if (!Bind())
        return;
    if (canvas != nullptr) {
        canvas->Fill(r, colour);
        return;
    }
    if (!OnPixels(port))
        return;
    Ink(colour);
    PaintRect(&box);
}

void Painter::Clear(const Rect &r)
{
    if (Bind() && canvas != nullptr)
        canvas->Clear(r);
}

bool Painter::Drawn()
{
    if (!Bind())
        return false;
    if (canvas != nullptr)
        return true;
    NotDrawn();
    return false;
}

void Painter::Frame(const Rect &r, const RGBColor &colour)
{
    if (Drawn())
        canvas->Frame(r, colour);
}

void Painter::Line(short h0, short v0, short h1, short v1, const RGBColor &colour, short width)
{
    if (Drawn())
        canvas->Line(h0, v0, h1, v1, colour, width);
}

void Painter::Oval(const Rect &r, const RGBColor &colour, short width)
{
    if (Drawn())
        canvas->Oval(r, colour, width);
}

void Painter::FillOval(const Rect &r, const RGBColor &colour)
{
    if (Drawn())
        canvas->Oval(r, colour, -1);
}

void Painter::Pattern(const Rect &r, PixPatHandle pattern)
{
    if (Drawn())
        canvas->Pattern(r, pattern);
}

/* -- pictures and copies -- */

void Painter::Picture(PicHandle pic, const Rect &dst)
{
    Rect box = dst;

    if (!Bind())
        return;
    if (canvas != nullptr)
        canvas->Picture(pic, dst);
    else if (OnPixels(port))
        DrawPicHandle(pic, &box);
}

void Painter::Icon(CIconHandle icon, const Rect &dst)
{
    if (Drawn())
        canvas->Icon(icon, dst);
}

void Painter::Copy(const Surface &from, const Rect &src, const Rect &dst, Blit mode)
{
    if (port == nullptr || from.port == nullptr)
        return;
    if (canvas != nullptr) {
        canvas->Copy(from.port, src, dst, mode);
        return;
    }
    if (mode != Blit::Copy || from.port->target != NULL || !OnPixels(from.port) || !OnPixels(port)) {
        NotDrawn();
        return;
    }
    CopyBits(GetPortBitMapForCopyBits(from.port), GetPortBitMapForCopyBits(port), &src, &dst, 0, NULL);
}

void Painter::CopyMasked(const Surface &from, const Surface &mask, const Rect &src, const Rect &maskRect,
                         const Rect &dst)
{
    if (from.port != nullptr && mask.port != nullptr && Drawn())
        canvas->CopyMasked(from.port, mask.port, src, maskRect, dst);
}

void Painter::Sprite(DrawProcPtr draw, SWFrameRec *sprite, const Rect &src, const Rect &dst)
{
    if (Drawn())
        canvas->Sprite(draw, sprite, src, dst);
}

void Painter::OwnPixels()
{
    if (canvas != nullptr)
        canvas->OwnPixels();
}

Painter::Composition::Composition(Painter &painter, const Rect &area) : canvas(painter.canvas), began(false)
{
    if (canvas != nullptr)
        began = canvas->BeginComposition(area);
}

Painter::Composition::~Composition()
{
    if (began)
        canvas->EndComposition();
}

/* -- text -- */

/* from the port's pen, in the port's font, onto the canvas; the pen ends after the text */
void Painter::CanvasText(const std::string &text, const RGBColor &colour)
{
    TextLayout::Setup setup;
    TextLayout::Pen at;

    if (!PortTextSetup(&setup, &at))
        return;
    canvas->Text(setup, at, text.data(), (uint32_t)text.size(), colour);
    port->pnLoc.h = at.h;
    port->pnDeviceOffset = at.deviceOffset;
}

int Painter::CanvasWrapped(const Rect &box, const void *text, uint32_t length, const RGBColor &colour, int scroll,
                           const TextSpan *spans, int spanCount)
{
    TextLayout::Setup setup;
    TextLayout::Pen at;

    if (!PortTextSetup(&setup, &at))
        return 0;
    return canvas->TextWrapped(setup, box, text, length, colour, scroll, spans, spanCount);
}

short Painter::Text(short h, short v, const std::string &text, const TextSpec &spec)
{
    if (!Drawn())
        return h;
    Apply(spec);
    MoveTo(h, v);
    CanvasText(text, spec.colour);
    KeepPen();
    return pen.h;
}

// FUNCTION: EVNOVA 0x004cd1f0
// MAC: _DrawIndString i386 0x00072fd8 ppc 0x0007be50
short Painter::Text(const std::string &text, const TextSpec &spec)
{
    if (!penSet || !Drawn())
        return pen.h;
    Apply(spec);
    port->pnLoc = pen;
    port->pnDeviceOffset = penOffset;
    CanvasText(text, spec.colour);
    KeepPen();
    return pen.h;
}

void Painter::Pen(short h, short v)
{
    pen.h = h;
    pen.v = v;
    penOffset = 0;
    penSet = true;
}

// FUNCTION: EVNOVA 0x004622f0
short Painter::Text(short left, short right, short v, const std::string &text, const TextSpec &spec, Align align)
{
    short h = left;

    if (!Drawn())
        return h;
    Apply(spec);
    if (align == Align::Center)
        h = (short)((left + right) / 2 - MeasureCString(text.c_str()) / 2);
    else
        h = (short)(right - MeasureCString(text.c_str()));
    MoveTo(h, v);
    CanvasText(text, spec.colour);
    KeepPen();
    return pen.h;
}

void Painter::TextBox(const Rect &box, const void *text, int32_t length, const TextSpec &spec,
                      const RGBColor &background)
{
    if (!Drawn())
        return;
    Apply(spec);
    canvas->Fill(box, background);
    CanvasWrapped(box, text, (uint32_t)length, spec.colour, 0, NULL, 0);
}

int Painter::TextSpans(const Rect &box, const void *text, uint32_t length, const TextSpec &spec, int scroll,
                       const TextSpan *spans, int spanCount)
{
    if (!Drawn())
        return 0;
    Apply(spec);
    return CanvasWrapped(box, text, length, spec.colour, scroll, spans, spanCount);
}

short Painter::Measure(const std::string &text, Style style)
{
    if (!Bind())
        return 0;
    Font(style, false);
    return MeasureCString(text.c_str());
}

short Painter::DrawnWidth(const void *text, int length, Style style)
{
    if (!Bind())
        return 0;
    Font(style, false);
    return TextWidthMacRoman((const char *)text, length);
}

int Painter::WrappedHeight(const void *text, int32_t length, int width, Style style)
{
    if (!Bind())
        return 0;
    Font(style, false);
    return MeasureWrappedText((const char *)text, length, width);
}

int Painter::SpansHeight(const Rect &box, const void *text, uint32_t length, Style style)
{
    Rect r = box;

    if (!Bind())
        return 0;
    Font(style, false);
    return MeasureTextSpans(text, length, &r);
}

int Painter::LineHeight(Style style)
{
    if (!Bind())
        return 0;
    Font(style, false);
    return TextLineHeight();
}

/* -- Toolbox records: measured through the current port, drawn here -- */

void Painter::EditSetText(TEHandle te, const void *text, int32_t length, Style style)
{
    if (!Bind())
        return;
    Font(style, false);
    TESetText(text, length, te);
}

void Painter::EditMeasure(TEHandle te, Style style)
{
    if (!Bind())
        return;
    Font(style, false);
    TECalText(te);
}

// FUNCTION: EVNOVA 0x004bcf30
void Painter::EditDraw(TEHandle te, const Rect &area, const TextSpec &spec, const RGBColor &background)
{
    if (!Bind())
        return;
    Fill(area, background);
    TextSpans(area, (*te)->text, (uint32_t)(*te)->textLength, spec, (*te)->viewRect.top - (*te)->destRect.top,
              NULL, 0);
    ScreenEndScene();
}

void Painter::EditScroll(TEHandle te, short dv, const TextSpec &spec, const RGBColor &background)
{
    if (!Bind())
        return;
    TEPinScroll(0, dv, te);
    EditDraw(te, (*te)->viewRect, spec, background);
    nv_UpdateDialogWindow(0);
}

void Painter::List(ScrollRec **list, Style style)
{
    if (!Bind())
        return;
    Font(style, false);
    listPort = port;
    LUpdate(list);
    listPort = nullptr;
}

void Painter::ListSelect(ScrollRec **list, bool on, Point cell, Style style)
{
    if (!Bind())
        return;
    Font(style, false);
    listPort = port;
    LSetSelect(on, cell, list);
    listPort = nullptr;
}

Surface Painter::ListSurface()
{
    return Surface(listPort);
}

} // namespace UI
