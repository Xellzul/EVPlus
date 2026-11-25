/* profiler.h -- the in-flight profiler: time per frame stage, shown live in the comm line (Shift + the
   show-framerate key) and written to profile.txt in the game folder (Ctrl + that key). Measures only while on. */
#pragma once

#include <string>

namespace Profiler {

enum Stage {
    Frame,          /* the whole PlayGame loop pass */
    Wait,           /* HandleTimeAdjustment: the frame limiter */
    Sim,            /* DoPlayGameWork */
    AI,             /* inside Sim */
    Collisions,     /* inside Sim */
    Sprites,        /* sprite world process + animate, PostAnimateHandler */
    Compose,        /* Screen::ComposeView: view enlarged onto the page, overlays */
    Upload,         /* platform: the page into the texture */
    Render,         /* platform: the texture to the window + present */
    kStages
};

void Toggle();
bool On();
void Begin(Stage stage);
void End(Stage stage);
void FrameEnd();                        /* after each PlayGame loop pass */
bool LiveLine(std::string &line);       /* true every 2 s with the line to show */
bool WriteReport(std::string &where);   /* appends to profile.txt; false = not on or not written */

}
