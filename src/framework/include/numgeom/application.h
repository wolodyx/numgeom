#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "numgeom/framework_export.h"
#include "numgeom/orthobasis.h"
#include "numgeom/trimesh.h"

class QWindow;

class GpuManager;
class Scene;
class SceneObject;

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

  void SetText(const std::string& text, const glm::ivec2& screen_position,
               const glm::vec4& color = glm::vec4(1.0f), int font_size = 32,
               const std::string& font_path = "");

  //!@{
  //! Манипуляции камерой и запрос ее состояния.

  void FitScene();

  void ZoomCamera(float k);

  //! Позиция камеры в глобальной системе координат.
  glm::vec3 CameraPosition() const;

  //! Перемещение камеры вдоль плоскости экрана
  //! в направлении экранного вектора `(dx,dy)`.
  void TranslateCamera(int x, int y, int dx, int dy);

  void RotateCamera(int x, int y, int dx, int dy);

  void OrientCamera(const OrthoBasis<float>&);

  glm::mat4 GetViewMatrix() const;

  glm::mat4 GetProjectionMatrix() const;

  //!@}

  //!@{
  //! Методы для управления рисованием.

  void Update();
  //!@}

  //!@{
  //! Взаимодействие со сценой.

  const Scene& GetScene() const;
  Scene& GetScene();

  void ClearScene();
  //!@}

  GpuManager* GetGpuManager();

  void SetViewportSizeFunction(std::function<std::tuple<uint32_t, uint32_t>()>);

  class Inner;
  Inner* GetInnerInterface();
  const Inner* GetInnerInterface() const;

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  class State;
  State* m_pimpl;
};
#endif // !NUMGEOM_FRAMEWORK_APPLICATION_H
