#include "intersection.h"

#include "numgeom/drawable.h"
#include "numgeom/ray.h"

#include "Mathematics/DistRay3Triangle3.h"
#include "Mathematics/DistRaySegment.h"
#include "Mathematics/DistPointRay.h"
#include "Mathematics/DistPointSegment.h"
#include "Mathematics/IntrLine3Plane3.h"

using gte::Ray3;
using gte::Segment3;
using gte::Triangle3;
using gte::Vector3;
using RTQuery = gte::DCPQuery<double, Ray3<double>, Triangle3<double>>;
using PTQuery = gte::DCPQuery<double, Vector3<double>, Triangle3<double>>;
using RSQuery = gte::DCPQuery<double, Ray3<double>, Segment3<double>>;
using PRQuery = gte::DCPQuery<double, Vector3<double>, Ray3<double>>;
using PSQuery = gte::DCPQuery<double, Vector3<double>, Segment3<double>>;

namespace {
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
}

glm::vec3 IntersectRayWithDrawable(const Ray& ray1, const Drawable2* drawable) {
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
  Ray3<double> ray(
      Vector3<double>({ray1.origin.x, ray1.origin.y, ray1.origin.z}),
      Vector3<double>({ray1.direction.x, ray1.direction.y, ray1.direction.z})
  );
  RTQuery rtQuery{};
  RayProximity result;
  for (glm::u32vec3 t : cells) {
    glm::vec3 p1 = verts[t.x];
    glm::vec3 p2 = verts[t.y];
    glm::vec3 p3 = verts[t.z];
    Triangle3<double> triangle{
        Vector3<double>({p1.x, p1.y, p1.z}),
        Vector3<double>({p2.x, p2.y, p2.z}),
        Vector3<double>({p3.x, p3.y, p3.z})};
    auto rtResult = rtQuery(ray, triangle);

    if (rtResult.parameter <= 0.0)
      continue;

    if (rtResult.sqrDistance < result.dist2
        || rtResult.sqrDistance == result.dist2 && rtResult.parameter < result.tRay) {
      result.dist2 = rtResult.sqrDistance;
      result.tRay = rtResult.parameter;
    }
  }
  return ray1.origin + ray1.direction * result.tRay;
}
