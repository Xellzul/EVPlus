/* prefsdisplay.cpp -- the Display group of Preferences: the resolutions it offers, their names, the UI scale
   range. */
#include <math.h>
#include "burger.h"
#include "platform.h"
#include "prefsdisplay.h"

namespace {

constexpr uint32_t kDLOGType = 0x444c4f47;
constexpr short kOriginalWidth = 1024;
constexpr short kOriginalHeight = 768;
constexpr short kSmallHeight = 600;
constexpr double kRatioTolerance = 0.01;        /* two ratios this close are the same */
constexpr double kUIScaleMaxFloor = 1.0;      /* the largest UI scale offered is never below this */
constexpr double kUIScaleStep = 0.1;

struct AspectName {
    double ratio;
    double tolerance;
    const char *name;
};

constexpr AspectName kAspectNames[] = {
    { 4.0 / 3.0,   kRatioTolerance, "4:3" },
    { 5.0 / 4.0,   kRatioTolerance, "5:4" },
    { 16.0 / 10.0, kRatioTolerance, "16:10" },
    { 16.0 / 9.0,  kRatioTolerance, "16:9" },
    { 2.36,        0.03,            "21:9" },       /* sold as 21:9: 2560 x 1080, 3440 x 1440, ... */
    { 32.0 / 9.0,  kRatioTolerance, "32:9" },
};

bool SameRatio(double a, double b)
{
    return fabs(a - b) <= b * kRatioTolerance;
}

using Size = PixelSize;

Size DesktopSize()
{
    Size s;

    PlatDesktopSize(&s.width, &s.height);
    if (s.width <= 0 || s.height <= 0)
        s = { kOriginalWidth, kOriginalHeight };
    return s;
}

/* the largest dialog the open game files and plug-ins have (DLOG bounds) */
Size LargestDialog()
{
    Size largest = { 1, 1 };
    int32_t count = CountResources(kDLOGType);

    for (int32_t i = 1; i <= count; i++) {
        Handle h = GetIndResource(kDLOGType, (short)i);
        if (h == NULL)
            continue;
        if (GetAHandleSize(h) >= 8) {
            const unsigned char *t = (const unsigned char *)*h;
            short height = (short)((t[4] << 8 | t[5]) - (t[0] << 8 | t[1]));
            short width = (short)((t[6] << 8 | t[7]) - (t[2] << 8 | t[3]));
            if (width > largest.width)
                largest.width = width;
            if (height > largest.height)
                largest.height = height;
        }
        ReleaseResource(h);
    }
    return largest;
}

} // namespace

PixelSize GameSize(Resolution r)
{
    Size desktop = DesktopSize();

    return { r.width != 0 ? r.width : desktop.width, r.height != 0 ? r.height : desktop.height };
}

PixelSize DrawSize(Resolution r)
{
    Size desktop = DesktopSize();
    Size game = GameSize(r);
    Size draw = game;
    double fit = (double)desktop.width / game.width < (double)desktop.height / game.height
                     ? (double)desktop.width / game.width
                     : (double)desktop.height / game.height;

    if (fit > 1.0) {
        draw.width = 2 * (int)(game.width * fit / 2 + 0.5);
        draw.height = (int)(game.height * fit + 0.5);
    }
    return draw;
}

double BaseUIScale(PixelSize draw)
{
    double scale = (double)draw.height / kOriginalHeight;

    if ((double)draw.width / kOriginalWidth < scale)
        scale = (double)draw.width / kOriginalWidth;
    return scale > 1.0 ? scale : 1.0;
}

