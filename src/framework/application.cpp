#include "numgeom/application.h"

#include <format>
#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "numgeom/gpumanager.h"
#include "numgeom/trimesh.h"


namespace
{;
class Camera
{
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_up;
public:

    Camera()
    {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
        m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    Camera(
        const glm::vec3& pos,
        const glm::vec3& dir,
        const glm::vec3& up)
        : m_position(pos), m_direction(dir), m_up(up)
    {
    }

    glm::mat4 viewMatrix() const
    {
        return glm::lookAt(
            m_position,
            m_position + m_direction,
            m_up
        );
    }

    void translate(const glm::vec3& v)
    {
        m_position += v;
    }
};


/**
\brief Формируем проективную матрицу.
`fovy = glm::radians(45.0f)`.
*/
glm::mat4 computeProjectionMatrix(float fovy, int width, int height)
{
    // Adjust for Vulkan-OpenGL clip space differences.
    glm::mat4 m {
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 0.5f, 0.5f,
        0.0f,  0.0f, 0.0f, 1.0f
    };
    glm::mat4 p = glm::perspective(
        fovy,
        width / static_cast<float>(height),
        0.01f,
        100.0f
    );
    return m * p;
}
}


struct Application::Impl
{
    Camera camera;

    const float fovy = glm::radians(45.0f);

    int windowWidth = 0;
    int windowHeight = 0;

    TriMesh::Ptr scene;
    GpuManager* gpuManager = nullptr;
};


Application::Application(int argc, char* argv[])
{
    m_pimpl = new Impl();
}


Application::~Application()
{
    delete m_pimpl;
}


void Application::fitScene()
{
    std::cout << "Fit scene" << std::endl;
}


void Application::zoomCamera()
{
    std::cout << "Zoom camera" << std::endl;
}


void Application::translateCamera(int x, int y, int dx, int dy)
{
    if(dx == 0 && dy == 0)
        return;

    glm::vec3 trans(
        -static_cast<float>(dx),
        +static_cast<float>(dy),
        0.0f
    );

    float fov = glm::radians(45.0);
    //float k = 2.0f * std::tan(0.5f*fov)
    //    * std::fabs();
    //trans *= k;

    std::cout <<
        std::format(
            "Translate camera: point({},{}), delta({},{})",
            x, y, dx, dy
        ) << std::endl;
}


void Application::rotateCamera()
{
    std::cout << "Rotate camera" << std::endl;
}


void Application::changeWindowSize(int width, int height)
{
    if(m_pimpl->windowWidth == width && m_pimpl->windowHeight == height)
        return;

    m_pimpl->windowWidth = width;
    m_pimpl->windowHeight = height;
}


void Application::update()
{
}


void Application::connectWithWindow(QWindow* window)
{
    if(m_pimpl->gpuManager)
        return;
    m_pimpl->gpuManager = new GpuManager(this, window);
}
