# EV Nova CE (CMakePresets.json)
#
#   cmake --preset windows && cmake --build build/windows    -> build/windows/evnova64.exe (llvm-mingw)
#   cmake --preset linux && cmake --build build/linux        -> build/linux/evnova64 (clang)
#   cmake --preset macos && cmake --build build/macos        -> build/macos/evnova64 (Apple silicon, on a Mac)
#
# Needs: llvm-mingw (default /opt/llvm-mingw) or clang; sh, make and xz once, to build FFmpeg (downloaded, or
# -DFFMPEG_TARBALL=/path offline; or pass -DFFMPEG_ROOT=/path). The FFmpeg libraries and its licence are copied next
# to the program. SDL3 is built once too, as a static library (downloaded, or -DSDL3_TARBALL=/path offline; or
# -DSDL3_ROOT=/path). Linux also needs fontconfig (the system's); macOS the Xcode command line tools.
cmake_minimum_required(VERSION 3.24)
project(EVNova C CXX)
set(CMAKE_CXX_STANDARD 17)
if(WIN32)
  enable_language(RC)
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "the game needs clang: pass -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++")
endif()

set(SRC ${CMAKE_SOURCE_DIR}/sources)

# A pinned release tarball: USER_TARBALL when given, else NAME downloaded from URL into DOWNLOAD_DIR (once, for
# every build). Either is checked against SHA256. OUT gets its path; HINT is the way out named when the download
# fails.
set(DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/build/downloads CACHE PATH "where the pinned library releases are downloaded to")
function(pinned_tarball OUT URL NAME SHA256 USER_TARBALL HINT)
  set(tar ${USER_TARBALL})
  if(NOT tar)
    set(tar ${DOWNLOAD_DIR}/${NAME})
    if(NOT EXISTS ${tar})
      message(STATUS "downloading ${URL}")
      file(DOWNLOAD ${URL} ${tar}.part EXPECTED_HASH SHA256=${SHA256} STATUS dl)
      list(GET dl 0 rc)
      if(rc)
        file(REMOVE ${tar}.part)
        message(FATAL_ERROR "${NAME} download failed (${dl}); ${HINT}")
      endif()
      file(RENAME ${tar}.part ${tar})
    endif()
  endif()
  file(SHA256 ${tar} sum)
  if(NOT sum STREQUAL SHA256)
    message(FATAL_ERROR "${tar}: SHA256 ${sum}, expected ${SHA256}")
  endif()
  set(${OUT} ${tar} PARENT_SCOPE)
endfunction()

# -- FFmpeg
# Built once by cmake/build-ffmpeg.sh into <build>/ffmpeg from the pinned release: downloaded into build/downloads, or
# -DFFMPEG_TARBALL=/path/to/ffmpeg-<ver>.tar.xz (offline); either is checked against the SHA256. Or take an
# install with -DFFMPEG_ROOT=/path (include/, and lib/*.dll.a + bin/*.dll on Windows, lib/*.dylib on macOS,
# lib/*.so elsewhere).
set(FFMPEG_VERSION 9.0.2)
set(FFMPEG_SHA256 8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e)
set(FFMPEG_TARBALL "" CACHE FILEPATH "ffmpeg-${FFMPEG_VERSION}.tar.xz for offline builds (empty: download it)")
set(FFMPEG_ROOT "" CACHE PATH "FFmpeg install (empty: build FFmpeg ${FFMPEG_VERSION} with cmake/build-ffmpeg.sh)")
if(WIN32)
  set(_ff_lib .dll.a)
  set(_ff_target ${LLVM_MINGW})
elseif(APPLE)
  set(_ff_lib .dylib)
  set(_ff_target native)
else()
  set(_ff_lib .so)
  set(_ff_target native)
