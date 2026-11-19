/* macport.h -- Mac OS Toolbox types and shims for the Windows port. Notes: Research/notes/macport.md */
#include "gametypes.h"
#include "int32.h"
#include "burgertypes.h"
#include "keycodes.h"

#ifndef MACPORT_H
#define MACPORT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char *StringPtr;
typedef uint32_t OSType;

/* QuickDraw basics (partial) */
typedef struct Point {
    short v;
    short h;
} Point;

#pragma pack(push, 1)
typedef struct EventRecord {
    unsigned short what;
    int32_t message;
    int32_t when;
    Point where;
    short modifiers;
} EventRecord;
#pragma pack(pop)

typedef struct FSSpec {
    short vRefNum;
    int32_t parID;
    Str255 name;
} FSSpec;

typedef struct CGrafPort *CWindowPtr;
typedef struct ControlRecord {
    int32_t kind;
    int32_t itemNo;
    int32_t value;
    int32_t minValue;
    int32_t maxValue;
    Rect box;
    void *data;
    DialogPtr owner;
    Boolean enabled;
} ControlRecord;
typedef struct ControlRecord *ControlPtr;
/* the Toolbox names the port's records answer to */
typedef ControlPtr ControlRef;
typedef Image_t *CIconHandle;
typedef Image_t *PixPatHandle;
typedef void *Movie;

typedef struct PixMap {
    Rect bounds;
    int32_t pixelSize;
    void *baseAddr;
    unsigned char *ctab;
    int32_t originV;
    int32_t width;
    int32_t height;
    int32_t rowBytes;
} PixMap, *PixMapPtr;
typedef struct CGrafPort {
    Boolean offscreen;
    Boolean bigEndian;              /* 16-bit pixels big-endian: a NewGWorldFromPtr with k16BE555PixelFormat */
    char filler1[2];
    PixMap pix;
    int32_t depth;
    void *pixels;                   /* the port's own pixel memory; NULL for a port over pixels it does not own */
    short lastFont;                 /* the font, size and style text was last drawn or measured with here: */
    short lastSize;                 /* what an unknown font number leaves in use (port/qdtext.c) */
    short lastFace;
    Boolean hasLastFont;
    RGBColor foreColor;
    RGBColor backColor;
    Point pnLoc;
    Point pnSize;
    short txFont;
    short txFace;
    short txSize;
} CGrafPort, *CGrafPtr;
/* one port record serves every port, so these are Carbon's: a GrafPtr is a CGrafPtr */
typedef CGrafPtr GrafPtr;
typedef CWindowPtr WindowRef;
typedef struct BitMap *BitMapPtr;

typedef struct GWorldContainer {
    CGrafPtr world;
    Rect bounds;
} GWorldContainer;

extern void GetIndString(unsigned char *str, int strListID, int index);
extern void GetIniIndString(char *out, int32_t listID, int32_t index);
extern void nv_UpdateDialogWindow(int all);
EXTERN_CPP_BEGIN
extern GWorldContainer iconsPort;
EXTERN_CPP_END
extern void SelectDialogItemText(DialogPtr dlg, int32_t item, int32_t start, int32_t end);
extern void FrameRect(const Rect *r);
extern void GetFNum(const unsigned char *name, short *familyID);
/* the font test page (port/fonttest.c) and what it asks the font cache */
extern void DebugFontScreen(void);
extern short FontFamilyCount(void);
extern const char *FontFamilyName(short familyID);
extern void FontRealName(short familyID, short size, short style, char *out, int outSize);
extern void DrawTextRun(const void *text, uint32_t byteCount, Rect *box,
                        Boolean clipped, int height);
extern CGrafPtr CurrentGrafPort;
extern void CopyBits(BitMapPtr srcBits, BitMapPtr dstBits,
                     const Rect *srcRect, const Rect *dstRect,
                     int32_t mode, void *maskRgn);
extern BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port);
extern void BlockMove(const void *src, void *dst, uint32_t len);
extern void CopyMask(BitMapPtr srcBits, BitMapPtr maskBits, BitMapPtr dstBits,
                     Rect *srcRect, Rect *maskRect, Rect *dstRect);
/* the engine's older spelling of a port's bits, (BitMap *)*GetPortPixMap; the port has one call */
#define GetPortPixMapBits(port) GetPortBitMapForCopyBits(port)
/* a GWorldContainer's bits for CopyBits: the mac takes its GWorld's port, the win port the container */
#define GWorldContainerBits(gwc) ((BitMapPtr)GetGWCPixMap(gwc))
/* the same as a CopyBits source through the GWorld's pixmap (the status panels' spelling) */
#define GWorldContainerPixMap(gwc) GWorldContainerBits(gwc)
/* drawing into a GWorldContainer: the mac sets its GWorld's port, the win port the container */
#define GWorldContainerSetPort(gwc) SetGWorldContainer(gwc)
/* a Point in hand: the mac passes it whole, the win port's PtInRect takes h and v */
#define PtInRectPoint(pt, r) PtInRect((pt).h, (pt).v, r)
extern void *GetPortVisRgn(void *port);
extern short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds,
                       void *theGDH);
