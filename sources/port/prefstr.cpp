/* prefstr.cpp -- the prefs file: OpenPrefs, ClosePrefs, GetIniIndString (win 0x004bcfa0..0x004bd0c0, tu-map object O28; the next function starts the loaddata.cpp object). Notes: Research/notes/prefstr.md */
/*
 * prefstr.cpp — read one indexed string out of the port's prefs file
 * (win 0x004bd000).
 *
 * The engine's STR# resources became numbered groups in EVNova.ini: the group
 * is named by the resource id and each string by "S" plus its index, so this
 * is what GetIndString ends up calling.
 */
#include <stddef.h>
#include <string.h>
#include "gameconst.h"
#include "gametypes.h"
#include "render.h"
#include "crashlog.h"
#include "player.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "stellar.h"

/* the prefs and data-file object's globals: each resource type's file-data record, then its handle */
PrefsFileRec gPrefs;
unsigned char stellarFileData[1120];
Handle stellarFileHand;
unsigned char systemFileData[428];
Handle systemFileHand;
unsigned char shipClassFileData[1860];
Handle shipClassFileHand;
unsigned char weaponFileData[136];
Handle weaponFileHand;
unsigned char outfitSystemFileData[1028];
Handle outfitSystemFileHand;
unsigned char dudeFileData[88];
Handle dudeFileHand;
unsigned char govtFileData[192];
Handle govtFileHand;
unsigned char personFileData[400];
Handle personFileHand;
unsigned char disasterFileData[284];
Handle disasterFileHand;
unsigned char nebulaFileData[520];
Handle nebulaFileHand;
unsigned char boomFileData[8];
Handle boomFileHand;
unsigned char fleetFileData[308];
Handle fleetFileHand;
unsigned char junkFileData[676];
Handle junkFileHand;
unsigned char textFileData[8192];
Handle textFileHand;
unsigned char cronFileData[824];
Handle cronFileHand;
unsigned char asteroidFileData[40];
Handle asteroidFileHand;
unsigned char rankFileData[152];
Handle rankFileHand;
unsigned char charFileData[364];
Handle charFileHand;
unsigned char intfFileData[168];
Handle intfFileHand;
unsigned char colorFileData[244];
Handle colorFileHand;
char noSavePilotFiles;
char noUpdatePilotData;
PilotRec **playerFileHand;
PilotRec2 **altPlayerFileHand;
char pilotsFolder[2048];
char gPrefsDirectory[2054];

#define kPrefStringMax 0x100

static const char kPrefsFileName[] = "9:EVNova.ini";

/* opened on first use, so nothing has to sequence it against startup */
// FUNCTION: EVNOVA 0x004bcfa0
void OpenPrefs(void)
{
    PrefsFileRec *p;

    p = &gPrefs;
    if (p->file == NULL) {
        p->file = (struct PrefsNode *)OpenPrefsFile(kPrefsFileName);
    }
}

// FUNCTION: EVNOVA 0x004bcfd0
void ClosePrefs(void)
{
    PrefsFileRec *p;

    p = &gPrefs;
    if (p->file != NULL) {
        ClosePrefsFile(p->file);
        p->file = NULL;
    }
}

// FUNCTION: EVNOVA 0x004bd000
void GetIniIndString(char *out, int32_t listID, int32_t index)
{
    char group[kPrefStringMax];
    char key[0x40];
    struct PrefsNode *found;

    if (out != NULL) {
        out[0] = 0;
        NumberToAscii(group, listID, 0);
        found = PrefsFindGroup(gPrefs.file, group);
        if (found != NULL) {
            key[0] = 'S';
            NumberToAscii(&key[1], index, 0);
            PrefsGetString(found, key, "", out, kPrefStringMax);
        }
    }
}
