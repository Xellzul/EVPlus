/* atadpcm.cpp -- the Sound Toolkit's samples: decoding IMA ADPCM (Mac `ima4' sounds) into 16-bit PCM when a
   sound is loaded. Notes: Research/notes/atadpcm.md */

#include "burgerchan.h"

#define kMaxStepIndex 88

static int indexTable[16] = {                    /* 0x005794e8 */
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int stepsizeTable[kMaxStepIndex + 1] = {  /* 0x00579528 */
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

#define kImaPacketBytes     34                  /* a 2-byte header, then 32 bytes of nibbles */
#define kImaPacketSamples   64
#define kImaHeaderIndex     0x7f                /* the header's step index; the rest is the predictor */
#define kImaResyncDiff      0x7f                /* how far the running predictor may be from the header's */

/* One IMA ADPCM nibble: the next sample from the running predictor and step index. */
static short ImaNibble(int nibble, int *predictor, int *index)
{
    int step;
    int diff;

    step = stepsizeTable[*index];
    diff = step >> 3;
    if (nibble & 4)
        diff += step;
    if (nibble & 2)
        diff += step >> 1;
    if (nibble & 1)
        diff += step >> 2;
    if (nibble & 8)
        *predictor -= diff;
    else
        *predictor += diff;
    if (*predictor > 32767)
        *predictor = 32767;
    else if (*predictor < -32768)
        *predictor = -32768;
    *index += indexTable[nibble];
    if (*index < 0)
        *index = 0;
    else if (*index > kMaxStepIndex)
        *index = kMaxStepIndex;
    return (short)*predictor;
}

/* Mac `ima4' data: packets of 64 samples, the nibbles low one first, decoded as one stream from silence.
   A packet's header (big-endian: the predictor's top 9 bits, the step index) is taken
   only where the stream has drifted from it -- QuickTime's rule, and FFmpeg's: the header keeps only 9
   bits of the predictor, so re-seeding from every one would add a step of up to 127 at each packet. */
void ImaDecodePackets(const unsigned char *src, ULong packets, short *out)
{
    ULong p;
    int i;
    int header;
    int headerPredictor;
    int headerIndex;
    int predictor;
    int index;

    predictor = 0;
    index = 0;
    for (p = 0; p < packets; p++) {
        header = (src[0] << 8) | src[1];
        headerPredictor = (short)(header & ~kImaHeaderIndex);
        headerIndex = header & kImaHeaderIndex;
        if (headerIndex > kMaxStepIndex)
            headerIndex = kMaxStepIndex;
        if (headerIndex != index || headerPredictor - predictor > kImaResyncDiff
                || predictor - headerPredictor > kImaResyncDiff) {
            predictor = headerPredictor;
            index = headerIndex;
        }
        src += 2;
        for (i = 0; i < kImaPacketSamples / 2; i++) {
            *out++ = ImaNibble(src[i] & 0xf, &predictor, &index);
            *out++ = ImaNibble(src[i] >> 4, &predictor, &index);
        }
        src += kImaPacketSamples / 2;
    }
}

/* Headerless IMA ADPCM, one stream from silence, the nibbles high one first. */
void ImaDecodeStream(const unsigned char *src, ULong samples, short *out)
{
    ULong i;
    int predictor;
    int index;

    predictor = 0;
    index = 0;
    for (i = 0; i < samples; i++)
        out[i] = ImaNibble((i & 1) ? (src[i / 2] & 0xf) : (src[i / 2] >> 4), &predictor, &index);
}
