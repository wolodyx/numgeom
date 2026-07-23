#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "numgeom/framework_enums.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/trimesh.h"

class QWindow;

class ApplicationImpl;
class FgObject;
class FgText;
class Scene;
class SceneObject;
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

  FgObject* AddFgImage(const std::string& image_filename);
  FgObject* AddFgImage(const unsigned char* image_data, size_t image_data_size);
  FgText* AddFgText(const std::string& text);
  bool Remove(Scene*, const FgObject*);

  VkSceneRenderer* GetRenderer();

  SampleCount GetSampleCount() const;
  bool SetSampleCount(SampleCount);

  void Sync();

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  ApplicationImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_APPLICATION_H
