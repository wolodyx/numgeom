#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "numgeom/framework_export.h"
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

  /** \brief Установка логотипа приложения.
  \param image_filename Имя файла с изображением логотипа.
  \param screen_position Позиция левого верхнего угла логотипа на экране.
  */
  void SetLogo(const std::string& image_filename,
               const glm::ivec2& screen_position);
  void SetLogo(const unsigned char* image_data, size_t image_data_size,
               const glm::ivec2& screen_position);

  //! Добавление текста на передний план сцены.
  ScreenText* SetText(const std::string& text);

  void AddAxisIndicator();

  //!@{
  //! Методы для управления рисованием.

  void Update();
  //!@}

  //!@{
  //! Взаимодействие со сценой.

  const Scene* GetActiveScene() const;
  Scene* GetActiveScene();

  const Scene* GetForegroundScene() const;
  Scene* GetForegroundScene();

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
