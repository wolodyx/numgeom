#include "numgeom/gizmoobserver.h"

GizmoObserver::GizmoObserver() {}

GizmoObserver::~GizmoObserver() {}

void GizmoObserver::Enable(bool enabled) {
  enabled_ = enabled;
}

bool GizmoObserver::Enabled() const { return enabled_; }
