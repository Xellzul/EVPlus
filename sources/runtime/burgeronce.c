// COMPILE: -lang c++ -bool off
/* burgeronce.c -- BurgerLib W9Win95Code: the single-instance guard and the active flag. Notes: Research/notes/burgeronce.md */

#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

void *Burger::Win32::m_pInstance;                /* 0x008676a8 */

#ifdef __cplusplus
extern "C" {
#endif

Word Win95ApplsActive = 1;                       /* 0x00578248 */

static void *InstanceLock;                       /* 0x008676ac, mapping */
static int32_t Once;                                /* 0x008676b0, registered */

#define kMappingProtect     2                    /* PAGE_READONLY   */
#define kMappingSize        0x20
#define kAlreadyExists      0xb7                 /* ERROR_ALREADY_EXISTS */

// FUNCTION: EVNOVA 0x004f9bd0
void UnlockMe(void)
{
    if (InstanceLock != 0) {
        CloseHandle(InstanceLock);
        InstanceLock = 0;
    }
}

// FUNCTION: EVNOVA 0x004f9c00
int32_t Win95Allow1Instance(const char *name)
{
    InstanceLock = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, kMappingProtect, 0,
                                      kMappingSize, name);
    if (InstanceLock != 0 && GetLastError() != kAlreadyExists) {
        if (Once == 0) {
            atexit(UnlockMe);
            Once = 1;
        }
        return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif
