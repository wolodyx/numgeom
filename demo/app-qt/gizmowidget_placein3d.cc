#include "gizmowidget_placein3d.h"

#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qtoolbutton.h"

#include "numgeom/gizmo_placein3d.h"

class GizmoWidget_PlaceIn3d::Ui {
 public:
  Ui(QWidget* parent);
  QLineEdit* pos_x_value;
  QLineEdit* pos_y_value;
  QLineEdit* pos_z_value;
  QCheckBox* pos_x_lock;
  QCheckBox* pos_y_lock;
  QCheckBox* pos_z_lock;
  QToolButton* pos_tool_button;
};

GizmoWidget_PlaceIn3d::Ui::Ui(QWidget* parent) {
  auto xLabel = new QLabel(tr("x"), parent);
  auto yLabel = new QLabel(tr("y"), parent);
  auto zLabel = new QLabel(tr("z"), parent);

  pos_x_value = new QLineEdit(parent);
  pos_y_value = new QLineEdit(parent);
  pos_z_value = new QLineEdit(parent);

  pos_x_lock = new QCheckBox(tr(""), parent);
  pos_y_lock = new QCheckBox(tr(""), parent);
  pos_z_lock = new QCheckBox(tr(""), parent);

  pos_tool_button = new QToolButton(parent);
  pos_tool_button->setPopupMode(QToolButton::MenuButtonPopup);

  auto s = new QBoxLayout(QBoxLayout::LeftToRight, parent);
  s->addWidget(xLabel);
  s->addWidget(pos_x_value);
  s->addWidget(pos_x_lock);
  s->addWidget(yLabel);
  s->addWidget(pos_y_value);
  s->addWidget(pos_y_lock);
  s->addWidget(zLabel);
  s->addWidget(pos_z_value);
  s->addWidget(pos_z_lock);
  s->addWidget(pos_tool_button);

  parent->setLayout(s);
}

GizmoWidget_PlaceIn3d::GizmoWidget_PlaceIn3d(Gizmo_PlaceIn3d* gizmo,
                                             QWidget* parent)
    : GizmoWidget(gizmo, parent) {
  ui_ = new Ui(this);

  auto pos = gizmo->GetPosition();
  ui_->pos_x_value->setText(QString::number(pos.x));
  ui_->pos_y_value->setText(QString::number(pos.y));
  ui_->pos_z_value->setText(QString::number(pos.z));

  connect(ui_->pos_x_value, &QLineEdit::textEdited, this,
          &GizmoWidget_PlaceIn3d::onCoordEdited);
  connect(ui_->pos_y_value, &QLineEdit::textEdited, this,
          &GizmoWidget_PlaceIn3d::onCoordEdited);
  connect(ui_->pos_z_value, &QLineEdit::textEdited, this,
          &GizmoWidget_PlaceIn3d::onCoordEdited);
  connect(ui_->pos_x_value, &QLineEdit::editingFinished, this,
          &GizmoWidget_PlaceIn3d::onCoordEditingFinished);
  connect(ui_->pos_y_value, &QLineEdit::editingFinished, this,
          &GizmoWidget_PlaceIn3d::onCoordEditingFinished);
  connect(ui_->pos_z_value, &QLineEdit::editingFinished, this,
          &GizmoWidget_PlaceIn3d::onCoordEditingFinished);
}

GizmoWidget_PlaceIn3d::~GizmoWidget_PlaceIn3d() {
  delete ui_;
}

void GizmoWidget_PlaceIn3d::UpdateContent() {
  Gizmo_PlaceIn3d* gizmo = dynamic_cast<Gizmo_PlaceIn3d*>(this->GetGizmo());
  auto pos = gizmo->GetPosition();
  ui_->pos_x_value->setText(QString::number(pos.x));
  ui_->pos_y_value->setText(QString::number(pos.y));
  ui_->pos_z_value->setText(QString::number(pos.z));
}

void GizmoWidget_PlaceIn3d::onCoordEdited(const QString& txt) {
  auto* gizmo = dynamic_cast<Gizmo_PlaceIn3d*>(this->GetGizmo());
  this->EnableNotification(false);
  bool ok;
  txt.toDouble(&ok);
   if(!ok)
    return;

  float x = ui_->pos_x_value->text().toFloat();
  float y = ui_->pos_y_value->text().toFloat();
  float z = ui_->pos_z_value->text().toFloat();
  gizmo->SetPosition(glm::vec3{x,y,z});
}

void GizmoWidget_PlaceIn3d::onCoordEditingFinished() {
  this->EnableNotification(true);
}
