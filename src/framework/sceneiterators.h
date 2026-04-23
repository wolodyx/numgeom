#ifndef NUMGEOM_FRAMEWORK_SCENEITERATORS_H
#define NUMGEOM_FRAMEWORK_SCENEITERATORS_H

#include "glm/glm.hpp"

#include "numgeom/iterator.h"

class Drawable;
class Drawable2;
class Scene;

void GetElementsCount(const Scene& scene, size_t& n_verts, size_t& n_cells);

//! Итератор по координатам вершин сцены.
Iterator<glm::vec3> GetVertexIterator(const Scene&);

//! Итератор по нормалям, заданным в вершинах треугольников сцены.
Iterator<glm::vec3> GetNormalIterator(const Scene&);

//! Итератор по цветам, связанных с вершинами сцены.
Iterator<glm::vec3> GetColorIterator(const Scene&);

//! Итератор по индексам треугольников сцены.
Iterator<glm::u32vec3> GetTriaIterator(const Scene&);

Iterator<Drawable2*> GetTriaDrawables(const Iterator<Drawable*>&);

#endif // !NUMGEOM_FRAMEWORK_SCENEITERATORS_H
