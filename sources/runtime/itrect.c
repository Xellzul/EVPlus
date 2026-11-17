/* itrect.c -- the interface tool's rectangle helpers (left, top, right, bottom as longs). Notes: Research/notes/itrect.md */

#include "burgertypes.h"
#include "burgerplat.h"

/* DT_GetColorARGB (an ITColor from a 0xAARRGGBB value). */
// FUNCTION: EVNOVA 0x00519cf0
ITColor DT_GetColorARGB(ULong argb)
{
    ITColor color;
    int32_t err;

    err = 0;
    color.alpha = (argb >> 24) & 0xff;
    color.red = (argb >> 16) & 0xff;
    color.green = (argb >> 8) & 0xff;
    color.blue = argb & 0xff;
    return color;
}

/* DT_SetRect (SetRect). */
// FUNCTION: EVNOVA 0x00519d30
ITRect *DT_SetRect(ITRect *r, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    int32_t err;

    err = 0;
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
    return r;
}

/* DT_IsPointInRect (PtInRect). */
// FUNCTION: EVNOVA 0x00519d58
unsigned char DT_IsPointInRect(const ITPoint *pt, const ITRect *r)
{
    unsigned char inside;
    int32_t err;

    err = 0;
    inside = 0;
    if (pt->h >= r->left && pt->v >= r->top && pt->h < r->right && pt->v < r->bottom)
        inside = 1;
    return inside;
}

/* DT_IsRectReallyEmpty (nothing to draw: turned inside out, or a single point). */
// FUNCTION: EVNOVA 0x00519d83
unsigned char DT_IsRectReallyEmpty(const ITRect *r)
{
    unsigned char none;
    int32_t err;

    err = 0;
    none = 0;
    if (r->right < r->left || r->bottom < r->top ||
        (r->left == r->right && r->top == r->bottom))
        none = 1;
    return none;
}

/* DT_IsRectEmpty (EmptyRect). */
// FUNCTION: EVNOVA 0x00519daf
unsigned char DT_IsRectEmpty(const ITRect *r)
{
    unsigned char empty;
    int32_t err;

    err = 0;
    empty = 0;
    if (r->left >= r->right || r->top >= r->bottom)
        empty = 1;
    return empty;
}

/* DT_IsRectEqual (EqualRect). */
// FUNCTION: EVNOVA 0x00519dcd
unsigned char DT_IsRectEqual(const ITRect *a, const ITRect *b)
{
    unsigned char equal;
    int32_t err;

    err = 0;
    equal = 0;
    if (a->left == b->left && a->top == b->top && a->right == b->right &&
        a->bottom == b->bottom)
        equal = 1;
    return equal;
}

/* DT_RectOffset (OffsetRect). */
// FUNCTION: EVNOVA 0x00519e00
ITRect *DT_RectOffset(ITRect *r, int32_t dh, int32_t dv)
{
    int32_t err;

    err = 0;
    r->left += dh;
    r->top += dv;
    r->right += dh;
    r->bottom += dv;
    return r;
}

/* DT_RectInset (InsetRect). */
// FUNCTION: EVNOVA 0x00519e22
ITRect *DT_RectInset(ITRect *r, int32_t dh, int32_t dv)
{
    int32_t err;

    err = 0;
    r->left += dh;
    r->top += dv;
    r->right -= dh;
    r->bottom -= dv;
    return r;
}

/* DT_RectCenter (`r' sized like `size', centred in `bounds' with bounds' origin at 0). */
// FUNCTION: EVNOVA 0x00519e44
ITRect *DT_RectCenter(ITRect *r, const ITRect *size, const ITRect *bounds)
{
    int32_t width;
    int32_t height;
    int32_t err;

    err = 0;
    width = size->right - size->left;
    height = size->bottom - size->top;
    r->right = width + (r->left = (bounds->right - bounds->left) / 2 - width / 2);
    r->bottom = height + (r->top = (bounds->bottom - bounds->top) / 2 - height / 2);
    return r;
}

/* DT_RectIntersect (SectRect, without the empty test). */
// FUNCTION: EVNOVA 0x00519eb6
ITRect *DT_RectIntersect(ITRect *r, const ITRect *a, const ITRect *b)
{
    int32_t err;

    err = 0;
    r->left = a->left > b->left ? a->left : b->left;
    r->top = a->top > b->top ? a->top : b->top;
    r->right = a->right < b->right ? a->right : b->right;
    r->bottom = a->bottom < b->bottom ? a->bottom : b->bottom;
    return r;
}

/* a bevel's light and dark shades of one component; the middle arm of LIGHTER never runs */
#define LIGHTER(c)  ((c) <= 0xa0 ? (c) + 0x40 : (c) <= 0xa0 ? (c) + (0xff - (c)) / 2 : 0xff)
#define DARKER(c)   ((c) >= 0x60 ? (c) - 0x30 : (c) >= 0x30 ? (c) / 2 : 0)

/* DT_ColorLighten (a colour lightened for a bevel's lit edge; alpha kept). */
// FUNCTION: EVNOVA 0x00519f06
void DT_ColorLighten(ITColor *color)
{
    int32_t err, err2;

    err2 = err = 0;
    color->red = LIGHTER(color->red);
    color->green = LIGHTER(color->green);
    color->blue = LIGHTER(color->blue);
}

/* DT_ColorDarken (a colour darkened for a bevel's shaded edge; alpha kept). */
// FUNCTION: EVNOVA 0x00519fb1
void DT_ColorDarken(ITColor *color)
{
    int32_t err, err2;

    err2 = err = 0;
    color->red = DARKER(color->red);
    color->green = DARKER(color->green);
    color->blue = DARKER(color->blue);
}

