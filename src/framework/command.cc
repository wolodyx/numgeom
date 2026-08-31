#include "numgeom/command.h"

#include "numgeom/gizmo.h"
#include "numgeom/iteratorimpl.hpp"
#include "numgeom/phantom.h"
#include "numgeom/workbench.h"

Command::Command(Workbench* workbench) {
  workbench_ = workbench;
  phantom_ = new Phantom(workbench_->GetScene());
}

Command::~Command() {
  delete phantom_;
}

void Command::Setup() {
}

void Command::Cleanup() {
}

Iterator<Gizmo*> Command::GetGizmos() {
  auto it_impl = new IteratorImpl_StdIterator<std::list<Gizmo*>::const_iterator>(
      gizmos_.begin(), gizmos_.end());
  return Iterator<Gizmo*>(it_impl);
}

Workbench* Command::GetWorkbench() {
  return workbench_;
}

Scene* Command::GetScene() {
  return workbench_->GetScene();
}

void Command::Register(Gizmo* gizmo) {
  if (!gizmo)
    return;
  gizmos_.push_back(gizmo);
}

void Command::Recover() {
  for (Gizmo* gizmo : gizmos_) {
    gizmo->Reset();
  }
}

Phantom* Command::GetPhantom() {
  return phantom_;
}

void Command::UpdateScreen() {
  auto wb = this->GetWorkbench();
  wb->UpdateScreen();
}
