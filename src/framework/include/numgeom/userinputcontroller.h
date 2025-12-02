#ifndef numgeom_framework_userinputcontroller_h
#define numgeom_framework_userinputcontroller_h

#include "glm/glm.hpp"

class Application;


class UserInputController
{
public:

    UserInputController(Application* app);

    ~UserInputController();

    void keyPressed(int key);

    void keyReleased(int key);

    void mouseLeftButtonDown(int x, int y);

    void mouseLeftButtonUp(int x, int y);

    void mouseMiddleButtonDown(int x, int y);

    void mouseMiddleButtonUp(int x, int y);

    void mouseRightButtonDown(int x, int y);

    void mouseRightButtonUp(int x, int y);

    void mouseMove(int x, int y);

    void mouseWheelRotate(int count);

private:
    UserInputController(const UserInputController&) = delete;
    UserInputController& operator=(const UserInputController&) = delete;

private:
    struct Impl;
    Impl* m_pimpl;
};

#endif // !numgeom_framework_userinputcontroller_h
