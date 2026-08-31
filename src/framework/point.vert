#version 420 core

layout(std140, set = 0, binding = 0) uniform VertexBufferObject{
  uniform mat4 mvp_matrix;
  uniform mat4 mv_matrix;
  uniform mat3 normal_matrix;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in uint in_object_id;

layout(location = 0) out vec4 out_color;
layout(location = 1) flat out uint out_object_id;

void main() {
  gl_Position = mvp_matrix * vec4(in_position, 1.0);
  gl_PointSize = 5.0;
  out_color = vec4(in_color, 1.0);
  out_object_id = in_object_id;
}