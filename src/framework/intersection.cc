#include "intersection.h"

#include <cmath>
#include <limits>

#include "numgeom/drawable.h"
#include "numgeom/ray.h"

namespace {
//! Квадрат дистанции от точки до отрезка.
//! @param p Точка.
//! @param a Начало отрезка.
//! @param b Конец отрезка.
//! @param u Параметр ближайшей точки на отрезке.
//! @return Квадрат дистанции.
float SqDistPointSegment(const glm::vec3& p, const glm::vec3& a,
                         const glm::vec3& b, float* u) {
  const float kEpsilon = 1e-12f;
  glm::vec3 ab = b - a;
  glm::vec3 ap = p - a;
  float denom = glm::dot(ab, ab);
  float d = glm::dot(ap, ab);
  if (denom <= kEpsilon) {
    *u = 0.0f;
    return glm::dot(ap, ap);
  }
  if (d <= 0.0f) {
    *u = 0.0f;
    return glm::dot(ap, ap);
  }
  if (d >= denom) {
    *u = 1.0f;
    glm::vec3 bp = p - b;
    return glm::dot(bp, bp);
  }
  *u = d / denom;
  glm::vec3 closest = a + ab * (*u);
  glm::vec3 diff = p - closest;
  return glm::dot(diff, diff);
}

//! Квадрат дистанции от луча до отрезка.
//! @param ray Луч.
//! @param a Начало отрезка.
//! @param b Конец отрезка.
//! @param sRay Параметр ближайшей точки на луче.
//! @return Квадрат дистанции.
float SqDistRaySegment(const Ray& ray, const glm::vec3& a, const glm::vec3& b,
                       float* sRay) {
  const float kEpsilon = 1e-12f;
  glm::vec3 d1 = ray.direction;
  glm::vec3 d2 = b - a;
  glm::vec3 r = ray.origin - a;

  float a11 = glm::dot(d1, d1);
  float a22 = glm::dot(d2, d2);
  float a12 = glm::dot(d1, d2);
  float b1 = glm::dot(r, d1);
  float b2 = glm::dot(r, d2);

  float s = 0.0f;  // Параметр на луче.
  float u = 0.0f;  // Параметр на отрезке.

  if (a11 <= kEpsilon && a22 <= kEpsilon) {
    s = 0.0f;
    u = 0.0f;
  } else if (a11 <= kEpsilon) {
    s = 0.0f;
    u = b2 / a22;
    u = (u < 0.0f) ? 0.0f : (u > 1.0f ? 1.0f : u);
  } else {
    float c = glm::dot(d1, r);
    if (a22 <= kEpsilon) {
      u = 0.0f;
      s = -c / a11;
      if (s < 0.0f) s = 0.0f;
    } else {
      float denom = a11 * a22 - a12 * a12;
      if (denom > kEpsilon) {
        s = (a12 * b2 - a22 * b1) / denom;
        if (s < 0.0f) s = 0.0f;
      } else {
        s = 0.0f;
      }
      u = (a12 * s + b2) / a22;
      if (u < 0.0f) {
        u = 0.0f;
        s = -c / a11;
        if (s < 0.0f) s = 0.0f;
      } else if (u > 1.0f) {
        u = 1.0f;
        s = (a12 - c) / a11;
        if (s < 0.0f) s = 0.0f;
      }
    }
  }

  glm::vec3 closestRay = ray.origin + d1 * s;
  glm::vec3 closestSeg = a + d2 * u;
  glm::vec3 diff = closestRay - closestSeg;
  *sRay = s;
  return glm::dot(diff, diff);
}

//! Ближайшая точка луча к треугольнику, если луч его не пересекает.
//! @param ray Луч.
//! @param pts Координаты вершин треугольника.
//! @param t Параметр ближайшей точки на луче.
//! @return Квадрат дистанции от луча до треугольника.
float ClosestOnMiss(const Ray& ray, glm::vec3* pts, float* t) {
  float bestDist2 = std::numeric_limits<float>::max();
  float bestT = 0.0f;

  // Дистанция от луча до вершин треугольника.
  for (int i = 0; i < 3; ++i) {
    float s = glm::dot(pts[i] - ray.origin, ray.direction);
    if (s < 0.0f) s = 0.0f;
    glm::vec3 closest = ray.origin + ray.direction * s;
    glm::vec3 diff = closest - pts[i];
    float d2 = glm::dot(diff, diff);
    if (d2 < bestDist2) {
      bestDist2 = d2;
      bestT = s;
    }
  }

  // Дистанция от луча до рёбер треугольника.
  for (int i = 0; i < 3; ++i) {
    int j = (i + 1) % 3;
    float s;
    float d2 = SqDistRaySegment(ray, pts[i], pts[j], &s);
    if (d2 < bestDist2) {
      bestDist2 = d2;
      bestT = s;
    }
  }

  *t = bestT;
  return bestDist2;
}

struct RayProximity
{
    RayProximity()
    {
        tRay = std::numeric_limits<float>::max();
        dist2 = std::numeric_limits<float>::max();
    }

