#include "vulkanwindow.h"

#include "qevent.h"

#include "vulkanwindowrenderer.h"

#include "numgeom/application.h"
#include "numgeom/userinputcontroller.h"


VulkanWindow::VulkanWindow(Application* app)
{
    m_app = app;
    m_userInputController = new UserInputController(app);
}


VulkanWindow::~VulkanWindow()
{
    delete m_userInputController;
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


void VulkanWindow::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyPressed(key);
    QVulkanWindow::keyPressEvent(event);
}


void VulkanWindow::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyReleased(key);
    QVulkanWindow::keyPressEvent(event);
}


void VulkanWindow::mousePressEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    if(event->button() == Qt::LeftButton)
        m_userInputController->mouseLeftButtonDown(pt.x(), pt.y());
    else if(event->button() == Qt::RightButton)
        m_userInputController->mouseRightButtonDown(pt.x(), pt.y());
    else if(event->button() == Qt::MiddleButton)
        m_userInputController->mouseMiddleButtonDown(pt.x(), pt.y());
    else {
        event->ignore();
        return;
    }

    event->accept();
}


void VulkanWindow::mouseReleaseEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    if(event->button() == Qt::LeftButton)
        m_userInputController->mouseLeftButtonUp(pt.x(), pt.y());
    else if(event->button() == Qt::RightButton)
        m_userInputController->mouseRightButtonUp(pt.x(), pt.y());
    else if(event->button() == Qt::MiddleButton)
        m_userInputController->mouseMiddleButtonUp(pt.x(), pt.y());
    else {
        event->ignore();
        return;
    }

    event->accept();
}


void VulkanWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    m_userInputController->mouseMove(pt.x(), pt.y());
    event->accept();
}


void VulkanWindow::wheelEvent(QWheelEvent* event)
{
    m_userInputController->mouseWheelRotate(event->delta());
    event->accept();
}
