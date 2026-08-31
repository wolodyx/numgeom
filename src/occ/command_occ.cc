#include "command_occ.h"

#include "workbench_occ.h"

Command_OCC::Command_OCC(Workbench* workbench) : Command(workbench) {
}

Command_OCC::~Command_OCC() {
}

Workbench_OCC* Command_OCC::GetUnderlyingWorkbench() {
  return dynamic_cast<Workbench_OCC*>(this->GetWorkbench());
}
