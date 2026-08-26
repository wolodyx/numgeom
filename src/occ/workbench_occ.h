#ifndef NUMGEOM_OCC_WORKBENCH_OCC_H
#define NUMGEOM_OCC_WORKBENCH_OCC_H

#include <filesystem>

#include "Poly_Triangulation.hxx"
#include "TDocStd_Document.hxx"

#include "numgeom/workbench.h"

class SceneObject;

class Workbench_OCC : public Workbench {
 public:
  Workbench_OCC();
  ~Workbench_OCC();
  std::string GetName() const override;
  void Setup() override;
  void Cleanup() override;
  bool LoadFromFile(const std::filesystem::path&) override;

  void Clear() override;
  bool Move(Drawable* drawable, const glm::vec3& dir);

 private:
  void Update();

 private:
  Handle(TDocStd_Document) document_;
  SceneObject* so_ = nullptr;
};
#endif // !NUMGEOM_OCC_WORKBENCH_OCC_H
