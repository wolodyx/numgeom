#ifndef NUMGEOM_FRAMEWORK_COMMAND_H
#define NUMGEOM_FRAMEWORK_COMMAND_H

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"

class SceneWidget;
class Workbench;

class FRAMEWORK_EXPORT Command {
 public:
  Command(Workbench*);
  virtual ~Command();
  virtual bool Execute() = 0;
  virtual void Recover() {}
  void Setup();
  void Cleanup();
  Iterator<SceneWidget*> GetWidgets();
  bool Activate(SceneWidget*);

 protected:
  Workbench* GetWorkbench();

 private:
  Workbench* workbench_;
};
#endif // !NUMGEOM_FRAMEWORK_COMMAND_H
