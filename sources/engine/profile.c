// COMPILE: -lang c++ -bool off
/* profile.c -- the engine's own 64-slot sampling profiler (win TU region 0x0046fbf0..0x0046ff00). Notes: Research/notes/profile.md */
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "burger.h"

#define kProfileSlots 64

static unsigned long long lastMicroseconds[kProfileSlots];
uint32_t iterations[kProfileSlots];
uint32_t totalMilliseconds[kProfileSlots];    /* milliseconds */
char name[kProfileSlots][32];
short dependency[kProfileSlots];          /* -1 = none */
short lastEntered;
Boolean done[kProfileSlots];          /* dump bookkeeping */

EXTERN_C_BEGIN

/* print_dependents (mac _print_dependents). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fa30
// MAC: _print_dependents i386 0x000a4e86 ppc 0x000b0b80
void print_dependents(FILE *f, short slot, short depth)
{
    short i;

    for (i = 0; i < depth * 2; i++)
        fprintf(f, " ");
    fprintf(f, "%s:", name[slot]);
    for (i = strlen(name[slot]) + depth * 2; i < 32; i++)
        fprintf(f, " ");
    fprintf(f, " %8i iterations  %8i ms  %8.1f ms average",
            iterations[slot], totalMilliseconds[slot],
            totalMilliseconds[slot] / (float)iterations[slot]);
    if (dependency[slot] != -1) {
        for (i = 0; i < (depth + 1) * 2; i++)
            fprintf(f, " ");
        fprintf(f, "(%.1f%%)",
                totalMilliseconds[slot] * 100.0 / (float)totalMilliseconds[dependency[slot]]);
    }
    fprintf(f, "\n");
    done[slot] = 1;
    for (i = 0; i < kProfileSlots; i++) {
        if (done[i] == 0 && dependency[i] == slot)
            print_dependents(f, i, depth + 1);
    }
}

/* MyProfileInit (mac _MyProfileInit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fbf0
// MAC: _MyProfileInit i386 0x000a514f ppc 0x000b0eb8
void MyProfileInit(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++)
        strcpy(name[i], "");
    MyProfileReset();
}

/* MyProfileReset. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fc10
// MAC: _MyProfileReset i386 0x000a4d23 ppc 0x000b0988
void MyProfileReset(void)
{
    short i;

    for (i = 0; i < kProfileSlots; i++) {
        lastMicroseconds[i] = 0;
        totalMilliseconds[i] = 0;
        dependency[i] = -1;
        iterations[i] = 0;
    }
    lastEntered = -1;
}

/* MyProfileSetName (mac _MyProfileSetName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fde0
// MAC: _MyProfileSetName i386 0x000a4d8a ppc 0x000b09f0
void MyProfileSetName(short slot, const char *slotName)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
    strncpy(name[slot], slotName, 31);
}

/* MyProfileEnter (mac _MyProfileEnter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe20
// MAC: _MyProfileEnter i386 0x000a4dbd ppc 0x000b0a14
void MyProfileEnter(short slot)
{
    if (slot < 0 || slot >= kProfileSlots)
        return;
    lastMicroseconds[slot] = TimerGetMicroseconds();
    if (lastEntered != -1 && dependency[slot] == -1)
        dependency[slot] = lastEntered;
    lastEntered = slot;
}

/* MyProfileExit (mac _MyProfileExit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046fe80
// MAC: _MyProfileExit i386 0x000a4e11 ppc 0x000b0a94
void MyProfileExit(short slot)
{
    unsigned long long now;

    if (slot < 0 || slot >= kProfileSlots)
        return;
    now = TimerGetMicroseconds();
    totalMilliseconds[slot] += ((now - lastMicroseconds[slot]) / 1000);
    iterations[slot]++;
    if (lastEntered == slot) {
        if (dependency[slot] != -1)
            lastEntered = dependency[slot];
        else
            lastEntered = -1;
    }
}

/* MyProfileDump (mac _MyProfileDump). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ff00
// MAC: _MyProfileDump i386 0x000a5088 ppc 0x000b0dc0
void MyProfileDump(const char *path)
{
    FILE *f;
    short i;

    f = fopen(path, "r");
    if (f == 0)
        return;
    fclose(f);
    f = fopen(path, "w");
    if (f != 0) {
        fprintf(f, "Profile report:\n\n");
        for (i = 0; i < kProfileSlots; i++)
            done[i] = 0;
        for (i = 0; i < kProfileSlots; i++) {
            if (iterations[i] != 0 && done[i] == 0)
                print_dependents(f, i, 0);
        }
        fclose(f);
    }
}

EXTERN_C_END
