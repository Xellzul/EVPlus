/* atsound.c -- the Sound Toolkit: bringing the sound layer up and down, starting and halting sounds over the channel table, and parsing a Mac `snd ' resource out of memory. Notes: Research/notes/atsound.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"

#include "burgerchan.h"

extern int32_t _ST_CreateSoundPump(void);
extern void _ST_DisposeSoundPump(void);                  /* 0x005076a0 */

/* A zeroed block; the exe took it from the process heap, which answers a request for no bytes with a
   block, so a zero size still gets one byte. */
static void *AllocClear(ULong size)
{
    return calloc(1, size != 0 ? size : 1);
}


int32_t gMaxVoices;                               /* 0x00870958 channels */
int32_t gCookieCounter;                               /* 0x00870df4 */

#define kMaxChannels    kSoundChannels
/* the mix is 16-bit stereo at this rate (16.16): what the exe chose on a machine with DirectSound */
#define kOutputRate     0xac440000UL                     /* 44100.0 */


// FUNCTION: EVNOVA 0x004d6290
int32_t ST_Open(unsigned short channels, ULong device)
{
    int32_t err, e;

    err = 0;
    if (gSoundToolOpen != 0)
        goto done;

    e = CriticalCreate(&gCriticalSection);
    if (e != 0) {
        err = e;
        goto done;
    }

    gMaxVoices = channels;
    if (gMaxVoices > kMaxChannels)
        gMaxVoices = kMaxChannels;

    gCookieCounter = 1;
    gNumVoices = 0;
    memset(gTheVoices, 0, kChannelTable);

    if (device == 0)
        gSampleRate = kOutputRate;
    else
        gSampleRate = device;

    e = _ST_CreateSoundPump();
    if (e != 0) {
        err = e;
        goto done;
    }

    gSoundToolOpen = 1;
    ST_HaltSound(0);
done:
    return err;
}

/* pooled doubles of a function the linker stripped; nothing reads them */
static double DAT_00576b10[7] = {
    440.0, 16384.0, 6.2831852 * 440.0, 6.2831852, 64.0, 128.0, 65536.0
};

/* the device's volume as the exe found it when the Sound Tool opened (to put back at close): not read
   any more, there is no device volume on the platform layer. ST_GetSysVolume still computes from these,
   which stay 0, so its answer means nothing; no caller uses it. */
ULong gSaveLeftVolume;                              /* 0x008700a4, 4 B */
ULong gSaveRightVolume;                              /* 0x00870100, 4 B */
ULong gSaveMaxVolume;                              /* 0x00870dcc, 4 B */

#define kDefaultRate 0x10000

// FUNCTION: EVNOVA 0x004d6440
void ST_Close(void)
{
    int32_t err, e;

    err = 0;
    if (gSoundToolOpen != 0) {
        ST_HaltSound(0);
        _ST_DisposeSoundPump();
        e = CriticalDispose(&gCriticalSection);
        if (e != 0)
            err = e;
        gSoundToolOpen = 0;
    }
}

// FUNCTION: EVNOVA 0x004d64b0
ULong ST_GetSysVolume(void)
{
    int32_t err;
    ULong level;

    err = 0;
    level = 0;
    level = ((((gSaveLeftVolume + gSaveRightVolume) >> 1) << 8)
             + gSaveMaxVolume - 1) >> 8;
    return level;
}

// FUNCTION: EVNOVA 0x004d64d0
void ST_DisposeSound(void *sound)
{
    int32_t err;

    err = 0;
    if (sound != 0 && ((intptr_t)sound & 1) == 0)
        free(sound);
}

// FUNCTION: EVNOVA 0x004d64f0
int32_t ST_PlaySound(void *snd, unsigned short priority, unsigned short level)
{
    SoundRequest req;
    int32_t err;
    int32_t result;

    err = 0;
    result = 0;
    req.handle = snd;
    req.priority = priority;
    req.leftLevel = level;
    req.rightLevel = level;
    req.rate = kDefaultRate;
    req.done = 0;
    req.unk10 = 0;
    req.id = 0;
    result = ST_PlaySoundParam(&req);
    return result;
}

