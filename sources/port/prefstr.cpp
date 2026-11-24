/* prefstr.cpp -- the data-file records' globals and the pilot/prefs folder names (tu-map object O28). */
#include "player.h"
#include "gameutil.h"

/* the prefs and data-file object's globals: each resource type's file-data record, then its handle */
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
