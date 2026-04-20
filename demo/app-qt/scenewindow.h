#ifndef NUMGEOM_EXAMPLES_APPQT_SCENEWINDOW_H_
#define NUMGEOM_EXAMPLES_APPQT_SCENEWINDOW_H_

#include "qwindow.h"

#include "numgeom/trimesh.h"

class Application;
class GpuManager;
class UserInputController;

/** \class SceneWindow
\brief Окно графического приложения, в котором отображается сцена.
*/
class SceneWindow : public QWindow {
 public:
  SceneWindow(Application*);
  ~SceneWindow();

 private:
  void keyPressEvent(QKeyEvent*) override;
  void keyReleaseEvent(QKeyEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;

  void resizeEvent(QResizeEvent*) override;
  void exposeEvent(QExposeEvent*) override;
  bool event(QEvent*) override;

 private:
  GpuManager* gpu_manager_;
  UserInputController* user_input_controller_;
};
#endif  // NUMGEOM_EXAMPLES_APPQT_SCENEWINDOW_H_
