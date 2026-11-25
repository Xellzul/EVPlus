#!/bin/sh
# build-shaders.sh -- the GPU view's fragment shaders (the GLSL .frag files beside this) into viewshaders.h, the
# blobs platform_sdl.cpp hands SDL_CreateGPUShader: one table a shader, an entry a format.
#   SPIR-V (Vulkan)       glslc, from shaderc or the Vulkan SDK
#   MSL (Metal)           spirv-cross, from the SPIR-V
#   DXIL (Direct3D 12)    spirv-cross to HLSL, then dxc (DXC=path for another one)
# The bindings follow SDL_CreateGPUShader's rules for each format; the inputs are the ones SDL's GPU renderer's
# vertex shaders give (location 0 the colour, 1 the texture coordinate). Run it after changing a .frag; the
# header is kept in the repository.
set -e
cd "$(dirname "$0")"
dxc=${DXC:-dxc}
for tool in glslc spirv-cross "$dxc"; do
    command -v "$tool" > /dev/null || { echo "build-shaders.sh: $tool not found" >&2; exit 1; }
done
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
out=$work/viewshaders.h

# bytes of a file as C initialisers, 16 a line
bytes() {
    od -An -v -tx1 "$1" | sed -e 's/ *\([0-9a-f][0-9a-f]\)/0x\1, /g' -e 's/, $/,/' -e 's/^/    /'
}

# array name, file
blob() {
    echo "static const unsigned char $1[] = {"
    bytes "$2"
    echo "};"
}

# The HLSL of a SPIR-V fragment shader with the inputs named as the renderer's vertex shaders name their outputs:
# spirv-cross calls location n TEXCOORDn, D3D12 links the stages by name.
hlsl() {
    spirv-cross "$1" --hlsl --shader-model 60 --output "$2.raw"
    sed -e 's/ : TEXCOORD0;$/ : COLOR0;/' -e 's/ : TEXCOORD1;$/ : TEXCOORD0;/' "$2.raw" > "$2"
    sed -n '/^struct SPIRV_Cross_Input/,/^};/p' "$2" | grep -q ' : COLOR0;$' ||
        { echo "build-shaders.sh: $1: no colour input in the HLSL" >&2; exit 1; }
}

{
    echo "/* viewshaders.h -- made by build-shaders.sh from the .frag files beside it; not to be edited. A table a"
    echo "   shader, its blobs by format; the includer defines ShaderBlob { format, code, size, entry }. */"
    echo "#pragma once"
} > "$out"

# file stem, C name
for pair in orblend:OrBlend brightness:Brightness; do
    stem=${pair%%:*}
    name=${pair##*:}
    # unoptimised: every input stays declared, as D3D12 needs the colour before the coordinate
    glslc --target-env=vulkan1.0 -fshader-stage=frag -o "$work/$stem.spv" "$stem.frag"
    spirv-cross "$work/$stem.spv" --msl --msl-version 20100 --msl-decoration-binding --output "$work/$stem.msl"
    hlsl "$work/$stem.spv" "$work/$stem.hlsl"
    "$dxc" -T ps_6_0 -E main -Fo "$work/$stem.dxil" "$work/$stem.hlsl" > /dev/null
    {
        echo
        blob "k${name}Spirv" "$work/$stem.spv"
        blob "k${name}Msl" "$work/$stem.msl"
        blob "k${name}Dxil" "$work/$stem.dxil"
        echo "static const ShaderBlob k${name}Shader[] = {"
        echo "    { SDL_GPU_SHADERFORMAT_SPIRV, k${name}Spirv, sizeof(k${name}Spirv), \"main\" },"
        echo "    { SDL_GPU_SHADERFORMAT_MSL, k${name}Msl, sizeof(k${name}Msl), \"main0\" },"
        echo "    { SDL_GPU_SHADERFORMAT_DXIL, k${name}Dxil, sizeof(k${name}Dxil), \"main\" },"
        echo "};"
    } >> "$out"
done
cp "$out" viewshaders.h
echo "viewshaders.h: SPIR-V, MSL, DXIL"