extern PixMapPtr GetGWorldPixMap(CGrafPtr world);
/* QuickTime's GWorld calls (media/quicktime.c); DisposeGWorld frees these too */
extern short NewGWorldFromPtr(CGrafPtr *gw, OSType pixelFormat, const Rect *bounds, void *cTable,
                              void *aGDevice, int32_t flags, void *baseAddr, int32_t rowBytes);
extern Boolean LockPixels(PixMapPtr pm);
extern void GetGWorld(CGrafPtr *port, void **device);
extern void SetGWorld(CGrafPtr port, void *device);
extern PixMapPtr GetWindowPixMap(CWindowPtr window);
extern void QDGetCIconBounds(Image_t *icon, Rect *r);
extern PicHandle GetPictureNoGamma(int picID);

extern void CopyBitsNone(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsCopy8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsOr8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsCopy16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsAddOver16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsAdMax16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect);
extern void CopyBitsScale8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xStart, int32_t yStart);
extern void CopyMaskNone(PixMapPtr src, PixMapPtr mask, PixMapPtr dst,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);
extern void CopyMask16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
        Rect *srcRect, Rect *maskRect, Rect *dstRect);

extern CGrafPtr GetCWindowGrafPort(CWindowPtr window);
extern Rect *GetWindowPortRect(CWindowPtr window);
extern Rect *GetCWindowPortRect(CWindowPtr window);
extern PixMapPtr GetCWindowPixMap(CWindowPtr window);
extern PixMapPtr GetGWCPixMap(GWorldContainer *gwc);

typedef struct SWFrameRec {
    CGrafPtr framePort;
    void *frameDevice;
    PixMapPtr framePixHndl;
    PixMapPtr framePix;
    char *frameBaseAddr;
    uint32_t frameRowBytes;
    short leftAlignFactor;
    short rightAlignFactor;
    Boolean isFrameLocked;
    Boolean isWindowFrame;
    Boolean interlacingIsOn;
    Boolean skipOddLines;
    short frameDepth;
    Rect frameRect;
    short hotSpotH;
    short hotSpotV;
    Rect collisionInset;
    Point offsetPoint;
    void *maskRgn;
    CGrafPtr maskPort;
    void *maskDevice;
    PixMapPtr maskPixHndl;
    PixMapPtr maskPix;
    char *maskBaseAddr;
    Boolean tileMaskIsSolid;
    Boolean sharesGWorld;
    Boolean usesCollisionInset;
    Boolean framePad57;
    unsigned short useCount;
    short worldRectOffset;
    unsigned short numScanLines;
    unsigned short pixelShift;
    uint32_t *scanLinePtrArray;
    void *rleDataH;
    void *rleTokenStart;
    int32_t userData;
} SWFrameRec;

typedef SWFrameRec *FramePtr;

typedef void (*DrawProcPtr)(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                            Rect *srcRect, Rect *dstRect);
struct SpriteWorldRec;
typedef void (*WorldProcPtr)(struct SpriteWorldRec *spriteWorldP);

typedef struct SpriteWorldRec {
    struct SpriteLayerRec *headSpriteLayerP;
    struct SpriteLayerRec *tailSpriteLayerP;
    struct SpriteLayerRec *deadSpriteLayerP;
    struct UpdateRectStruct *headUpdateRectP;
    SWFrameRec *extraBackFrameP;
    SWFrameRec *backFrameP;
    SWFrameRec *workFrameP;
    SWFrameRec *windowFrameP;
    DrawProcPtr offscreenDrawProc;
    DrawProcPtr screenDrawProc;
    void (*doubleRectDrawProc)(SWFrameRec *, SWFrameRec *, Rect *, Rect *, Rect *, Rect *);
    WorldProcPtr postEraseCallBack;
    WorldProcPtr postDrawCallBack;
    Rect windRect;
    Rect backRect;
    Rect originalWindRect;
    Rect originalBackRect;
    Rect visScrollRect;
    Rect oldVisScrollRect;
    Rect offscreenScrollRect;
    short horizScrollRectOffset;
    short vertScrollRectOffset;
    short horizScrollDelta;
    short vertScrollDelta;
    Rect scrollRectMoveBounds;
    void (*worldMoveProc)(struct SpriteWorldRec *, struct SpriteRec *);
    struct SpriteRec *followSpriteP;
    struct TileMapStruct **tileLayerArray;
    short lastActiveTileLayer;
    Boolean tilingIsInitialized;
    Boolean tilingIsOn;
    short **tilingCache;
    short numTilingCacheRows;
    short numTilingCacheCols;
    SWFrameRec **tileFrameArray;
    short *curTileImage;
    short maxNumTiles;
    short tileWidth;
    short tileHeight;
    int32_t numTilesChanged;
    Rect *changedTiles;
    int32_t changedTilesArraySize;
    WorldProcPtr tileChangeProc;
    void (*tileRectDrawProc)(struct SpriteWorldRec *, Rect *, Boolean);
    DrawProcPtr tileMaskDrawProc;
    DrawProcPtr partialMaskDrawProc;
    void *mainSWGDH;
    short pixelDepth;
    short frameInterval;
    uint32_t runningTimeCount;
    unsigned long long lastMicroseconds;
    uint32_t timeOfLastFrame;
    Boolean frameHasOccurred;
    Boolean thereAreNonScrollingLayers;
    Boolean pad1;
    RGBColor backgroundColor;
    int32_t backgroundValue;
    Boolean fastBlit;
    char swFillerED[0xf8 - 0xed];
} SpriteWorldRec, *SpriteWorldPtr;

