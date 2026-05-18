#ifndef NUMGEOM_FRAMEWORK_VKSCENERENDERER_H
#define NUMGEOM_FRAMEWORK_VKSCENERENDERER_H

#include <any>
#include <cstdint>
#include <functional>

#include "volk.h"

#include "glm/glm.hpp"

class Application;
class Scene;

/**
Класс связывает состояние приложения (Application) с GPU и руководит
отрисовкой данных в окне графического приложения.
*/
class VkSceneRenderer {
 public:
  struct Impl;

 public:
  VkSceneRenderer(Application*);

  ~VkSceneRenderer();

  //! Запрос на обновление связанного со сценой изображения.
  //! Если сцена не передана, то обновляется текущая сцена приложения.
  bool Update(Scene* = nullptr);

  //! Возвращает экземпляр vulkan:
  //! существующий или, если отсутствует, то вновь созданный.
  VkInstance GetInstance() const;

  //! Задает внешнюю поверхность vulkan.
  void SetSurface(VkSurfaceKHR);

  //! Инициализация объектов vulkan и всей подсистемы взаимодействия с GPU.
  bool Initialize();

  void Finalize();

 private:
  VkSceneRenderer(const VkSceneRenderer&) = delete;
  VkSceneRenderer& operator=(const VkSceneRenderer&) = delete;

 private:
  Impl* m_pimpl;
};
#endif // !NUMGEOM_FRAMEWORK_VKSCENERENDERER_H
