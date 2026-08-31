#ifndef NUMGEOM_FRAMEWORK_GIZMO_REALNUMBER_H
#define NUMGEOM_FRAMEWORK_GIZMO_REALNUMBER_H

#include <functional>

#include "numgeom/gizmo.h"

class FRAMEWORK_EXPORT Gizmo_RealNumber : public Gizmo {
 public:
  Gizmo_RealNumber(Command*, const std::string& title, double init_value = 0.0);
  virtual ~Gizmo_RealNumber();
  void Reset() override;

  double GetNumber() const;
  bool SetNumber(double);
  void SetFilter(std::function<bool(double)>);
  bool IsValid() const;
  void SetSpan(double min, double max);
  double GetMin() const;
  double GetMax() const;


 private:
  double number_ = 0.0;
  double init_value_ = 0.0;
  std::function<bool(double)> filter_;
  double min_ = 0.0;
  double max_ = 10.0;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMO_REALNUMBER_H
