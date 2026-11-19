#!/bin/sh
# Builds the minimal LGPL FFmpeg the game's media layer uses (sources/media/), as shared libraries: 64-bit
# Windows DLLs with llvm-mingw, or with "native" this system's own (clang), and installs its licence as
# share/licenses/FFmpeg-COPYING.LGPLv2.1.txt. CMake runs this on the first configure (into <build>/ffmpeg)
# unless -DFFMPEG_ROOT points at an existing install.
#
#   cmake/build-ffmpeg.sh <ffmpeg-x.y.z.tar.xz> <install dir> [llvm-mingw root | native]
#
# Needs: sh, make, tar with xz. No nasm: the hand-written x86 assembly is off (--disable-x86asm).
# LGPL: no --enable-gpl / --enable-nonfree.
set -e

tarball=$1
prefix=$2
llvm=${3:-/opt/llvm-mingw}
if [ -z "$tarball" ] || [ -z "$prefix" ]; then
    echo "usage: $0 <ffmpeg tarball> <install dir> [llvm-mingw root | native]" >&2
    exit 2
fi

work=$prefix.work
rm -rf "$work"
mkdir -p "$work"
tar -xJf "$tarball" -C "$work"
src=$(find "$work" -mindepth 1 -maxdepth 1 -type d -name 'ffmpeg-*' | head -n 1)

cd "$src"
if [ "$llvm" = native ] && [ "$(uname)" = Darwin ]; then
    # found beside the program (@rpath), on the macOS the game is built for
    export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-11.0}
    set -- --cc=clang --cxx=clang++ --ld=clang --enable-pthreads --install-name-dir=@rpath
elif [ "$llvm" = native ]; then
    set -- --cc=clang --cxx=clang++ --ld=clang --enable-pthreads
else
    t=$llvm/bin/x86_64-w64-mingw32
    set -- \
        --target-os=mingw32 --arch=x86_64 --enable-cross-compile --cross-prefix="$t-" \
        --cc="$t-clang" --cxx="$t-clang++" --ld="$t-clang" \
        --ar="$llvm/bin/llvm-ar" --ranlib="$llvm/bin/llvm-ranlib" --nm="$llvm/bin/llvm-nm" \
        --strip="$llvm/bin/llvm-strip" --windres="$t-windres" \
        --enable-w32threads --disable-pthreads
fi
./configure "$@" \
    --prefix="$prefix" \
    --enable-shared --disable-static \
    --disable-programs --disable-doc --disable-network --disable-autodetect --disable-x86asm \
    --disable-debug \
    --disable-everything \
    --disable-avdevice --disable-avfilter \
    --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
    --enable-protocol=file \
    --enable-demuxer=mov,mp3,aiff,wav \
    --enable-parser=mpeg4video,mpegaudio,h263,mjpeg \
    --enable-decoder=mpeg4,h263,cinepak,svq1,svq3,qtrle,mjpeg,rpza,smc,rawvideo,8bps \
    --enable-decoder=mp3float,mp3,mp2float,mp2,aac,alac,qdm2,qdmc,mace3,mace6,adpcm_ima_qt \
    --enable-decoder=pcm_s16be,pcm_s16le,pcm_s24be,pcm_s8,pcm_u8,pcm_mulaw,pcm_alaw,pcm_f32be,gsm
make
make install
mkdir -p "$prefix/share/licenses"
cp COPYING.LGPLv2.1 "$prefix/share/licenses/FFmpeg-COPYING.LGPLv2.1.txt"
cd /
rm -rf "$work"
