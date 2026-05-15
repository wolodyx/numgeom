#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/trimesh.h"

class QWindow;

class Scene;
class SceneObject;
class ScreenText;
class VkSceneRenderer;

/** \class Application
\brief Сущность, в которой содержится все состояние приложения.

Класс Application:
* связывает камеру с событиями от мыши;
* управляет выборкой рисуемых объектов;
* управляет модулями приложения;
* управляет моделями данных;

*/
class FRAMEWORK_EXPORT Application {
 public:
  Application(int argc, char* argv[]);

  ~Application();

  Scene* AddAxisIndicator();

  //!@{
  //! Методы для управления рисованием.

  void Update();
  //!@}

  //!@{
  //! Взаимодействие со сценами.

  //! Добавляет новую сцену с именем. Если задан `background_scene`,
  //! то создаваемая сцена будет выводиться на переднем плане.
  Scene* AddScene(const std::string&, Scene* background_scene = nullptr);

  bool RemoveScene(Scene*);

  Scene* GetScene(const std::string&);
  const Scene* GetScene(const std::string&) const;

  bool SetActiveScene(Scene*);

  const Scene* GetActiveScene() const;
  Scene* GetActiveScene();

  Iterator<Scene*> GetForegroundScenes(Scene*);

  Scene* GetBackgroundScene(Scene*);
  //!@}

  VkSceneRenderer* GetRenderer();

  class Inner;
  Inner* GetInnerInterface();
  const Inner* GetInnerInterface() const;

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  class State;
  State* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_APPLICATION_H
