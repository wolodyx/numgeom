#version 420 core

layout(set = 0, binding = 2) uniform sampler2D logo_texture;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(logo_texture, in_uv);
    // If the texture has no alpha, out_color.a will be 1.0
}