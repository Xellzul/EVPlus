/* burgerchan.h -- the sound channel table. Notes: Research/notes/burgerchan.md */

#ifndef BURGERCHAN_H
#define BURGERCHAN_H

#define kSoundChannels 16
#define kChannelBytes  sizeof(SoundChannel)                /* 0x38 on 32-bit */
#define kChannelTable  (kSoundChannels * kChannelBytes)   /* 0x380 on 32-bit */

/* A playing channel's position in its samples. */
typedef struct MixVoice {
    uint32_t pos;                                /* the next sample */
    uint32_t total;                              /* samples in the sound */
    int32_t lastLeft;                            /* the last levelled sample, for interpolation */
    int32_t lastRight;
} MixVoice;

typedef struct SoundChannel {
    uint32_t id;                            /* +0x00 */
    void *sound;                                 /* +0x04 */
    uint32_t stream;                        /* +0x08, 16.16 step */
    uint32_t device;                        /* +0x0c, 16.16 */
    void (*done)(int32_t reason, void *sound, void *ref);   /* +0x10 */
    void *data;                                  /* +0x14 */
    MixVoice voice;                              /* +0x18 */
    int32_t unk2C;                                /* +0x2c */
    unsigned short leftLevel;                    /* +0x30 */
    unsigned short rightLevel;                   /* +0x32 */
    unsigned short priority;                     /* +0x34 */
    unsigned char finished;                         /* +0x36 */
    unsigned char pad37;                         /* +0x37 */
} SoundChannel;

#ifdef __cplusplus
extern "C" {
#endif

extern SoundChannel gTheVoices[kSoundChannels];      /* atsound.cpp */

#ifdef __cplusplus
}
#endif

#endif
