#version 420 core

layout(std140, set = 0, binding = 0) uniform VertexBufferObject{
  uniform mat4 mvp_matrix;
  uniform mat4 mv_matrix;
  uniform mat3 normal_matrix;
};

// Параметры, передаваемые через push-constants.
// grid_params.x -- размер ячейки (шаг сетки).
// grid_params.y -- полуразмер квада (в 2 раза больше дальней плоскости
//                   отсечения камеры, чтобы край квада не попадал в кадр).
// grid_params.z -- ближняя граница тумана.
// grid_params.w -- дальняя граница тумана.
// camera_pos.xz -- мировая позиция камеры (квад всегда под камерой).
// Block должен совпадать с фрагментным шейдером (background не используется
// в вершинном шейдере, но объявляется для совпадения двоичного интерфейса).

layout(push_constant) uniform PushConstants {
  vec4 grid_params;
  vec4 grid_color;
  vec4 camera_pos;
  vec4 background;
};

layout(location = 0) out vec2 out_grid_coord;
layout(location = 1) out vec3 out_view_pos;

void main() {
  // Генерируем квад (2 треугольника) без вершинного буфера.
  // 6 вершин: (0,1,2) и (0,2,3).
  vec2 corners[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
  );

  // Увеличиваем квад до значения, покрывающего весь видимый пол (в 2 раза
  // больше дальней плоскости отсечения), и центрируем его под камерой
  // (плоскость y = 0). Так сетка следует за камерой без видимых краёв.
  vec2 m_pos = corners[gl_VertexIndex] * grid_params.y + camera_pos.xz;

  vec3 world = vec3(m_pos.x, 0.0, m_pos.y);

  out_grid_coord = world.xz / grid_params.x + 0.5;
  out_view_pos = (mv_matrix * vec4(world, 1.0)).xyz;
  gl_Position = mvp_matrix * vec4(world, 1.0);
}
