#ifndef numgeom_app_vulkanwindow_h
#define numgeom_app_vulkanwindow_h

#include <QVulkanWindow>

#include "numgeom/trimesh.h"

class Application;
class UserInputController;
class VulkanWindowRenderer;


class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow(Application*);
    ~VulkanWindow();
    QVulkanWindowRenderer* createRenderer() override;

    void updateGeometry(CTriMesh::Ptr);

private:
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    Application* m_app;
    UserInputController* m_userInputController;
    VulkanWindowRenderer* m_renderer = nullptr;
};
#endif // !numgeom_app_vulkanwindow_h