#define kMinPriority 1
#define kMaxLevel    0x80
#define kIdStep      2

// FUNCTION: EVNOVA 0x004d6550
ULong ST_PlaySoundParam(SoundRequest *req)
{
    ULong *raw;
    ULong id;
    ULong i;
    char preempt;
    SoundChannel *src;
    SoundChannel *dst;
    int32_t tail;

    tail = 0;
    id = 0;
    preempt = 0;
    raw = (ULong *)req->handle;
    if (req->handle != 0 && gSoundToolOpen != 0) {
        if (req->priority < kMinPriority)
            req->priority = kMinPriority;
        if (req->rightLevel > kMaxLevel)
            req->rightLevel = kMaxLevel;
        if (req->leftLevel > kMaxLevel)
            req->leftLevel = kMaxLevel;

        i = 0;
        CriticalEnter(&gCriticalSection);
        while (i < kSoundChannels && gTheVoices[i].sound != 0 && preempt == 0) {
            if (req->priority >= gTheVoices[i].priority
                && req->rightLevel + req->leftLevel
                   >= gTheVoices[i].leftLevel + gTheVoices[i].rightLevel)
                preempt = 1;
            else
                i++;
        }

        if (i >= kSoundChannels) {
            CriticalLeave(&gCriticalSection);
            return 0;
        }

        if (gNumVoices >= kSoundChannels)
            gNumVoices = kSoundChannels - 1;

        src = &gTheVoices[i];
        dst = &gTheVoices[i + 1];
        tail = (gNumVoices - i) * kChannelBytes;
        if (tail != 0)
            memmove(dst, src, tail);

        gTheVoices[i].sound = req->handle;
        gTheVoices[i].data = (char *)req->handle + 0x10;
        gTheVoices[i].voice.valprev = 0;
        gTheVoices[i].voice.index = 0;
        gTheVoices[i].voice.pos = 0;
        gTheVoices[i].voice.total = raw[1];
        gTheVoices[i].voice.lastLeft = 0;
        gTheVoices[i].voice.lastRight = 0;

        gTheVoices[i].device = UFix32_Divide(gSampleRate, raw[2]);
        gTheVoices[i].stream = UFix32_Multiply(gTheVoices[i].device, req->rate);
        gTheVoices[i].done = req->done;
        gTheVoices[i].priority = req->priority;
        gTheVoices[i].leftLevel = req->leftLevel;
        gTheVoices[i].rightLevel = req->rightLevel;
        gTheVoices[i].unk2C = req->unk10;
        gTheVoices[i].finished = 0;

        if (req->id != 0) {
            gTheVoices[i].id = req->id;
        } else {
            gTheVoices[i].id = gCookieCounter;
            gCookieCounter += kIdStep;
        }

        id = gTheVoices[i].id;
        gNumVoices++;
        CriticalLeave(&gCriticalSection);
    }
    return id;
}

#define kHaltReason    2

unsigned char gSoundToolOpen;                      /* 0x00870e12, live flag */
int32_t gNumVoices;                               /* 0x00870db0, in use */
SoundChannel gTheVoices[kSoundChannels];       /* 0x0086f718, 0x380 B */
Mutex gCriticalSection;                             /* 0x0086dc88, the lock (16 bytes of storage in the exe) */
SoundRequest gSoundVoiceStatus;                       /* 0x008707dc, callback ref */

/* Counts the channels this sound occupies; a null sound counts them all. */
// FUNCTION: EVNOVA 0x004d6770
unsigned short ST_IsSoundPlaying(void *sound)
{
    int32_t found;
    ULong i;
    int32_t err;

    err = 0;
    found = 0;
    if (gSoundToolOpen != 0) {
        i = 0;
        CriticalEnter(&gCriticalSection);
        while (i < gNumVoices) {
            if ((void *)(uintptr_t)gTheVoices[i].id == sound
                || gTheVoices[i].sound == sound
                || sound == 0)
                found = found + 1;
            i = i + 1;
        }
        CriticalLeave(&gCriticalSection);
    }
    return found;
}

