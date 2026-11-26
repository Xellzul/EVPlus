/* gworld.cpp -- the GWorldContainer wrapper around an offscreen drawing world (win 0x0046f740, 0x0046f850, 0x0046f920, 0x0046f940). Notes: Research/notes/gworld.md */
#include "render.h"
#include "painter.h"


// FUNCTION: EVNOVA 0x0046f740
// MAC: _CreateGWorldContainer i386 0x000a4517 ppc 0x000b001c
void CreateGWorldContainer(GWorldContainer *gwc, int32_t flags, short depth,
                           Rect *bounds)
{
    if (gwc != 0) {
        short useDepth;

        // BUG: depth 0 and below already match `depth < 8', so the next two branches never run and 1-bit masks
        // are made 16-bit
        if (depth < 8)
            useDepth = 16;
        else if (depth == 0)
            useDepth = 16;
        else if (depth <= 0)
            useDepth = 8;
        else
            useDepth = depth;
        if (NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
        gwc->bounds.top = bounds->top;
        gwc->bounds.left = bounds->left;
        gwc->bounds.bottom = bounds->bottom;
        gwc->bounds.right = bounds->right;
        UI::Surface(*gwc).Prepare();
    }
}

// FUNCTION: EVNOVA 0x0046f850
// MAC: _CreateGWorldContainerFromPictResource i386 0x000a46b7 ppc 0x000b0208
void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, int32_t flags,
                                           short depth, short picID)
{
    PicHandle pic;
    Rect bounds;

    pic = GetPicture(picID);
    if (pic != 0) {
        QDGetPictureBounds(pic, &bounds);
        OffsetRect(&bounds, -bounds.left, -bounds.top);
        CreateGWorldContainer(gwc, flags, depth, &bounds);
        UI::Painter(UI::Surface(*gwc)).Picture(pic, gwc->bounds);
        QDReleaseResource(pic);
    } else {
        gwc->world = 0;
    }
}

// FUNCTION: EVNOVA 0x0046f920
// MAC: _DisposeGWorldContainer i386 0x000a4669 ppc 0x000b0194
void DisposeGWorldContainer(GWorldContainer *gwc)
{
    if (gwc->world != NULL) {
        DisposeGWorld(gwc->world);
        gwc->world = NULL;
    }
}

// FUNCTION: EVNOVA 0x0046f940
// MAC: _GetGWCPixMap i386 0x000a469e ppc 0x000b01f0
PixMapPtr GetGWCPixMap(GWorldContainer *gwc)
{
    if (gwc != 0 && gwc->world != 0)
        return GetCWindowPixMap(gwc->world);
    return 0;
}
