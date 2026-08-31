#include "gizmowidget.h"

#include <cassert>

#include "numgeom/gizmo.h"
#include "numgeom/gizmoobserver.h"

namespace {
class GizmoObserver_GizmoWidget : public GizmoObserver {
 public:
  GizmoObserver_GizmoWidget(GizmoWidget* widget) : widget_(widget) {}
  void ContentUpdated(Gizmo* gizmo) override {
    assert(widget_->GetGizmo() == gizmo);
    widget_->UpdateContent();
  }
  void Activated(Gizmo* gizmo) override {
    assert(widget_->GetGizmo() == gizmo);
    widget_->GizmoActivated();
  }
  void Enabled(Gizmo* gizmo, bool enabled) override {
    assert(widget_->GetGizmo() == gizmo);
    widget_->GizmoEnabled(enabled);
  }

 private:
  GizmoWidget* widget_;
};
}

GizmoWidget::GizmoWidget(Gizmo* gizmo, QWidget* parent) : QGroupBox(parent) {
  gizmo_ = gizmo;
  observer_ = new GizmoObserver_GizmoWidget(this);
  gizmo_->Attach(observer_);
  this->setTitle(QString::fromStdString(gizmo->GetTitle()));
}

GizmoWidget::~GizmoWidget() {
  gizmo_->Detach(observer_);
  delete observer_;
}

Gizmo* GizmoWidget::GetGizmo() { return gizmo_; }

void GizmoWidget::EnableNotification(bool enable) {
  observer_->Enable(enable);
}

void GizmoWidget::GizmoEnabled(bool enable) {
  this->setEnabled(enable);
}
