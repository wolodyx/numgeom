#ifndef numgeom_app_vulkanwindowrenderer_h
#define numgeom_app_vulkanwindowrenderer_h

// Заголовочный файл vulkan следует подключать раньше, чем те же самые файлы Qt,
// чтобы воспользоваться непосредственно функциями vulkan.
#include <vulkan/vulkan.h>

#include <QVulkanWindowRenderer>

class VulkanWindowRenderer : public QVulkanWindowRenderer
{
public:
    VulkanWindowRenderer(QVulkanWindow *w);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    QVulkanWindow *m_window;
    float m_green = 0;
};

#endif // !numgeom_app_vulkanwindowrenderer_h
