#ifndef NUMGEOM_APPQT_COMMANDWIDGET_H
#define NUMGEOM_APPQT_COMMANDWIDGET_H

#include <list>

#include "qwidget.h"

class Command;
class GizmoWidget;

class CommandWidget : public QWidget {
  Q_OBJECT;
 public:
   CommandWidget(Command*, QWidget* = nullptr);
   ~CommandWidget();

 private:
  std::list<GizmoWidget*> gizmo_widgets_;
};
#endif // !NUMGEOM_APPQT_COMMANDWIDGET_H
