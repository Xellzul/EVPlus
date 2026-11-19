/* burger.h -- the statically linked BurgerLib (a cdecl build) and the runtime band. Notes: Research/notes/burger.md */
#ifndef BURGER_H
#define BURGER_H

#include "burgertypes.h"
#include "keycodes.h"

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* memory manager (MMMemoryManager) */
extern void * AllocAPointer(Word32 MemSize);
extern void DeallocAPointer(void *MemPtr);
extern FILE *FOpenExpandedPath(const char *path, const char *mode);
extern void * AllocAPointerClear(uint32_t size);
extern void * ResizeAPointer(void *mem, uint32_t size);

extern int32_t SetDisplayToSize(uint32_t w, uint32_t h, uint32_t depth, uint32_t flags);
extern ULong TimerGetTicks(void);                 /* atticks.c: milliseconds; only differences mean anything */
extern ULong TimerGetTickRate(void);                 /* atticks.c: the tick rate that goes with it */
extern void DebugXString(const char *string);

/* the lock (atthreadtbl.c) */
extern int32_t CriticalDispose(void *lock);
extern int32_t CriticalCreate(void *lock);

/* lists, images, directories, paths and the prefs file */
struct AudioDesc;
struct DirectorySearch_t;
struct Image_t;
struct ListNode;
struct ListRec;
struct PrefsNode;
struct RezFile;
extern uint32_t GetAPointerSize(void *mem);
extern void DetachResource(Handle h);
extern void LinkedListInit(struct ListRec *list);
extern void LinkedListAddNewEntryEnd(struct ListRec *list, void *data);
extern void *LinkedListGetEntryData(struct ListRec *list, ULong index);
extern void LinkedListAddNewEntryBegin(struct ListRec *list, void *data);   /* prepend */
extern int ImageParseCicn(struct Image_t *out, const Word8 *in);
extern Word ImageParsePict(struct Image_t *ImagePtr, const Word8 *InputPtr);
extern Word OpenADirectory(struct DirectorySearch_t *dir, const char *name);
extern Word GetADirectoryEntryExtension(struct DirectorySearch_t *dir,
                                        const char *ext);
extern void CloseADirectory(struct DirectorySearch_t *dir);
extern void ImageRemapIndexed(struct Image_t *img, const Word8 *remap);
extern Word PaletteFindColorIndex(const Word8 *pal, Word red, Word green,
                                  Word blue, Word count);
extern int32_t ParseSoundFileImage(struct AudioDesc *info, Word8 *data,
                                ULong size);
extern void ExpandAPathToBuffer(char *dst, const char *path);   /* BurgerLib form */
extern void ExpandAPathToBufferNative(char *dst, const char *path);   /* a Windows path, UTF-8 */
extern Word DoesFileExistNative(const char *name);
extern Word GetADirectoryEntry(struct DirectorySearch_t *dir);
extern Word DeleteAFileNative(const char *name);
extern Word ResourceGetRezNum(struct RezFile *rez, const char *name);
extern Handle ResourceLoadHandle(struct RezFile *rez, int32_t index);
extern Handle ResourceLoadHandleByName(struct RezFile *rez, const char *name);
extern unsigned int SetAPrefix(unsigned int prefix, const char *path);
extern void *LoadAFile(const char *fileName, uint32_t *length);
extern char *GetFileExtension(const char *name);
extern Handle StrCopyHandle(const char *text);
extern char *StrCopy(const char *text);
extern void StripLeadingAndTrailingWhiteSpace(char *text);
extern void ExtractAString2(const char *src, ULong *size, Word flags, char *dst, ULong dstSize);
extern Word BombFlag;
extern char ErrorMsg[512];                       /* burgerfatal.c */
extern Word IAmExiting;                          /* burgerfatal2.c */
extern void *PrefFileNewFromMemory(const char *text, uint32_t size);
extern void ClosePrefsFile(struct PrefsNode *file);
extern void *OpenPrefsFile(const char *name);
extern struct PrefsNode *PrefsFindGroup(struct PrefsNode *file, const char *name);
extern void PrefsGetString(struct PrefsNode *group, const char *key, const char *dflt,
                           char *out, int32_t outSize);
extern int stricmp(const char *s1, const char *s2);   /* MSL Extras */
extern int32_t WritePrefsFile(struct PrefsNode *file, const char *path);

