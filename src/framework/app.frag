#version 420 core

layout (std140, set = 0, binding = 1) uniform FragmentBufferObject{
  uniform vec3 light_pos;
  uniform vec3 view_pos;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() {
  float ambient = 0.1;
  vec3 normal = normalize(in_normal);
  vec3 light_dir = normalize(light_pos - in_position);
  float diffuse = max(dot(normal,light_dir),0.0);
  float specular = 0.0;
  if(diffuse > 0) {
    vec3 view_dir = normalize(view_pos - in_position);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float shininess = 32.0;
    specular = 0.5 * pow(max(dot(view_dir,reflect_dir),0.0),shininess);
  }
  vec3 color = (ambient + diffuse + specular) * in_color.xyz;
  out_color = vec4(color, in_color.w);
}
