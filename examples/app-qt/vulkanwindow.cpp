#include "vulkanwindow.h"

#include "vulkanwindowrenderer.h"


VulkanWindow::VulkanWindow()
{
}


QVulkanWindowRenderer* VulkanWindow::createRenderer()
{
    m_renderer = new VulkanWindowRenderer(this);
    return m_renderer;
}


void VulkanWindow::updateGeometry(CTriMesh::Ptr mesh)
{
    if(m_renderer)
        m_renderer->setModel(mesh);
}
