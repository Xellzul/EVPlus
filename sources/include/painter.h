/* painter.h -- the interface's drawing: text styles, surfaces, the Painter; a call names its colour, font, place */
#pragma once

#include <string>
#include "scaleddraw.h"

namespace UI {

class Canvas;

/* a text style: font and size read from the game's data at each draw; an unknown font is drawn as Small */
enum class Style : unsigned char {
    Small,          /* smallTextFont at smallTextSize: lists, descriptions, labels */
    Status,         /* the status bar's font at its first size */
    StatusLarge,    /* the status bar's font at its second size */
    Menu,           /* the main menu's font */
    Button,         /* the picture buttons' labels */
    Title,          /* Times 18 */
    System,         /* font 0 at 12 */
    Comm,           /* font 0 at the comm strip's size */
    Version,        /* Small's font at 9 */
};

enum class Align : unsigned char { Center, Right };

/* how a copy combines with what is there */
enum class Blit : unsigned char {
    Copy,
    AddOver,
    AdMax,
};

inline constexpr RGBColor kBlack = { 0, 0, 0 };
inline constexpr RGBColor kWhite = { 0xffff, 0xffff, 0xffff };

struct TextSpec {
    Style style;
    RGBColor colour;
    bool bold = false;
};

/* something drawn on or copied from (screen, offscreen world, dialog, sprite frame); it does not own it */
class Surface {
public:
    explicit Surface(const GWorldContainer &world);
    explicit Surface(SWFrameRec *frame);

    static Surface Screen();                /* the game window */
    static Surface Load();                  /* the scratch page a dialog is drawn on before it is shown */
    static Surface Dialog(DialogPtr dlg);

    /* black ink, white paper, plain Small text: what the Toolbox's own drawing finds on a world just made */
    void Prepare() const;

private:
    friend class Painter;

    Surface(CGrafPtr port, SWFrameRec *frame) : port(port), frame(frame) {}

    CGrafPtr port;
    SWFrameRec *frame;
};

/* draws on one surface; Painters nest; a surface of a null world, dialog or frame draws nothing */
class Painter {
public:
    explicit Painter(const Surface &target);
    ~Painter();
    Painter(const Painter &) = delete;
    Painter &operator=(const Painter &) = delete;

    /* -- shapes -- */
    void Fill(const Rect &r, const RGBColor &colour);
    /* see-through on a surface shown over something (the screen over the space view); black elsewhere */
    void Clear(const Rect &r);
    void Frame(const Rect &r, const RGBColor &colour);         /* one pixel wide, inside r */
    void Line(short h0, short v0, short h1, short v1, const RGBColor &colour, short width = 1);
    void Oval(const Rect &r, const RGBColor &colour, short width = 1);     /* its outline, inside r */
    void FillOval(const Rect &r, const RGBColor &colour);
    void Pattern(const Rect &r, PixPatHandle pattern);

    /* -- pictures and copies -- */
    void Picture(PicHandle pic, const Rect &dst);              /* stretched to dst */
    void Icon(CIconHandle icon, const Rect &dst);              /* its pixels of colour 0 are left out */
    void Copy(const Surface &from, const Rect &src, const Rect &dst, Blit mode = Blit::Copy);
    void CopyMasked(const Surface &from, const Surface &mask, const Rect &src, const Rect &maskRect,
                    const Rect &dst);
    /* a sprite frame through its draw proc; on a sprite-frame surface only */
    void Sprite(DrawProcPtr draw, SWFrameRec *sprite, const Rect &src, const Rect &dst);

    /* while it lives, pieces drawn into area are put together and stretched onto a scaled surface as one */
    class Composition {
    public:
        Composition(Painter &painter, const Rect &area);
        ~Composition();
        Composition(const Composition &) = delete;
        Composition &operator=(const Composition &) = delete;

    private:
        ArtComposition art;
        Canvas *canvas;
        bool began;                 /* the canvas began one for this: ended with it */
    };

    /* -- text: a string's bytes are the game's own (Mac Roman); the baseline is at v -- */

    /* from h on; returns the pen's h after the text */
    short Text(short h, short v, const std::string &text, const TextSpec &spec);
    /* continues where this painter's last Text ended or Pen put it; draws nothing before one */
    short Text(const std::string &text, const TextSpec &spec);
    void Pen(short h, short v);
    /* placed between left and right: centred, or ending at right */
    short Text(short left, short right, short v, const std::string &text, const TextSpec &spec, Align align);
    /* word-wrapped into box over a filled background */
    void TextBox(const Rect &box, const void *text, int32_t length, const TextSpec &spec,
                 const RGBColor &background);
    void TextBox(const Rect &box, const std::string &text, const TextSpec &spec, const RGBColor &background)
    {
        TextBox(box, text.data(), (int32_t)text.size(), spec, background);
    }
    /* wrapped over what is there, scrolled up by scroll rows, spans in their colours; returns all lines' height */
    int TextSpans(const Rect &box, const void *text, uint32_t length, const TextSpec &spec, int scroll,
                  const TextSpan *spans, int spanCount);

    /* Measure: the layout width (bytes up to a NUL as ANSI, '&' a mark); DrawnWidth: the width Text draws */
    short Measure(const std::string &text, Style style);
    short DrawnWidth(const void *text, int length, Style style);
    /* the height of text wrapped as TextBox, TextSpans wrap it; a line's height */
    int WrappedHeight(const void *text, int32_t length, int width, Style style);
    int SpansHeight(const Rect &box, const void *text, uint32_t length, Style style);
    int LineHeight(Style style);

    /* -- Toolbox controls that draw themselves: a TextEdit record, a List Manager list -- */
    void EditSetText(TEHandle te, const void *text, int32_t length, Style style);  /* wrapped in the style */
    void EditMeasure(TEHandle te, Style style);                                    /* wrapped again */
    void EditDraw(TEHandle te, const Rect &area, const TextSpec &spec, const RGBColor &background);
    /* scrolled by dv rows, pinned to its ends, and drawn */
    void EditScroll(TEHandle te, short dv, const TextSpec &spec, const RGBColor &background);
    void List(ScrollRec **list, Style style);
    void ListSelect(ScrollRec **list, bool on, Point cell, Style style);           /* a row (de)selected, drawn */
    /* the surface of the list being drawn, for its draw proc; draws nothing outside List and ListSelect */
    static Surface ListSurface();

private:
    bool Bind();
    void Ink(const RGBColor &colour);
    void Font(Style style, bool bold);
    void Apply(const TextSpec &spec);
    void KeepPen();
    void CanvasText(const std::string &text, const RGBColor &colour);
    int CanvasWrapped(const Rect &box, const void *text, uint32_t length, const RGBColor &colour, int scroll,
                      const TextSpan *spans, int spanCount);

    CGrafPtr port;
    SWFrameRec *frame;
    Canvas *canvas;                 /* the port's, when it has a GPU target: drawn when its last Painter is done */
    CGrafPtr previous;
    Point pen;                      /* where the next continued Text starts */
    int32_t penOffset;
    bool penSet = false;
};

} // namespace UI