    RayProximity(float t, float d2)
    {
        tRay = t;
        dist2 = d2;
    }

    float tRay;
    float dist2;
};

//! Пересечение луча с треугольником.
//! @param ray Луч.
//! @param pts Координаты вершин треугольника.
//! @param t Параметр точки пересечения луча с треугольником.
//! @return Квадрат дистанции от луча треугольника (если пересекаются, то 0).
float Intersect(const Ray& ray, glm::vec3* pts, float* t) {
  const float kEpsilon = 1e-12f;

  glm::vec3 edge0 = pts[1] - pts[0];
  glm::vec3 edge1 = pts[2] - pts[0];
  glm::vec3 pvec = glm::cross(ray.direction, edge1);
  float det = glm::dot(edge0, pvec);

  // Луч параллелен плоскости треугольника.
  if (std::fabs(det) < kEpsilon)
    return ClosestOnMiss(ray, pts, t);

  float inv_det = 1.0f / det;
  glm::vec3 tvec = ray.origin - pts[0];
  float u = inv_det * glm::dot(tvec, pvec);
  if (u < 0.0f || u > 1.0f)
    return ClosestOnMiss(ray, pts, t);

  glm::vec3 qvec = glm::cross(tvec, edge0);
  float v = inv_det * glm::dot(ray.direction, qvec);
  if (v < 0.0f || u + v > 1.0f)
    return ClosestOnMiss(ray, pts, t);

  float param = inv_det * glm::dot(edge1, qvec);
  if (param < 0.0f)
    return ClosestOnMiss(ray, pts, t);

  *t = param;
  return 0.0f;
}
}

glm::vec3 IntersectRayWithDrawable(const Ray& ray, const Drawable2* drawable) {
  // Подготавливаем массивы вершин и треугольников.
  std::vector<glm::vec3> verts(drawable->GetVertsCount());
  auto it_v = verts.begin();
  for (glm::vec3 v : drawable->GetVertices())
    (*it_v++) = v;
  std::vector<glm::u32vec3> cells(drawable->GetCellsCount());
  auto it_t = cells.begin();
  for (glm::u32vec3 t : drawable->GetTriangles())
    (*it_t++) = t;

  // Вычисляем пересечение луча с объектом.
  RayProximity result;
  glm::vec3 tr[3];
  for (glm::u32vec3 t : cells) {
    tr[0] = verts[t.x];
    tr[1] = verts[t.y];
    tr[2] = verts[t.z];
    float parameter;
    float sqrDistance = Intersect(ray, tr, &parameter);
    if (parameter <= 0.0)
      continue;
    if (sqrDistance < result.dist2
        || sqrDistance == result.dist2 && parameter < result.tRay) {
      result.dist2 = sqrDistance;
      result.tRay = parameter;
    }
  }
  return ray.origin + ray.direction * result.tRay;
}
