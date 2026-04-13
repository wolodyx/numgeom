#ifndef NUMGEOM_CORE_SHAPES_H
#define NUMGEOM_CORE_SHAPES_H

#include <array>

#include "numgeom/core_export.h"
#include "numgeom/trimesh.h"

CORE_EXPORT TriMesh::Ptr MakeBox(
    const std::array<glm::vec3, 8>& corners);

CORE_EXPORT TriMesh::Ptr MakeSphere(const glm::vec3& center,
                                    float radius, size_t nSlices,
                                    size_t nStacks);

/**
\brief Make a cylinder of triangles without bases.
\param bottom_center The center of the cylinder bottom base.
\param top_center The center of the cylinder top base.
\param radius The base radius.
\param segments The number of segments approximating the base circle (>=3).
\return The mesh of the cylinder.
*/
CORE_EXPORT TriMesh::Ptr MakeCylinder(const glm::vec3& bottom_center,
                                      const glm::vec3& top_center,
                                      float radius, size_t segments);

/**
\brief Make a cone of triangles without a bottom.
\param base_center The center of the cone base.
\param apex The top of the cone.
\param radius The base radius.
\param segments The number of segments approximating the base circle (>=3).
\return The mesh of the cone.
*/
CORE_EXPORT TriMesh::Ptr MakeCone(const glm::vec3& base_center,
                                  const glm::vec3& apex, float radius,
                                  size_t segments);

#endif  // !NUMGEOM_CORE_SHAPES_H
