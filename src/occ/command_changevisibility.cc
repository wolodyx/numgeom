#include "command_changevisibility.h"

#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"
#include "numgeom/userinputcontroller.h"

#include "workbench_occ.h"

Command_ChangeVisibility::Command_ChangeVisibility(Workbench* workbench)
    : Command_OCC(workbench) {
}

Command_ChangeVisibility::~Command_ChangeVisibility() {
}

bool Command_ChangeVisibility::Execute() {
  auto workbench = this->GetWorkbench();
  auto uic = workbench->GetUserInputController();
  bool has_selected = false;
  for (Drawable* obj : uic->SelectedObjects()) {
    bool is_visible = obj->IsVisible();
    obj->SetVisibility(!is_visible);
    has_selected = true;
  }
  if (!has_selected) {
    Scene* scene = workbench->GetScene();
    for (SceneObject* so : scene->Objects()) {
      so->SetVisible();
      for (Drawable* d : so->Drawables()) {
        d->SetVisibility();
      }
    }
  }
  uic->ClearSelections();
  return true;
}