endif()
if(NOT FFMPEG_ROOT)
  set(FFMPEG_ROOT ${CMAKE_BINARY_DIR}/ffmpeg)
  if(NOT EXISTS ${FFMPEG_ROOT}/lib/libavformat${_ff_lib})
    set(_ff_name ffmpeg-${FFMPEG_VERSION}.tar.xz)
    pinned_tarball(_ff_tar https://ffmpeg.org/releases/${_ff_name} ${_ff_name} ${FFMPEG_SHA256} "${FFMPEG_TARBALL}"
                   "pass -DFFMPEG_TARBALL=/path/to/${_ff_name} or -DFFMPEG_ROOT=/path/to/ffmpeg")
    message(STATUS "building FFmpeg from ${_ff_tar} (once, a few minutes)")
    execute_process(COMMAND ${CMAKE_COMMAND} -E env MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                    "FFMPEG_ARCH=${CMAKE_OSX_ARCHITECTURES}" sh ${CMAKE_SOURCE_DIR}/cmake/build-ffmpeg.sh ${_ff_tar} ${FFMPEG_ROOT} ${_ff_target}
                    RESULT_VARIABLE rc OUTPUT_FILE ${CMAKE_BINARY_DIR}/ffmpeg-build.log
                    ERROR_FILE ${CMAKE_BINARY_DIR}/ffmpeg-build.log)
    if(rc OR NOT EXISTS ${FFMPEG_ROOT}/lib/libavformat${_ff_lib})
      message(FATAL_ERROR "FFmpeg build failed, see ${CMAKE_BINARY_DIR}/ffmpeg-build.log")
    endif()
  endif()
endif()
set(FFMPEG_LIBS avformat avcodec swscale swresample avutil)
set(FFMPEG_IMPORT "")
set(FFMPEG_DLLS "")
foreach(l ${FFMPEG_LIBS})
  list(APPEND FFMPEG_IMPORT ${FFMPEG_ROOT}/lib/lib${l}${_ff_lib})
  if(NOT WIN32)
    # the library the program is linked against, under the name it asks for: its file's version cut to the
    # major one, "lib<name>.so.<major>" (Linux) or "lib<name>.<major>.dylib" (macOS)
    get_filename_component(_ff_real ${FFMPEG_ROOT}/lib/lib${l}${_ff_lib} REALPATH)
    if(APPLE)
      string(REGEX REPLACE "^(.*/lib${l}\\.[0-9]+)\\..*dylib$" "\\1.dylib" _ff_so "${_ff_real}")
    else()
      string(REGEX REPLACE "^(.*/lib${l}\\.so\\.[0-9]+)(\\..*)?$" "\\1" _ff_so "${_ff_real}")
    endif()
    get_filename_component(_ff_so_name ${_ff_so} NAME)
    set(_ff_so ${FFMPEG_ROOT}/lib/${_ff_so_name})
    if(NOT EXISTS ${_ff_so})
      message(FATAL_ERROR "FFmpeg: no ${_ff_so} beside ${_ff_real}")
    endif()
    list(APPEND FFMPEG_DLLS ${_ff_so})
  endif()
