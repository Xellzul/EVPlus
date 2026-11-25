#version 450
// kPlatDrawOr (platform.h): a sprite through the original renderer's 16-bit blend, on 5-bit channels. The
// vertex colour holds the draw's PlatOrBlend as whole numbers: r = the weights (6 bits each, red lowest),
// g = the overs and b = the tint (x1r5g5b5 order), a = keep | own << 10. The view under the sprite is read
// from its copy, at the same pixel.

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;
layout(set = 2, binding = 1) uniform sampler2D u_view;

// 8-bit channels to 5 bits, the low 3 dropped
uvec3 Codes(vec3 c)
{
    return uvec3(c * 255.0 + 0.5) >> 3;
}

// each channel times k / 32, rounded down; only k's low 10 bits count, as in the 32-bit pair math
uvec3 Scale(uvec3 v, uvec3 k)
{
    return (v * k >> 5) & 31u;
}

void main()
{
    vec4 texel = texture(u_image, v_uv);
    if (texel.a < 0.5)
        discard;
    uvec4 p = uvec4(v_color + 0.5);
    uvec3 weight = uvec3(p.r, p.r >> 6, p.r >> 12) & 63u;
    uvec3 over = uvec3(p.g >> 10, p.g >> 5, p.g) & 31u;
    uvec3 tint = uvec3(p.b >> 10, p.b >> 5, p.b) & 31u;
    uint keep = p.a & 1023u;
    uint own = p.a >> 10;

    uvec3 src = Codes(texel.rgb);
    if (own < 32u)
        src = Scale(src, uvec3(own)) | Scale(tint, uvec3(32u - own));
    uvec3 c = Scale(src, weight) | over;
    if (keep > 1u)
        c |= Scale(Codes(texelFetch(u_view, ivec2(gl_FragCoord.xy), 0).rgb), uvec3(keep));
    o_color = vec4(vec3((c << 3) | (c >> 2)) / 255.0, 1.0);
}
