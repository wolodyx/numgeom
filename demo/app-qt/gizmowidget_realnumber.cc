#include "gizmowidget_realnumber.h"

#include "qboxlayout.h"
#include "qlineedit.h"
#include "qslider.h"

#include "numgeom/gizmo_realnumber.h"

class GizmoWidget_RealNumber::Ui {
 public:
  Ui(QWidget* parent);
  QSlider* slider;
  QLineEdit* leCoeff;
};

GizmoWidget_RealNumber::Ui::Ui(QWidget* parent) {
  slider = new QSlider(Qt::Horizontal, parent);
  slider->setMinimum(0);
  slider->setMaximum(1000);
  slider->setSliderPosition(500);
  leCoeff = new QLineEdit(parent);
  auto ly = new QBoxLayout(QBoxLayout::LeftToRight, parent);
  ly->addWidget(slider);
  ly->addWidget(leCoeff);
  parent->setLayout(ly);
}

GizmoWidget_RealNumber::GizmoWidget_RealNumber(Gizmo_RealNumber* gizmo,
                                               QWidget* parent)
    : GizmoWidget(gizmo, parent) {
  ui_ = new Ui(this);
  connect(ui_->slider, &QSlider::valueChanged, this,
          &GizmoWidget_RealNumber::onSliderValueChanged);
  ui_->leCoeff->setText(QString::number(gizmo->GetNumber()));
  connect(ui_->leCoeff, &QLineEdit::textChanged, this,
          &GizmoWidget_RealNumber::onValueChanged);
}

GizmoWidget_RealNumber::~GizmoWidget_RealNumber() {
  delete ui_;
}

void GizmoWidget_RealNumber::onSliderValueChanged(int value) {
  Gizmo_RealNumber* gizmo = dynamic_cast<Gizmo_RealNumber*>(this->GetGizmo());
  double t = value / 1000.0;
  double v = gizmo->GetMin() + (gizmo->GetMax() - gizmo->GetMin()) * t;
  bool is_valid_value = gizmo->SetNumber(v);
  this->SetInvalid(!is_valid_value);
  ui_->leCoeff->setText(QString::number(gizmo->GetNumber()));
}

void GizmoWidget_RealNumber::onValueChanged(const QString& text) {
  bool ok;
  double v = text.toDouble(&ok);
  if (!ok) {
    this->SetInvalid();
    return;
  }
  Gizmo_RealNumber* gizmo = dynamic_cast<Gizmo_RealNumber*>(this->GetGizmo());
  bool is_valid_value = gizmo->SetNumber(v);
  this->SetInvalid(!is_valid_value);
  if (is_valid_value) {
    double span = gizmo->GetMax() - gizmo->GetMin();
    int pos = span != 0.0
                  ? static_cast<int>((gizmo->GetNumber() - gizmo->GetMin()) /
                                     span * 1000.0)
                  : 0;
    pos = qBound(0, pos, 1000);
    ui_->slider->setSliderPosition(pos);
  }
}

void GizmoWidget_RealNumber::SetInvalid(bool invalid) {
  if (!invalid) {
    ui_->leCoeff->setStyleSheet("");
    return;
  }
  ui_->leCoeff->setStyleSheet("color: red;");
}

void GizmoWidget_RealNumber::UpdateContent() {
  Gizmo_RealNumber* gizmo = dynamic_cast<Gizmo_RealNumber*>(this->GetGizmo());
  ui_->leCoeff->setText(QString::number(gizmo->GetNumber()));
}
