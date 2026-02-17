#include "numgeom/application.h"

#include <format>
#include <iostream>

#include "numgeom/gpumanager.h"
#include "numgeom/trimesh.h"

#include "camera.h"


struct Application::Impl
{
    Camera camera;

    const float fovy = glm::radians(45.0f);

    CTriMesh::Ptr scene;
    glm::vec3 worldMin, worldMax;

    GpuManager* gpuManager;

    void updateScene(CTriMesh::Ptr newScene)
    {
        if(scene == newScene)
            return;
        scene = newScene;

        worldMin = glm::vec3(+FLT_MAX, +FLT_MAX, +FLT_MAX);
        worldMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        if(!scene)
            return;
        for(size_t i = 0; i < scene->NbNodes(); ++i) {
            auto pt = scene->GetNode(i);
            worldMin.x = std::min(worldMin.x, static_cast<float>(pt.x));
            worldMin.y = std::min(worldMin.y, static_cast<float>(pt.y));
            worldMin.z = std::min(worldMin.z, static_cast<float>(pt.z));
            worldMax.x = std::max(worldMax.x, static_cast<float>(pt.x));
            worldMax.y = std::max(worldMax.y, static_cast<float>(pt.y));
            worldMax.z = std::max(worldMax.z, static_cast<float>(pt.z));
        }
        camera.fitBox(worldMin, worldMax);
    }
};


Application::Application(int argc, char* argv[])
{
    m_pimpl = new Impl();
    m_pimpl->gpuManager = new GpuManager(this);
}


Application::~Application()
{
    delete m_pimpl;
}


void Application::fitScene()
{
    m_pimpl->camera.fitBox(m_pimpl->worldMin, m_pimpl->worldMax);
    this->update();
}


void Application::zoomCamera(float k)
{
    m_pimpl->camera.zoom(k);
    this->update();
}


void Application::translateCamera(int x, int y, int dx, int dy)
{
    if(dx == 0 && dy == 0)
        return;

    glm::vec2 screenOffset(
        static_cast<float>(dx),
        static_cast<float>(dy)
    );
    m_pimpl->camera.translate(screenOffset);
    this->update();
}


void Application::rotateCamera()
{
    std::cout << "Rotate camera" << std::endl;
}


glm::mat4 Application::getViewMatrix() const
{
    auto x = m_pimpl->camera.viewMatrix();
    // std::cout << std::endl;
    // for(int i = 0; i < 4; ++i) {
    //     for(int j = 0; j < 4; ++j) {
    //         std::cout << x[i][j] << ' ';
    //     }
    //     std::cout << std::endl;
    // }
    return x;
}


glm::mat4 Application::getProjectionMatrix() const
{
    return m_pimpl->camera.projectionMatrix();
}


void Application::update()
{
    m_pimpl->gpuManager->update();
}


GpuManager* Application::gpuManager()
{
    return m_pimpl->gpuManager;
}


CTriMesh::Ptr Application::geometry() const
{
    return m_pimpl->scene;
}


void Application::add(CTriMesh::Ptr mesh)
{
    m_pimpl->updateScene(mesh);
    this->update();
}


void Application::clearScene()
{
    if(!m_pimpl->scene)
        return;
    m_pimpl->updateScene(TriMesh::Ptr());
    this->update();
}


void Application::set_aspect_function(std::function<float()> func)
{
    m_pimpl->camera.setAspectFunction(func);
}
