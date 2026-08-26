#ifndef NUMGEOM_FRAMEWORK_APPCONTEXT_H
#define NUMGEOM_FRAMEWORK_APPCONTEXT_H

#include <functional>

#include "numgeom/framework_enums.h"
#include "numgeom/framework_export.h"

class Workbench;

class FRAMEWORK_EXPORT AppContext {
 public:

  SampleCount GetSampleCount() const;
  bool SetSampleCount(SampleCount);

  SelectionMode GetSelectionMode() const;
  void SetSelectionMode(SelectionMode);

  void SetContextMenuFunction(std::function<void(Workbench*,int,int)>);
  std::function<void(Workbench*,int,int)> GetContextMenuFunction();

 private:
  SelectionMode selection_mode_ = SelectionMode::Disable;
  std::function<void(Workbench*,int,int)> cm_function_;
};
#endif // !NUMGEOM_FRAMEWORK_APPCONTEXT_H
