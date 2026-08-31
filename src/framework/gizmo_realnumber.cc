#include "numgeom/gizmo_realnumber.h"

Gizmo_RealNumber::Gizmo_RealNumber(Command* command, const std::string& title,
                                   double init_value)
    : Gizmo(command, title) {
  init_value_ = init_value;
  number_ = init_value;
}

Gizmo_RealNumber::~Gizmo_RealNumber() {
}

double Gizmo_RealNumber::GetNumber() const {
  return number_;
}

bool Gizmo_RealNumber::SetNumber(double v) {
  number_ = v;
  this->NotifyAboutContentChanges();
  return this->IsValid();
}

void Gizmo_RealNumber::SetFilter(std::function<bool(double)> filter) {
  filter_ = filter;
}

bool Gizmo_RealNumber::IsValid() const {
  if (!filter_)
    return true;
  return filter_(number_);
}

void Gizmo_RealNumber::SetSpan(double min, double max) {
  if (min > max)
    std::swap(min, max);
  min_ = min;
  max_ = max;
}

double Gizmo_RealNumber::GetMin() const {
  return min_;
}

double Gizmo_RealNumber::GetMax() const {
  return max_;
}

void Gizmo_RealNumber::Reset() {
  number_ = init_value_;
  this->NotifyAboutContentChanges();
}
