/* gamepaths.h -- where the game's files are: UTF-8 paths in the system's own form (platform.h) */
#pragma once
#include <string>

struct GamePaths {
    std::string exe;      /* the program's folder; "" = current */
    std::string prefs;    /* the preferences folder; "" = none, the preferences lie beside the program */
    std::string root;     /* the game's folder */
    std::string data;     /* root/Nova Files */
    std::string plugins;  /* root/Nova Plug-Ins, or root/Plug-Ins when OpenDataFiles finds only that */
    std::string pilots;   /* root/<STR# 130, 3>, set by InitDataFiles */
};
extern GamePaths paths;

/* the program's and the preferences folder (made when missing), the game's folder beside the program */
void InitPaths();
/* the game's folder, with the default data and plug-ins folders in it */
void SetGameRoot(const std::string &root);
/* `name' in `folder'; "" folder = `name' alone */
std::string JoinPath(const std::string &folder, const char *name);
/* the last part of a path: past the last separator */
const char *PathTail(const char *path);
/* the pilot file of a pilot name (Pascal string, Western text); New Pilot names hold no ':' '/' '\' */
std::string PilotFilePath(const unsigned char *pname);
