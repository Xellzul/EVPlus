/* portcolors.cpp -- the QuickDraw colour globals the port had to supply (0x00575acc .. 0x00575af0, blackRGB). Notes: Research/notes/portcolors.md */
#include "macport.h"

RGBColor blackRGB;

RGBColor *blackColor = &blackRGB;                /* 0x00575acc */

static RGBColor whiteRGB = { 0xffff, 0xffff, 0xffff };   /* 0x00575ad0 */
RGBColor *whiteColor = &whiteRGB;                /* 0x00575ad8 */

RGBColor redRGB = { 0xffff, 0x0000, 0x0000 };     /* 0x00575adc */
RGBColor *redColor = &redRGB;                    /* 0x00575ae4 */

static RGBColor cyanRGB = { 0x0000, 0xffff, 0xffff };    /* 0x00575ae8 */
RGBColor *cyanColor = &cyanRGB;                  /* 0x00575af0 */

extern const RGBColor betterRGB = { 0x4000, 0xffff, 0x4000 };
extern const RGBColor worseRGB = { 0xffff, 0x4000, 0x4000 };
