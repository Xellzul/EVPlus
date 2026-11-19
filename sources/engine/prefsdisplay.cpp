/* prefsdisplay.cpp -- the Display group of Preferences: the resolutions it offers, their names, the UI scale
   range. The resolution list: each aspect ratio no wider than the desktop (4:3, 16:9, the desktop's own)
   at 600 lines, 768 lines (the original game's 1024 x 768) and the desktop's height. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "macport.h"
#include "burger.h"
#include "platform.h"
#include "prefsdisplay.h"

#define kDLOGType           0x444c4f47
#define kOriginalWidth      1024
#define kOriginalHeight     768
#define kSmallHeight        600
#define kRatioTolerance     0.01        /* two ratios this close are the same */
#define kUIScaleMin         1.0

typedef struct AspectName {
    double ratio;
    double tolerance;
    const char *name;
} AspectName;

static const AspectName kAspectNames[] = {
    { 4.0 / 3.0,   kRatioTolerance, "4:3" },
    { 5.0 / 4.0,   kRatioTolerance, "5:4" },
    { 16.0 / 10.0, kRatioTolerance, "16:10" },
    { 16.0 / 9.0,  kRatioTolerance, "16:9" },
    { 2.36,        0.03,            "21:9" },
    { 32.0 / 9.0,  kRatioTolerance, "32:9" },
};
#define kAspectNameCount (sizeof(kAspectNames) / sizeof(kAspectNames[0]))

static int SameRatio(double a, double b)
{
    return fabs(a - b) <= b * kRatioTolerance;
}

static void DesktopSize(int *width, int *height)
{
    PlatDesktopSize(width, height);
    if (*width <= 0 || *height <= 0) {
        *width = kOriginalWidth;
        *height = kOriginalHeight;
    }
}

/* the size shown for a kept resolution: 0 stands for the desktop's */
static void ShownSize(short width, short height, int *w, int *h)
{
    int desktopWidth;
    int desktopHeight;

    DesktopSize(&desktopWidth, &desktopHeight);
    *w = width != 0 ? width : desktopWidth;
    *h = height != 0 ? height : desktopHeight;
}

static void AddChoice(ResolutionChoice *out, short *count, int width, int height)
{
    short i;

    if (*count >= kResolutionChoiceMax)
        return;
    for (i = 0; i < *count; i++)
        if (out[i].width == width && out[i].height == height)
            return;
    out[*count].width = (short)width;
    out[*count].height = (short)height;
    *count += 1;
}

short ResolutionChoices(ResolutionChoice *out)
{
    double ratios[3];
    int heights[3];
    int desktopWidth;
    int desktopHeight;
    double desktopRatio;
    short ratioCount;
    short count;
    short r;
    short h;
    int width;

    DesktopSize(&desktopWidth, &desktopHeight);
    desktopRatio = (double)desktopWidth / desktopHeight;
    ratioCount = 0;
    if (4.0 / 3.0 <= desktopRatio * (1 + kRatioTolerance))
        ratios[ratioCount++] = 4.0 / 3.0;
    if (16.0 / 9.0 <= desktopRatio * (1 + kRatioTolerance))
        ratios[ratioCount++] = 16.0 / 9.0;
    if (ratioCount == 0 || !SameRatio(desktopRatio, ratios[ratioCount - 1]))
        ratios[ratioCount++] = desktopRatio;
    heights[0] = kSmallHeight;
    heights[1] = kOriginalHeight;
    heights[2] = desktopHeight;

    count = 0;
    for (r = 0; r < ratioCount; r++) {
        for (h = 0; h < 3; h++) {
            if (heights[h] > desktopHeight)
                continue;
            if (heights[h] == desktopHeight && SameRatio(ratios[r], desktopRatio)) {
                AddChoice(out, &count, 0, 0);
                continue;
            }
            width = 2 * (int)floor(heights[h] * ratios[r] / 2 + 0.5);
            if (width <= desktopWidth)
                AddChoice(out, &count, width, heights[h]);
        }
    }
    return count;
}

/* the entry of the list, -1 = none (a size set by hand in the preferences file) */
short FindResolutionChoice(const ResolutionChoice *list, short count, short width, short height)
{
    short i;

    for (i = 0; i < count; i++)
        if (list[i].width == width && list[i].height == height)
            return i;
    return -1;
}

void ResolutionSizeText(short width, short height, char *out)
{
    int w;
    int h;

    ShownSize(width, height, &w, &h);
    snprintf(out, kDisplayTextMax, "%d x %d", w, h);
}

void ResolutionAspectText(const ResolutionChoice *list, short count, short width, short height, char *out)
{
    const char *name;
    const char *note;
    char ratio[16];
    double value;
    size_t i;
    int w;
    int h;

    ShownSize(width, height, &w, &h);
    value = (double)w / h;
    name = NULL;
    for (i = 0; i < kAspectNameCount && name == NULL; i++)
        if (fabs(value - kAspectNames[i].ratio) <= kAspectNames[i].ratio * kAspectNames[i].tolerance)
            name = kAspectNames[i].name;
    if (name == NULL) {
        snprintf(ratio, sizeof(ratio), "%.2f:1", value);
        name = ratio;
    }
    note = "";
    if (FindResolutionChoice(list, count, width, height) < 0)
        note = " (custom)";
    else if (width == 0 && height == 0)
        note = " (native)";
    else if (width == kOriginalWidth && height == kOriginalHeight)
        note = " (original)";
    snprintf(out, kDisplayTextMax, "%s%s", name, note);
}

/* The largest UI scale at which every dialog (the game's and plug-ins' DLOGs, and one of ownWidth x
   ownHeight) still fits a screen of the resolution, in steps of 0.1; never below 1.0. */
double MaxUIScale(short width, short height, short ownWidth, short ownHeight)
{
    static short largestWidth;
    static short largestHeight;
    const unsigned char *t;
    Handle h;
    int32_t count;
    int32_t i;
    short w;
    short v;
    int screenWidth;
    int screenHeight;
    double scale;

    if (largestWidth == 0) {
        largestWidth = ownWidth;
        largestHeight = ownHeight;
        count = CountResources(kDLOGType);
        for (i = 1; i <= count; i++) {
            h = GetIndResource(kDLOGType, (short)i);
            if (h == NULL)
                continue;
            if (GetAHandleSize(h) >= 8) {
                t = (const unsigned char *)*h;
                v = (short)((t[4] << 8 | t[5]) - (t[0] << 8 | t[1]));
                w = (short)((t[6] << 8 | t[7]) - (t[2] << 8 | t[3]));
                if (w > largestWidth)
                    largestWidth = w;
                if (v > largestHeight)
                    largestHeight = v;
            }
            ReleaseResource(h);
        }
    }
    ShownSize(width, height, &screenWidth, &screenHeight);
    scale = (double)screenWidth / largestWidth;
    if ((double)screenHeight / largestHeight < scale)
        scale = (double)screenHeight / largestHeight;
    scale = floor(scale * 10 + 1e-6) / 10;
    return scale < kUIScaleMin ? kUIScaleMin : scale;
}

void UIScaleText(double scale, char *out)
{
    if (fabs(scale * 10 - floor(scale * 10 + 0.5)) < 1e-6)
        snprintf(out, kDisplayTextMax, "%.1f", scale);
    else
        snprintf(out, kDisplayTextMax, "%.2f", scale);
}
