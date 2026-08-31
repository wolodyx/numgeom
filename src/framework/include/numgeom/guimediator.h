#ifndef NUMGEOM_FRAMEWORK_GUIMEDIATOR_H
#define NUMGEOM_FRAMEWORK_GUIMEDIATOR_H

#include "numgeom/framework_export.h"

class Outcome;

//! Посредник, передающий инструкции графическому интерфейсу от фреймворка.
class FRAMEWORK_EXPORT GuiMediator {
 public:
  GuiMediator();
  virtual ~GuiMediator();
  virtual void EnableCommandMode() = 0;
  virtual void DisableCommandMode() = 0;
  virtual void ShowMessage(const Outcome&) = 0;
};
#endif // !NUMGEOM_FRAMEWORK_GUIMEDIATOR_H
