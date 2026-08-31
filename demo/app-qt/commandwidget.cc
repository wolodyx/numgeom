#include "commandwidget.h"

#include "qboxlayout.h"
#include "qicon.h"
#include "qsize.h"
#include "qtoolbutton.h"

#include "numgeom/application.h"
#include "numgeom/command.h"

#include "numgeom/gizmo_filepath.h"
#include "numgeom/gizmo_placein3d.h"
#include "numgeom/gizmo_realnumber.h"

#include "gizmowidget_filepath.h"
#include "gizmowidget_placein3d.h"
#include "gizmowidget_realnumber.h"

namespace {
GizmoWidget* CreateGizmoWidget(Gizmo* gizmo) {
  if (auto g = dynamic_cast<Gizmo_FilePath*>(gizmo))
    return new GizmoWidget_FilePath(g);
  if (auto g = dynamic_cast<Gizmo_PlaceIn3d*>(gizmo))
    return new GizmoWidget_PlaceIn3d(g);
  if (auto g = dynamic_cast<Gizmo_RealNumber*>(gizmo))
    return new GizmoWidget_RealNumber(g);
  return nullptr;
}
}

CommandWidget::CommandWidget(Command* command, QWidget* parent)
    : QWidget(parent) {
  assert(command != nullptr);
  auto ly = new QBoxLayout(QBoxLayout::TopToBottom, this);
  for (auto gizmo : command->GetGizmos()) {
    auto w = CreateGizmoWidget(gizmo);
    assert(w != nullptr);
    gizmo_widgets_.push_back(w);
    if (0)
      w->setFocus();
    ly->addWidget(w);
  }

  auto* buttons_ly = new QBoxLayout(QBoxLayout::LeftToRight);
  auto* execute_button = new QToolButton(this);
  execute_button->setIcon(
      QIcon(QString::fromUtf8(":/resources/icons/check.ico")));
  execute_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  execute_button->setToolTip(tr("Execute"));
  auto* cancel_button = new QToolButton(this);
  cancel_button->setIcon(
      QIcon(QString::fromUtf8(":/resources/icons/cancel.ico")));
  cancel_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  cancel_button->setToolTip(tr("Cancel"));
  buttons_ly->addWidget(execute_button);
  buttons_ly->addWidget(cancel_button);
  ly->addLayout(buttons_ly);
  connect(execute_button, &QToolButton::clicked, this,
          []() {
            Application* app = Application::Get();
            app->ExecuteCommand();
          });
  connect(cancel_button, &QToolButton::clicked, this,
          []() {
            Application* app = Application::Get();
            app->CancelCommand();
          });

  ly->addStretch(1);
}

CommandWidget::~CommandWidget() {
  for (auto w : gizmo_widgets_)
    delete w;
}
