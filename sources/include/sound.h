/*
 * sound.h -- sound effects, music and voices.
 *
 * One declaration per name for the whole tree.
 * Addresses are the win 1.0.10 exe's.
 */
#ifndef SOUND_H
#define SOUND_H
#include "macport.h"
#include "gameconst.h"
#include "soundtool.h"

/* --- one declaration per name --- */
extern Boolean PlaySoundDistance(int32_t unused, Sound *sound, short flags,
                              FloatPoint *pos, FloatPoint *listener);
                                                      /* 0x004692e0 (engineutils.cpp) */
extern void AdjustSoundVolume(void);                  /* 0x0046ab60 (mac _AdjustSoundVolume) */
extern void HandleVoiceSound(void);              /* 0x004311f0 */
extern void LoadSounds(void);                            /* 0x004b0740 */
extern void PlaySound(Sound *snd, short id, short leftVol, short rightVol);      /* 0x0046aad0 */
extern uint32_t lastMoviesTaskTime;
extern short pendingVoiceGovt;          /* 0x00735400 (globaldefs.cpp) */
extern short voiceResourceCount[kVoiceSets][kVoiceMessages]; /* 0x005979da gameloop.cpp */
extern short pendingVoiceMsg;           /* 0x007353fe (globaldefs.cpp) */
extern short pendingVoiceParity;        /* 0x00735402 (globaldefs.cpp) */

extern void SetupIntroMusic(void);                   /* 0x004ab5d0 */
extern void StopIntroMusic(Boolean fadeOut);            /* 0x004ab820 */
extern void HandleIntroMusic(Boolean force);            /* 0x004ab8d0 */
/* the engine's calls that keep the intro music fed */
#define ServiceIntroMusic(force) HandleIntroMusic(force)
extern SoundRequest warpUpSoundSpec;                 /* 0x00591a98 (mac name) */
extern SoundRequest planetSoundSpec;                 /* 0x00591ab4 (mac name) */

#endif /* SOUND_H */
