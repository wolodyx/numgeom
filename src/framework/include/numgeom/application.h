#ifndef numgeom_framework_application_h
#define numgeom_framework_application_h

#include <functional>

#include "numgeom/framework_export.h"
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

  void SetAspectFunction(std::function<float()>);

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  struct Impl;
  Impl* m_pimpl;
};
#endif  // !numgeom_framework_application_h