// FUNCTION: EVNOVA 0x004d67d0
void ST_HaltSound(void *sound)
{
    ULong i;
    SoundChannel *src;
    void *snd;
    SoundChannel *dst;
    int32_t tail;
    void (*done)(int32_t, void *, void *);

    snd = 0;
    if (gSoundToolOpen != 0) {
        i = 0;
        CriticalEnter(&gCriticalSection);
        while (i < gNumVoices) {
            if ((void *)(uintptr_t)gTheVoices[i].id == sound
                || gTheVoices[i].sound == sound
                || sound == 0) {
                done = gTheVoices[i].done;
                snd = gTheVoices[i].sound;
                gNumVoices--;
                dst = &gTheVoices[i];
                src = dst + 1;
                tail = (gNumVoices - i) * kChannelBytes;
                if (tail != 0)
                    memmove(dst, src, tail);
                dst = &gTheVoices[gNumVoices];
                memset(dst, 0, kChannelBytes);
                if (done != 0)
                    done(kHaltReason, sound, &gSoundVoiceStatus);
            } else {
                i++;
            }
        }
        CriticalLeave(&gCriticalSection);
    }
}

/* Drop one channel and close the table up behind it. */
// FUNCTION: EVNOVA 0x004d6890
void _ST_HaltASound(short index)
{
    unsigned char ok;
    SoundChannel *src;
    SoundChannel *dst;
    int32_t tail;

    ok = 0;
    if (gSoundToolOpen != 0) {
        gNumVoices--;
        src = &gTheVoices[index + 1];
        dst = &gTheVoices[index];
        tail = (gNumVoices - index) * kChannelBytes;
        if (tail != 0)
            memmove(dst, src, tail);
        dst = &gTheVoices[gNumVoices];
        memset(dst, 0, kChannelBytes);
    }
}

ULong gSampleRate;                                   /* 0x0086ff90 */

/* Placement only: no reader in the exe. */
static unsigned char sSoundMemPad[8];                 /* 0x00865898 */

#define kStandardHeader   0x00
#define kCompressedHeader 0xfe
#define kExtendedHeader   0xff
#define kStandardSkip     0x16
#define kExtendedSkip     0x40
#define kCompressionNone  0x4e4f4e45UL                /* 'NONE' */
#define kCompressionIma4  0x696d6134UL                /* 'ima4' */
#define kSixteenBit       16

typedef struct SoundDesc {
    void *data;                                       /* +0x00 */
    ULong rate;                                       /* +0x04 */
    unsigned short bits;                              /* +0x08 */
    short channels;                                   /* +0x0a */
    ULong compression;                                /* +0x0c */
    ULong size;                                       /* +0x10 */
    unsigned char flags;                              /* +0x14 */
} SoundDesc;


#define kSndFormat1       1
#define kSndFormat2       2
#define kDataPointerFlag  0x8000
#define kSoundCmd         (kDataPointerFlag | 80)
#define kBufferCmd        (kDataPointerFlag | 81)
#define kDataFormatShorts 3                           /* short type, long init */
#define kSndCommandShorts 4                           /* short cmd, param1; long param2 */

/* A packed sound: this header, then IMA ADPCM, two samples a byte. */
typedef struct MemSound {
    ULong tag;
    ULong samples;
    ULong rate;
    ULong format;
} MemSound;

#define kMemSoundTag      0x646e7361UL                /* "asnd" in memory */
#define kMemSoundIma      0x6d494d41UL                /* "AMIm" in memory */
#define kImaPacketBytes   34
#define kImaPacketWords   (kImaPacketBytes / 2)

/* Pack a parsed sound as IMA ADPCM behind a MemSound header. Mac `ima4'
   data is already ADPCM and only loses each packet's two-byte header. */
