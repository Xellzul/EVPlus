/* prefsdialog.cpp -- the Preferences dialog: the items of the game's DITL 4003 without the two that did nothing
   (Share Processor Time, Check For Updates) plus Positional Stereo, in three groups, the third with the display
   settings, and a Gameplay group under Sound. Built in code (NewDialogFromItems); each check box and up/down control is a table entry. */
#include "dialogs.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "sound.h"
#include "cefix.h"
#include "prefsdisplay.h"
#include "dialog.h"

namespace {

constexpr short kWidth = 500;
constexpr short kHeight = 335;
constexpr short kMaxItems = 48;
constexpr short kMusicVolStrList = 0x88;
constexpr short kMaxMusicVol = 8;
constexpr short kMaxGamma = 6;
const char *const kGammaNames[kMaxGamma + 1] = {
    "Black Hole", "Darker", "Dark", "Normal", "Bright", "Brighter", "Burn Out Your Retinas"
};
constexpr short kBeepChannel = 1;
constexpr short kUpArrowPict = 134;
constexpr short kDownArrowPict = 135;
constexpr short kPlainStrIdx = 0x118;
constexpr short kRestartStrIdx = 0x10;       /* "(Changes will take effect next time you run Nova)" */

/* the grid of DITL 4003: columns 160 wide, rows 22 apart from the first below the headings */
constexpr short kColGraphics = 11;
constexpr short kColSound = 171;
constexpr short kColDisplay = 331;
constexpr short kColWidth = 160;
constexpr short kHeadingTop = 33;
constexpr short kFirstRow = 55;
constexpr short kRow = 22;

const char *const kFilterNames[kScaleFilterCount] = { "Nearest", "Linear", "Pixel art" };

short gTitleItem;                           /* the dialog's user items, for PrefsDialogUpdate */
short gNoticeItem;

/* the dialog's items, numbered as they are added */
class ItemList {
public:
    short add(unsigned char kind, short top, short left, short bottom, short right, const char *text = NULL,
              short resID = 0)
    {
        items[count] = { kind, top, left, bottom, right, text, resID };
        return ++count;
    }
    const DialogItemSpec *data() const { return items.data(); }
    short size() const { return count; }

private:
    std::array<DialogItemSpec, kMaxItems> items;
    short count = 0;
};

/* an up/down control's items: its value, the line below it (or 0), and the two arrows */
struct StepperItems {
    short value;
    short extra;
    short down;
    short up;
};

/* One group: a heading, then its items top down, placed as DITL 4003 placed its own. A check box takes a
   row; an up/down control two (its label, then the arrows beside its value), three with a line below. */
class Column {
public:
    Column(ItemList &items, short left, const char *heading, short headingTop = kHeadingTop)
        : items(items), left(left), top(headingTop + kFirstRow - kHeadingTop)
    {
        items.add(kDitlStaticText, headingTop, left, headingTop + 16, left + kColWidth, heading);
    }

    short check(const char *text)
    {
        short item = items.add(kDitlCheckBox, top, left, top + 18, left + kColWidth, text);
        top += kRow;
        return item;
    }

    StepperItems stepper(const char *label, bool extraLine = false)
    {
        StepperItems s;

        items.add(kDitlStaticText, top, left, top + 16, left + 106, label);
        s.value = items.add(kDitlStaticText, top + 19, left + 18, top + 35, left + 140, "");
        s.extra = extraLine ? items.add(kDitlStaticText, top + 35, left + 18, top + 51, left + kColWidth, "") : 0;
        s.down = items.add(kDitlPicture, top + 27, left + 1, top + 36, left + 12, NULL, kDownArrowPict);
        s.up = items.add(kDitlPicture, top + 18, left + 1, top + 27, left + 12, NULL, kUpArrowPict);
        top += extraLine ? 3 * kRow : 2 * kRow;
        return s;
    }

private:
    ItemList &items;
    short left;
    short top;
};

/* a check box and the setting it shows (inverted: the setting says "no ...") */
struct CheckBox {
    short item;
    unsigned char *setting;
    bool inverted;
};

unsigned char *Flag(Boolean &setting) { return (unsigned char *)&setting; }
unsigned char *Flag(unsigned char &setting) { return &setting; }

class PrefsDialog;

/* an up/down control: what a click on an arrow does, and what it shows */
struct Stepper {
    StepperItems items;
    void (*step)(PrefsDialog &dialog, int direction);
    void (*show)(const PrefsDialog &dialog, char *value, char *extra);
};

/* the STR# entry for a value, or ??? past its end */
void ListText(short list, short value, short max, char *out)
{
    Str255 text;

    if (value < 0 || value > max) {
        snprintf(out, kDisplayTextMax, "???");
        return;
    }
    GetIndString(text, list, value + 1);
    snprintf(out, kDisplayTextMax, "%.*s", text[0], (const char *)&text[1]);
}

/* the settings whose change needs a restart once a game is loaded (sprites are loaded with them) */
struct SpriteSettings {
    unsigned char shipAnimations;
    unsigned char engineGlows;
    unsigned char runningLights;
    unsigned char weaponGlows;

