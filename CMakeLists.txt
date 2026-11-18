# EV Nova CE with llvm-mingw
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

# Four members of qtmlClient.lib define Mac Toolbox functions the port defines too (CopyBits, NewGWorld,
# Delay, ...). The port's are the ones the game uses, so those members are linked from copies with QuickTime's
# definitions renamed: no clash, and no member calls one of these names itself, so nothing else changes.
# QuickTime's DisposeGWorld/GetPixBaseAddr/GetGWorldDevice/GetGWorldPixMap get their exe-address names: the
# game calls QuickTime's four by those (burgerpict.c, tradedlg.c). The rest get a QT_ prefix.
set(_qd_dups GetPort PenSize MoveTo Line BackColor SectRect PaintRect EraseRect FrameOval PaintOval RectInRgn
    CopyBits CopyMask DisposePixPat GetPixPat FillCRect RGBForeColor GetGDevice Color2Index GetPicture)
set(_qt_members QDOffScreenShim QuickdrawShim OSUtilsShim TextUtilsShim)
set(_qt_renames_QDOffScreenShim _DisposeGWorld=_FUN_004fc310 _GetPixBaseAddr=_FUN_004fc370
    _GetGWorldDevice=_FUN_004fc390 _GetGWorldPixMap=_FUN_004fc3b0 _NewGWorld=_QT_NewGWorld)
set(_qt_renames_QuickdrawShim "")
foreach(s ${_qd_dups})
  list(APPEND _qt_renames_QuickdrawShim _${s}=_QT_${s})
endforeach()
set(_qt_renames_OSUtilsShim _Delay=_QT_Delay)
set(_qt_renames_TextUtilsShim _NumToString=_QT_NumToString)

set(QT_RENAMED "")
set(_x ${CMAKE_BINARY_DIR}/qtsdk/extract)
execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-ar t ${QTML_CLIENT_LIB} OUTPUT_VARIABLE _members)
foreach(m ${_qt_members})
  set(_out ${CMAKE_BINARY_DIR}/qtsdk/${m}_renamed.obj)
  list(APPEND QT_RENAMED ${_out})
  if(EXISTS ${_out} AND NOT ${QTML_CLIENT_LIB} IS_NEWER_THAN ${_out}
     AND NOT ${CMAKE_CURRENT_LIST_FILE} IS_NEWER_THAN ${_out})
    continue()
  endif()
  file(REMOVE_RECURSE ${_x})
  file(MAKE_DIRECTORY ${_x})
  string(REGEX MATCH "[^\n]*\\\\${m}\\.obj" _m "${_members}")
  execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-ar x ${QTML_CLIENT_LIB} "${_m}" WORKING_DIRECTORY ${_x}
                  RESULT_VARIABLE rc1)
  set(_args "")
  foreach(r ${_qt_renames_${m}})
    list(APPEND _args --redefine-sym ${r})
  endforeach()
  execute_process(COMMAND ${LLVM_MINGW}/bin/llvm-objcopy ${_args} "${_x}/${_m}" ${_out} RESULT_VARIABLE rc2)
  if(NOT _m OR rc1 OR rc2 OR NOT EXISTS ${_out})
    message(FATAL_ERROR "could not extract/rename ${m}.obj from ${QTML_CLIENT_LIB}")
  endif()
endforeach()

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
set_target_properties(evnova_ce PROPERTIES OUTPUT_NAME "EV Nova CE" LINKER_LANGUAGE CXX)
target_link_libraries(evnova_ce PRIVATE ${QT_RENAMED} ${QTML_CLIENT_LIB} ${WIN_LIBS} gdiplus)
target_link_options(evnova_ce PRIVATE -static -Wl,--gc-sections "-Wl,-Map=${CMAKE_BINARY_DIR}/evnova_ce.map"
  # qtmlClient.lib was built with MSVC and asks for its C runtime; mingw's is linked instead
  -Wl,-Xlink=-nodefaultlib:LIBCMT -Wl,-Xlink=-nodefaultlib:OLDNAMES
  # CE's 2 GB memory pool needs the large-address-aware bit
  -Wl,--large-address-aware)
