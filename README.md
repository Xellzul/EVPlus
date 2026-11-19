# Escape Velocity Plus

## Build (clang)

You need CMake, Ninja, and `sh`, `make`, `xz` for the first configure. Windows (cross-built): llvm-mingw (default
`/opt/llvm-mingw`). Linux: clang and fontconfig. macOS (Apple silicon, built on a Mac): the Xcode command line tools.

`cmake --preset windows` then `cmake --build build/windows` -> `build/windows/evnova64.exe` + FFmpeg DLLs

`cmake --preset linux` then `cmake --build build/linux` -> `build/linux/evnova64` + FFmpeg libraries

`cmake --preset macos` then `cmake --build build/macos` -> `build/macos/evnova64` + FFmpeg libraries

The first configure of each downloads the pinned releases into `build/downloads` (FFmpeg 9.0.2, SDL 3.4.20,
FreeType 2.14.3 for the text, yyjson 0.13.0 for the preferences) and builds them into that build's folder.
Offline: pass `-DFFMPEG_TARBALL=/path/to/ffmpeg-9.0.2.tar.xz`, `-DSDL3_TARBALL=/path/to/SDL3-3.4.20.tar.gz`,
`-DFREETYPE_TARBALL=/path/to/freetype-2.14.3.tar.xz`, `-DYYJSON_TARBALL=/path/to/yyjson-0.13.0.tar.gz`.
Other options: `-DLLVM_MINGW=/path/to/llvm-mingw`, `-DFFMPEG_ROOT=/path/to/ffmpeg`, `-DSDL3_ROOT=/path/to/sdl3`,
`-DFREETYPE_ROOT=/path/to/freetype`.

Run the program from the game's folder (the one with `Nova Files`), its libraries beside it.

## Credits:
andrews05 for CE patches - https://github.com/andrews05/EV-Nova-CE

Portions of this software are copyright © 2026 The FreeType Project (https://freetype.org). All rights reserved.

yyjson (https://github.com/ibireme/yyjson): Copyright (c) 2020 YaoYuan <ibireme@gmail.com>, MIT License.
