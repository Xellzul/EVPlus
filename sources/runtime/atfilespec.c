/* atfilespec.c -- what is left of AmbrosiaTools' file tool: FT_Open and FT_Close. The exe linked the whole
   library (folder specs, native files, files of files, memory files, the file-type map; from 0x004da12a and
   from 0x00509fa2) and the game called nothing of it but these two: its files go through BurgerLib
   (burgerfilemgr.c). The library is in git history. */

#include "burgertypes.h"
#include "burger.h"

unsigned char gFT_Open;                      /* 0x00870e08 */

/* The exe's FT_Open also filled the tool's tables and looked up the program's folder; all of it was read
   by the tool alone. */
// FUNCTION: EVNOVA 0x004da12a
int32_t FT_Open(void)
{
    int32_t err;

    if (gFT_Open != 0)
        return 0;
    err = PlatformOpen();
    if (err != 0)
        return err;
    SystemSetQuitProc(FT_Close);
    gFT_Open = 1;
    return 0;
}

// FUNCTION: EVNOVA 0x004da2ad
void FT_Close(void)
{
    gFT_Open = 0;
}
