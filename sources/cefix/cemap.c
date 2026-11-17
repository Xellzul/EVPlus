/* cemap.c -- EV Nova CE: the galaxy map's border grid, recomputed over its visible part only.
   Notes: Research/cefix-plan.md */

#include "macport.h"
#include "stellar.h"
#include "cefix.h"

int ceMapBordersVisibleWidth = kCEBordersWidth;
int ceMapBordersVisibleHeight = kCEBordersHeight;

/* The map item's size in grid cells, taken before each rebuild. */
// BUG (CE F15.12/F16): the limit only ever shrinks, so one rebuild with a smaller rect would keep every
// later rebuild cut to it; every caller passes the map item's rect today, so it stays the dialog size
void CE_RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, Rect *r)
{
    short width;
    short height;

    width = (r->right - r->left) / 2;
    if (width < ceMapBordersVisibleWidth)
        ceMapBordersVisibleWidth = width;
    height = (r->bottom - r->top) / 2;
    if (height < ceMapBordersVisibleHeight)
        ceMapBordersVisibleHeight = height;
    RecalcMapInfluence(left, top, right, bottom, r);
}
