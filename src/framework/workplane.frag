#version 420 core

//! Количество вспомогательных линий между главными линиями сетки.
const int GridMinorsPerMajor = 4;

// Параметры, передаваемые через push-constants.
// grid_params.x -- размер ячейки (шаг сетки).
// grid_params.z -- ближняя граница тумана.
// grid_params.w -- дальняя граница тумана.
// background.rgb -- цвет фона сцены для затухания линий туманом.
layout(push_constant) uniform PushConstants {
  vec4 grid_params;
  vec4 grid_color;
  vec4 camera_pos;
  vec4 background;
};

layout(location = 0) in vec2 in_grid_coord;
layout(location = 1) in vec3 in_view_pos;

layout(location = 0) out vec4 out_color;

void main() {
  float fog_near = grid_params.z;
  float fog_far = grid_params.w;
  vec3 backdrop = background.rgb;

  // Коэффициенты затухания (туман) для линий по расстоянию от камеры.
  // Основные линии и оси затухают в диапазоне [fog_near, fog_far],
  // вспомогательные исчезают раньше ([fog_near*0.5, fog_far*0.85]).
  float view_dist = length(in_view_pos);
  float major_fog = 1.0 - smoothstep(fog_near, fog_far, view_dist);
  float minor_fog = 1.0 - smoothstep(fog_near * 0.5, fog_far * 0.85, view_dist);

  // Индекс ближайшей линии сетки к текущему пикселю (по каждой из осей X и Z).
  ivec2 line_index = ivec2(floor(in_grid_coord));

  // Подбираем ширину, цвет и затухание для ближайшей линии по каждой из осей.
  vec2 line_width;
  vec3 line_color0;
  vec3 line_color1;
  vec2 line_fog;
  for (int i = 0; i < 2; ++i) {
    float width;
    vec3 color;
    float fog;
    if (line_index[i] == 0) {
      // Оси координат -- яркие и жирные: X -- красная, Z -- синяя.
      width = 5.0;
      color = (i == 0) ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 0.0, 1.0);
      fog = major_fog;
    } else if (line_index[i] % GridMinorsPerMajor == 0) {
      // Главные линии сетки.
      width = 3.0;
      color = grid_color.rgb * 0.7;
      fog = major_fog;
    } else {
      // Вспомогательные линии сетки.
      width = 2.0;
      color = grid_color.rgb * 0.35;
      fog = minor_fog;
    }
    line_width[i] = width;
    line_fog[i] = fog;
    if (i == 0)
      line_color0 = color;
    else
      line_color1 = color;
  }

  // Расстояние (в координатах сетки) от центра ближайшей линии.
  vec2 line_dist = abs(0.5 - fract(in_grid_coord)) * 2.0;

  // Маска линии с антиалиасингом: fwidth компенсирует масштаб, поэтому
  // видимая толщина линий не зависит от расстояния до камеры.
  vec2 line_mask = 1.0 - clamp(line_dist / (fwidth(in_grid_coord) * line_width),
                               0.0, 1.0);

  vec2 blend_factors = line_mask * line_fog;

  // Затемняем линии вблизи осей координат, чтобы оси выглядели поверх них.
  for (int i = 0; i < 2; ++i)
    if (line_index[1 - i] == 0 && line_index[i] != 0)
      blend_factors[i] *= smoothstep(0.0, 0.5, line_dist[1 - i]);

  // Итоговый цвет линии -- максимум из вкладов по двум осям
  // (вместо сложения, чтобы избежать ярких пятен на пересечениях).
  vec3 line_color = max(line_color0 * blend_factors.x,
                        line_color1 * blend_factors.y);
  float line_alpha = max(blend_factors.x, blend_factors.y);

  // Отбрасываем фрагменты вне линий, сохраняя поведение глубины прежним.
  if (line_alpha < 0.005)
    discard;

  // Смешиваем цвет линии с фоном сцены, имитируя туман. Альфа-смешивание
  // не используется (подключение object_id R32_UINT не поддерживает блендинг),
  // поэтому затухание выполняется в сторону цвета фона.
  out_color = vec4(mix(backdrop, line_color, line_alpha), 1.0);
}
