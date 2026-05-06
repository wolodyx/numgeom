#version 420 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

void main() {
    // Screen-space to NDC transformation is done on CPU, pass through
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_uv = in_uv;
}