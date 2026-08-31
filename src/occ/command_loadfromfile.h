#ifndef NUMGEOM_OCC_COMMAND_LOADFROMFILE_H
#define NUMGEOM_OCC_COMMAND_LOADFROMFILE_H

#include <filesystem>

#include "numgeom/orthobasis.h"

#include "command_occ.h"

class Gizmo_FilePath;
class Gizmo_PlaceIn3d;
class Gizmo_RealNumber;

class Command_LoadFromFile : public Command_OCC {
 public:
  Command_LoadFromFile(Workbench*);
  ~Command_LoadFromFile();
  std::string GetDisplayName() const override { return "Load model from file"; }
  Outcome Execute() override;
  void Recover() override;
  void Updated(Gizmo*) override;

 private:
  std::filesystem::path GetModelFilePath() const;
  glm::mat4x4 GetModelTransformation() const;
  float GetScaleCoeff() const;

 private:
  Gizmo_FilePath* gizmo_1_;
  Gizmo_PlaceIn3d* gizmo_2_;
  Gizmo_RealNumber* gizmo_3_;
};
#endif // !NUMGEOM_OCC_COMMAND_LOADFROMFILE_H