extern void SWSetPortToWindow(SpriteWorldPtr spriteWorld);

typedef struct SpriteLayerRec {
    struct SpriteLayerRec *nextSpriteLayerP;
    struct SpriteLayerRec *prevSpriteLayerP;
    struct SpriteRec *headSpriteP;
    struct SpriteRec *tailSpriteP;
    void (*layerCallBack)(struct SpriteWorldRec *);
    int32_t userData;
    short tileLayer;
    Boolean isPaused;
    Boolean layerIsNonScrolling;
} SpriteLayerRec, *SpriteLayerPtr;
typedef struct RLEPixelData {
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    unsigned short palette;
    unsigned short frames;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned short reserved3;
} RLEPixelData, *RLEPixelDataPtr, **RLEPixelDataHdl;
#define kRLEResourceHeaderSize  sizeof(RLEPixelData)

#define noErr           0
#define memFullErr      (-108)
#define resNotFound     (-192)
#define srcCopy         0
#define addOver         34              /* QuickDraw arithmetic transfer modes */
#define adMax           37
#define kControlInactivePart 255
#define noNewDevice     2               /* QuickTime GWorldFlags */
#define k16BE555PixelFormat 0x00000010  /* QuickTime pixel formats */
#define k16LE555PixelFormat 0x4c353535  /* 'L555' */

#define SW_ROWBYTES(frame, rows) \
        ((frame)->scanLinePtrArray[(rows) - (frame)->frameRect.top])
#define SW_PIXELBYTES(frame, pixels)    ((pixels) << (frame)->pixelShift)
#define SW_SPLIT15(c) ( (((c) & 0x3e0) << 15) | ((c) & 0x7c1f) )

#define ABS(x) (((x)<0) ? (-(x)) : (x))
#define SIGN(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define SW_PIXELPTR(frame, x, y) \
    ((frame)->frameBaseAddr + SW_ROWBYTES(frame, y) + SW_PIXELBYTES(frame, x))
#define SW_PIXEL16AT(frame, xBytes, y)  \
    (*(unsigned short *)( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + (xBytes) ))

#define SW_PIXELPTR16(frame, x, y)  \
    ((unsigned short *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + ((x)+(x)) ))
#define SW_PIXELPTR32(frame, x, y)  \
    ((uint32_t  *) ( (frame)->frameBaseAddr + SW_ROWBYTES(frame,y) + ((x) << 2) ))

/* DEPTH is the switch variable FOREACH_DEPTH_DO binds. */
#define FOREACH_DEPTH_DO(depth, instructions)   \
    switch ( depth )    \
    {   case 16:    { int DEPTH=16; instructions } break;   \
        case 32:    { int DEPTH=32; instructions } break; }

#define GET_DEPTH_PIXEL(frame, x, y)            \
    ( DEPTH==16 ?   *SW_PIXELPTR16(frame,x,y) : \
                    *SW_PIXELPTR32(frame,x,y) )

#define SET_DEPTH_PIXEL(frame, x, y, c)         \
    { if(DEPTH==16) *SW_PIXELPTR16(frame,x,y)=c; else   \
                    *SW_PIXELPTR32(frame,x,y)=c; }

#define BP_CLIP_RECT(frameRectP, srcRect, dstRect) \
    if (dstRect.top < frameRectP->top) \
    { \
        srcRect.top += frameRectP->top - dstRect.top; \
        dstRect.top =  frameRectP->top; \
    } \
    if (dstRect.bottom > frameRectP->bottom) \
    { \
        srcRect.bottom -= dstRect.bottom - frameRectP->bottom; \
        dstRect.bottom = frameRectP->bottom; \
    } \
    if (dstRect.bottom <= dstRect.top) \
        return; \
    if (dstRect.left < frameRectP->left) \
    { \
        srcRect.left += frameRectP->left - dstRect.left; \
        dstRect.left = frameRectP->left; \
    } \
    if (dstRect.right > frameRectP->right) \
    { \
        srcRect.right -= dstRect.right - frameRectP->right; \
        dstRect.right = frameRectP->right; \
    } \
    if (dstRect.right <= dstRect.left) \
        return;

