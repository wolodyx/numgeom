#ifndef NUMGEOM_FRAMEWORK_VKSCENERENDERER_H
#define NUMGEOM_FRAMEWORK_VKSCENERENDERER_H

#include <any>
#include <cstdint>
#include <functional>

#include "volk.h"

#include "glm/glm.hpp"

class Application;

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

  //! Запрос на обновление изображения в окне приложения.
  bool update();

  //! Возвращает экземпляр vulkan:
  //! существующий или, если отсутствует, то вновь созданный.
  VkInstance instance() const;

  //! Задает внешнюю поверхность vulkan.
  void setSurface(VkSurfaceKHR);

  //! Инициализация объектов vulkan и всей подсистемы взаимодействия с GPU.
  bool initialize();

  void setImageExtentFunction(std::function<std::tuple<uint32_t, uint32_t>()>);

  void finalize();

 private:
  VkSceneRenderer(const VkSceneRenderer&) = delete;
  VkSceneRenderer& operator=(const VkSceneRenderer&) = delete;

 private:
  Impl* m_pimpl;
};
#endif // !NUMGEOM_FRAMEWORK_VKSCENERENDERER_H
