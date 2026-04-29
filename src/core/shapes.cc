#include "numgeom/shapes.h"

#include <cassert>
#include <cmath>

#include "glm/gtx/quaternion.hpp"

TriMesh::Ptr MakeBox(const std::array<glm::vec3, 8>& corners) {
  TriMesh::Ptr mesh = TriMesh::Create(8, 12);
  size_t iNode = 0;
  for (const auto& p : corners) mesh->GetNode(iNode++) = p;

  mesh->GetCell(0) = TriMesh::Cell(0, 2, 1);
  mesh->GetCell(1) = TriMesh::Cell(0, 3, 2);
  mesh->GetCell(2) = TriMesh::Cell(4, 5, 6);
  mesh->GetCell(3) = TriMesh::Cell(4, 6, 7);
  mesh->GetCell(4) = TriMesh::Cell(0, 1, 5);
  mesh->GetCell(5) = TriMesh::Cell(0, 5, 4);
  mesh->GetCell(6) = TriMesh::Cell(3, 6, 2);
  mesh->GetCell(7) = TriMesh::Cell(3, 7, 6);
  mesh->GetCell(8) = TriMesh::Cell(0, 7, 3);
  mesh->GetCell(9) = TriMesh::Cell(0, 4, 7);
  mesh->GetCell(10) = TriMesh::Cell(1, 2, 6);
  mesh->GetCell(11) = TriMesh::Cell(1, 6, 5);

  return mesh;
}

TriMesh::Ptr MakeSphere(const glm::vec3& center, float radius,
                        size_t nSlices, size_t nStacks) {
  assert(nStacks > 2 && nSlices > 3);

  size_t nNodes = 2 + (nStacks - 1) * nSlices;
  size_t nTriangles = 2 * nSlices + 2 * (nStacks - 2) * nSlices;
  TriMesh::Ptr mesh = TriMesh::Create(nNodes, nTriangles);

  size_t iNode = 0;
  // Add point of (u,v) = (-pi/2,0).
  mesh->GetNode(iNode++) = center + glm::vec3(0, 0, -radius);
  for (size_t iStack = 0; iStack < nStacks - 1; ++iStack) {
    double u = (iStack + 1) * M_PI / nStacks - M_PI_2;
    double cos_u = std::cos(u);
    double sin_u = std::sin(u);

    for (size_t iSlice = 0; iSlice < nSlices; ++iSlice) {
      double v = 2.0 * M_PI / nSlices * iSlice;
      double cos_v = std::cos(v);
      double sin_v = std::sin(v);

      glm::vec3 p(radius * cos_u * cos_v, radius * cos_u * sin_v,
                          radius * sin_u);
      mesh->GetNode(iNode++) = center + p;
    }
  }
  // Add point of (u,v) = (pi/2,0).
  mesh->GetNode(iNode++) = center + glm::vec3(0, 0, +radius);
  assert(iNode == nNodes);

  size_t iTria = 0;

  // Triangles at the south pole.
  for (size_t i = 0; i < nSlices; ++i)
    mesh->GetCell(iTria++) = TriMesh::Cell(0, 1 + (i + 1) % nSlices, i + 1);

  // Triangles at the internal stacks.
  for (size_t iStack = 1; iStack < nStacks - 1; ++iStack) {
    size_t downStartNode = 1 + (iStack - 1) * nSlices;
    size_t upStartNode = 1 + iStack * nSlices;
    for (size_t iSlice = 0; iSlice < nSlices; ++iSlice) {
      size_t p00 = downStartNode + iSlice;
      size_t p01 = downStartNode + (iSlice + 1) % nSlices;
      size_t p10 = upStartNode + iSlice;
      size_t p11 = upStartNode + (iSlice + 1) % nSlices;
      mesh->GetCell(iTria++) = TriMesh::Cell(p00, p01, p11);
      mesh->GetCell(iTria++) = TriMesh::Cell(p00, p11, p10);
    }
  }

  // Triangles at the north pole.
  size_t poleNode = nNodes - 1;
  size_t stackStartNode = nNodes - 1 - nSlices;
  for (size_t i = 0; i < nSlices; ++i)
    mesh->GetCell(iTria++) = TriMesh::Cell(poleNode, stackStartNode + i,
                                           stackStartNode + (i + 1) % nSlices);

  assert(iTria == nTriangles);

  return mesh;
}

TriMesh::Ptr MakeCylinder(const glm::vec3& bottom_center,
                          const glm::vec3& top_center, float radius,
                          size_t segments) {
  if (segments < 3) segments = 3;
  glm::vec3 axis = top_center - bottom_center;
  float height = glm::length(axis);
  if (height < 1e-6f) return TriMesh::Ptr();
  glm::vec3 dir = axis / height;
  glm::vec3 u, v;
  if (fabs(dir.x) < 0.9f) {
    u = glm::normalize(glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f)));
  } else {
    u = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
  v = glm::normalize(glm::cross(u, dir));

  TriMesh::Ptr mesh = TriMesh::Create(2 * segments, 2 * segments);

  float angleStep = 2.0f * glm::pi<float>() / segments;
  size_t node_index = 0;
  for (int i = 0; i < segments; ++i) {
      float angle = i * angleStep;
      glm::vec3 offset = radius * (std::cos(angle) * u + std::sin(angle) * v);
      mesh->GetNode(node_index++) = bottom_center + offset;
      mesh->GetNode(node_index++) = top_center + offset;
  }

  size_t cell_index = 0;
  for (int i = 0; i < segments; ++i) {
      int i_next = (i + 1) % segments;
      unsigned int low0 = 2 * i;
      unsigned int top0 = 2 * i + 1;
      unsigned int low1 = 2 * i_next;
      unsigned int top1 = 2 * i_next + 1;
      // Треугольник 1: (нижняя текущая, верхняя текущая, нижняя следующая)
      mesh->GetCell(cell_index++) = TriMesh::Cell(low0, top0, low1);
      mesh->GetCell(cell_index++) = TriMesh::Cell(top0, top1, low1);
  }

  return mesh;
}

TriMesh::Ptr MakeCone(const glm::vec3& baseCenter,
                      const glm::vec3& apex,
                      float radius, size_t segments) {
  if (segments < 3) segments = 3;

  glm::vec3 axis = apex - baseCenter;
  float height = glm::length(axis);
  if (height < 1e-6f) return TriMesh::Ptr();

  glm::vec3 dir = axis / height;

  // `u`, `v` -- это два ортогональных векторов, перпендикулярных `dir`.
  glm::vec3 u, v;
  if (fabs(dir.x) < 0.9f) {
    u = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
  } else {
    u = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
  }
  u = glm::normalize(u);
  v = glm::normalize(glm::cross(dir, u));

  TriMesh::Ptr mesh = TriMesh::Create(segments + 1, segments);

  // Генерация точек на окружности основания
  float angleStep = 2.0f * glm::pi<float>() / segments;
  mesh->GetNode(0) = apex;

  for (size_t i = 0; i < segments; ++i) {
    float angle = i * angleStep;
    glm::vec3 offset = radius * (std::cos(angle) * u + std::sin(angle) * v);
    mesh->GetNode(i + 1) = baseCenter + offset;
  }

  for (size_t i = 0; i < segments; ++i) {
    size_t i1 = 1 + i;
    size_t i2 = 1 + ((i + 1) % segments);
    mesh->GetCell(i) = TriMesh::Cell(0, i1, i2);
  }
  return mesh;
}
