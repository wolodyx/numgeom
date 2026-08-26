#include "numgeom/command.h"

Command::Command(Workbench* workbench) {
  workbench_ = workbench;
}

Command::~Command() {
}

void Command::Setup() {
}

void Command::Cleanup() {
}

Iterator<SceneWidget*> Command::GetWidgets() {
  return Iterator<SceneWidget*>();
}

bool Command::Activate(SceneWidget*) {
  return false;
}

Workbench* Command::GetWorkbench() {
  return workbench_;
}
