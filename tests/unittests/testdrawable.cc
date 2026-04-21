#include "gtest/gtest.h"

#include "numgeom/drawable_sphere.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"

template<typename DrawableType>
class SceneObject_WithDrawable : public SceneObject {
 public:
   template<class... _Types>
   SceneObject_WithDrawable(Scene* scene, _Types&&... _Args) : SceneObject(scene) {
    this->AddDrawable<DrawableType>(_Args...);
  }
  virtual ~SceneObject_WithDrawable() {}
};

template<typename DrawableType, class... _Types>
Drawable* AddDrawable(Scene& scene, _Types&&... _Args) {
  auto o = scene.AddObject<SceneObject_WithDrawable<DrawableType>>(_Args...);
  return *o->Drawables();
}

TEST(Drawable, DrawableSphereGetVertices) {
  Scene scene;
  const glm::vec3 center(0.0, 0.0, 0.0);
  Drawable* d = AddDrawable<Drawable2_Sphere>(scene, center, 1.0f, 10, 10);
  EXPECT_EQ(d->GetVertsCount(), 92);
  EXPECT_EQ(d->GetCellsCount(), 180);
  AlignedBoundBox boxCalculatedFromVerts;
  for (auto pt : d->GetVertices())
    boxCalculatedFromVerts.Expand(pt);
  // Реализация Drawable2_Sphere::GetBoundBox использует центр сферы и радиус
  // для вычисления габаритной коробки. В то время как коробка, вычисленная по
  // вершинам основывается на дискретности сферы и дает различие около 0.05.
  // С увеличением граней, различие будет приближаться к нулю.
  EXPECT_TRUE(boxCalculatedFromVerts.IsEqual(d->GetBoundBox(),0.05));
}

TEST(Drawable, DrawableSphereGetNormals) {
  Scene scene;
  const glm::vec3 center(0.0, 0.0, 0.0);
  Drawable* d = AddDrawable<Drawable2_Sphere>(scene, center, 1.0f, 10, 10);
  auto d2 = Drawable2::Cast(d);
  auto it_normal = d2->GetNormals();
  auto it_vertex = d2->GetVertices();
  for (auto n : it_normal) {
    glm::vec3 pt = (*it_vertex++);
    glm::vec3 n2 = glm::normalize(pt - center);
    ASSERT_TRUE(glm::length(n - n2) < 1.0e-6);
  }
}

TEST(Drawable, DrawableSphereGetTriangles) {
  Scene scene;
  const glm::vec3 center(0.0, 0.0, 0.0);
  Drawable* d = AddDrawable<Drawable2_Sphere>(scene, center, 1.0f, 10, 10);
  Drawable2* d2 = Drawable2::Cast(d);
  for (auto tr : d2->GetTriangles()) {
    ASSERT_TRUE(tr.x != tr.y && tr.x != tr.z && tr.y != tr.z);
    ASSERT_LT(tr.x, d2->GetCellsCount());
    ASSERT_LT(tr.y, d2->GetCellsCount());
    ASSERT_LT(tr.z, d2->GetCellsCount());
  }
}
