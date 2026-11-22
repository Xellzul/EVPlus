/* burgerchan.h -- the Sound Toolkit's insides: the channel table (atsound.cpp), the resampler (atsnddrv.cpp),
   the SDL voices (atsndvoice.cpp) and the sample decoders (atadpcm.cpp).
   Notes: Research/notes/burgerchan.md */

#ifndef BURGERCHAN_H
#define BURGERCHAN_H

#include "burger.h"
#include "soundtool.h"

#define kSoundChannels 32                        /* rows of the channel table, all heard */
#define kChannelBytes  sizeof(SoundChannel)

/* A playing channel's position in its samples. */
typedef struct MixVoice {
    uint32_t pos;                                /* the next sample */
    uint32_t total;                              /* samples in the sound */
    int32_t lastLeft;                            /* the last levelled sample, for interpolation */
    int32_t lastRight;
} MixVoice;

typedef struct SoundChannel {
    uint32_t id;
    Sound *sound;
    const short *data;                           /* the sound's samples */
    MixVoice voice;
    unsigned short leftLevel;
    unsigned short rightLevel;
    unsigned short priority;
    unsigned char out;                           /* the SDL voice this row plays on; every row keeps its own */
} SoundChannel;

/* the channel table: gNumVoices in use (atsound.cpp) */
extern SoundChannel gTheVoices[kSoundChannels];
extern int32_t gNumVoices;
extern unsigned char gSoundToolOpen;
extern void _ST_HaltASound(short index);
/* ST_Open with its voices rendered by _ST_RenderVoices instead of played (soundtest.cpp) */
extern int32_t _ST_OpenOffline(void);

/* the resampler (atsnddrv.cpp) */
extern unsigned char PCM_Mixer(const short *data, short **out, ULong frames, ULong step,
                               MixVoice *voice, unsigned short left, unsigned short right);

/* the SDL voices (atsndvoice.cpp), one a row */
extern int32_t _ST_OpenVoices(bool offline);
extern void _ST_CloseVoices(void);
extern void _ST_StartVoice(const SoundChannel *chan, int32_t rate);
extern void _ST_StopVoice(unsigned char out);
extern void _ST_ReapVoices(void);                /* drops the rows whose voice has played out */
/* offline: the next frames of the mix as PlatVoicesRender (platform.h) makes it */
extern void _ST_RenderVoices(float *out, float *unclamped, int frames);

/* IMA ADPCM to 16-bit PCM (atadpcm.cpp) */
extern void ImaDecodePackets(const unsigned char *src, ULong packets, short *out);
extern void ImaDecodeStream(const unsigned char *src, ULong samples, short *out);

#endif
