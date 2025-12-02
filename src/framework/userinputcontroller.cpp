#include "numgeom/userinputcontroller.h"

#include "numgeom/application.h"


struct MouseButtonState
{
    bool down = false;
    int xDown = 0, yDown = 0;
    int xPrev = 0, yPrev = 0;
    glm::vec3 clickedPoint;
    glm::vec3 prevDraggedPoint;
    glm::vec3 nextDraggedPoint;
};


struct UserInputController::Impl
{
    Application* app;
    MouseButtonState mouseLeftButtonState;
    MouseButtonState mouseMiddleButtonState;
    MouseButtonState mouseRightButtonState;
};


UserInputController::UserInputController(Application* app)
{
    m_pimpl = new Impl {
        .app = app
    };
}


UserInputController::~UserInputController()
{
    delete m_pimpl;
}


void UserInputController::mouseLeftButtonDown(int x, int y)
{
    m_pimpl->mouseLeftButtonState.down = true;
    m_pimpl->mouseLeftButtonState.xDown = x;
    m_pimpl->mouseLeftButtonState.yDown = y;
    m_pimpl->mouseLeftButtonState.xPrev = x;
    m_pimpl->mouseLeftButtonState.yPrev = y;
}


void UserInputController::mouseMiddleButtonDown(int x, int y)
{
    m_pimpl->mouseMiddleButtonState.down = true;
    m_pimpl->mouseMiddleButtonState.xDown = x;
    m_pimpl->mouseMiddleButtonState.yDown = y;
    m_pimpl->mouseMiddleButtonState.xPrev = x;
    m_pimpl->mouseMiddleButtonState.yPrev = y;
}


void UserInputController::mouseRightButtonDown(int x, int y)
{
    m_pimpl->mouseRightButtonState.down = true;
    m_pimpl->mouseRightButtonState.xDown = x;
    m_pimpl->mouseRightButtonState.yDown = y;
    m_pimpl->mouseRightButtonState.xPrev = x;
    m_pimpl->mouseRightButtonState.yPrev = y;
}


void UserInputController::mouseLeftButtonUp(int x, int y)
{
    if(!m_pimpl->mouseLeftButtonState.down)
        return;

    m_pimpl->mouseLeftButtonState.down = false;

    bool beClicked = m_pimpl->mouseLeftButtonState.xDown == x
                  && m_pimpl->mouseLeftButtonState.yDown == y;
}


void UserInputController::mouseMiddleButtonUp(int x, int y)
{
}


void UserInputController::mouseRightButtonUp(int x, int y)
{
}


void UserInputController::mouseMove(int x, int y)
{
    if(m_pimpl->mouseLeftButtonState.down)
    {
        const int dx = x - m_pimpl->mouseLeftButtonState.xPrev;
        const int dy = y - m_pimpl->mouseLeftButtonState.yPrev;

        m_pimpl->app->translateCamera(x, y, dx, dy);

        m_pimpl->mouseLeftButtonState.xPrev = x;
        m_pimpl->mouseLeftButtonState.yPrev = y;
    }
    else if(m_pimpl->mouseMiddleButtonState.down)
    {
        const int dx = x - m_pimpl->mouseMiddleButtonState.xPrev;
        const int dy = y - m_pimpl->mouseMiddleButtonState.yPrev;

        // ...

        m_pimpl->mouseMiddleButtonState.xPrev = x;
        m_pimpl->mouseMiddleButtonState.yPrev = y;
    }
    else if(m_pimpl->mouseRightButtonState.down)
    {
        const int dx = x - m_pimpl->mouseRightButtonState.xPrev;
        const int dy = y - m_pimpl->mouseRightButtonState.yPrev;

        // ...

        m_pimpl->mouseRightButtonState.xPrev = x;
        m_pimpl->mouseRightButtonState.yPrev = y;
    }
}


void UserInputController::mouseWheelRotate(int count)
{
}


void UserInputController::keyPressed(int key)
{
}


void UserInputController::keyReleased(int key)
{
}
