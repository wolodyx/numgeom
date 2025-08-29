#include "vulkanwidget.h"

#include <QLayout>

#include "vulkanwindow.h"
#include "vulkanwindowrenderer.h"


#include <iostream>
VulkanWidget::VulkanWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QVulkanInstance vulkanInstance;
    // std::cout << "Print extensions" << std::endl;
    // for(const auto& ext : vulkanInstance.supportedExtensions()) {
    //     std::cout << "extension = " << ext.name.toStdString();
    // }
    vulkanInstance.setExtensions({"VK_KHR_surface", "VK_KHR_xcb_surface"});
    vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if(!vulkanInstance.create())
        throw std::runtime_error("Vulkan instance creating error");

    // Контейнер для Vulkan
    auto vulkanWindow = new VulkanWindow();
    vulkanWindow->setVulkanInstance(&vulkanInstance);
    //auto x = QVulkanInstance::surfaceForWindow(vulkanWindow);
    QWidget *vulkanContainer = QWidget::createWindowContainer(vulkanWindow, this);
    vulkanContainer->setMinimumSize(400, 300);
    layout->addWidget(vulkanContainer);
}
