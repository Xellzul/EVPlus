/* gameutil.h -- strings, numbers, dates, randomness and resource plumbing. Notes: Research/notes/gameutil.md */
#ifndef GAMEUTIL_H
#define GAMEUTIL_H
#include <string>
#include "burger.h"
#include "shipobj.h"

extern void CenterPict(void *pict, Rect *bounds);
extern void DateToLongString(DateTimeRec date, char *out);
extern void DateToMedString(DateTimeRec date, char *out);
extern void DrawCommaNumber(int32_t value);
extern void StrcatCommaNumber(int32_t value, char *dst);
extern std::string shieldString[6];
extern std::string armorString[3];
extern std::string newbieString[10];
extern Boolean LoadPluginString(short id, void *dst);
extern Boolean LoadPluginString(short id, std::string &dst);
extern void AdjustHandleSize(Handle h, int32_t newSize);
extern void PutSpriteInLayer(struct SpriteRec *sprite,
                             struct SpriteLayerRec *layer);
extern Boolean EVBugResourceFieldTest(short field);
struct RLEPixelData;
extern void FixupRLEPixelData(unsigned char *tokens, struct RLEPixelData *header);
extern void GetCharResourceIntroIDs(const unsigned char *name, short *a, short *b,
                         short *c);
extern void FixupResourceByType(Handle h, uint32_t type);
extern void *LinkedListNew(void);
extern short FindSystemFromStellar(short stellar);
extern short GetCharResourceShipType(const unsigned char *name);
extern void GetPlayerContributeBits(uint32_t *lo, uint32_t *hi);
extern void GetDefaultCharResourceName(unsigned char *name);
extern Handle GetNamedResource(uint32_t type, const unsigned char *name);
extern void GetRandomIndString(unsigned char *str, short strListID);
extern void HideCursorSprite(void);
extern void LoadAdvice(void);
extern void LoadDescription(char *dst, size_t dstSize, short id);
/* dësc `id' parsed into a text buffer, cut to it */
template <size_t N>
void LoadDescription(char (&dst)[N], short id)
{
    LoadDescription(dst, N, id);
}
extern void LoadObjectData(void);
extern void LoadStrings(void);
extern void ParseTextDescription(void);
extern void PlayQTMovieFromDesc(short id);
extern short PlayerHasFightersOfOutfitType(short outfitIndex);
extern short Rand(short range);
extern void RandSeedFromClock(void);
extern int32_t RoundD(double value);
extern Boolean ResourceExists(uint32_t type, short id);
extern Boolean nv_ResourceExists(uint32_t type, short id);
extern ResRecord *nv_FindResourceRecord(uint32_t type, short id);
extern Boolean ShipIsDerelict(ShipObj *ship);
extern short StellarNumHyperLinks(SpobRecord *spob);
extern void TrimResourceName(unsigned char *name);
extern int32_t TruncF32(float value);
extern Handle colorFileHand;
extern RGBColor escortMenuColor;
extern Handle boomFileHand;
extern Handle cronFileHand;
extern unsigned char descMovieName[256];
extern short descFlags;
extern Handle textFileHand;
extern short descRankGovt[2];
extern Handle dudeFileHand;
extern Handle fleetFileHand;
extern Handle govtFileHand;
extern Handle junkFileHand;
extern Handle nebulaFileHand;
extern Handle disasterFileHand;
extern Handle outfitSystemFileHand;
extern std::string outfitShortNames[512];
extern Handle personFileHand;
extern uint64_t randomSeed;
extern Handle rankFileHand;
extern int32_t gResourcesReleased;
extern struct ListRec *gRezFiles;                /* the open RezFiles, BurgerLib's list */
extern Handle asteroidFileHand;
extern Handle shipClassFileHand;
extern std::string shipMovieFilenames[768];
extern std::string shipyardShortNames[768];
extern Handle stellarFileHand;
extern Handle systemFileHand;
extern Handle weaponFileHand;
extern short jumpSoundLength[2];
extern short pstrcmp(const unsigned char *a, const unsigned char *b);

extern void pstrcpy(unsigned char *dst, const unsigned char *src);
extern void CullNameString(unsigned char *name);
extern short WarpSoundDuration(void);

extern int32_t ResourceInit(RezFile *rez, const char *path, uint32_t start);
extern char *ResourceLoadByName(RezFile *rez, const char *name);
extern std::string SaveBuffer(void);
extern short CountEVPlugins(const char *path, const char *ext, FILE *log);
extern void ResourceDetach(RezFile *rez, int32_t index);
extern Word ResourceGetIDFromHandle(RezFile *rez, Handle h, int32_t *idFound,
                                    char *nameBuf, Word nameBufSize);
extern void ResourcePurgeCache(RezFile *rez);
extern void ResourceRelease(RezFile *rez, int32_t index);
struct DudeRecord;
extern short SelectShipFieldFromDude(struct DudeRecord *dude,
                                     unsigned char allowAll);
extern short GenericRandomShipSpawn(short sys, short reserve);
extern short SpecificDudeSpawn(short dude, short sys, short reserve,
                               unsigned char allowAll);
extern void ShowPersonHailQuote(short persID);

#endif
