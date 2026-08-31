#ifndef NUMGEOM_OCC_COMMAND_OCC_H
#define NUMGEOM_OCC_COMMAND_OCC_H

#include "numgeom/command.h"

class Workbench_OCC;

class Command_OCC : public Command {
 public:
  Command_OCC(Workbench*);
  virtual ~Command_OCC();

  Workbench_OCC* GetUnderlyingWorkbench();
};
#endif // !NUMGEOM_OCC_COMMAND_OCC_H
