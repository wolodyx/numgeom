#ifndef numgeom_framework_application_h
#define numgeom_framework_application_h

#include "numgeom/framework_export.h"


/** \class Application
\brief Сущность, в которой содержится все состояние приложения.

Класс Application:
* связывает камеру с событиями от мыши;
* управляет выборкой рисуемых объектов;
* управляет модулями приложения;
* управляет моделями данных;

*/
class FRAMEWORK_EXPORT Application
{
public:

    Application(int argc, char* argv[]);

    ~Application();

    //!@{
    //! Методы для управления камерой.
    void fitScene();
    void zoomCamera();
    void translateCamera(int x, int y, int dx, int dy);
    void rotateCamera();
    //!@}


    //!@{
    //! Методы для управления рисованием.

    void changeWindowSize(int width, int height);

    void update();
    //!@}

private:
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    struct Impl;
    Impl* m_pimpl;
};
#endif // !numgeom_framework_application_h
