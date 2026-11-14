/* excrtl.c -- MSL's x86 exception runtime: the structured-exception unwind handler. Notes: Research/notes/excrtl.md */
#include "int32.h"

#define kUnwindFlags            6                /* EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND */
#define kContinueSearch         1                /* ExceptionContinueSearch */
#define kCollidedUnwind         3                /* ExceptionCollidedUnwind */

typedef struct ExceptionRecord {
    uint32_t code;                          /* +0x00 */
    uint32_t flags;                         /* +0x04 */
} ExceptionRecord;

/* __SEHUnwindHandler (MSL excrtl, `?__SEHUnwindHandler@@YA..' there). STATE: MATCH, exe 10 /
   obj 10. OPEN: none */
// FUNCTION: EVNOVA 0x00534550
int32_t SEHUnwindHandler(ExceptionRecord *record, void *frame, void *context, void **dispatcher)
{
    if ((record->flags & kUnwindFlags) == 0)
        return kContinueSearch;
    *dispatcher = frame;
    return kCollidedUnwind;
}
