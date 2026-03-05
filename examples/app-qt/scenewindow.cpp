#include "scenewindow.h"

#include <iostream>

#include "qevent.h"
#include "qvulkaninstance.h"

#include "numgeom/application.h"
#include "numgeom/gpumanager.h"
#include "numgeom/userinputcontroller.h"


SceneWindow::SceneWindow(Application* app)
{
    m_gpuManager = app->gpuManager();
    m_userInputController = new UserInputController(app);
}


SceneWindow::~SceneWindow()
{
    delete m_userInputController;
}


void SceneWindow::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyPressed(key);
    QWindow::keyPressEvent(event);
}


void SceneWindow::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();
    m_userInputController->keyReleased(key);
    QWindow::keyPressEvent(event);
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
    int numDegrees = angleDelta.y() / 8;
    m_userInputController->mouseWheelRotate(numDegrees);
    event->accept();
}


void SceneWindow::resizeEvent(QResizeEvent* event)
{
}


void SceneWindow::exposeEvent(QExposeEvent* event)
{
    if(this->isExposed()) {
        m_gpuManager->update();
    } else {
        //m_gpuManager->finalize();
    }
}


bool SceneWindow::event(QEvent* e)
{
    switch(e->type()) {
        case QEvent::Paint:
        case QEvent::UpdateRequest:
            m_gpuManager->update();
            break;
        case QEvent::PlatformSurface: {
            auto* pse = static_cast<QPlatformSurfaceEvent*>(e);
            if(pse->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                m_gpuManager->finalize();
            }
            break;
        }
    }
    return QWindow::event(e);
}
