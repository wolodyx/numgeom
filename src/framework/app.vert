#version 420 core

layout(std140, set = 0, binding = 0) uniform VertexBufferObject{
  uniform mat4 mvp_matrix;
  uniform mat4 mv_matrix;
  uniform mat3 normal_matrix;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_color;

void main() {
  gl_Position = mvp_matrix * vec4(in_position, 1.0);
  out_position = in_position;
  out_color = vec4(in_color, 1.0);
  out_normal = normalize(in_normal);
}
