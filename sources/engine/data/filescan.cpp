/* filescan.cpp -- count the files of one extension in a directory, logging each (win 0x0046f690). Notes: Research/notes/filescan.md */

#include "burger.h"
#include "crashlog.h"
#include "gamepaths.h"
#include "platform.h"

/* a folder scan: the files (not folders, not empty) whose extension is `ext' */
struct PluginScan {
    const char *folder;
    const char *ext;
    FILE *log;
    Word count;
};

static bool Wanted(const PluginScan *scan, const char *name, int isFolder, uint64_t size)
{
    return !isFolder && size != 0 && stricmp(GetFileExtension(name), scan->ext) == 0;
}

static void OpenFound(const char *name, int isFolder, uint64_t size, void *ctx)
{
    PluginScan *scan = (PluginScan *)ctx;

    if (!Wanted(scan, name, isFolder, size))
        return;
    crashlog_print("  ");
    crashlog_print(name);
    if (scan->log != NULL) {
        fprintf(scan->log, "  %s", name);
        fflush(scan->log);
    }
    if (EVOpenResourceFile(JoinPath(scan->folder, name).c_str()) == -1) {
        crashlog_print(" - failed or has no resource fork");
        if (scan->log != NULL)
            fprintf(scan->log, " - failed or has no resource fork");
    }
    crashlog_print("\n");
    if (scan->log != NULL) {
        fprintf(scan->log, "\n");
        fflush(scan->log);
    }
}

static void CountFound(const char *name, int isFolder, uint64_t size, void *ctx)
{
    PluginScan *scan = (PluginScan *)ctx;

    if (!Wanted(scan, name, isFolder, size))
        return;
    if (scan->log != NULL) {
        fprintf(scan->log, "  %s\n", name);
        fflush(scan->log);
    }
    scan->count++;
}

// FUNCTION: EVNOVA 0x0046f500
short OpenEVPlugins(const char *path, const char *ext, FILE *log)
{
    PluginScan scan = { path, ext, log, 0 };

    if (ext != NULL && *ext == '.')
        scan.ext++;
    if (!PlatListFolder(path, OpenFound, &scan))
        return -1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046f690
short CountEVPlugins(const char *path, const char *ext, FILE *log)
{
    PluginScan scan = { path, ext, log, 0 };

    if (ext != NULL && *ext == '.')
        scan.ext++;
    if (!PlatListFolder(path, CountFound, &scan))
        return 0;
    return scan.count;
}
