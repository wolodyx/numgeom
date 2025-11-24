#ifndef numgeom_app_vulkanwindow_h
#define numgeom_app_vulkanwindow_h

#include <QVulkanWindow>

#include "numgeom/trimesh.h"

class VulkanWindowRenderer;


class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow();
    QVulkanWindowRenderer* createRenderer() override;

    void updateGeometry(CTriMesh::Ptr);

private:
    VulkanWindowRenderer* m_renderer = nullptr;
};
#endif // !numgeom_app_vulkanwindow_h