// FUNCTION: EVNOVA 0x004d6900
void *_ST_SoundRefCreate(SoundDesc *desc)
{
    int32_t err = 0;
    short *pcm = 0;
    unsigned short *temp = 0;
    MemSound *block = 0;
    unsigned char *raw;
    ULong count;
    ULong samples;
    ULong bytes;
    struct adpcm_state state;

    if (desc->compression == kCompressionIma4 && desc->bits == kSixteenBit) {
        if (desc->flags != 0) {
            count = desc->size * 2;
            samples = (count + 7) & ~7;
            bytes = samples / 2;
            bytes += sizeof(MemSound);
            block = AllocClear(bytes);
            if (block != 0)
                BlockMoveData(desc->data, block + 1, desc->size);
        } else {
            count = desc->size * 2;
            samples = (count - count / kImaPacketBytes + 7) & ~7;
            bytes = samples / 2;
            block = AllocClear(bytes + sizeof(MemSound));
            if (block != 0) {
                unsigned short *src;
                unsigned short *dst;
                unsigned short v;
                ULong i;

                src = desc->data;
                dst = (unsigned short *)(block + 1);
                for (i = 0; i < desc->size / 2; i++) {
                    v = *src++;
                    if (i % kImaPacketWords != 0) {
                        v = ((v << 4) & 0xf0f0) | ((v >> 4) & 0x0f0f);
                        *dst++ = v;
                    }
                }
            }
        }
    } else {
        if (desc->bits == 8) {
            count = desc->size;
            samples = (count + 7) & ~7;
            bytes = samples * 2;
            raw = desc->data;
            temp = AllocClear(bytes);
            if (temp != 0) {
                unsigned short *dst;
                unsigned short v;
                ULong i;

                dst = temp;
                if (desc->flags != 0) {
                    for (i = 0; i < count; i++) {
                        v = *raw++;
                        v |= v << 8;
                        *dst++ = v;
                    }
                } else {
                    for (i = 0; i < count; i++) {
                        v = *raw++;
                        v += 0x80;
                        v |= v << 8;
                        *dst++ = v;
                    }
                }
            } else
                return 0;
            pcm = (short *)temp;
        } else {
            count = desc->size / 2;
            samples = (count + 7) & ~7;
            bytes = samples * 2;
            if (bytes == desc->size) {
                pcm = desc->data;
            } else {
                temp = AllocClear(bytes);
                if (temp == 0)
                    return 0;
                BlockMoveData(desc->data, temp, desc->size);
                pcm = (short *)temp;
            }
        }
        block = AllocClear(bytes / 4 + sizeof(MemSound));
        if (block != 0) {
            state.valprev = 0;
            state.index = 0;
            adpcm_coder(pcm, (char *)(block + 1), samples, &state);
        }
    }

    if (block != 0) {
        block->tag = kMemSoundTag;
        block->samples = samples;
        block->rate = desc->rate;
        block->format = kMemSoundIma;
    }
    if (temp != 0)
        free(temp);
    return block;
}

#define kVoiceStepShift   4                           /* 16.16 rate to the decoder's 20.12 */
#define kSoundTick        3
#define kSoundFinished    1

/* Mix every playing channel into a zeroed stereo 16-bit buffer, then tell
   each owner it ticked and retire the channels that ran out. Channels past
   the voice limit are decoded without being heard. */
typedef void (*MixerNotifyProc)(int32_t reason, ULong id, SoundRequest *request);