#define SW_MIN(a, b)            ((a) < (b) ? (a) : (b))
#define SW_MAX(a, b)            ((a) > (b) ? (a) : (b))

/* a sprite frame's RLE stream: a token word carries a kTokenShift-shifted opcode and a byte count */
#define kEndShapeToken          0
#define kLineStartToken         1
#define kDrawPixelsToken        2
#define kSkipPixelsToken        3

typedef struct SpriteRec SpriteRec, *SpritePtr;

typedef void (*FrameProcPtr)(SpritePtr srcSpriteP, SWFrameRec *curFrameP,
                             int32_t *frameIndex);
typedef void (*MoveProcPtr)(SpritePtr srcSpriteP);
typedef void (*CollideProcPtr)(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *sectRect);

typedef enum RemovalType {
    kSWDontRemoveSprite = 0,
    kSWRemoveSprite,
    kSWRemoveAndDisposeSprite
} RemovalType;

typedef enum AdvanceType {
    kSWWrapAroundMode = 0,
    kSWPatrollingMode
} AdvanceType;

struct SpriteRec {
    SpriteLayerRec *parentSpriteLayerP;
    SpritePtr nextSpriteP;
    SpritePtr prevSpriteP;
    SpritePtr nextActiveSpriteP;
    SpritePtr nextIdleSpriteP;
    unsigned char isVisible;
    Boolean needsToBeDrawn;
    Boolean needsToBeErased;
    Boolean onTop;
    Boolean justBecameVisible;
    char pad1b;
    Rect destFrameRect;
    Rect oldFrameRect;
    Rect deltaFrameRect;
    DrawProcPtr frameDrawProc;
    short tileDepth;
    Boolean destRectIsVisible;
    Boolean oldRectIsVisible;
    Rect clippedSourceRect;
    Rect destOffscreenRect;
    SWFrameRec **frameArray;
    SWFrameRec *curFrameP;
    int32_t numFrames;
    int32_t maxFrames;
    int32_t frameTimeInterval;
    uint32_t timeOfLastFrameChange;
    int32_t frameAdvance;
    int32_t curFrameIndex;
    int32_t firstFrameIndex;
    int32_t lastFrameIndex;
    FrameProcPtr frameChangeProc;
    int32_t moveTimeInterval;
    uint32_t timeOfLastMove;
    short horizMoveDelta;
    short vertMoveDelta;
    Rect moveBoundsRect;
    MoveProcPtr spriteMoveProc;
    MoveProcPtr realSpriteMoveProc;
    CollideProcPtr spriteCollideProc;
    CGrafPtr sharedPictGWorld;
    CGrafPtr sharedMaskGWorld;
    RemovalType spriteRemoval;
    AdvanceType frameAdvanceMode;
    short translucency;
    short tintRed;
    short tintGreen;
    short tintBlue;
    short tintMode;
    uint32_t tintColor;
    uint32_t rotation;
    short scaledWidth;
    short scaledHeight;
    uint32_t colorValue;
    RGBColor colorRGB;
    Boolean flippedHorizontal;
    Boolean flippedVertical;
    short pad2;
    int32_t userData;
    char **rleDataH;
    Boolean collisionsDisabled;
    Boolean ownsRLEData;
    Boolean isEmptySprite;
    Boolean srD3;
};

typedef struct TERec {
    char                  *text;
    int32_t                   textLength;
    short                  txFont;
    short                  txSize;
    Rect                   destRect;
    Rect                   viewRect;
    int32_t                   height;
    CGrafPtr               inPort;
} TERec, **TEHandle;

#define TESetLineMetrics(te) ((void)0)
#define TEMeasureText(te, width)     MeasureWrappedText((*(te))->text, (*(te))->textLength, (width))
#define TETextHeight(te) ((*(te))->height)
#define TEWrappedHeight(te) ((*(te))->height)

struct ScrollRec;
typedef void (*ListDrawProcPtr)(short lMessage, unsigned char lSelect, Rect *lRect,
                                Point *lCell, short lDataOffset, short lDataLen,
                                struct ScrollRec **lHandle);
typedef struct ScrollRec {
    CGrafPtr port;
    Rect rView;
    Rect dataBounds;
    Point indent;
    Point cellSize;
    ListDrawProcPtr drawProc;
    Boolean hasScrollBar;
    int32_t scroll;
    char **cells;
    short *cellArray;
    short maxIndex;
    Point lastClick;
    int32_t clikTime;
    char unk3c[4];
    Boolean drawIt;
} ScrollRec;

typedef struct DialogRecord {
    CGrafPtr port;
    Rect gwBounds;
    Rect bounds;
    Rect portRect;
    int32_t itemCount;
    int32_t editItem;
    int32_t defaultItem;
    int32_t cancelItem;
    void *items;
    void *updateProc;
    void *eventProc;
    int32_t needsUpdate;
    int32_t visible;
} DialogRecord;

