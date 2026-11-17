// COMPILE: -lang c++ -bool off
/* gworld.c -- the GWorldContainer wrapper around an offscreen drawing world (win TU region 0x0046f740..0x0046f95a). Notes: Research/notes/gworld.md */
#include "macport.h"
#include "render.h"
#include "cefix.h"

static CGrafPtr savedPort;
static void *savedDevice;


// FUNCTION: EVNOVA 0x0046f740
// MAC: _CreateGWorldContainer i386 0x000a4517 ppc 0x000b001c
void CreateGWorldContainer(GWorldContainer *gwc, int32_t flags, short depth,
                           Rect *bounds)
{
    if (gwc != 0) {
        RGBColor fore = { 0, 0, 0 };
        RGBColor back = { 0xffff, 0xffff, 0xffff };
        short useDepth;

        GetPort(&savedPort);
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
#ifdef CEFIX /* F11.01 */
        if (CE_NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
#else
        if (NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
#endif
        gwc->bounds.top = bounds->top;
        gwc->bounds.left = bounds->left;
        gwc->bounds.bottom = bounds->bottom;
        gwc->bounds.right = bounds->right;
        SetPort(gwc->world);
        RGBForeColor(&fore);
        RGBBackColor(&back);
        TextFont(smallTextFont);
        TextFace(0);
        TextSize(smallTextSize);
        SetPort(savedPort);
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
        SetGWorldContainer(gwc);
        DrawPicHandle(pic, &gwc->bounds);
        UnsetGWorldContainer(gwc);
        QDReleaseResource(pic);
    } else {
        gwc->world = 0;
    }
}

// FUNCTION: EVNOVA 0x0046f8e0
// MAC: _SetGWorldContainer i386 0x000a4613 ppc 0x000b0120
void SetGWorldContainer(GWorldContainer *gwc)
{
    GetPort(&savedPort);
    savedDevice = GetGWorldDevice(savedPort);
    if (gwc->world != 0)
        SetPort(gwc->world);
}

// FUNCTION: EVNOVA 0x0046f910
// MAC: _UnsetGWorldContainer i386 0x000a464a ppc 0x000b0180
void UnsetGWorldContainer(GWorldContainer *gwc)
{
    SetPort(savedPort);
}

// FUNCTION: EVNOVA 0x0046f920
// MAC: _DisposeGWorldContainer i386 0x000a4669 ppc 0x000b0194
void DisposeGWorldContainer(GWorldContainer *gwc)
{
    if (gwc->world != NULL) {
        DisposeGWorld(gwc->world);
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
