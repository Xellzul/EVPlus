# Escape Velocity Plus

**EV Nova 1.0.10 for Windows x86 Community Edition** (Not complete, not tested)


## Build (clang)

You need: llvm-mingw (default `/opt/llvm-mingw`), CMake, Ninja, and `sh`, `make`, `xz` for the first configure,
which downloads FFmpeg 9.0.2 into `build/ffmpeg` Offline: pass `-DFFMPEG_TARBALL=/path/to/ffmpeg-9.0.2.tar.xz` instead.
The first configure also downloads SDL 3.4.20 and builds it as a static library into `build/sdl3`. Offline: pass
`-DSDL3_TARBALL=/path/to/SDL3-3.4.20.tar.gz`.

`cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake`

`cmake --build build`

Outputs `build/EV Nova CE.exe` + DLLs. Options: `-DLLVM_MINGW=/path/to/llvm-mingw`,
`-DFFMPEG_ROOT=/path/to/ffmpeg`, `-DSDL3_ROOT=/path/to/sdl3`

## Credits:
andrews05 for CE patches - https://github.com/andrews05/EV-Nova-CE 