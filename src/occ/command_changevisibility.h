#ifndef NUMGEOM_OCC_COMMAND_CHANGEVISIBILITY_H
#define NUMGEOM_OCC_COMMAND_CHANGEVISIBILITY_H

#include "command_occ.h"

class Command_ChangeVisibility : public Command_OCC {
 public:
  Command_ChangeVisibility(Workbench*);
  ~Command_ChangeVisibility();
  bool Execute() override;
};
#endif // !NUMGEOM_OCC_COMMAND_CHANGEVISIBILITY_H