typedef struct Particle {
    short lifeRemaining;
    int32_t horizLoc;
    int32_t vertLoc;
    int32_t oldHorizLoc;
    int32_t oldVertLoc;
    int32_t horizSpeed;
    int32_t vertSpeed;
    int32_t drawnHorizLoc;
    int32_t drawnVertLoc;
    int32_t shade;
    uint32_t color;
} Particle, *ParticlePtr;

/* Toolbox GetTime shim — port implementation at 0x004bbb10 */
extern void GetTime(DateTimeRec *dtr);
extern void IncrementDate(DateTimeRec *date, int32_t daysToAdd);

extern char *PtoCstr(unsigned char *str);
extern unsigned char *CtoPstr(char *str);

#define kAutoKey       5
#define kBlue555  0x001f
#define kCharCodeMask  0xff
#define kDIBHeaderSize 0x28
#define kEscapeChar    0x1b
#define kGreen555 0x03e0
#define kKeyDown       3
#define kMouseDown     1
#define kNullEvent   0
#define kPictType 0x50494354   /* 'PICT' */
#define kSoundType 0x736e6420  /* 'snd ' */
#define kStringType 0x53545220 /* 'STR ' */
#define kStringListType 0x53545223UL /* 'STR#' */
#define kRed555   0x7c00
#define kReturnChar    0x0d
#define kTabChar       0x09
#define IsReturnChar(c) ((c) == kReturnChar)
#define kDownArrowChar  0x0a
#define kEndChar       0x93
#define kEnterChar     0x03
#define kHomeChar      0x91
#define kLeftArrowChar  0x96
#define kPageDownChar   0x94
#define kPageUpChar     0x92
#define kRightArrowChar 0x15
#define kUpArrowChar    0x0b
#define kUpdateEvt     6




#define shiftKey    0x0200
#define optionKey   0x0800
#define controlKey  0x1000
#define diskMask    0x0080
#define everyEvent  0xffff

/* a byte pointer stepped through a cast lvalue: gcc then keeps the pointer in its stack slot, which 1.1.1's bytes witness */
#define PtrAddBytes(p, n) (*(char **)&(p) += (n))

/* a mac virtual key code in the engine's numbering of the KeyMap: 1.0.10 tests it 16 bits at a time, 1.1.1 a byte at a time */
#define MacKey(k) (k)

/* the character in a key event: 1.0.10's filters hold it in a char */
typedef int EventChar;
#define EventCharOf(ev) ((ev)->message & kCharCodeMask)

/* 1.1.1's filters hold that character in a short (EventKey) or a char (EventKeyByte), and some keep the
   event's what in a short (EventWhat) */
#define EventKey EventChar
#define EventKeyByte EventChar
#define EventWhat int

/* a character held in a local across tolower's calls in the win port and 1.0.10; 1.1.1 reads its source again */
#define HeldChar(held, src) (held)

#define kRLEDResType 0x726c9144UL

extern void *NewPortPixels(int width, int rows, int bits);
extern short Alert(short alertID, void *filterProc);
#define StopAlert(id, filter) Alert(id, filter)
extern void BackColor(const RGBColor *color);
extern Boolean Button(void);
extern void ClosePrefs(void);
EXTERN_CPP_BEGIN
extern Rect screenMinusDockRect;
EXTERN_CPP_END
extern void BusyIdle(void);
extern void Delay(uint32_t numTicks, uint32_t *finalTicks);
extern void DisposeCIcon(Image_t *icon);
extern void DrawCString(const char *text);
extern void DrawPicHandle(PicHandle pic, Rect *dstRect);
extern void DrawString(const unsigned char *str);
extern void DrawText(const void *textBuf, int firstByte, int byteCount);
extern void EraseRect(Rect *r);
extern void ExitToShell(void);
extern void SetupMonitor(void);
extern void DisposePixPat(Image_t *pat);
extern void InitDataFiles(void);
extern void MakeAlias(char *name);
extern short EVOpenResourceFile(const char *path);
extern void SwapEndianByDescriptor(unsigned char *data, unsigned short *desc);
extern void LoadMenuListItem(ControlRecord *rec, int32_t menuID, unsigned char *promptTitle,
                         int32_t resType);
extern void LoadControlResource(ControlRecord *rec, uint32_t resID);
extern int32_t PopUpListSelect(DialogPtr dlg, ControlRecord *rec);
extern void EditTextKey(ControlRecord *rec, int32_t key, int32_t mods);
void DrawBevel(Rect *r, const RGBColor *fill, const RGBColor *light, const RGBColor *dark);
extern void LUpdate(void *list);
#define ListUpdate(rgn, list) LUpdate(list)
struct PlatEvent;
extern void WinGameEvent(const struct PlatEvent *ev);
extern void ShutdownWinGame(void);
extern void FillRectWithColor(Rect *r, RGBColor *color);
extern short FindFirstCoLocatedSystem(short sys);
extern short FindPriorLoadedShipSprite(short value, short count, short field);
extern void DisposeClonedSprite(SpritePtr *p);
extern void ShowWindowNoFlash(void *arg);
extern void DisposeDialogNoFlash(void *dlg);
extern void FlushEvents(unsigned short whichMask, unsigned short stopMask);
extern void FrameOval(Rect *r);
extern CWindowPtr FrontWindow(void *window);
extern Image_t *GetCIcon(short iconID);
extern void *GetCWindowVisRgn(void *window);
extern Rect *GetDialogPortRect(DialogPtr dlg);
struct FontSpec;
extern int GetFont(short fontID, short size, short style, struct FontSpec *spec);
/* the procs a dialog record carries: its update (draw) proc and its event filter */
typedef void (*DialogUpdateProc)(void);
typedef Boolean (*DialogEventProc)(DialogPtr dlg, EventRecord *ev, short *itemHit);

