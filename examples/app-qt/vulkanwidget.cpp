#include "vulkanwidget.h"

#include "qapplication.h"
#include "qevent.h"
#include "qlayout.h"
#include "qscreen.h"

#include "vulkanwindow.h"
#include "vulkanwindowrenderer.h"


VulkanWidget::VulkanWidget(QWidget* parent, Application* app)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    vulkanInstance.setExtensions({"VK_KHR_surface", "VK_KHR_xcb_surface"});
    vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if(!vulkanInstance.create())
        qFatal("Vulkan instance creating error");

    m_vulkanWindow = new VulkanWindow(app);
    m_vulkanWindow->setVulkanInstance(&vulkanInstance);
    QWidget* vulkanContainer = QWidget::createWindowContainer(m_vulkanWindow, this);
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


void VulkanWidget::updateGeometry(CTriMesh::Ptr mesh)
{
    m_vulkanWindow->updateGeometry(mesh);
}


void VulkanWidget::keyPressEvent(QKeyEvent* event)
{
    std::cout << "Key pressed (VulkanWidget)." << std::endl;
    if(event->key() == Qt::Key_Space)
        std::cout << "Key space pressed (VulkanWidget)." << std::endl;

    QWidget::keyPressEvent(event);
}
