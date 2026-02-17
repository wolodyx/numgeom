#include "scenewindow.h"

#include "qevent.h"

#include "vulkanwindowrenderer.h"

#include "numgeom/application.h"
#include "numgeom/userinputcontroller.h"


SceneWindow::SceneWindow(Application* app)
{
    m_app = app;
    m_userInputController = new UserInputController(app);

    m_vulkanInstance.setExtensions({"VK_KHR_surface", "VK_KHR_xcb_surface"});
    m_vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if(!m_vulkanInstance.create())
        qFatal("Vulkan instance creating error");
    this->setVulkanInstance(&m_vulkanInstance);
}


SceneWindow::~SceneWindow()
{
    delete m_userInputController;
}


QVulkanWindowRenderer* SceneWindow::createRenderer()
{
    m_renderer = new VulkanWindowRenderer(this);
    return m_renderer;
}


void SceneWindow::updateGeometry(CTriMesh::Ptr mesh)
{
    if(m_renderer)
        m_renderer->setModel(mesh);
}


void SceneWindow::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyPressed(key);
    QVulkanWindow::keyPressEvent(event);
}


void SceneWindow::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyReleased(key);
    QVulkanWindow::keyPressEvent(event);
}


void SceneWindow::mousePressEvent(QMouseEvent* event)
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


void SceneWindow::mouseReleaseEvent(QMouseEvent* event)
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


void SceneWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    m_userInputController->mouseMove(pt.x(), pt.y());
    event->accept();
}


void SceneWindow::wheelEvent(QWheelEvent* event)
{
    QPoint angleDelta = event->angleDelta();
    m_userInputController->mouseWheelRotate(angleDelta.y());
    event->accept();
}