/* runtime-block globals */
extern uint32_t gSaveLeftVolume;
extern uint32_t gSaveMaxVolume;
extern int32_t MouseClicked;
/* a lock (atthreadtbl.c: CriticalCreate .. CriticalLeave take its address) */
typedef struct Mutex {
    struct PlatMutex *section;
    ULong magic;
    unsigned char live;
} Mutex;
extern Mutex gCriticalSection;                   /* the sound mixer's lock (atsound.c) */
extern unsigned char gSoundToolOpen;
extern Word8 ResourceJustLoaded;                 /* 0x00870e11, set by ResourceLoadHandle */
/* handles */
extern void **AllocAHandle2(Word32 MemSize, Word Flag);
extern void DeallocAHandle(void *MemHandle);
extern uint32_t GetAHandleSize(Handle h);
extern Handle ResizeAHandle(Handle h, uint32_t size);
extern char * LockAHandle(Handle h);
extern void UnlockAHandle(Handle h);
extern void SetHandlePurgeFlag(void **handle, Word flag);

/* QuickTime calls, media/quicktime.c */
extern void HLock(Handle h);                                         /* 0x004f1470 */
extern void HUnlock(Handle h);
extern void HPurge(Handle h);
extern short EnterMovies(void);
extern void ExitMovies(void);
extern void SetTrackGWorld(void *track, void *movieData, int32_t a3,
                           short (*proc)(void), int32_t a5);
extern int32_t GetTimeBaseFlags(void *timeBase);
extern void SetTimeBaseFlags(void *timeBase, int32_t flags);

extern void KeyboardInit(void);
extern void SetDefaultPrefixs(void);
extern void ST_DisposeSound(void *snd);
extern unsigned short ST_IsSoundPlaying(void *snd);


/* the resource manager (engine/resmgr.c) */
extern Handle GetResource(uint32_t type, short id);
extern int32_t CountResources(uint32_t type);
extern Handle GetIndResource(uint32_t type, short index);
extern void ReleaseResource(Handle h);
extern void GetResInfo(Handle h, short *id, uint32_t *type,
                       unsigned char *name);

extern unsigned long long TimerGetMicroseconds(void);
extern void LockVideoPage(void);
extern void ST_HaltSound(void *sound);
extern void ST_Close(void);
extern ULong UFix32_Divide(ULong device, ULong rate);
extern ULong UFix32_Multiply(ULong ratio, ULong rate);
extern unsigned int DoesFileExist(const char *path);
extern char *NumberToAscii(char *dst, ULong value, ULong flags);
extern void OSCursorShow(void);
extern void OSCursorHide(void);
extern void OSCursorReset(void);
extern unsigned char OSCursorVisibleFlag;        /* burgermisc2.c */
extern void ReleaseVideo(void);
extern Word32 LastTick;
extern Word32 ReadTick(void);
extern void WaitTicks(Word tickCount);
#define ResetLastTick() LastTick=ReadTick()

/* BurgerLib's own constants */
#define kOutOfMemory    -7004
#define kNoDevice       -7401      /* sound: no output device */
#define kMillisPerSecond 1000
#define kPathBufferSize 0x800


#define Swap32(v)  ((((v) >> 24) & 0xffUL) | (((v) >> 8) & 0xff00UL) \
                    | (((v) & 0xff00UL) << 8) | (((v) & 0xffUL) << 24))
#define Swap16(v)  ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))

/* BurgerLib's file manager: the prefix table, the drive letters */
#define PREFIXMAX           35
#define PREFIXBOOT          32                   /* "*:" */
#define PREFIXPREFS         33                   /* "@:" */
#define PREFIXSYSTEM        34                   /* "$:" */
#define kPrefixCurrent      8
#define kPrefixApp          9
#define kNoVolume           ((Word)-1)

/* BurgerLib's memory manager, number and string helpers, fixed-point maths */
#define kHandleFixed        0x40                 /* fixed memory flag */
#define kNoTerminator       0x4000               /* NumberToAscii flag */
#define kExtractStrDelimitLF 1                   /* ExtractAString2: stop at the end of a line */
#define kFixedShift         16                   /* Fixed32 is 16.16 */
#define kUnityRate          0x10000              /* a sound rate of 1.0 */

/* QuickDraw's Style bits, as the port's font code spells them */
#define kFontBold           0x01
#define kFontItalic         0x02

/* BurgerLib's palette and PixMap code (QuickDraw's rowBytes: bit 15 marks a PixMap) */
#define kPaletteEntries     0x100
#define kPaletteBytes       (kPaletteEntries * 3)
#define kPixMapFlag         0x8000
#define kPixMapFlagMask     0xc000
#define kRowBytesMask       0x3fff

/* MacTypes.h's booleans, which no -bool off TU has built in: byte-wide for C (an enum under
   -enum min; spelled as macros here so that no TU's internal numbering moves); the win port's
   C++ has its plain ints from burgertypes.h */
