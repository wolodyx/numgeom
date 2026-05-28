#version 420 core

// Push constants for quad parameters
layout(push_constant) uniform PushConstants {
    vec2 position;   // screen-space position (top-left)
    vec2 size;       // screen-space size (width, height)
    vec2 screenSize; // screen dimensions (width, height)
} pc;

vec2 positions[4] = vec2[](
    vec2(0.0, 0.0),  // top-left
    vec2(1.0, 0.0),  // top-right
    vec2(1.0, 1.0),  // bottom-right
    vec2(0.0, 1.0)   // bottom-left
);

vec2 uvs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

int indices[6] = int[](0, 1, 2, 0, 2, 3);

layout(location = 0) out vec2 out_uv;

void main() {
    int idx = indices[gl_VertexIndex];
    vec2 local_pos = positions[idx];
    vec2 screen_pos = pc.position + local_pos * pc.size;
    float ndc_x = (screen_pos.x * 2.0 / pc.screenSize.x) - 1.0;
    float ndc_y = (screen_pos.y * 2.0 / pc.screenSize.y) - 1.0;
    gl_Position = vec4(ndc_x, ndc_y, 0.0, 1.0);
    out_uv = uvs[idx];
}
