#ifndef numgeom_app_vulkanwidget_h
#define numgeom_app_vulkanwidget_h

#include "QWidget"
#include "QVulkanInstance"


class VulkanWidget : public QWidget
{
    Q_OBJECT
public:
    VulkanWidget(QWidget *parent = nullptr);

    void saveAsPng(const QString& filename);

private:
    QVulkanInstance vulkanInstance;
};
#endif // !numgeom_app_vulkanwidget_h
