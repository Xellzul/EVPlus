/* profiler.cpp -- the in-flight profiler (profiler.h) */
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include "platform.h"
#include "gamepaths.h"
#include "screen.h"
#include "text.h"
#include "viewscale.h"
#include "profiler.h"

namespace Profiler {

namespace {

constexpr int kKept = 8192;                 /* frames in the report, ~3 min at 47.6 fps */
constexpr uint64_t kLongFrame = 250000;     /* us: a frame with a dialog or landing in it, left out */
constexpr uint64_t kLiveEvery = 2000000;    /* us between live lines */

const char *const kNames[kStages] = {
    "frame", "wait", "sim", "  AI", "  collisions", "sprites", "compose", "upload", "render"
};

bool on;
bool fresh;                                 /* the frame the profiler was turned on in: not counted */
uint64_t started[kStages];                  /* 0 = not begun this frame */
uint64_t spent[kStages];                    /* us this frame */
float kept[kStages][kKept];                 /* ms per frame, a ring */
int keptNext;
int keptCount;
uint32_t frames;                            /* all frames since on */
uint32_t longFrames;
uint64_t onWall;
uint64_t onCpu;
double liveSum[kStages];                    /* ms since the last live line */
uint32_t liveFrames;                        /* frames counted in liveSum */
uint32_t liveAll;                           /* all frames since the last live line */
uint64_t liveWall;
uint64_t liveCpu;

/* frame time not in any top-level stage */
double Other(const double *ms)
{
    double other = ms[Frame] - ms[Wait] - ms[Sim] - ms[Sprites] - ms[Compose] - ms[Upload] - ms[Render];

    return other > 0 ? other : 0;
}

double Percent(uint64_t part, uint64_t whole)
{
    return whole != 0 ? part * 100.0 / whole : 0;
}

/* the live line's window starts now */
void RestartLive()
{
    int s;

    for (s = 0; s < kStages; s++)
        liveSum[s] = 0;
    liveFrames = 0;
    liveAll = 0;
    liveWall = PlatMicroseconds();
    liveCpu = PlatCpuMicroseconds();
}

}

bool On()
{
    return on;
}

void Toggle()
{
    PlatVideoTimes discard;
    int s;

    on = !on;
    if (!on)
        return;
    for (s = 0; s < kStages; s++) {
        started[s] = 0;
        spent[s] = 0;
    }
    PlatVideoTakeTimes(&discard);
    fresh = true;
    keptNext = 0;
    keptCount = 0;
    frames = 0;
    longFrames = 0;
    onWall = PlatMicroseconds();
    onCpu = PlatCpuMicroseconds();
    RestartLive();
}

void Begin(Stage stage)
{
    if (on)
        started[stage] = PlatMicroseconds();
}

void End(Stage stage)
{
    if (!on || started[stage] == 0)
        return;
    spent[stage] += PlatMicroseconds() - started[stage];
    started[stage] = 0;
}

void FrameEnd()
{
    PlatVideoTimes video;
    int s;

    if (!on)
        return;
    PlatVideoTakeTimes(&video);
    spent[Upload] += video.uploadNs / 1000;
    spent[Render] += video.renderNs / 1000;
    if (!fresh) {
        frames++;
        liveAll++;
        if (spent[Frame] > kLongFrame) {
            longFrames++;
        } else {
            for (s = 0; s < kStages; s++) {
                kept[s][keptNext] = spent[s] / 1000.0f;
                liveSum[s] += spent[s] / 1000.0;
            }
            keptNext = (keptNext + 1) % kKept;
            if (keptCount < kKept)
                keptCount++;
            liveFrames++;
        }
    }
    fresh = false;
    for (s = 0; s < kStages; s++)
        spent[s] = 0;
}

bool LiveLine(std::string &line)
{
    double ms[kStages];
    uint64_t now;
    uint64_t cpu;
    int s;

    if (!on)
        return false;
    now = PlatMicroseconds();
    if (now - liveWall < kLiveEvery || liveFrames == 0)
        return false;
    cpu = PlatCpuMicroseconds();
    for (s = 0; s < kStages; s++)
        ms[s] = liveSum[s] / liveFrames;
    line = TextF("%.1f fps %.2f ms: wait %.2f sim %.2f (ai %.2f col %.2f) spr %.2f cmp %.2f up %.2f rnd %.2f "
                 "oth %.2f, CPU %.0f%%",
                 liveAll * 1e6 / (now - liveWall), ms[Frame], ms[Wait], ms[Sim], ms[AI], ms[Collisions],
                 ms[Sprites], ms[Compose], ms[Upload], ms[Render], Other(ms), Percent(cpu - liveCpu, now - liveWall));
    RestartLive();
    return true;
}

bool WriteReport(std::string &where)
{
    std::vector<float> column;
    double avg[kStages];
    double p95;
    double peak;
    uint64_t wall;
    uint64_t cpu;
    time_t now;
    char stamp[32];
    FILE *f;
    int s;
    int i;

    if (!on || keptCount == 0)
        return false;
    where = JoinPath(paths.root, "profile.txt");
    f = PlatFileOpen(where.c_str(), "a");
    if (f == NULL)
        return false;
    wall = PlatMicroseconds() - onWall;
    cpu = PlatCpuMicroseconds() - onCpu;
    now = time(NULL);
    stamp[0] = 0;
    if (localtime(&now) != NULL)
        strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "Profile %s: page %dx%d, view %dx%d (game %d wide)\n", stamp, Screen::Get().width,
            Screen::Get().height, View::width, View::height, View::gameWidth);
    fprintf(f, "%.1f s on, %u frames (%u over %d ms left out), CPU %.0f%% of one core\n", wall / 1e6, frames,
            longFrames, (int)(kLongFrame / 1000), Percent(cpu, wall));
    fprintf(f, "last %d frames, ms:     avg      p95      max   %% frame\n", keptCount);
    for (s = 0; s < kStages; s++) {
        column.assign(kept[s], kept[s] + keptCount);
        avg[s] = 0;
        for (i = 0; i < keptCount; i++)
            avg[s] += column[i];
        avg[s] /= keptCount;
        std::nth_element(column.begin(), column.begin() + keptCount * 95 / 100, column.end());
        p95 = column[keptCount * 95 / 100];
        peak = *std::max_element(column.begin(), column.end());
        fprintf(f, "  %-20s %8.3f %8.3f %8.3f %7.1f\n", kNames[s], avg[s], p95, peak,
                avg[Frame] != 0 ? avg[s] * 100 / avg[Frame] : 0.0);
    }
    fprintf(f, "  %-20s %8.3f\n\n", "other", Other(avg));
    fclose(f);
    RestartLive();         /* the "written" note stays up until the next live line */
    return true;
}

}
