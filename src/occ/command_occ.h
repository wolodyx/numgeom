#ifndef NUMGEOM_OCC_COMMAND_OCC_H
#define NUMGEOM_OCC_COMMAND_OCC_H

#include "numgeom/command.h"

class Command_OCC : public Command {
 public:
  Command_OCC(Workbench*);
  virtual ~Command_OCC();
};
#endif // !NUMGEOM_OCC_COMMAND_OCC_H