#ifndef __cplusplus
#define false ((unsigned char)0)
#define true  ((unsigned char)1)
#endif

/* video globals */
extern Word VideoColorDepth;
extern uint32_t VideoWidth;
extern char *VideoPointer;
extern uint32_t ScreenWidth;
extern uint32_t ScreenHeight;
extern int32_t ScreenFlags;                         /* burgergraphics.c */
extern Word Win95ApplsActive;                    /* burgeronce.c */
extern char *VideoOffscreen;

/* the engine's names for BurgerLib's display globals, as the mac build spells them */
#define videoBase VideoPointer
#define gVideoWidth ScreenWidth
#define gVideoHeight ScreenHeight

/* the hyperspace white-out (port addition, burgervideomode.c): blend from..to of 256 over 60 Hz ticks */
#define kWhiteNone          0
#define kWhiteSolid         256
#define kWhiteFadeTicks     90                      /* 1.1.1's CGDisplayFade duration, 1.5 s */
extern void VideoFadeWhite(Word from, Word to, uint32_t ticks);

/* sound globals (burgersndwin.c) */
extern Word MusicVolume;
extern Word SystemState;
extern ULong BurgerVoiceCount;                       /* BurgerVoiceCount */

struct MixVoice;
struct TextBuilder;

extern int32_t ST_Open(unsigned short channels, ULong device);
extern int32_t fgetstr(char *buf, int32_t size, FILE *stream);
extern void fwritelong(int32_t value, FILE *stream);
extern void fwritestr(const char *s, FILE *stream);
extern int32_t fgetlong(FILE *stream);
extern void *LoadAFileFP(FILE *fp, uint32_t *length);
extern ULong fgetfilesize(FILE *fp);     /* BurgerLib fgetfilesize */
extern int32_t CreateDirectoryPathNative(char *path);            /* BurgerLib CreateDirectoryPathNative */
extern char *ParseQuotedString(char *out, int32_t outSize, const char *src);

extern void StreamHandleInitPut(struct TextBuilder *b);
extern void StreamHandleDestroy(struct TextBuilder *b);
extern void StreamHandleEndSave(struct TextBuilder *b);
extern void StreamHandlePutByte(struct TextBuilder *b, int32_t ch);
extern void StreamHandlePutStringNoZero(struct TextBuilder *b, const char *s);
extern void FastMemSet16(void *dst, ULong value, ULong bytes);
extern void ImaDecodePackets(const unsigned char *src, ULong packets, short *out);   /* atadpcm.c */
extern void ImaDecodeStream(const unsigned char *src, ULong samples, short *out);
extern unsigned char PCM_Mixer(const short *data, short **out, ULong frames, ULong step,
                               struct MixVoice *voice, unsigned short left, unsigned short right);

extern int32_t gNumVoices;
extern ULong gSaveRightVolume;
extern ULong gSampleRate;
extern ULong BurgerSndExitIn;                    /* burgerfile.c */
extern int32_t BurgerSndKillProcs[3];               /* burgerfile.c */

extern ULong UFix32_To_UInt32(ULong rate);
extern void CriticalEnter(void *lock);
extern void CriticalLeave(void *lock);

extern void GetDateTime(uint32_t *seed);
extern void ResourceDestroy(struct RezFile *rez);
extern char *ConvertNativePathToPath(const char *path);
extern void *LoadAFileHandle(const char *path);
extern void **AllocAHandleClear(ULong size);
extern void DrawARect16(int x, int y, Word Width, Word Height, Word Color);
extern void KeyboardFlush(void);
extern int32_t CreateDirectoryPath(const char *path);
extern void Fatal(const char *fmt, ...);
extern void NonFatal(const char *fmt, ...);


/* the guarded copy the exe's file and sound tools inline */
static inline void BlockMoveData(const void *src, void *dst, ULong size)
{
    if (size != 0)
        memmove(dst, src, size);
}
extern short OpenEVPlugins(const char *path, const char *ext,
                           FILE *log);
extern void ImageDestroy(struct Image_t *ImagePtr);
extern void ImageDelete(struct Image_t *ImagePtr);
extern unsigned int KeyboardKbhit(void);
extern unsigned int KeyboardGetch(void);
extern void ProcessSystemEvents(void);
extern unsigned int OSCursorIsVisible(void);
extern void CStr2PStr(char *dst, const char *src);
extern int32_t ST_PlaySound(void *snd, unsigned short priority, unsigned short level);
extern ULong ST_PlaySoundParam(SoundRequest *req);

#ifdef __cplusplus
}
#endif

#endif
