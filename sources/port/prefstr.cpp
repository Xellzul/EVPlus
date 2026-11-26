/* prefstr.cpp -- the data-file record handles and the pilot/prefs folder names (tu-map object O28). */
#include "player.h"
#include "gameutil.h"

/* the prefs and data-file object's globals: each resource type's file-data handle */
Handle stellarFileHand;
Handle systemFileHand;
Handle shipClassFileHand;
Handle weaponFileHand;
Handle outfitSystemFileHand;
Handle dudeFileHand;
Handle govtFileHand;
Handle personFileHand;
Handle disasterFileHand;
Handle nebulaFileHand;
Handle boomFileHand;
Handle fleetFileHand;
Handle junkFileHand;
Handle textFileHand;
Handle cronFileHand;
Handle asteroidFileHand;
Handle rankFileHand;
Handle charFileHand;
Handle intfFileHand;
Handle colorFileHand;
char noSavePilotFiles;
char noUpdatePilotData;
PilotRec **playerFileHand;
PilotRec2 **altPlayerFileHand;
