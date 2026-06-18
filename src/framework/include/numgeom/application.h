#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/trimesh.h"

class QWindow;

class ApplicationImpl;
class FgImage;
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

  bool Update(Scene*);
  //!@}

  //!@{
  //! Взаимодействие со сценами.

  //! Добавляет новую сцену с именем. Если задан `background_scene`,
  //! то создаваемая сцена будет выводиться на переднем плане.
  Scene* AddScene(const std::string&, Scene* background_scene = nullptr);

  bool RemoveScene(Scene*);

  Scene* GetScene(const std::string&);
  const Scene* GetScene(const std::string&) const;

  Iterator<Scene*> GetForegroundScenes(Scene*);

  Scene* GetBackgroundScene(Scene*);
  //!@}

  FgImage* AddFgImage(const std::string& image_filename);
  FgImage* AddFgImage(const unsigned char* image_data, size_t image_data_size);
  ScreenText* AddScreenText(const std::string& text);
  bool AddFgImage(Scene*, FgImage*, const glm::ivec2& screen_position);
  bool AddScreenText(Scene*, ScreenText*, const glm::ivec2& screen_position);
  bool Remove(Scene*, const ScreenText*);
  bool Remove(Scene*, const FgImage*);

  VkSceneRenderer* GetRenderer();

  void Sync();

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  ApplicationImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_APPLICATION_H
