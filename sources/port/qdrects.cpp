/* qdrects.cpp -- QuickDraw's Rect maths: OffsetRect, SetRect, InsetRect, SectRect, PtInRect. */
#include "macport.h"

// FUNCTION: EVNOVA 0x004b8d50
void OffsetRect(Rect *r, int dh, int dv)
{
    if (r == NULL)
        return;
    r->left += dh;
    r->right += dh;
    r->top += dv;
    r->bottom += dv;
}

// FUNCTION: EVNOVA 0x004b8d80
void SetRect(Rect *r, int left, int top, int right, int bottom)
{
    if (r == NULL)
        return;
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
}

// FUNCTION: EVNOVA 0x004b8dc0
void InsetRect(Rect *r, int dh, int dv)
{
    if (r == NULL)
        return;
    r->left += dh;
    r->right -= dh;
    r->top += dv;
    r->bottom -= dv;
}

// FUNCTION: EVNOVA 0x004b8df0
int SectRect(const Rect *a, const Rect *b, Rect *dst)
{
    int meet;
    int edge;
    int other;
    Rect scratch;

    if (dst == NULL)
        dst = &scratch;

    edge = a->left;
    other = b->left;
    if (edge < other)
        edge = other;
    dst->left = edge;

    edge = a->right;
    other = b->right;
    if (edge >= other)
        edge = other;
    dst->right = edge;

    edge = a->top;
    other = b->top;
    if (edge < other)
        edge = other;
    dst->top = edge;

    edge = a->bottom;
    other = b->bottom;
    if (edge >= other)
        edge = other;
    dst->bottom = edge;

    meet = 1;
    if (dst->left >= dst->right || dst->top >= dst->bottom) {
        meet = 0;
        dst->left = 0;
        dst->right = 0;
        dst->top = 0;
        dst->bottom = 0;
    }
    return meet;
}

// FUNCTION: EVNOVA 0x004b8e90
int PtInRect(int h, int v, Rect *r)
{
    int inside;

    inside = 0;
    if (h < r->right && h >= r->left && v < r->bottom && v >= r->top)
        inside = 1;
    return inside;
}
