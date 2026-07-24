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

  void keyPressed(int key);

  void keyReleased(int key);

  void mouseLeftButtonDown(int x, int y);

  void mouseLeftButtonUp(int x, int y);

  void mouseMiddleButtonDown(int x, int y);

  void mouseMiddleButtonUp(int x, int y);

  void mouseRightButtonDown(int x, int y);

  void mouseRightButtonUp(int x, int y);

  void mouseMove(int x, int y);

  void mouseWheelRotate(int count);

 private:
  UserInputController(const UserInputController&) = delete;
  UserInputController& operator=(const UserInputController&) = delete;

 private:
  struct Impl;
  Impl* impl_;
};

#endif  // !numgeom_framework_userinputcontroller_h
