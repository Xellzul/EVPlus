#version 450
// The finished frame through the brightness: each channel c shown as c to the power of the exponent.

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0) uniform Brightness {
    vec4 exponent;          // x; the rest unused
};

void main()
{
    vec3 c = max(texture(u_frame, v_uv).rgb, vec3(0.0));
    o_color = vec4(pow(c, vec3(exponent.x)), 1.0);
}
