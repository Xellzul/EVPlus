/* soundtool.h -- the Sound Toolkit as the game uses it: loading `snd ' resources, playing, halting and
   disposing them. The mixer behind it is in burgerchan.h. */
#pragma once

#include <stdint.h>

/* a loaded sound: 16-bit PCM ready for the mixer */
struct Sound;

#define kUnityRate          0x10000              /* a sound rate of 1.0 */

/* ST_PlaySoundParam's request; it clamps priority and levels in place.
   rate is signed: AdjustWarpSoundSpeed converts a double into it. */
typedef struct SoundRequest {
    Sound *sound;
    uint32_t id;                                 /* 0 = assign one */
    int32_t rate;                                /* 16.16, kUnityRate = as recorded */
    unsigned short priority;
    unsigned short leftLevel;                    /* 0..0x80 */
    unsigned short rightLevel;
} SoundRequest;

/* the toolkit: channels heard at once, the output rate (16.16, 0 = the default) */
extern int32_t ST_Open(unsigned short channels, uint32_t rate);
extern void ST_Close(void);

extern Sound *ST_LoadSndResource(short id);
extern Sound *ST_LoadSndResourceIndirect(void **handle, int32_t flags);
extern void ST_DisposeSound(Sound *sound);

/* a loaded sound's length in samples, its rate (16.16) and its samples */
extern uint32_t ST_SoundSamples(const Sound *sound);
extern uint32_t ST_SoundRate(const Sound *sound);
extern const short *ST_SoundData(const Sound *sound);

/* each returns the id of the channel started, 0 if none was free */
extern int32_t ST_PlaySound(Sound *sound, unsigned short priority, unsigned short level);
extern int32_t ST_PlaySoundStereo(Sound *sound, unsigned short priority, unsigned short left,
                                  unsigned short right);
extern uint32_t ST_PlaySoundParam(SoundRequest *req);

/* the channels playing this sound (or this id); a null sound counts every channel, id 0 none */
extern unsigned short ST_IsSoundPlaying(const Sound *sound);
extern unsigned short ST_IsSoundIdPlaying(uint32_t id);

/* stop the channels playing this sound (or this id); a null sound stops every channel, id 0 none */
extern void ST_HaltSound(const Sound *sound);
extern void ST_HaltSoundId(uint32_t id);
