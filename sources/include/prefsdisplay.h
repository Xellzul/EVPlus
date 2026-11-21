/* prefsdisplay.h -- the Display group of Preferences: the resolutions it offers, their names, the UI scale
   range. */
#pragma once

#include <array>

constexpr int kDisplayTextMax = 40;

/* a resolution as the preferences keep it: 0 x 0 = the desktop's */
struct Resolution {
    short width;
    short height;

    bool operator==(const Resolution &other) const
    {
        return width == other.width && height == other.height;
    }
};

struct PixelSize {
    int width;
    int height;
};

/* the size a resolution stands for (0 x 0: the desktop's) */
PixelSize GameSize(Resolution r);
/* the size the game at a resolution is drawn at: the largest of its shape the desktop holds, never smaller */
PixelSize DrawSize(Resolution r);
/* the UI scale at which a screen of that size shows the interface as 1024 x 768 does, fitted; never below 1 */
double BaseUIScale(PixelSize draw);

/* The resolutions offered, for the desktop as it is: each aspect ratio no wider than the desktop (4:3, 16:9,
   the desktop's own) at 600 lines, 768 lines (the original game's 1024 x 768) and the desktop's height. */
class ResolutionList {
public:
    ResolutionList();

    int size() const { return count; }
    const Resolution &operator[](int i) const { return list[i]; }
    int find(Resolution r) const;                       /* -1: not offered (set by hand in the file) */
    Resolution step(Resolution r, int direction) const; /* the next one up (+1) or down (-1) */
    void sizeText(Resolution r, char *out) const;       /* "1024 x 768" */
    void aspectText(Resolution r, char *out) const;     /* "4:3 (original)" */

private:
    void add(int width, int height);

    std::array<Resolution, 12> list;
    int count;
};

/* the largest UI scale at which every dialog still fits the resolution (one of ownWidth x ownHeight too) */
double MaxUIScale(Resolution r, short ownWidth, short ownHeight);
/* the smallest UI scale: one screen pixel per pixel of the interface */
double MinUIScale(Resolution r);
/* the UI scale one step (0.1) up or down, kept between min and max */
double StepUIScale(double scale, int direction, double min, double max);
void UIScaleText(double scale, char *out);

