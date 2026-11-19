# Escape Velocity Plus

**EV Nova 1.0.10 for Windows x86 Community Edition** (Not complete, not tested)


## Build (clang)

You need: llvm-mingw (default `/opt/llvm-mingw`), CMake, Ninja, and `sh`, `make`, `xz` for the first configure,
which downloads FFmpeg 9.0.2 into `build/ffmpeg` Offline: pass `-DFFMPEG_TARBALL=/path/to/ffmpeg-9.0.2.tar.xz` instead.
The first configure also downloads SDL 3.4.20 and builds it as a static library into `build/sdl3`. Offline: pass
`-DSDL3_TARBALL=/path/to/SDL3-3.4.20.tar.gz`.
The same goes for FreeType 2.14.3 (draws the text), built into `build/freetype`. Offline: pass
`-DFREETYPE_TARBALL=/path/to/freetype-2.14.3.tar.xz`.
And yyjson 0.13.0 (reads and writes the preferences), unpacked into `build/yyjson` and built with the game. Offline:
pass `-DYYJSON_TARBALL=/path/to/yyjson-0.13.0.tar.gz`.

`cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake`

`cmake --build build`

Outputs `build/evnova64.exe` + DLLs. Options: `-DLLVM_MINGW=/path/to/llvm-mingw`,
`-DFFMPEG_ROOT=/path/to/ffmpeg`, `-DSDL3_ROOT=/path/to/sdl3`, `-DFREETYPE_ROOT=/path/to/freetype`

## Credits:
andrews05 for CE patches - https://github.com/andrews05/EV-Nova-CE

Portions of this software are copyright © 2026 The FreeType Project (https://freetype.org). All rights reserved.

yyjson (https://github.com/ibireme/yyjson): Copyright (c) 2020 YaoYuan <ibireme@gmail.com>, MIT License.
