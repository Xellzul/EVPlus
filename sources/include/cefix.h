/* cefix.h -- the EV Nova Community Edition fixes (sources/cefix/), now part of the game.
   Plan, conventions, site list: Research/cefix-plan.md, Research/cefix-sites.json */
#ifndef CEFIX_H
#define CEFIX_H

#include "keycodes.h"

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

struct Rect;
struct DialogRecord;

/* cestartup.cpp */
#define kCEIniSection       "EV Nova"

extern int ceX2Hotkey;
extern unsigned char ceRunInAWindow;

void CE_Startup(void);
void CE_ReadX2Hotkey(void);
unsigned char CE_IsWindowed(void);
void CE_ToggleFullscreen(void);

/* cepaths.cpp */

int32_t CE_DecryptIfNecessary(void *buf, uint32_t len, uint32_t key);
short CE_OpenNovaFilesFallback(const char *path, const char *ext, FILE *log);
short CE_OpenPlugInsFallback(const char *path, const char *ext, FILE *log);
void CE_ExpandFilesPath(char *dst, const char *path);
void CE_ExpandPlugInsPath(char *dst, const char *path);

/* cetext.cpp */

void CE_DrawVersion(void);

/* cedialog.cpp */
int32_t CE_TruncF32(float value);
int32_t CE_RoundD(double value);

/* cedebug.cpp */

struct RGBColor;

unsigned char CE_CheckDebugKeys(ScanCode scanCode);
char *CE_NumberToAscii(char *dst, int32_t value, uint32_t flags);
void CE_SetFlashColor(const struct RGBColor *color);

/* cemap.cpp */
#define kCEBordersWidth     860         /* the border grid in 2 px cells: maps up to 1720x1024 */
#define kCEBordersHeight    512

extern int ceMapBordersVisibleWidth;
extern int ceMapBordersVisibleHeight;

void CE_RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, struct Rect *r);

#ifdef __cplusplus
}
#endif

#endif /* CEFIX_H */
