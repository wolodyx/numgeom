#ifndef numgeom_framework_gpumanager_h
#define numgeom_framework_gpumanager_h

#include <any>
#include <cstdint>
#include <functional>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

class Application;


/**
Менеджер GPU связывает состояние приложения (Application) с GPU и руководит
отрисовкой данных в окне графического приложения.
*/
class GpuManager
{
public:
    struct Impl;

public:

    GpuManager(Application*);

    ~GpuManager();

    //! Запрос на обновление изображения в окне приложения.
    bool update();

    //! Возвращает экземпляр vulkan:
    //! существующий или, если отсутствует, то вновь созданный.
    VkInstance instance() const;

    //! Задает внешнюю поверхность vulkan.
    void setSurface(VkSurfaceKHR);

    //! Инициализация объектов vulkan и всей подсистемы взаимодействия с GPU.
    bool initialize();

    void setImageExtentFunction(std::function<std::tuple<uint32_t,uint32_t>()>);

private:
    GpuManager(const GpuManager&) = delete;
    GpuManager& operator=(const GpuManager&) = delete;

private:
    Impl* m_pimpl;
};
#endif // !numgeom_framework_gpumanager_h
