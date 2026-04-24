#include "numgeom/sceneobject_polytriangulation.h"

#include "numgeom/drawable_polytriangulation.h"

SceneObject_PolyTriangulation::SceneObject_PolyTriangulation(
    Scene* scene,
    Handle(Poly_Triangulation) triangulation) : SceneObject(scene) {
  auto d = this->AddDrawable<Drawable2_PolyTriangulation>(triangulation);
  d->SetColor(153, 149, 140); //< Кварцевый цвет.
}

SceneObject_PolyTriangulation::~SceneObject_PolyTriangulation() {
}
