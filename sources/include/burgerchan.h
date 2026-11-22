/* burgerchan.h -- the Sound Toolkit's insides: the channel table (atsound.cpp), the mixer and its output
   (atsnddrv.cpp) and the sample decoders (atadpcm.cpp). Notes: Research/notes/burgerchan.md */

#ifndef BURGERCHAN_H
#define BURGERCHAN_H

#include "burger.h"
#include "soundtool.h"

#define kSoundChannels 16
#define kChannelBytes  sizeof(SoundChannel)
#define kChannelTable  (kSoundChannels * kChannelBytes)

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
    uint32_t stream;                             /* 16.16 step */
    const short *data;                           /* the sound's samples */
    MixVoice voice;
    unsigned short leftLevel;
    unsigned short rightLevel;
    unsigned short priority;
    unsigned char finished;
    unsigned char pad37;
} SoundChannel;

/* the channel table: gNumVoices in use, the first gMaxVoices heard (atsound.cpp) */
extern SoundChannel gTheVoices[kSoundChannels];
extern int32_t gNumVoices;
extern int32_t gMaxVoices;
extern uint32_t gSampleRate;                     /* the output rate, 16.16 */
extern unsigned char gSoundToolOpen;
extern Mutex gCriticalSection;                   /* held while the table or the mix changes */
extern void _ST_HaltASound(short index);

/* the mixer and its output (atsnddrv.cpp) */
extern int32_t _ST_CreateSoundPump(void);
extern void _ST_DisposeSoundPump(void);
extern void _ST_MixVoices(short *buffer, ULong frames);
extern void _ST_MixerDispatch(short *buffer, ULong frames);
extern unsigned char PCM_Mixer(const short *data, short **out, ULong frames, ULong step,
                               MixVoice *voice, unsigned short left, unsigned short right);

/* IMA ADPCM to 16-bit PCM (atadpcm.cpp) */
extern void ImaDecodePackets(const unsigned char *src, ULong packets, short *out);
extern void ImaDecodeStream(const unsigned char *src, ULong samples, short *out);

#endif