extern void GetGlobalMouse(Point *pt);
extern void GetMouse(Point *pt);
extern DialogPtr GetNewDialog(short dlogID, DialogUpdateProc updateProc,
                              DialogEventProc eventProc);
extern PicHandle GetPicture(int picID);
extern Image_t *GetPixPat(short id);
extern void GetPort(CGrafPtr *port);
extern void InitFonts(void);
extern short InitializeQTML(int32_t flags);
extern void InsetRect(Rect *r, int dh, int dv);
extern void InvertRect(Rect *r);
extern unsigned char KeyCheck(short scanCode);
extern int32_t GetKeyModifiers(void);
extern void KeyboardScanCodeToString(char *buf, uint32_t size, uint32_t code);
extern unsigned int KeyIsDown(unsigned int scanCode);
extern int32_t KeyboardGet2(void);
extern void Line(short dh, short dv);
extern void LineTo(short h, short v);
extern void *ST_LoadSndResource(short id);
extern short MeasureCString(const char *text);
extern void MoveTo(short h, short v);
extern void OffsetRect(Rect *r, int dh, int dv);
extern void OpenDataFiles(void);
extern void OpenPrefs(void);
extern void PStr2CStr(char *dst, const unsigned char *src);
extern void PaintOval(Rect *r);
extern void PenSize(short width, short height);
extern void CloseMovieFile(short resRefNum);
/* QuickDraw's PtInRect under a link name of its own: on 64-bit Windows names are not decorated, and
   user32's PtInRect (which SDL uses) would clash with it */
#define PtInRect QD_PtInRect
extern int PtInRect(int h, int v, Rect *r);
extern void QDReleaseResource(PicHandle pic);
extern void RGBForeColor(const RGBColor *color);
extern void ForeColor(int32_t color);
extern void *NewPtr(int32_t size);
extern void DisposePtr(void *ptr);
#define TickCount() ReadTick()
extern CGrafPtr GetWindowPort(CWindowPtr window);
#define QDFlushPortBuffer(port, region) ScreenEndScene()
extern void PtToAngle(const Rect *r, Point pt, short *angle);
extern Boolean RectInRgn(Rect *r, void *rgn);
extern void RedrawDialog(DialogPtr dlg);
extern void RedrawDialogs(int a);
extern void ResetScreenRect(void);
extern void ScreenEndScene(void);
extern int SectRect(const Rect *a, const Rect *b, Rect *dst);
extern uint32_t Color2Index(RGBColor *color);
extern void SetDialogDefaultItem(DialogPtr dlg, int32_t value);
extern short Random(void);
extern short SWSetSpriteWorldScreenDrawProc(SpriteWorldRec *spriteWorld,
                                            DrawProcPtr proc);
extern void SetMovieBox(Movie movie, Rect *box);
extern void DisposeMovie(Movie movie);
extern void GetMovieBox(Movie movie, Rect *box);
extern int32_t GetMovieDuration(Movie movie);
extern void GoToBeginningOfMovie(Movie movie);
extern Boolean IsMovieDone(Movie movie);
extern void LoadMovieIntoRam(Movie movie, int32_t time, int32_t duration, int32_t flags);
extern void MoviesTask(Movie movie, int32_t maxMs);
extern void Microseconds(UnsignedWide *now);
extern void StringToNum(const unsigned char *str, int32_t *value);
extern void *GetMovieTimeBase(Movie movie);
extern void SetMovieRate(Movie movie, int32_t rate);
extern void SetMoviePlayHints(Movie movie, int32_t a2, int32_t a3);
extern void *GetMovieIndTrackType(Movie movie, int32_t a, int32_t type, int32_t b);
extern void SetMovieGWorld(Movie movie, void *gworld, int32_t a3);
extern void StartMovie(Movie movie);
extern void StopMovie(Movie movie);
extern void SetPort(CGrafPtr port);
extern void SetPortWindowPort(CWindowPtr window);
extern void SetRect(Rect *r, int left, int top, int right, int bottom);
extern void FatalStartupError(short index);
extern void *ST_LoadSndResourceIndirect(void **handle, int32_t flags);
extern Boolean StillDown(void);
extern void StrcatOrdinalNumber(short n, char *dst, Boolean capitalize);
extern short StringWidth(const unsigned char *str);
extern void SysBeep(short duration);
extern void TerminateQTML(void);
extern void TETextBox(const void *text, int32_t length, Rect *box, short just);
extern short FSMakeFSSpec(int32_t vRefNum, int32_t dirID, const void *name, void *spec);
extern short OpenMovieFile(const void *spec, short *refNum, int32_t mode);
extern int32_t NewMovieFromFile(Movie *movie, short refNum, short *resId,
                             int32_t a4, int32_t a5, int32_t a6);
