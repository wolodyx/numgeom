#include "scenewindow2.h"

#include <iostream>

#include "qevent.h"
#include "qvulkaninstance.h"

#include "numgeom/application.h"
#include "numgeom/gpumanager.h"
#include "numgeom/userinputcontroller.h"


SceneWindow2::SceneWindow2(Application* app)
{
    m_app = app;
    m_userInputController = new UserInputController(app);
}


SceneWindow2::~SceneWindow2()
{
    delete m_userInputController;
}


void SceneWindow2::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyPressed(key);
    QWindow::keyPressEvent(event);
}


void SceneWindow2::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyReleased(key);
    QWindow::keyPressEvent(event);
}


void SceneWindow2::mousePressEvent(QMouseEvent* event)
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


void SceneWindow2::mouseReleaseEvent(QMouseEvent* event)
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


void SceneWindow2::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    m_userInputController->mouseMove(pt.x(), pt.y());
    event->accept();
}


void SceneWindow2::wheelEvent(QWheelEvent* event)
{
    QPoint angleDelta = event->angleDelta();
    int numDegrees = angleDelta.y() / 8;
    m_userInputController->mouseWheelRotate(numDegrees);
    event->accept();
}


void SceneWindow2::resizeEvent(QResizeEvent* event)
{
    QSize sz = event->size();
    std::cout << "Window size is (" << sz.width() << ", " << sz.height() << ")" << std::endl;
    m_app->update();
}


void SceneWindow2::exposeEvent(QExposeEvent* event)
{
    if(!this->isExposed()) {
        QWindow::exposeEvent(event);
        return;
    }
    static int i = 0;
    //std::cout << std::format("{}. expose event {}", i++, isExposed())  << std::endl;
    QWindow::exposeEvent(event);
}


bool SceneWindow2::event(QEvent* e)
{
    switch(e->type()) {
    case QEvent::Paint:
        std::cout << "Paint" << std::endl;
        break;
    case QEvent::UpdateRequest:
        std::cout << "UpdateRequest" << std::endl;
        break;
    case QEvent::PlatformSurface:
    {
        auto* pse = static_cast<QPlatformSurfaceEvent*>(e);
        std::cout << "PlatformSurface: ";
        switch(pse->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            std::cout << "SurfaceAboutToBeDestroyed" << std::endl;
            break;
        case QPlatformSurfaceEvent::SurfaceCreated:
            std::cout << "SurfaceCreated" << std::endl;
            break;
        }
        break;
    }
    case QEvent::MouseMove:
        break;
    case QEvent::Enter:
        //std::cout << "Enter event." << std::endl;
        break;
    case QEvent::Leave:
        //std::cout << "Leave event." << std::endl;
        break;
    case QEvent::Expose:
        //std::cout << "Expose event." << std::endl;
        break;
    default:
        //std::cout << "e->type() = " << e->type() << std::endl;
        break;
    }
    return QWindow::event(e);
}