// FUNCTION: EVNOVA 0x004d6b90
void _ST_MixVoices(short *buffer, ULong frames)
{
    int32_t err = 0;
    int32_t i;
    SoundChannel *chan;
    short *out;
    short *p;
    void *mix;
    MixerNotifyProc done;
    ULong id;

    out = buffer;
    p = buffer;
    memset(mix = buffer, 0, frames * 4);
    for (i = 0; i < gNumVoices; i++) {
        chan = &gTheVoices[i];
        out = buffer;
        if (i < gMaxVoices)
            chan->finished = ADPCM_Mixer(chan->data, &out, frames, chan->stream >> kVoiceStepShift,
                                       &chan->voice, chan->leftLevel, chan->rightLevel);
        else
            chan->finished = ADPCM_Mixer(chan->data, 0, frames, chan->stream >> kVoiceStepShift,
                                       &chan->voice, chan->leftLevel, chan->rightLevel);
    }

    for (i = 0; i < gNumVoices; i++) {
        done = (MixerNotifyProc)gTheVoices[i].done;
        id = gTheVoices[i].id;
        if (done != 0) {
            gSoundVoiceStatus.handle = gTheVoices[i].sound;
            gSoundVoiceStatus.priority = gTheVoices[i].priority;
            gSoundVoiceStatus.leftLevel = gTheVoices[i].leftLevel;
            gSoundVoiceStatus.rightLevel = gTheVoices[i].rightLevel;
            gSoundVoiceStatus.rate = kUnityRate;
            gSoundVoiceStatus.done = gTheVoices[i].done;
            gSoundVoiceStatus.unk10 = gTheVoices[i].unk2C;
            gSoundVoiceStatus.id = id;
            gSoundVoiceStatus.totalSamples = gTheVoices[i].voice.total;
            done(kSoundTick, id, &gSoundVoiceStatus);
        }
        if (gTheVoices[i].finished != 0) {
            _ST_HaltASound(i);
            i--;
            if (done != 0)
                done(kSoundFinished, id, &gSoundVoiceStatus);
        }
    }
}

// FUNCTION: EVNOVA 0x004d6d00
void _ST_MixerDispatch(short *buffer, ULong frames)
{
    CriticalEnter(&gCriticalSection);
    _ST_MixVoices(buffer, frames);
    CriticalLeave(&gCriticalSection);
}

/* Where the sound header sits in a `snd ' resource: the data offset its
   first soundCmd or bufferCmd carries. 0 if there is none. */
// FUNCTION: EVNOVA 0x004d6d30
int32_t _ST_SndDataOffset(void **handle)
{
    short *p;
    int32_t offset;
    short count;
    unsigned char found;
    short bufferCmd;
    short soundCmd;
    short cmd;

    offset = 0;
    found = 0;
    bufferCmd = kBufferCmd;
    soundCmd = kSoundCmd;
    p = *handle;
    switch ((long long)Swap16(p[0])) {
    case kSndFormat1:
        count = Swap16(p[1]);
        p = p + 2;
        p = p + count * kDataFormatShorts;
        break;
    case kSndFormat2:
        p = p + 2;
        break;
    default:
        found = 1;
        break;
    }

    count = Swap16(p[0]);
    p = p + 1;
    while (count > 0 && found == 0) {
        cmd = Swap16(p[0]);
        if (cmd == bufferCmd || cmd == soundCmd) {
            offset = Swap32(((int32_t *)p)[1]);
            found = 1;
        } else {
            p = p + kSndCommandShorts;
            count--;
        }
    }
    return offset;
}

/* The Sound Manager's three sample headers (big-endian), as far as they are read. */
typedef struct SoundHeader {
    ULong samplePtr;                                  /* +0x00 */
    ULong length;                                     /* +0x04 */
    ULong sampleRate;                                 /* +0x08, 16.16 */
    ULong loopStart;                                  /* +0x0c */
    ULong loopEnd;                                    /* +0x10 */
    unsigned char encode;                             /* +0x14 */
    unsigned char baseFrequency;                      /* +0x15 */
} SoundHeader;

typedef struct ExtSoundHeader {
    ULong samplePtr;                                  /* +0x00 */
    ULong numChannels;                                /* +0x04 */
    ULong sampleRate;                                 /* +0x08 */
    ULong loopStart;                                  /* +0x0c */
    ULong loopEnd;                                    /* +0x10 */
    unsigned char encode;                             /* +0x14 */
    unsigned char baseFrequency;                      /* +0x15 */
    unsigned char numFrames[4];                       /* +0x16 */
    unsigned char AIFFSampleRate[10];                 /* +0x1a */
    ULong markerChunk;                                /* +0x24 */
    ULong instrumentChunks;                           /* +0x28 */
    ULong AESRecording;                               /* +0x2c */
    unsigned short sampleSize;                        /* +0x30 */
} ExtSoundHeader;

