#version 450
// The view drawn in 5-bit codes, shown through the brightness: 5-bit value v becomes level[v / 4][v % 4].

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_view;
layout(set = 3, binding = 0) uniform Ramp {
    vec4 level[8];
};

float Shown(uint v)
{
    return level[v >> 2][v & 3u];
}

void main()
{
    uvec3 c = uvec3(texture(u_view, v_uv).rgb * 255.0 + 0.5) >> 3;
    o_color = vec4(Shown(c.r), Shown(c.g), Shown(c.b), 1.0);
}
