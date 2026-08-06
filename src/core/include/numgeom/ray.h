#ifndef NUMGEOM_CORE_RAY_H
#define NUMGEOM_CORE_RAY_H

#include "glm/glm.hpp"

class Ray {
 public:
  Ray() = default;
  Ray(const glm::vec3& origin, const glm::vec3& direction);

 public:
  glm::vec3 origin;
  glm::vec3 direction;
};
#endif // !NUMGEOM_CORE_RAY_H
