#include "vulkanwindow.h"

#include "vulkanwindowrenderer.h"


VulkanWindow::VulkanWindow()
{
}


QVulkanWindowRenderer* VulkanWindow::createRenderer()
{
    return new VulkanWindowRenderer(this);
}
