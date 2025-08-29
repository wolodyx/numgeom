#ifndef numgeom_app_vulkanwindow_h
#define numgeom_app_vulkanwindow_h

#include <QVulkanWindow>


class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow();
    QVulkanWindowRenderer* createRenderer() override;
};
#endif // !numgeom_app_vulkanwindow_h
