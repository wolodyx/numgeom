#version 420 core

layout(set = 0, binding = 3) uniform sampler2D text_texture;
layout(set = 0, binding = 4) uniform TextColor {
    vec4 color;
} text_color;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
    float alpha = texture(text_texture, in_uv).r;
    out_color = vec4(text_color.color.rgb, text_color.color.a * alpha);
}