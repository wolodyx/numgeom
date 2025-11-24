#ifndef numgeom_app_vulkanwidget_h
#define numgeom_app_vulkanwidget_h

#include "QWidget"
#include "QVulkanInstance"

#include "numgeom/trimesh.h"

class VulkanWindow;


class VulkanWidget : public QWidget
{
    Q_OBJECT
public:
    VulkanWidget(QWidget *parent = nullptr);

    void saveAsPng(const QString& filename);

    void updateGeometry(CTriMesh::Ptr);

private:
    QVulkanInstance vulkanInstance;
    VulkanWindow* m_vulkanWindow;
};
#endif // !numgeom_app_vulkanwidget_h
