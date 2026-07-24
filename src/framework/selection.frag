#version 420 core

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput scene_color_input;
layout(set = 0, binding = 1, input_attachment_index = 1) uniform usubpassInput object_id_input;
layout(std140, set = 0, binding = 2) uniform SelectionData {
  uint selected_ids[64];
  uint selected_count;
};

layout(location = 0) out vec4 out_color;

void main() {
  vec4 scene_color = subpassLoad(scene_color_input);
  uvec4 object_id_values = subpassLoad(object_id_input);
  uint object_id = object_id_values.x;

  bool selected = false;
  if (object_id != 0u && selected_count > 0u) {
    for (uint i = 0u; i < selected_count; ++i) {
      if (selected_ids[i] == object_id) {
        selected = true;
        break;
      }
    }
  }

  if (selected) {
    out_color = vec4(mix(scene_color.rgb, vec3(1.0, 1.0, 0.0), 0.5), scene_color.a);
  } else {
    out_color = scene_color;
  }
}
