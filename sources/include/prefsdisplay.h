/* prefsdisplay.h -- the Display group of Preferences: the resolutions it offers, their names, the UI scale
   range. */
#ifndef PREFSDISPLAY_H
#define PREFSDISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#define kResolutionChoiceMax    12
#define kDisplayTextMax         40

/* a resolution as the preferences keep it: 0 x 0 = the desktop's */
typedef struct ResolutionChoice {
    short width;
    short height;
} ResolutionChoice;

short ResolutionChoices(ResolutionChoice *out);
short FindResolutionChoice(const ResolutionChoice *list, short count, short width, short height);
void ResolutionSizeText(short width, short height, char *out);
void ResolutionAspectText(const ResolutionChoice *list, short count, short width, short height, char *out);
double MaxUIScale(short width, short height, short ownWidth, short ownHeight);
void UIScaleText(double scale, char *out);

#ifdef __cplusplus
}
#endif

#endif /* PREFSDISPLAY_H */
