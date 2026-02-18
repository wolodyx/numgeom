#ifndef numgeom_framework_application_h
#define numgeom_framework_application_h

#include <functional>

#include "numgeom/framework_export.h"
#include "numgeom/trimesh.h"

class QWindow;

class GpuManager;


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
    //! Манипуляции камерой и запрос ее состояния.

    void fitScene();

    void zoomCamera(float k);

    //! Перемещение камеры вдоль плоскости экрана
    //! в направлении экранного вектора `(dx,dy)`.
    void translateCamera(int x, int y, int dx, int dy);

    void rotateCamera(int x, int y, int dx, int dy);

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix() const;

    //!@}


    //!@{
    //! Методы для управления рисованием.

    void update();
    //!@}

    //!@{
    //! Взаимодействие со сценой.

    CTriMesh::Ptr geometry() const;

    void add(CTriMesh::Ptr);

    void clearScene();
    //!@}

    GpuManager* gpuManager();

    void set_aspect_function(std::function<float()>);

private:
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    struct Impl;
    Impl* m_pimpl;
};
#endif // !numgeom_framework_application_h
