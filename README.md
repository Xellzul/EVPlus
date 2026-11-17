# EvNovaDecompile

**EV Nova 1.0.10 for Windows x86**

**EV Nova 1.0.10 for Windows x86 Community Edition** (Not complete, not tested)


## Build (clang)

You need: llvm-mingw (default `/opt/llvm-mingw`), CMake, Ninja, and `unar` (package `unarchiver`) to unpack
`input-files/QT6SDK_Win.hqx` (see input-files/README.md) for QuickTime's `qtmlClient.lib` on the first configure.

`cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake`

`cmake --build build`

Outputs `build/EV Nova.exe` and `build/EV Nova CE.exe`. Options: `-DLLVM_MINGW=/path/to/llvm-mingw`,
`-DQTML_CLIENT_LIB=/path/to/qtmlClient.lib` (skips unpacking the SDK).

## Credits:
andrews05 for CE patches - https://github.com/andrews05/EV-Nova-CE 