#ifndef NUMGEOM_FRAMEWORK_INTERSECTION_H
#define NUMGEOM_FRAMEWORK_INTERSECTION_H

#include "glm/glm.hpp"

class Drawable2;
class Ray;

glm::vec3 IntersectRayWithDrawable(const Ray& ray, const Drawable2* drawable);

#endif // !NUMGEOM_FRAMEWORK_INTERSECTION_H
