#include "vulkanwidget.h"

#include "qapplication.h"
#include "qlayout.h"
#include "qscreen.h"

#include "vulkanwindow.h"
#include "vulkanwindowrenderer.h"


VulkanWidget::VulkanWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    vulkanInstance.setExtensions({"VK_KHR_surface", "VK_KHR_xcb_surface"});
    vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if(!vulkanInstance.create())
        qFatal("Vulkan instance creating error");

    auto vulkanWindow = new VulkanWindow();
    vulkanWindow->setVulkanInstance(&vulkanInstance);
    QWidget* vulkanContainer = QWidget::createWindowContainer(vulkanWindow, this);
    layout->addWidget(vulkanContainer);
}


void VulkanWidget::saveAsPng(const QString& filename)
{
    QScreen* screen = QApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(
        0,
        this->mapToGlobal(QPoint(0,0)).x(),
        this->mapToGlobal(QPoint(0,0)).y(),
        this->width(),
        this->height()
    );
    screenshot.save(filename, "PNG");
}
