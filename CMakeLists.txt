# EV Nova CE with llvm-mingw
#
#   cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake
#   cmake --build build                      -> build/EV Nova CE.exe
#
# Needs: llvm-mingw (default /opt/llvm-mingw); sh, make and xz once, to build FFmpeg (downloaded, or
# -DFFMPEG_TARBALL=/path offline; or pass -DFFMPEG_ROOT=/path). The FFmpeg DLLs and its licence are copied next to the exe.
cmake_minimum_required(VERSION 3.20)
project(EVNova C CXX RC)

set(SRC ${CMAKE_SOURCE_DIR}/sources)

# -- FFmpeg
# Built once by cmake/build-ffmpeg.sh into build/ffmpeg from the pinned release: downloaded into build/, or
# -DFFMPEG_TARBALL=/path/to/ffmpeg-<ver>.tar.xz (offline); either is checked against the SHA256. Or take an
# install with -DFFMPEG_ROOT=/path (include/, lib/*.dll.a, bin/*.dll).
set(FFMPEG_VERSION 9.0.2)
set(FFMPEG_SHA256 8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e)
set(FFMPEG_TARBALL "" CACHE FILEPATH "ffmpeg-${FFMPEG_VERSION}.tar.xz for offline builds (empty: download it)")
set(FFMPEG_ROOT "" CACHE PATH "FFmpeg install (empty: build FFmpeg ${FFMPEG_VERSION} with cmake/build-ffmpeg.sh)")
if(NOT FFMPEG_ROOT)
  set(FFMPEG_ROOT ${CMAKE_BINARY_DIR}/ffmpeg)
  if(NOT EXISTS ${FFMPEG_ROOT}/lib/libavformat.dll.a)
    set(_ff_name ffmpeg-${FFMPEG_VERSION}.tar.xz)
    set(_ff_tar ${FFMPEG_TARBALL})
    if(NOT _ff_tar)
      set(_ff_tar ${CMAKE_BINARY_DIR}/${_ff_name})
      if(NOT EXISTS ${_ff_tar})
        message(STATUS "downloading https://ffmpeg.org/releases/${_ff_name}")
        file(DOWNLOAD https://ffmpeg.org/releases/${_ff_name} ${_ff_tar}.part
             EXPECTED_HASH SHA256=${FFMPEG_SHA256} STATUS _ff_dl)
        list(GET _ff_dl 0 _ff_rc)
        if(_ff_rc)
          file(REMOVE ${_ff_tar}.part)
          message(FATAL_ERROR "FFmpeg download failed (${_ff_dl}); pass -DFFMPEG_TARBALL=/path/to/"
                              "${_ff_name} or -DFFMPEG_ROOT=/path/to/ffmpeg")
        endif()
        file(RENAME ${_ff_tar}.part ${_ff_tar})
      endif()
    endif()
    file(SHA256 ${_ff_tar} _ff_sum)
    if(NOT _ff_sum STREQUAL FFMPEG_SHA256)
      message(FATAL_ERROR "${_ff_tar}: SHA256 ${_ff_sum}, expected ${FFMPEG_SHA256}")
    endif()
    message(STATUS "building FFmpeg from ${_ff_tar} (once, a few minutes)")
    execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/cmake/build-ffmpeg.sh ${_ff_tar} ${FFMPEG_ROOT} ${LLVM_MINGW}
                    RESULT_VARIABLE rc OUTPUT_FILE ${CMAKE_BINARY_DIR}/ffmpeg-build.log
                    ERROR_FILE ${CMAKE_BINARY_DIR}/ffmpeg-build.log)
    if(rc OR NOT EXISTS ${FFMPEG_ROOT}/lib/libavformat.dll.a)
      message(FATAL_ERROR "FFmpeg build failed, see ${CMAKE_BINARY_DIR}/ffmpeg-build.log")
    endif()
  endif()
endif()
set(FFMPEG_LIBS avformat avcodec swscale swresample avutil)
set(FFMPEG_IMPORT "")
foreach(l ${FFMPEG_LIBS})
  list(APPEND FFMPEG_IMPORT ${FFMPEG_ROOT}/lib/lib${l}.dll.a)
endforeach()
file(GLOB FFMPEG_DLLS ${FFMPEG_ROOT}/bin/*.dll)
file(GLOB FFMPEG_LICENSE ${FFMPEG_ROOT}/share/licenses/FFmpeg-COPYING.LGPLv2.1.txt)

# -- the sources, C or C++ by each file's `// COMPILE: -lang c++` marker ----------------------------
file(GLOB_RECURSE ALL_C CONFIGURE_DEPENDS ${SRC}/*.c)
set(GAME_SRC "")
foreach(f ${ALL_C})
  file(STRINGS ${f} marker LIMIT_COUNT 30 REGEX "^[ \t]*//[ \t]*COMPILE:.*-lang c\\+\\+")
  if(marker)
    set_source_files_properties(${f} PROPERTIES LANGUAGE CXX)
  endif()
  list(APPEND GAME_SRC ${f})
endforeach()

# MW's -enum min, signed plain char, its FP evaluation in double; the decompile's pointer/aliasing habits;
# one section per function so the link drops what nothing uses (as MW's linker does)
set(EVNOVA_FLAGS -O2 -ffunction-sections -fdata-sections -fsigned-char -fshort-enums -fno-strict-aliasing
    -fwrapv -fpascal-strings -fasm-blocks -ffp-eval-method=double -w
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)

set(WIN_LIBS kernel32 user32 gdi32 winmm advapi32 shell32 comdlg32)

add_executable(evnova_ce WIN32 ${GAME_SRC} ${SRC}/rsrc/evnova.rc)
target_include_directories(evnova_ce PRIVATE ${SRC}/include)
target_compile_options(evnova_ce PRIVATE ${EVNOVA_FLAGS})
# the only file that sees FFmpeg's headers: their structs must keep FFmpeg's enum sizes
target_include_directories(evnova_ce PRIVATE ${FFMPEG_ROOT}/include)
set_source_files_properties(${SRC}/media/media.c PROPERTIES COMPILE_OPTIONS -fno-short-enums)
set_target_properties(evnova_ce PROPERTIES OUTPUT_NAME "EV Nova CE" LINKER_LANGUAGE CXX)
target_link_libraries(evnova_ce PRIVATE ${FFMPEG_IMPORT} ${WIN_LIBS} gdiplus)
target_link_options(evnova_ce PRIVATE -static -Wl,--gc-sections "-Wl,-Map=${CMAKE_BINARY_DIR}/evnova_ce.map"
  # CE's 2 GB memory pool needs the large-address-aware bit
  -Wl,--large-address-aware)
add_custom_command(TARGET evnova_ce POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_DLLS} ${FFMPEG_LICENSE} $<TARGET_FILE_DIR:evnova_ce>)