typedef struct CmpSoundHeader {
    ULong samplePtr;                                  /* +0x00 */
    ULong numChannels;                                /* +0x04 */
    ULong sampleRate;                                 /* +0x08 */
    ULong loopStart;                                  /* +0x0c */
    ULong loopEnd;                                    /* +0x10 */
    unsigned char encode;                             /* +0x14 */
    unsigned char baseFrequency;                      /* +0x15 */
    unsigned char numFrames[4];                       /* +0x16 */
    unsigned char AIFFSampleRate[10];                 /* +0x1a */
    ULong markerChunk;                                /* +0x24 */
    ULong format;                                     /* +0x28 */
    ULong futureUse2;                                 /* +0x2c */
    ULong stateVars;                                  /* +0x30 */
    ULong leftOverSamples;                            /* +0x34 */
    short compressionID;                              /* +0x38 */
    short packetSize;                                 /* +0x3a */
    short snthID;                                     /* +0x3c */
    unsigned short sampleSize;                        /* +0x3e */
} CmpSoundHeader;

// FUNCTION: EVNOVA 0x004d6e60
void *ST_LoadSndResourceIndirect(void **handle, int32_t flags)
{
    int32_t err = 0;
    void *result = 0;
    SoundDesc desc;
    SoundHeader *header;
    ExtSoundHeader *ext;
    CmpSoundHeader *cmp;
    ULong compression;
    int32_t offset;
    int32_t count;
    int32_t i;
    int32_t handleSize;
    int32_t size;
    ULong rateStatus;
    unsigned char stereo;

    if ((unsigned char)flags != 0)
        offset = 0;
    else
        offset = _ST_SndDataOffset(handle);

    if (offset >= 0) {
        stereo = 0;
        header = (SoundHeader *)((unsigned char *)*handle + offset);

        switch (header->encode) {
        case kStandardHeader:
            offset += kStandardSkip;
            desc.rate = Swap32(header->sampleRate);
            desc.bits = 8;
            desc.channels = 1;
            desc.compression = kCompressionNone;
            break;
        case kExtendedHeader:
            ext = (ExtSoundHeader *)header;
            if ((long long)Swap32(ext->numChannels) > 1)
                return 0;
            offset += kExtendedSkip;
            desc.rate = Swap32(ext->sampleRate);
            desc.bits = Swap16(ext->sampleSize);
            desc.channels = Swap32(ext->numChannels);
            desc.compression = kCompressionNone;
            break;
        case kCompressedHeader:
            cmp = (CmpSoundHeader *)header;
            if ((long long)Swap32(cmp->numChannels) > 1)
                return 0;
            offset += kExtendedSkip;
            compression = Swap32(cmp->format);
            if (compression != kCompressionNone && compression != kCompressionIma4)
                return 0;
            desc.rate = Swap32(cmp->sampleRate);
            desc.bits = Swap16(cmp->sampleSize);
            desc.channels = Swap32(cmp->numChannels);
            desc.compression = Swap32(cmp->format);
            break;
        }

        rateStatus = UFix32_Divide(gSampleRate, (long long)Swap32(header->sampleRate));

        handleSize = GetAHandleSize((Handle)handle);
        size = handleSize - offset;
        desc.data = (unsigned char *)*handle + offset;
        desc.size = size;
        desc.flags = flags;

        if (desc.bits == kSixteenBit && desc.compression == kCompressionNone) {
            count = desc.size / (desc.bits / 8);
            for (i = 0; i < count; i++)
                ((unsigned short *)desc.data)[i] = Swap16(((unsigned short *)desc.data)[i]);
        }

        result = _ST_SoundRefCreate(&desc);
    }

    return result;
}
