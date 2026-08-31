#version 420 core

layout(location = 0) in vec4 in_color;
layout(location = 1) flat in uint in_object_id;

layout(location = 0) out vec4 out_color;
layout(location = 1) out uint out_object_id;

void main() {
  out_color = in_color;
  out_object_id = in_object_id;
}