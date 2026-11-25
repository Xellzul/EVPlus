/* qdports.cpp -- QuickDraw ports: the current port, the screen port, GWorlds, pixel-map and window accessors */
#include "burger.h"
#include "macport.h"
#include "screen.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x004b8a70
BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port)
{
    BitMapPtr bits;

    if (port == NULL)
        bits = NULL;
    else
        bits = (BitMapPtr)&port->pix;
    return bits;
}

// FUNCTION: EVNOVA 0x004b8a90
void SetPort(CGrafPtr port)
{
    CurrentGrafPort = port;
}

// FUNCTION: EVNOVA 0x004b8aa0
void GetPort(CGrafPtr *port)
{
    *port = CurrentGrafPort;
}


CGrafPort gGamePort;

// FUNCTION: EVNOVA 0x004b8ae0
CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title)
{
    memset(&gGamePort, 0, sizeof(gGamePort));
    gGamePort.pnSize.v = 1;
    gGamePort.pnSize.h = gGamePort.pnSize.v;
    gGamePort.pix.bounds.top = bounds->top;
    gGamePort.pix.bounds.left = bounds->left;
    gGamePort.pix.bounds.bottom = bounds->bottom;
    gGamePort.pix.bounds.right = bounds->right;
    AttachGamePort();
    CurrentGrafPort = &gGamePort;
    return &gGamePort;
}

/* The game port onto the screen page as it is now (after it was opened or resized). */
void AttachGamePort(void)
{
    const Screen::Page &page = Screen::Get();

    gGamePort.pix.pixelSize = (page.depth + 7) & ~7;
    gGamePort.pix.baseAddr = page.pixels;
    gGamePort.pix.width = page.width;
    gGamePort.pix.height = page.height;
    gGamePort.pix.rowBytes = page.pitch;
    gGamePort.depth = page.depth;
}

/* The DIB is created top-down (negative biHeight) so row 0 is the top, and its rowBytes
   is the width rounded up to a multiple of eight. 16 maps to 15 because the port's DIBs
   are 555. */

/* The pixels of a port: zeroed, each row ending on a 4-byte boundary. The exe's are DIB sections, which
   GDI lays out so; NewGWorld takes that stride. */
#define kRowAlignBits   32

void *NewPortPixels(int width, int rows, int bits)
{
    size_t stride;

    stride = (size_t)((width * bits + kRowAlignBits - 1) / kRowAlignBits) * (kRowAlignBits / 8);
    return calloc(stride != 0 ? stride : 1, rows > 0 ? rows : 1);
}

// FUNCTION: EVNOVA 0x004bb870
short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds, void *theGDH)
{
    int bitCount;
    CGrafPtr gw;

    switch (depth) {
    case 16:
        bitCount = 15;
        break;
    default:
        bitCount = depth;
        break;
    }
    gw = (CGrafPtr)AllocAPointerClear(sizeof(CGrafPort));
    gw->pix.bounds.top = bounds->top;
    gw->pix.bounds.left = bounds->left;
    gw->pix.bounds.bottom = bounds->bottom;
    gw->pix.bounds.right = bounds->right;
    gw->pix.pixelSize = depth;
    gw->backColor.green = -1;
    gw->backColor.blue = gw->backColor.green;
    gw->backColor.red = gw->backColor.blue;
    gw->foreColor.green = 0;
    gw->foreColor.blue = gw->foreColor.green;
    gw->foreColor.red = gw->foreColor.blue;
    gw->pnSize.v = 1;
    gw->pnSize.h = gw->pnSize.v;
    gw->txFont = 0;
    gw->txFace = 0;
    gw->txSize = 9;
    gw->offscreen = 1;
    gw->pix.ctab = NULL;
    gw->pix.originV = 0;
    gw->pix.width = bounds->right - bounds->left;
    gw->pix.height = bounds->bottom - bounds->top;
    gw->depth = bitCount;
    /* one row more than the port is high, as the exe's bitmap has */
    gw->pixels = NewPortPixels(gw->pix.width, gw->pix.height + 1, depth);
    gw->pix.baseAddr = gw->pixels;
    gw->pix.rowBytes = (gw->pix.width * depth + 31) / 32 * 4;     /* rows padded to 4 bytes (NewPortPixels) */
    *out = gw;
    return 0;
}

// FUNCTION: EVNOVA 0x004bba00
void DisposeGWorld(CGrafPtr port)
{
    if (port == NULL)
        return;
    if (port->offscreen != 0)
        free(port->pixels);
    DeallocAPointer(port);
}

// FUNCTION: EVNOVA 0x004bba40
char *GetPixBaseAddr(PixMapPtr pm)
{
    return (char *)pm->baseAddr;
}

// FUNCTION: EVNOVA 0x004bba50
short GetPixRowBytes(PixMapPtr pm)
{
    return pm->rowBytes;
}

// FUNCTION: EVNOVA 0x004bba60
short GetPixDepth(PixMapPtr pm)
{
    return pm->pixelSize;
}

// FUNCTION: EVNOVA 0x004bba70
PixMapPtr GetGWorldPixMap(CGrafPtr world)
{
    return (PixMapPtr)world;
}


// FUNCTION: EVNOVA 0x004bba80
Rect *GetWindowPortRect(CWindowPtr window)
{
    return &window->pix.bounds;
}

// FUNCTION: EVNOVA 0x004bba90
Rect *GetCWindowPortRect(CWindowPtr window)
{
    return GetWindowPortRect(window);
}

// FUNCTION: EVNOVA 0x004bbaa0
CGrafPtr GetCWindowGrafPort(CWindowPtr window)
{
    return window;
}

// FUNCTION: EVNOVA 0x004bbab0
void SetPortWindowPort(CWindowPtr window)
{
    SetPort(GetCWindowGrafPort(window));
}

// FUNCTION: EVNOVA 0x004bbad0
void *GetGWorldDevice(CGrafPtr port)
{
    return NULL;
}

// FUNCTION: EVNOVA 0x004bbae0
PixMapPtr GetCWindowPixMap(CWindowPtr window)
{
    return &window->pix;
}

// FUNCTION: EVNOVA 0x004bbd20
void *GetGDevice(void)
{
    return NULL;
}
