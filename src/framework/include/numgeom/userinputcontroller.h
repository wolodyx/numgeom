#ifndef numgeom_framework_userinputcontroller_h
#define numgeom_framework_userinputcontroller_h

#include "glm/glm.hpp"

class Application;
class Scene;

/**
\class UserInputController
\brief Контроллер ввода пользователя.

Контроллер преобразует ввод пользователя в перемещение камеры и подсветку
объектов в сцене.
 */
class UserInputController {
 public:
  UserInputController(Application*, Scene*);

  ~UserInputController();

  void KeyPressed(int key);

  void KeyReleased(int key);

  void MouseLeftButtonDown(int x, int y);

  void MouseLeftButtonUp(int x, int y);

  void MouseMiddleButtonDown(int x, int y);

  void MouseMiddleButtonUp(int x, int y);

  void MouseRightButtonDown(int x, int y);

  void MouseRightButtonUp(int x, int y);

  void MouseMove(int x, int y);

  void MouseWheelRotate(int count);

 private:
  UserInputController(const UserInputController&) = delete;
  UserInputController& operator=(const UserInputController&) = delete;

 private:
  struct Impl;
  Impl* impl_;
};

#endif  // !numgeom_framework_userinputcontroller_h
