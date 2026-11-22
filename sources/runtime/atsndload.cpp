/* atsndload.cpp -- the Sound Toolkit's loader: a Mac `snd ' resource parsed into 16-bit PCM for the mixer.
   Notes: Research/notes/atsound.md */

#include "burgerchan.h"
#include "macport.h"

/* A loaded sound: this header, then `samples' 16-bit PCM samples in the machine's byte order. */
struct Sound {
    ULong tag;
    ULong samples;
    ULong rate;                                       /* 16.16 */
    ULong format;

    const short *Pcm() const { return (const short *)(this + 1); }
    short *Pcm() { return (short *)(this + 1); }
};

#define kMemSoundTag      0x646e7361UL                /* "asnd" in memory */
#define kMemSoundPcm      0x204d4350UL                /* "PCM " in memory */
#define kImaPacketBytes   34
#define kImaPacketSamples 64

uint32_t ST_SoundSamples(const Sound *sound)
{
    return sound->samples;
}

uint32_t ST_SoundRate(const Sound *sound)
{
    return sound->rate;
}

const short *ST_SoundData(const Sound *sound)
{
    return sound->Pcm();
}

// FUNCTION: EVNOVA 0x004d64d0
void ST_DisposeSound(Sound *sound)
{
    if (sound != 0 && ((intptr_t)sound & 1) == 0)
        free(sound);
}

/* A zeroed block; a zero size still gets one byte. */
static void *AllocClear(ULong size)
{
    return calloc(1, size != 0 ? size : 1);
}

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

/* A parsed sound as 16-bit PCM: `ima4' decoded, 8-bit samples widened by repeating the byte,
   16-bit ones as they are. Lengths are rounded up to 8 samples of silence. */
// FUNCTION: EVNOVA 0x004d6900
static Sound *_ST_SoundRefCreate(SoundDesc *desc)
{
    Sound *block;
    short *pcm;
    unsigned char *raw;
    unsigned short v;
    ULong count;
    ULong samples;
    ULong packets;
    ULong i;
    unsigned char ima;

    packets = 0;
    ima = desc->compression == kCompressionIma4 && desc->bits == kSixteenBit;
    if (ima) {
        if (desc->flags != 0) {
            count = desc->size * 2;                   /* bare nibbles, no packet headers */
        } else {
            packets = desc->size / kImaPacketBytes;
            count = packets * kImaPacketSamples;
        }
    } else if (desc->bits == 8) {
        count = desc->size;
    } else {
        count = desc->size / 2;
    }
    samples = (count + 7) & ~7;

    block = (Sound *)AllocClear(sizeof(Sound) + samples * sizeof(short));
    if (block == 0)
        return 0;
    pcm = block->Pcm();
    if (ima) {
        if (desc->flags != 0)
            ImaDecodeStream((const unsigned char *)desc->data, count, pcm);
        else
            ImaDecodePackets((const unsigned char *)desc->data, packets, pcm);
    } else if (desc->bits == 8) {
        raw = (unsigned char *)desc->data;
        for (i = 0; i < count; i++) {
            v = raw[i];
            if (desc->flags == 0)
                v ^= 0x80;
            v |= v << 8;
            pcm[i] = (short)v;
        }
    } else {
        memcpy(pcm, desc->data, count * sizeof(short));
    }

    block->tag = kMemSoundTag;
    block->samples = samples;
    block->rate = desc->rate;
    block->format = kMemSoundPcm;
    return block;
}

/* Where the sound header sits in a `snd ' resource: the data offset its
   first soundCmd or bufferCmd carries. 0 if there is none. */
// FUNCTION: EVNOVA 0x004d6d30
static int32_t _ST_SndDataOffset(void **handle)
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
    p = (short *)*handle;
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

/* flags != 0: the handle holds a bare sound header; its 8-bit samples are signed, its ima4 headerless */
// FUNCTION: EVNOVA 0x004d6e60
Sound *ST_LoadSndResourceIndirect(void **handle, int32_t flags)
{
    Sound *result = 0;
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

    if ((unsigned char)flags != 0)
        offset = 0;
    else
        offset = _ST_SndDataOffset(handle);

    if (offset >= 0) {
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

// FUNCTION: EVNOVA 0x004bc2a0
Sound *ST_LoadSndResource(short id)
{
    char **h;
    void **copy;
    uint32_t size;
    Sound *sound;

    /* the loader byte-swaps 16-bit samples in place, so it works on a copy */
    sound = NULL;
    h = GetResource(kSoundType, id);
    if (h != NULL) {
        LockAHandle(h);
        size = GetAHandleSize(h);
        copy = AllocAHandle2(size, 0);
        if (copy != NULL)
            memcpy(*copy, *h, size);
        ReleaseResource(h);
        if (copy != NULL) {
            sound = ST_LoadSndResourceIndirect(copy, 0);
            DeallocAHandle(copy);
        }
    }
    return sound;
}