extern int MeasureWrappedText(const char *text, int byteCount, int width);
extern void NumToString(int32_t value, unsigned char *str);
extern void QDGetPictureBounds(PicHandle pic, Rect *r);
extern void HideWindow(CWindowPtr window);
extern void *GetGWorldDevice(CGrafPtr port);
extern void DisposeGWorld(CGrafPtr port);
extern void DebugStr(const char *message);
extern short MemError(void);
extern void FillCRect(Rect *dstRect, Image_t *pat);
extern short GetFrameBiggestSize(SWFrameRec *frame);
extern void GetMenuItemText(ControlPtr ctl, int index, unsigned char *text);
extern void SWSetPostEraseCallBack(SpriteWorldPtr world, WorldProcPtr proc);
extern void SWSetPostDrawCallBack(SpriteWorldPtr world, WorldProcPtr proc);
extern void DisposeWindow(CWindowPtr window);
extern void *GetGDevice(void);
extern void BuildGammaRamp(float gamma);
extern int32_t ReplaceInHandle(char **h, int32_t offset, const char *find, int32_t findLen,
                         const char *repl, int32_t replLen);
extern void GetListCellRect(Rect *box, Point cell, ScrollRec **h);
extern Boolean LClick(Point where, short modifiers, ScrollRec **h);
extern void ParamText(const unsigned char *p0, const unsigned char *p1,
                      const unsigned char *p2, const unsigned char *p3);
EXTERN_CPP_BEGIN
extern char gParamText0[];
extern char gParamText1[];
extern char gParamText2[];
extern char gParamText3[];
EXTERN_CPP_END
extern void FadeGammaTo(uint32_t percent, uint32_t duration);
extern unsigned char SplitTwoLineString(const unsigned char *src, unsigned char *line1,
                                        unsigned char *line2);
extern void TextFace(short face);
extern void TextFont(short font);
extern void TextSize(short size);
extern short TextWidth(const char *text, int byteCount);
extern short TextWidthMacRoman(const char *text, int byteCount);
extern unsigned AnsiToUnicode(unsigned char c);             /* code page 1252, the typed text's */
/* port/utf8text.c: the game's text (a typed name, an alert) as UTF-8 for a path or the system, and a
   file's name as game text */
extern void TextToUtf8(char *out, const char *text, int outSize);
extern void Utf8ToText(char *out, const char *name, int outSize);
extern void UpdateAndPageFlip(void);
extern RGBColor *blackColor;
extern int32_t gCaretPhase;
EXTERN_CPP_BEGIN
extern int32_t gBlinkTick;
extern SWFrameRec *floatingEscortMenuFrame;
extern SWFrameRec *floatingMapFrame;
extern short gChicagoFont;
extern void *gClickDialog;
extern void *gClickItem;
extern uint32_t gClickTime;
extern int32_t gDialogCount;
extern DialogPtr gCurDialog;
extern unsigned char gGammaRamp[32];
extern int32_t gLastModifiers;
extern SpriteWorldPtr gSpriteWorld;

extern SpriteRec *masterShipSprite[768];
extern SpriteRec *masterShipGlowSprite[768];
extern SpriteRec *masterShipShieldSprite[768];
extern SpriteRec *masterShipLightSprite[768];
extern SpriteRec *masterShipWeaponSprite[768];
extern SpriteRec *masterShipAltSprite[768];
extern CGrafPort gGamePort;
extern RGBColor blackRGB;
extern int32_t gRezOpenPending;
EXTERN_CPP_END
extern void nv_BlitImageOr(Rect *dstRect, Image_t *icon);
extern int gDisableUpdatesMode;
extern RGBColor *whiteColor;
EXTERN_CPP_BEGIN
extern RGBColor bkgndGreen;
extern RGBColor brightGreen;
extern RGBColor dimGreen;
extern RGBColor dimmerGreen;
extern RGBColor rgbWhite;
extern RGBColor playerPaint;
extern RGBColor rgbLightGrey;
extern RGBColor rgbMedGrey;
extern RGBColor rgbDarkGrey;
extern RGBColor rgbLessDarkGrey;
extern RGBColor rgbRealDarkGrey;
extern RGBColor rgbRealDarkBlue;
extern RGBColor rgbBlack;
EXTERN_CPP_END

