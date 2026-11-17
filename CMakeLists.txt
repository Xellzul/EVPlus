# EV Nova CE with llvm-mingw -- no Docker, Wine or Metrowerks.
#
#   cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw.cmake
#   cmake --build build                      -> build/EV Nova CE.exe
#
# Needs: llvm-mingw (default /opt/llvm-mingw), and `unar` (package unarchiver) once, to unpack
# input-files/QT6SDK_Win.hqx for QuickTime's qtmlClient.lib -- or pass -DQTML_CLIENT_LIB=/path/qtmlClient.lib.
cmake_minimum_required(VERSION 3.20)
project(EVNova C CXX RC)

set(SRC ${CMAKE_SOURCE_DIR}/sources)

# -- QuickTime's client library, from the Apple SDK in input-files/ ---------------------------------
set(QTML_CLIENT_LIB "" CACHE FILEPATH "qtmlClient.lib (empty: unpack it from input-files/QT6SDK_Win.hqx)")
if(NOT QTML_CLIENT_LIB)
  set(_qt ${CMAKE_BINARY_DIR}/qtsdk)
  set(QTML_CLIENT_LIB ${_qt}/qtmlClient.lib)
  if(NOT EXISTS ${QTML_CLIENT_LIB})
    find_program(UNAR unar)
    if(NOT UNAR)
      message(FATAL_ERROR "qtmlClient.lib is unpacked from input-files/QT6SDK_Win.hqx with `unar`, which is not "
                          "installed: install it or pass -DQTML_CLIENT_LIB=/path/to/qtmlClient.lib")
    endif()
    execute_process(COMMAND ${UNAR} -q -f -o ${_qt} ${CMAKE_SOURCE_DIR}/input-files/QT6SDK_Win.hqx
                    RESULT_VARIABLE rc)
    if(rc OR NOT EXISTS ${_qt}/QT6SDK.zip)
      message(FATAL_ERROR "could not unpack input-files/QT6SDK_Win.hqx with unar")
    endif()
    file(ARCHIVE_EXTRACT INPUT ${_qt}/QT6SDK.zip DESTINATION ${_qt}/zip
         PATTERNS "*/QTDevWin/Libraries/qtmlClient.lib")
    file(GLOB_RECURSE _lib ${_qt}/zip/*qtmlClient.lib)
    file(COPY_FILE ${_lib} ${QTML_CLIENT_LIB})
  endif()
endif()

# QuickTime's own DisposeGWorld/GetPixBaseAddr/GetGWorldDevice/GetGWorldPixMap live beside the port's
# same-named emulation: the game calls QuickTime's four by their exe addresses (burgerpict.c, tradedlg.c), so
# that member is linked with those four renamed
set(QT_RENAMED ${CMAKE_BINARY_DIR}/qtsdk/QDOffScreenShim_renamed.obj)
if(NOT EXISTS ${QT_RENAMED} OR ${QTML_CLIENT_LIB} IS_NEWER_THAN ${QT_RENAMED})
  set(_x ${CMAKE_BINARY_DIR}/qtsdk/extract)
  file(REMOVE_RECURSE ${_x})
  file(MAKE_DIRECTORY ${_x})
  execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-ar t ${QTML_CLIENT_LIB} OUTPUT_VARIABLE _members)
  string(REGEX MATCH "[^\n]*QDOffScreenShim\\.obj" _m "${_members}")
  execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-ar x ${QTML_CLIENT_LIB} "${_m}" WORKING_DIRECTORY ${_x} RESULT_VARIABLE rc1)
  execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-objcopy
      --redefine-sym _DisposeGWorld=_FUN_004fc310 --redefine-sym _GetPixBaseAddr=_FUN_004fc370
      --redefine-sym _GetGWorldDevice=_FUN_004fc390 --redefine-sym _GetGWorldPixMap=_FUN_004fc3b0
      "${_x}/${_m}" ${QT_RENAMED} RESULT_VARIABLE rc2)
  if(rc1 OR rc2 OR NOT EXISTS ${QT_RENAMED})
    message(FATAL_ERROR "could not extract/rename QDOffScreenShim.obj from ${QTML_CLIENT_LIB}")
  endif()
endif()

# -- the sources, C or C++ by each file's `// COMPILE: -lang c++` marker ----------------------------
file(GLOB_RECURSE ALL_C CONFIGURE_DEPENDS ${SRC}/*.c)
set(GAME_SRC "")
set(RT_SRC "")
foreach(f ${ALL_C})
  file(RELATIVE_PATH rel ${SRC} ${f})
  if(rel MATCHES "^(include|mac|v111)/")
    continue()
  endif()
  file(STRINGS ${f} marker LIMIT_COUNT 30 REGEX "^[ \t]*//[ \t]*COMPILE:.*-lang c\\+\\+")
  if(marker)
    set_source_files_properties(${f} PROPERTIES LANGUAGE CXX)
  endif()
  if(rel MATCHES "^runtime/")
    list(APPEND RT_SRC ${f})
  else()
    list(APPEND GAME_SRC ${f})
  endif()
endforeach()

# MW's -enum min, signed plain char, its FP evaluation in double; the decompile's pointer/aliasing habits;
# one section per function so the link drops what nothing uses (as MW's linker does)
set(EVNOVA_FLAGS -O2 -ffunction-sections -fdata-sections -fsigned-char -fshort-enums -fno-strict-aliasing
    -fwrapv -fpascal-strings -fasm-blocks -ffp-eval-method=double -fcommon -w
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)

set(WIN_LIBS kernel32 user32 gdi32 winmm wsock32 advapi32 comctl32 shell32 version comdlg32 iphlpapi ole32
    oleaut32 opengl32)

# the runtime as an archive: members are pulled only when referenced, as with MW's rt.lib
add_library(evnova_ce_rt STATIC ${RT_SRC})
add_executable(evnova_ce WIN32 ${GAME_SRC} ${SRC}/rsrc/evnova.rc)
foreach(t evnova_ce evnova_ce_rt)
  target_include_directories(${t} PRIVATE ${SRC}/include)
  target_compile_options(${t} PRIVATE ${EVNOVA_FLAGS})
endforeach()
set_target_properties(evnova_ce PROPERTIES OUTPUT_NAME "EV Nova CE" LINKER_LANGUAGE CXX)
target_link_libraries(evnova_ce PRIVATE evnova_ce_rt ${QT_RENAMED} ${QTML_CLIENT_LIB} ${WIN_LIBS} gdiplus)
target_link_options(evnova_ce PRIVATE -static -Wl,--gc-sections "-Wl,-Map=${CMAKE_BINARY_DIR}/evnova_ce.map"
  # qtmlClient.lib was built with MSVC and asks for its C runtime; mingw's is linked instead
  -Wl,-Xlink=-nodefaultlib:LIBCMT -Wl,-Xlink=-nodefaultlib:OLDNAMES
  # it also carries QuickDraw stand-ins the port defines itself (CopyBits, NewGWorld, ...): the port's
  # objects come first on the line, so its definitions win, as they did under MW
  -Wl,--allow-multiple-definition
  # CE's 2 GB memory pool needs the large-address-aware bit
  -Wl,--large-address-aware)
