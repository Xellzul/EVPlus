/* atsnddrv.cpp -- the Sound Toolkit's resampler: one channel stretched and levelled into 16-bit stereo
   (atsndvoice.cpp renders each sound with it when it starts).
   Notes: Research/notes/atsnddrv.md */

#include "burgerchan.h"

#define kRateShift        12
#define kFracOne          0x100
#define kLevelShift       7

/* Read a voice at a 20.12 step, add it into *out (stereo 16-bit, frames long) with linear interpolation
   between the samples it lands on, and advance. out 0 only advances. 0xff once the voice has run out.
   Each call starts its step count afresh. */
// FUNCTION: EVNOVA 0x00507f90
unsigned char PCM_Mixer(const short *data, short **out, ULong frames,
                        ULong step, MixVoice *voice,
                        unsigned short left, unsigned short right)
{
    short *dst;
    ULong remaining;
    ULong pos;
    ULong accum;
    ULong lastAccum;
    ULong delta;
    int32_t sample;
    int32_t lastL;
    int32_t lastR;
    int32_t l;
    int32_t r;
    int32_t frac;
    int32_t s;
    char done;

    accum = lastAccum = 0;
    done = 0;
    pos = voice->pos;
    lastL = voice->lastLeft;
    lastR = voice->lastRight;
    dst = out != 0 ? *out : 0;
    remaining = voice->total - pos;

    while (remaining != 0) {
        sample = data[pos];
        remaining--;
        pos++;

        accum += step;
        delta = (accum >> kRateShift) - (lastAccum >> kRateShift);
        if (delta >= 1) {
            lastAccum = accum;
            l = (sample * left) >> kLevelShift;
            r = (sample * right) >> kLevelShift;
            for (frac = kFracOne / delta; frac <= kFracOne; frac += kFracOne / delta) {
                if (dst != 0) {
                    s = (((l - lastL) * frac) >> 8) + lastL + dst[0];
                    dst[0] = s < -32768 ? -32768 : (s > 32767 ? 32767 : s);
                    s = (((r - lastR) * frac) >> 8) + lastR + dst[1];
                    dst[1] = s < -32768 ? -32768 : (s > 32767 ? 32767 : s);
                    dst += 2;
                }
                if (--frames == 0)
                    goto finished;
            }
            lastL = l;
            lastR = r;
        }
    }

finished:
    if (remaining == 0)
        done = 0xff;
    voice->pos = pos;
    voice->lastLeft = lastL;
    voice->lastRight = lastR;
    if (out != 0)
        *out = dst;
    return done;
}