extern short GetSpriteXSize(SpriteRec *spr);
extern short GetSpriteYSize(SpriteRec *spr);
extern void SWSetCurrentFrameIndex(SpritePtr srcSpriteP, short frameIndex);
extern void SWSetSpriteLocation(SpritePtr srcSpriteP, short horizLoc,
                                short vertLoc);
extern void SWSetSpriteVisible(SpritePtr srcSpriteP, unsigned char isVisible);
extern void SWSetPortToWorkArea(SpriteWorldPtr sw);
extern void SWSetSpriteWorldMaxFPS(SpriteWorldPtr world, short fps);
EXTERN_CPP_BEGIN
extern PrefsFileRec gPrefs;
EXTERN_CPP_END

extern void SetDialogItemText(Handle item, const unsigned char *pstr);
extern void GetDialogItemText(Handle item, unsigned char *pstr);
extern void DisableControl(ControlPtr ctl);
extern void SetDialogVisible(DialogPtr dlg, unsigned char value);
extern void SizeDialogWindow(DialogPtr dlg, short width, short height,
                         Boolean update);
extern void OffsetDialogWindow(DialogPtr dlg, short dh, short dv, Boolean update);
extern void CreateGameWindow(void);
extern CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title);
extern int32_t GetControlValue(ControlPtr ctl);
extern void InvalDialogRect(DialogPtr dlg, Rect *box);
extern void ModalDialog(void *filterProc, short *itemHitPtr);
extern void DisposeDialog(DialogPtr dlg);
/* the body is empty, but every caller passes an argument */
extern void ShowMacWindow(void *arg);
extern void GetDialogItem(DialogPtr dlg, int32_t item, short *type, Handle *handle,
                          Rect *box);
void GetControlText(ControlPtr ctl, char *text);
extern CGrafPtr GetDialogPort(DialogPtr dlg);
extern void InvalDialogItem(DialogPtr dlg, int32_t item);
void SetControlText(ControlPtr ctl, const char *text);
extern void SetControlValue(ControlPtr ctl, int32_t value);
extern void SetDialogItem(DialogPtr dlg, int32_t item, short type, void *handle,
                          const Rect *box);
extern void SetControlTitle(ControlPtr ctl, const unsigned char *text);

extern void PaintRect(Rect *r);
EXTERN_CPP_BEGIN
extern Boolean gameLoaded;
extern Boolean gameInProgress;
extern Boolean inBackground;
extern Boolean creatingPilotFile;
extern Boolean onPlanet;
extern Boolean closeRequested;
EXTERN_CPP_END
/* invalidate the whole game window: the port only raises its redraw flag */
#define InvalWindowPortRect(window) (GetCWindowPortRect(window), gNeedUpdateFlag = 1)
extern Boolean gNeedUpdateFlag;

extern void LAutoScroll(ScrollRec **list);
extern void LScroll(int32_t unused, short delta, ScrollRec **list);
extern void LSetSelect(int track, Point pt, ScrollRec **list); /* 0x004d1cd0 */
extern void LSetDrawingMode(Boolean value, ScrollRec **list);          /* 0x004d1b70 */
extern void LSetCell(void *text, int32_t length, Point cell,
                     ScrollRec **list);                          /* 0x004d1b80 */
extern Boolean LGetSelect(int32_t unused, Point *cell, ScrollRec **list); /* 0x004d1d10 */
extern void LDispose(ScrollRec **list);                     /* 0x004d1b40 */
extern ScrollRec **nv_NewDialog(const Rect *rView, const Rect *dataBounds,
                                Point cellSize, ListDrawProcPtr drawProc, CGrafPtr port,
                                unsigned char hasScrollBar);     /* 0x004d1a60 */

extern TEHandle TENew(const Rect *destRect, const Rect *viewRect);
extern void TESetText(const void *text, int32_t length, TEHandle hTE);
extern void TECalText(TEHandle hTE);
extern void TEPinScroll(int dh, int dv, TEHandle hTE);
extern void TEUpdate(Rect *rUpdate, TEHandle hTE);
extern void TEDispose(TEHandle hTE);
EXTERN_CPP_BEGIN
extern CWindowPtr gWind;
extern GWorldContainer loadPort;
EXTERN_CPP_END

extern char *GetPixBaseAddr(PixMapPtr pm);
extern short GetPixRowBytes(PixMapPtr pm);
extern short GetPixDepth(PixMapPtr pm);

/* the port's blackColor and whiteColor are RGBColor globals, so its ForeColor(c) is RGBForeColor
   (ForeColor itself, parenthesised, takes the QuickDraw numbers) and its BackColor takes the RGBColor */
#define ForeColor(c) RGBForeColor(c)
#define RGBBackColor(c) BackColor(c)
#define GetWindowVisRgn(window) GetCWindowVisRgn(window)
/* the port keeps no window order: SelectWindow and BringToFront keep only their argument */
#define SelectWindow(window) ((void)(window))
#define BringToFront(window) ((void)(window))

#ifdef __cplusplus
}
#endif

#endif
