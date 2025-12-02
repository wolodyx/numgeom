#ifndef numgeom_app_vulkanwidget_h
#define numgeom_app_vulkanwidget_h

#include "QWidget"
#include "QVulkanInstance"

#include "numgeom/trimesh.h"

class Application;
class VulkanWindow;


class VulkanWidget : public QWidget
{
    Q_OBJECT
public:
    VulkanWidget(QWidget* parent, Application* app);

    void saveAsPng(const QString& filename);

    void updateGeometry(CTriMesh::Ptr);

private:
    void keyPressEvent(QKeyEvent*) override;

private:
    QVulkanInstance vulkanInstance;
    VulkanWindow* m_vulkanWindow;
};
#endif // !numgeom_app_vulkanwidget_h
