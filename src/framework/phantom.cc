#include "numgeom/phantom.h"

#include "numgeom/drawableimplementations.h"
#include "numgeom/iteratorimpl.hpp"

Phantom::Phantom(Scene* scene) : SceneObject(scene) {
  drawable0_ = this->AddDrawable<Drawable0_Cached>();
  drawable1_ = this->AddDrawable<Drawable1_Cached>();
  drawable2_ = this->AddDrawable<Drawable2_Cached>();
  drawable0_->SetColor(139, 128, 0);
  drawable1_->SetColor(139, 128, 0);
  drawable2_->SetColor(139, 128, 0);
}

Phantom::~Phantom() {}

void Phantom::ClearData() {
  drawable0_->ClearPoints();
  drawable0_->ClearCells();
  drawable1_->ClearPoints();
  drawable1_->ClearCells();
  drawable2_->ClearPoints();
  drawable2_->ClearCells();
}

void Phantom::SetTransformMatrix(const glm::mat4x4& m) {
  auto transform_function =
      [m](const glm::vec3& v) {
        return glm::vec3(glm::vec4(v,1.0f) * m);
      };
  drawable0_->SetTransformFunction(transform_function);
  drawable1_->SetTransformFunction(transform_function);
  drawable2_->SetTransformFunction(transform_function);
}

Drawable0_Cached* Phantom::GetPointCloud() const { return drawable0_; }

Drawable1_Cached* Phantom::GetWireframe() const { return drawable1_; }

Drawable2_Cached* Phantom::GetSurface() const { return drawable2_; }