endforeach()
if(WIN32)
  file(GLOB FFMPEG_DLLS ${FFMPEG_ROOT}/bin/*.dll)
endif()
file(GLOB FFMPEG_LICENSE ${FFMPEG_ROOT}/share/licenses/FFmpeg-COPYING.LGPLv2.1.txt)

# -- SDL3
# Built once into <build>/sdl3 from the pinned release, as a static library, with this build's generator and
# toolchain: downloaded into build/downloads, or -DSDL3_TARBALL=/path/to/SDL3-<ver>.tar.gz (offline); either is checked
# against the SHA256. Or take an install with -DSDL3_ROOT=/path (one that has the static library).
set(SDL3_VERSION 3.4.20)
set(SDL3_SHA256 f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f)
set(SDL3_TARBALL "" CACHE FILEPATH "SDL3-${SDL3_VERSION}.tar.gz for offline builds (empty: download it)")
set(SDL3_ROOT "" CACHE PATH "SDL3 install (empty: build SDL ${SDL3_VERSION} from its release tarball)")
if(NOT SDL3_ROOT)
  set(SDL3_ROOT ${CMAKE_BINARY_DIR}/sdl3)
  if(NOT EXISTS ${SDL3_ROOT}/lib/cmake/SDL3/SDL3Config.cmake)
    set(_sdl_name SDL3-${SDL3_VERSION}.tar.gz)
    pinned_tarball(_sdl_tar
                   https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/${_sdl_name}
                   ${_sdl_name} ${SDL3_SHA256} "${SDL3_TARBALL}"
                   "pass -DSDL3_TARBALL=/path/to/${_sdl_name} or -DSDL3_ROOT=/path/to/sdl3")
    message(STATUS "building SDL from ${_sdl_tar} (once, a minute or two)")
    set(_sdl_work ${SDL3_ROOT}.work)
    set(_sdl_log ${CMAKE_BINARY_DIR}/sdl3-build.log)
    file(REMOVE_RECURSE ${_sdl_work})
    file(ARCHIVE_EXTRACT INPUT ${_sdl_tar} DESTINATION ${_sdl_work})
    file(WRITE ${_sdl_log} "")
    macro(_sdl_run)
      execute_process(COMMAND ${CMAKE_COMMAND} ${ARGN} RESULT_VARIABLE _sdl_rc
                      OUTPUT_VARIABLE _sdl_out ERROR_VARIABLE _sdl_out)
      file(APPEND ${_sdl_log} "${_sdl_out}")
      if(_sdl_rc)
        message(FATAL_ERROR "SDL build failed, see ${_sdl_log}")
      endif()
    endmacro()
    set(_sdl_args -G ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${SDL3_ROOT}
        -DCMAKE_INSTALL_LIBDIR=lib -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF
        -DSDL_EXAMPLES=OFF -DSDL_INSTALL_DOCS=OFF)
    if(CMAKE_TOOLCHAIN_FILE)
      # a relative toolchain path is taken from the build directory, then from the source directory, as CMake does
      get_filename_component(_sdl_tc ${CMAKE_TOOLCHAIN_FILE} ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR})
      if(NOT EXISTS ${_sdl_tc})
        get_filename_component(_sdl_tc ${CMAKE_TOOLCHAIN_FILE} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
      endif()
      list(APPEND _sdl_args -DCMAKE_TOOLCHAIN_FILE=${_sdl_tc})
    else()
      list(APPEND _sdl_args -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
    endif()
    if(LLVM_MINGW)
      list(APPEND _sdl_args -DLLVM_MINGW=${LLVM_MINGW})
    endif()
    if(CMAKE_MAKE_PROGRAM)
      list(APPEND _sdl_args -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
    endif()
    if(APPLE)
      list(APPEND _sdl_args "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
           -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()
    _sdl_run(-S ${_sdl_work}/SDL3-${SDL3_VERSION} -B ${_sdl_work}/build ${_sdl_args})
    _sdl_run(--build ${_sdl_work}/build)
    _sdl_run(--install ${_sdl_work}/build)
    file(REMOVE_RECURSE ${_sdl_work})
  endif()
endif()
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-static PATHS ${SDL3_ROOT} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

# -- FreeType (draws the game's text)
# Built once into <build>/freetype from the pinned release, as a static library without its optional
# dependencies (no compressed or PNG fonts, no HarfBuzz), the same way as SDL: downloaded into build/downloads, or
# -DFREETYPE_TARBALL=/path/to/freetype-<ver>.tar.xz (offline); either is checked against the SHA256. Or take an
# install with -DFREETYPE_ROOT=/path (include/freetype2 and lib/libfreetype.a).
set(FREETYPE_VERSION 2.14.3)
set(FREETYPE_SHA256 36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f)
set(FREETYPE_TARBALL "" CACHE FILEPATH "freetype-${FREETYPE_VERSION}.tar.xz for offline builds (empty: download it)")
set(FREETYPE_ROOT "" CACHE PATH "FreeType install (empty: build FreeType ${FREETYPE_VERSION} from its release tarball)")
if(NOT FREETYPE_ROOT)
  set(FREETYPE_ROOT ${CMAKE_BINARY_DIR}/freetype)
  if(NOT EXISTS ${FREETYPE_ROOT}/lib/libfreetype.a)
    set(_ft_name freetype-${FREETYPE_VERSION}.tar.xz)
    pinned_tarball(_ft_tar https://download.savannah.gnu.org/releases/freetype/${_ft_name}
                   ${_ft_name} ${FREETYPE_SHA256} "${FREETYPE_TARBALL}"
                   "pass -DFREETYPE_TARBALL=/path/to/${_ft_name} or -DFREETYPE_ROOT=/path/to/freetype")
    message(STATUS "building FreeType from ${_ft_tar} (once, under a minute)")
    set(_ft_work ${FREETYPE_ROOT}.work)
    set(_ft_log ${CMAKE_BINARY_DIR}/freetype-build.log)
    file(REMOVE_RECURSE ${_ft_work})
    file(ARCHIVE_EXTRACT INPUT ${_ft_tar} DESTINATION ${_ft_work})
    file(WRITE ${_ft_log} "")
    macro(_ft_run)
      execute_process(COMMAND ${CMAKE_COMMAND} ${ARGN} RESULT_VARIABLE _ft_rc
                      OUTPUT_VARIABLE _ft_out ERROR_VARIABLE _ft_out)
      file(APPEND ${_ft_log} "${_ft_out}")
      if(_ft_rc)
        message(FATAL_ERROR "FreeType build failed, see ${_ft_log}")
      endif()
    endmacro()
    set(_ft_args -G ${CMAKE_GENERATOR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${FREETYPE_ROOT}
        -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON
        -DFT_DISABLE_PNG=ON -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON)
    if(CMAKE_TOOLCHAIN_FILE)
      get_filename_component(_ft_tc ${CMAKE_TOOLCHAIN_FILE} ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR})
      if(NOT EXISTS ${_ft_tc})
        get_filename_component(_ft_tc ${CMAKE_TOOLCHAIN_FILE} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
      endif()
      list(APPEND _ft_args -DCMAKE_TOOLCHAIN_FILE=${_ft_tc})
    else()
      list(APPEND _ft_args -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
    endif()
    if(LLVM_MINGW)
      list(APPEND _ft_args -DLLVM_MINGW=${LLVM_MINGW})
    endif()
    if(CMAKE_MAKE_PROGRAM)
      list(APPEND _ft_args -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM})
    endif()
    if(APPLE)
      list(APPEND _ft_args "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
           -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()
    _ft_run(-S ${_ft_work}/freetype-${FREETYPE_VERSION} -B ${_ft_work}/build ${_ft_args})
    _ft_run(--build ${_ft_work}/build)
    _ft_run(--install ${_ft_work}/build)
    file(REMOVE_RECURSE ${_ft_work})
  endif()
endif()
if(NOT EXISTS ${FREETYPE_ROOT}/lib/libfreetype.a OR NOT EXISTS ${FREETYPE_ROOT}/include/freetype2/ft2build.h)
  message(FATAL_ERROR "no FreeType in ${FREETYPE_ROOT} (include/freetype2/ft2build.h, lib/libfreetype.a)")
endif()

# -- yyjson (reads and writes the JSON preferences)
# Unpacked once into <build>/yyjson from the pinned release (GitHub's archive of the tag: downloaded into build/downloads,
# or -DYYJSON_TARBALL=/path/to/yyjson-<ver>.tar.gz offline; either is checked against the SHA256) and built as a
# static library with plain flags. Its file functions are compiled out: the game opens its own files.
set(YYJSON_VERSION 0.13.0)
set(YYJSON_SHA256 34e0f62a2bc11ab20d601e8ca1cc2b2079503aa45119a19133d89d19b94a0fae)
set(YYJSON_TARBALL "" CACHE FILEPATH "yyjson-${YYJSON_VERSION}.tar.gz for offline builds (empty: download it)")
set(YYJSON_DIR ${CMAKE_BINARY_DIR}/yyjson/yyjson-${YYJSON_VERSION})
if(NOT EXISTS ${YYJSON_DIR}/src/yyjson.h)
  set(_yy_name yyjson-${YYJSON_VERSION}.tar.gz)
  pinned_tarball(_yy_tar https://github.com/ibireme/yyjson/archive/refs/tags/${YYJSON_VERSION}.tar.gz
                 ${_yy_name} ${YYJSON_SHA256} "${YYJSON_TARBALL}" "pass -DYYJSON_TARBALL=/path/to/${_yy_name}")
  file(ARCHIVE_EXTRACT INPUT ${_yy_tar} DESTINATION ${CMAKE_BINARY_DIR}/yyjson)
endif()
set(YYJSON_DEFS YYJSON_DISABLE_FILE=1 YYJSON_DISABLE_INCR_READER=1 YYJSON_DISABLE_UTILS=1)
add_library(yyjson STATIC ${YYJSON_DIR}/src/yyjson.c)
target_compile_definitions(yyjson PRIVATE ${YYJSON_DEFS})
target_compile_options(yyjson PRIVATE -O2 -ffunction-sections -fdata-sections -w)

file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS ${SRC}/*.cpp)
if(WIN32)
  list(REMOVE_ITEM GAME_SRC ${SRC}/platform/platform_unix.cpp)
else()
  list(REMOVE_ITEM GAME_SRC ${SRC}/platform/platform_win.cpp)
endif()

# signed plain char (as MW), its FP evaluation in double; the decompile's pointer/aliasing habits;
set(EVNOVA_FLAGS -O2 -ffunction-sections -fdata-sections -fsigned-char -fno-strict-aliasing
    -fwrapv -fasm-blocks -ffp-eval-method=double
    -Wno-everything -Werror=pointer-to-int-cast -Werror=int-to-pointer-cast -Werror=void-pointer-to-int-cast
    -Werror=int-to-void-pointer-cast -Werror=format -Werror=non-pod-varargs
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)

if(WIN32)
  set(SYSTEM_LIBS kernel32 user32 gdi32 advapi32 shell32 comdlg32)
  # one file to hand out: everything but FFmpeg in the exe
  set(SYSTEM_LINK -static)
  add_executable(evnova_ce WIN32 ${GAME_SRC} ${SRC}/rsrc/evnova.rc)
  set(DEAD_CODE_LINK -Wl,--gc-sections "-Wl,-Map=${CMAKE_BINARY_DIR}/evnova_ce.map")
elseif(APPLE)
  set(SYSTEM_LIBS "-framework CoreText" "-framework CoreFoundation")
  set(SYSTEM_LINK "")
  add_executable(evnova_ce ${GAME_SRC})
  # FFmpeg from the program's folder
  set_target_properties(evnova_ce PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "@executable_path")
  set(DEAD_CODE_LINK -Wl,-dead_strip "-Wl,-map,${CMAKE_BINARY_DIR}/evnova_ce.map")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
  set(SYSTEM_LIBS PkgConfig::FONTCONFIG m)
  # FFmpeg from the program's folder, also for its libraries among themselves (RPATH, not RUNPATH); the game's
  # FreeType kept to itself, apart from the system's that fontconfig uses
  set(SYSTEM_LINK -Wl,--disable-new-dtags -Wl,--exclude-libs,libfreetype.a)
  add_executable(evnova_ce ${GAME_SRC})
  set_target_properties(evnova_ce PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "\$ORIGIN")
  set(DEAD_CODE_LINK -Wl,--gc-sections "-Wl,-Map=${CMAKE_BINARY_DIR}/evnova_ce.map")
endif()
target_include_directories(evnova_ce PRIVATE ${SRC}/include)
target_compile_options(evnova_ce PRIVATE ${EVNOVA_FLAGS})
target_include_directories(evnova_ce PRIVATE ${FFMPEG_ROOT}/include)
set_source_files_properties(${SRC}/port/fontraster.cpp PROPERTIES
    INCLUDE_DIRECTORIES ${FREETYPE_ROOT}/include/freetype2)
set_source_files_properties(${SRC}/port/prefsjson.cpp PROPERTIES
    COMPILE_DEFINITIONS "${YYJSON_DEFS}" INCLUDE_DIRECTORIES ${YYJSON_DIR}/src)
set_target_properties(evnova_ce PROPERTIES OUTPUT_NAME "evnova64" LINKER_LANGUAGE CXX)
target_link_libraries(evnova_ce PRIVATE ${FFMPEG_IMPORT} SDL3::SDL3-static ${FREETYPE_ROOT}/lib/libfreetype.a yyjson
    ${SYSTEM_LIBS})
target_link_options(evnova_ce PRIVATE ${SYSTEM_LINK} ${DEAD_CODE_LINK})
add_custom_command(TARGET evnova_ce POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_DLLS} ${FFMPEG_LICENSE} $<TARGET_FILE_DIR:evnova_ce>)
