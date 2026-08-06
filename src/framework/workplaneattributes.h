#ifndef NUMGEOM_FRAMEWORK_WORKPLANEATTRIBUTES_H
#define NUMGEOM_FRAMEWORK_WORKPLANEATTRIBUTES_H

#include "glm/glm.hpp"

//! Параметры отображения рабочей плоскости (workplane).
class WorkplaneAttributes {
 public:
  //! Шаг сетки (расстояние между соседними линиями).
  float spacing = 1.0f;
  //! Толщина линий сетки в мировых единицах.
  float line_thickness = 0.02f;
  //! Цвет линий сетки.
  glm::vec3 color = glm::vec3(0.5f, 0.5f, 0.5f);
};
#endif // !NUMGEOM_FRAMEWORK_WORKPLANE_H