    static SpriteSettings Now()
    {
        return { settings.shipAnimations, settings.engineGlows, settings.runningLights, settings.weaponGlows };
    }
    bool operator!=(const SpriteSettings &o) const
    {
        return shipAnimations != o.shipAnimations || engineGlows != o.engineGlows
               || runningLights != o.runningLights || weaponGlows != o.weaponGlows;
    }
};

class PrefsDialog {
public:
    PrefsDialog();
    ~PrefsDialog();
    void run();

    ResolutionList resolutions;

private:
    void show();
    bool handle(short item);                /* true: the dialog is done */
    bool needsRestart() const;
    void setCheck(short item, bool on);
    void setText(short item, const char *text);

    ItemList items;
    Dialog dialog;
    std::array<CheckBox, 16> checks;
    int checkCount = 0;
    std::array<Stepper, 6> steppers;
    int stepperCount = 0;
    short doneItem;
    short keysItem;
    short windowedItem;
    SpriteSettings savedSprites;
};

PrefsDialog::PrefsDialog()
{
    doneItem = items.add(kDitlButton, 302, 389, 322, 459, "OK");
    gTitleItem = items.add(kDitlUserItem, 5, 12, 28, 489);
    gNoticeItem = items.add(kDitlUserItem, 277, 69, 294, 431);
    keysItem = items.add(kDitlButton, 302, 49, 322, 184, "Key Settings");

    Column graphics(items, kColGraphics, "Graphics");
    Column sound(items, kColSound, "Sound");
    Column display(items, kColDisplay, "Display");
    Column gameplay(items, kColSound, "Gameplay", kFirstRow + 6 * kRow);   /* below Sound */

    auto check = [this](short item, unsigned char *setting, bool inverted = false) {
        checks[checkCount++] = { item, setting, inverted };
    };
    auto stepper = [this](StepperItems s, void (*step)(PrefsDialog &, int),
                          void (*show)(const PrefsDialog &, char *, char *)) {
        steppers[stepperCount++] = { s, step, show };
    };

    check(graphics.check("Ship Animations"), Flag(settings.shipAnimations));
    check(graphics.check("Engine Glows"), Flag(settings.engineGlows));
    check(graphics.check("Running Lights"), Flag(settings.runningLights));
    check(graphics.check("Weapon Effects"), Flag(settings.weaponGlows));
    check(graphics.check("Smoke Trails"), Flag(settings.noDebris), true);
    check(graphics.check("Parallax Starfield"), Flag(settings.parallaxStars));
    check(graphics.check("Hyperspace Effects"), Flag(settings.noHyperGamma), true);
    stepper(graphics.stepper("Brightness:"),
        [](PrefsDialog &, int direction) {
            short gamma = settings.gamma + direction;
            settings.gamma = gamma < 0 ? 0 : gamma > kMaxGamma ? kMaxGamma : gamma;
            ApplyBrightness();
        },
        [](const PrefsDialog &, char *value, char *) {
            snprintf(value, kDisplayTextMax, "%s",
                     settings.gamma >= 0 && settings.gamma <= kMaxGamma ? kGammaNames[settings.gamma] : "???");
        });

    check(sound.check("Intro Music"), Flag(settings.introMusic));
    check(sound.check("Ambient Sounds"), Flag(settings.ambientSounds));
    check(sound.check("QuickTime Movies"), Flag(settings.noMovies), true);
    check(sound.check("Positional Stereo"), Flag(settings.stereoPan));
    stepper(sound.stepper("Sound Volume:"),
        [](PrefsDialog &, int direction) {
            short volume = settings.volume + direction;
            settings.volume = volume < 0 ? 0 : volume > kMaxMusicVol ? kMaxMusicVol : volume;
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        },
        [](const PrefsDialog &, char *value, char *) {
            ListText(kMusicVolStrList, settings.volume, kMaxMusicVol, value);
        });

    check(gameplay.check("Asteroid Scaling"), Flag(settings.asteroidScaling));
    check(gameplay.check("Map Shows Prices"), Flag(settings.mapPrices));
    check(gameplay.check("Map Shows Disasters"), Flag(settings.mapDisasters));

    windowedItem = display.check("Run in a window");
    check(display.check("Whole-pixel scaling"), Flag(settings.wholePixelScaling));
    stepper(display.stepper("Resolution:", true),
        [](PrefsDialog &dialog, int direction) {
            Resolution r = dialog.resolutions.step({ settings.gameWidth, settings.gameHeight }, direction);
            settings.gameWidth = r.width;
            settings.gameHeight = r.height;
            double min = MinUIScale(r);
            double max = MaxUIScale(r, kWidth, kHeight);
            if (settings.uiScale < min)
                settings.uiScale = min;
            if (settings.uiScale > max)
                settings.uiScale = max;
        },
        [](const PrefsDialog &dialog, char *value, char *extra) {
            Resolution r = { settings.gameWidth, settings.gameHeight };
            dialog.resolutions.sizeText(r, value);
            dialog.resolutions.aspectText(r, extra);
        });
    stepper(display.stepper("Filter:"),
        [](PrefsDialog &, int direction) {
            short filter = settings.scaleFilter + direction;
            if (filter >= 0 && filter < kScaleFilterCount)
                settings.scaleFilter = filter;
            ApplyDisplayScaling();
        },
        [](const PrefsDialog &, char *value, char *) {
            snprintf(value, kDisplayTextMax, "%s", kFilterNames[settings.scaleFilter]);
        });
    stepper(display.stepper("UI scale:"),
        [](PrefsDialog &, int direction) {
            Resolution r = { settings.gameWidth, settings.gameHeight };
            double max = MaxUIScale(r, kWidth, kHeight);
            if (direction < 0 || settings.uiScale < max)
                settings.uiScale = StepUIScale(settings.uiScale, direction, MinUIScale(r), max);
        },
        [](const PrefsDialog &, char *value, char *) {
            UIScaleText(settings.uiScale, value);
        });

    savedSprites = SpriteSettings::Now();
    dialog.Open(kWidth, kHeight, items.data(), items.size(), PrefsDialogUpdate, NULL, &prefsDialog);
    SetDialogGWorld(prefsDialog);
    RedrawDialog(prefsDialog);
}

PrefsDialog::~PrefsDialog()
{
    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
    if (settings.introMusic == 0)
        StopIntroMusic(0);
}

void PrefsDialog::setCheck(short item, bool on)
{
    SetControlValue((ControlPtr)dialog.Item(item), on);
}

void PrefsDialog::setText(short item, const char *text)
{
    Handle h = dialog.Item(item);

    if (h != NULL)
        SetControlText((ControlPtr)h, text);
}

/* every item as the settings are now */
void PrefsDialog::show()
{
    char value[kDisplayTextMax];
    char extra[kDisplayTextMax];

    SetDialogGWorld(prefsDialog);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    for (int i = 0; i < checkCount; i++)
        setCheck(checks[i].item, (*checks[i].setting != 0) != checks[i].inverted);
    ceRunInAWindow = CE_IsWindowed();
    setCheck(windowedItem, ceRunInAWindow);
    for (int i = 0; i < stepperCount; i++) {
        extra[0] = 0;
        steppers[i].show(*this, value, extra);
        setText(steppers[i].items.value, value);
        if (steppers[i].items.extra != 0)
            setText(steppers[i].items.extra, extra);
    }
    majorPrefsChange = needsRestart();
}

bool PrefsDialog::needsRestart() const
{
    return settings.windowedPending != settings.windowed
           || (gameLoaded != 0 && SpriteSettings::Now() != savedSprites)
           || settings.gameWidth != startGameWidth || settings.gameHeight != startGameHeight;
}

bool PrefsDialog::handle(short item)
{
    if (item == doneItem) {
        SavePrefs();
        return true;
    }
    if (item == keysItem) {
        DoKeyDialog();
        if (gameLoaded != 0 && gPause != 0) {
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        }
        return false;
    }
    if (item == windowedItem) {
        ceRunInAWindow = (ceRunInAWindow == 0);
        CE_ToggleFullscreen();
        return false;
    }
    for (int i = 0; i < checkCount; i++) {
        if (item == checks[i].item) {
            *checks[i].setting = (*checks[i].setting == 0);
            if (checks[i].setting == Flag(settings.wholePixelScaling))
                ApplyDisplayScaling();
            return false;
        }
    }
    for (int i = 0; i < stepperCount; i++) {
        if (item == steppers[i].items.up || item == steppers[i].items.down) {
            steppers[i].step(*this, item == steppers[i].items.up ? 1 : -1);
            return false;
        }
    }
    return false;
}

void PrefsDialog::run()
{
    short item;
    bool done = false;

    while (!done) {
        show();
        item = dialog.Modal();
        done = handle(item);
        RedrawDialog(prefsDialog);
    }
}

} // namespace

// FUNCTION: EVNOVA 0x004884f0
void PrefsDialogUpdate(void)
{
    Rect box;

    SetDialogGWorld(prefsDialog);
    GetDialogItem(prefsDialog, gTitleItem, NULL, NULL, &box);
    RGBForeColor(&rgbBlack);
    EraseRect(&box);
    MoveTo(box.left, (box.bottom - 10));
    if (gameLoaded == 0 && gWind != NULL) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
    } else {
        TextFont(0);
        TextSize(12);
        DrawIndString(kGameStrList, kPlainStrIdx);
    }
    PenSize(1, 1);
    MoveTo(box.left, box.bottom);
    LineTo(box.right, box.bottom);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (majorPrefsChange != 0 && gWind != NULL) {
        GetDialogItem(prefsDialog, gNoticeItem, NULL, NULL, &box);
        EraseRect(&box);
        MoveTo(box.left, (box.bottom - 5));
        DrawIndString(kAlertStrList, kRestartStrIdx);
    }
}

// FUNCTION: EVNOVA 0x00488650
// MAC: _DoPrefsDialog i386 0x00016459 ppc 0x0001a1ec
void DoPrefsDialog(void)
{
    PrefsDialog dialog;

    dialog.run();
}
