#include "numgeom/gizmo.h"

#include "numgeom/command.h"
#include "numgeom/gizmoobserver.h"
#include "numgeom/workbench.h"

Gizmo::Gizmo(Command* command, const std::string& title)
    : SceneWidget(command->GetScene()) {
  command_ = command;
  title_ = title;
  command_->Register(this);
}

Gizmo::~Gizmo() {
}

void Gizmo::NotifyAboutContentChanges() {
  if (!this->IsEnabled())
    return;
  for (auto o : observers_) {
    if (o->Enabled())
      o->ContentUpdated(this);
  }
  command_->Updated(this);
}

void Gizmo::NotifyAboutEnabling() {
//   if (!this->IsEnabled())
//     return;
  for (auto o : observers_) {
    if (o->Enabled())
      o->Enabled(this, enabled_);
  }
  command_->Updated(this);
}

void Gizmo::Attach(GizmoObserver* o) {
  observers_.push_back(o);
}

void Gizmo::Detach(GizmoObserver* o) {
  auto it = std::find(observers_.begin(), observers_.end(), o);
  if (it != observers_.end())
    observers_.erase(it);
}

void Gizmo::UpdateScene() {
  Workbench* workbench = command_->GetWorkbench();
  workbench->UpdateScreen();
}

void Gizmo::SetEnabled(bool enabled) {
  enabled_ = enabled;
  this->SetVisible(enabled);
  this->NotifyAboutEnabling();
}

bool Gizmo::IsEnabled() const { return enabled_; }

void Gizmo::SetCharacteristicSize(float size) {
  if (characteristic_size_ == size)
    return;
  characteristic_size_ = size;
  this->UpdateSceneObject();
  this->SetDirty();
}

float Gizmo::GetCharacteristicSize() const { return characteristic_size_; }