ResolutionList::ResolutionList() : count(0)
{
    Size desktop = DesktopSize();
    double desktopRatio = (double)desktop.width / desktop.height;
    std::array<double, 3> ratios;
    int ratioCount = 0;

    if (4.0 / 3.0 <= desktopRatio * (1 + kRatioTolerance))
        ratios[ratioCount++] = 4.0 / 3.0;
    if (16.0 / 9.0 <= desktopRatio * (1 + kRatioTolerance))
        ratios[ratioCount++] = 16.0 / 9.0;
    if (ratioCount == 0 || !SameRatio(desktopRatio, ratios[ratioCount - 1]))
        ratios[ratioCount++] = desktopRatio;

    const int heights[] = { kSmallHeight, kOriginalHeight, desktop.height };
    for (int r = 0; r < ratioCount; r++) {
        for (int height : heights) {
            if (height > desktop.height)
                continue;
            if (height == desktop.height && SameRatio(ratios[r], desktopRatio)) {
                add(0, 0);
                continue;
            }
            int width = 2 * (int)floor(height * ratios[r] / 2 + 0.5);
            if (width <= desktop.width)
                add(width, height);
        }
    }
}

void ResolutionList::add(int width, int height)
{
    Resolution r = { (short)width, (short)height };

    if (count < (int)list.size() && find(r) < 0)
        list[count++] = r;
}

int ResolutionList::find(Resolution r) const
{
    for (int i = 0; i < count; i++)
        if (list[i] == r)
            return i;
    return -1;
}

/* from a resolution not in the list, up goes to the first, down to the last */
Resolution ResolutionList::step(Resolution r, int direction) const
{
    if (count == 0)
        return r;
    int i = find(r);
    if (i < 0)
        i = direction > 0 ? 0 : count - 1;
    else if (i + direction >= 0 && i + direction < count)
        i += direction;
    return list[i];
}

void ResolutionList::sizeText(Resolution r, char *out) const
{
    Size s = GameSize(r);

    snprintf(out, kDisplayTextMax, "%d x %d", s.width, s.height);
}

void ResolutionList::aspectText(Resolution r, char *out) const
{
    Size s = GameSize(r);
    double value = (double)s.width / s.height;
    char ratio[16];
    const char *name = NULL;

    for (const AspectName &a : kAspectNames)
        if (name == NULL && fabs(value - a.ratio) <= a.ratio * a.tolerance)
            name = a.name;
    if (name == NULL) {
        snprintf(ratio, sizeof(ratio), "%.2f:1", value);
        name = ratio;
    }
    const char *note = "";
    if (find(r) < 0)
        note = " (custom)";
    else if (r.width == 0 && r.height == 0)
        note = " (native)";
    else if (r.width == kOriginalWidth && r.height == kOriginalHeight)
        note = " (original)";
    snprintf(out, kDisplayTextMax, "%s%s", name, note);
}

/* every dialog = the game's and the plug-ins' DLOGs and one of ownWidth x ownHeight, on the screen as it is
   at UI scale 1.0; in steps of 0.1, never below 1.0 */
double MaxUIScale(Resolution r, short ownWidth, short ownHeight)
{
    Size draw = DrawSize(r);
    double base = BaseUIScale(draw);
    Size largest = LargestDialog();
    int width = largest.width > ownWidth ? largest.width : ownWidth;
    int height = largest.height > ownHeight ? largest.height : ownHeight;
    double scale = draw.width / base / width;

    if (draw.height / base / height < scale)
        scale = draw.height / base / height;
    scale = floor(scale * 10 + 1e-6) / 10;
    return scale < kUIScaleMaxFloor ? kUIScaleMaxFloor : scale;
}

double MinUIScale(Resolution r)
{
    return 1.0 / BaseUIScale(DrawSize(r));
}

/* a scale between two steps (one set by hand in the file) goes to the next step */
double StepUIScale(double scale, int direction, double min, double max)
{
    double steps;

    if (direction > 0)
        steps = floor(scale / kUIScaleStep + 1e-6) + 1;
    else
        steps = ceil(scale / kUIScaleStep - 1e-6) - 1;
    scale = floor(steps * kUIScaleStep * 10 + 0.5) / 10;
    if (scale > max)
        scale = max;
    if (scale < min)
        scale = min;
    return scale;
}

void UIScaleText(double scale, char *out)
{
    if (fabs(scale * 10 - floor(scale * 10 + 0.5)) < 1e-6)
        snprintf(out, kDisplayTextMax, "%.1f", scale);
    else
        snprintf(out, kDisplayTextMax, "%.2f", scale);
}
