#ifndef NUMGEOM_FRAMEWORK_SCENE_H
#define NUMGEOM_FRAMEWORK_SCENE_H

#include <list>
#include <string>

#include "glm/glm.hpp"

#include "numgeom/alignedboundbox.h"
#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/orthobasis.h"
#include "numgeom/trackedobject.h"

class FgImage;
class SceneImpl;
class SceneObject;
class ScreenText;

/**
\class Scene
\brief Сцена -- набор объектов для рендеринга.

Сцена состоит из так называемых 'объектов' типа `SceneObject`.
*/
class FRAMEWORK_EXPORT Scene : public TrackedObject {
 public:
  Scene(const std::string& name);
  ~Scene();

  std::string GetName() const;

  AlignedBoundBox GetBoundBox() const;

  glm::mat4 GetViewMatrix() const;
  glm::mat4 GetProjectionMatrix() const;
  glm::uvec2 GetScreenSize() const;

  Iterator<SceneObject*> Objects() const;

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
  //!@}

  //! Очищаем сцену от всех рисуемых объектов.
  void Clear();

  //! Добавление в сцену объекта по заданному типу и аргументам конструирования.
  template<typename ObjectType, class... _Types>
  SceneObject* AddObject(_Types&&... _Args) {
    auto item = new ObjectType(this, _Args...);
    this->AddObject(item);
    return item;
  }

  void SetViewportSizeFunction(std::function<std::tuple<uint32_t, uint32_t>()>);

  bool HasFgImages() const;
  Iterator<FgImage*> GetFgImages() const;
  glm::ivec2 GetScreenPosition(const FgImage*) const;
  void AddFgImage(FgImage*, const glm::ivec2&);
  bool HasScreenTexts() const;
  Iterator<ScreenText*> GetScreenTextObjects() const;
  glm::ivec2 GetScreenPosition(const ScreenText*) const;

  void SetVulkanSurface(uint64_t);
  uint64_t GetVulkanSurface() const;

 private:
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  void AddObject(SceneObject* _Object);

 private:
  SceneImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENE_H
