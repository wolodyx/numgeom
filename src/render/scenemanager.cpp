#include "numgeom/scenemanager.h"

#include <algorithm>
#include <list>

#include <GL/glew.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/ext/scalar_uint_sized.hpp>

#include "numgeom/sceneentity.h"


class SceneManager::Internal
{
public:
    std::list<SceneEntity*> entities;
};


SceneManager::SceneManager()
{
    myPimpl = new Internal;
}


SceneManager::~SceneManager()
{
    for(auto e : myPimpl->entities)
        delete e;
    delete myPimpl;
}


void SceneManager::update()
{
}


void SceneManager::draw()
{
    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    struct Vertex {
        glm::vec3 position;
        glm::vec<4, glm::uint8, glm::defaultp> color;
    };

    Vertex data[] = {
            {{-0.5, -0.5, 0.0},{255, 0, 0, 255}},
            {{0.5, -0.5, 0.0},{0, 255, 0, 255}},
            {{0.5, 0.5, 0.0},{0, 0, 255, 255}},
            {{-0.5, 0.5, 0.0},{255, 255, 0, 255}}
    };

    glBufferData(GL_ARRAY_BUFFER, 4*sizeof(Vertex), data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    GLuint element_buffer;
    glGenBuffers(1, &element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);

    uint16_t elements[] = {
            0, 1, 2,
            2, 3, 0
    };

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(uint16_t), elements, GL_STATIC_DRAW);

    glBindVertexArray(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}


bool SceneManager::add(SceneEntity* e)
{
    if(!e)
        return false;

    auto it = std::find(myPimpl->entities.begin(), myPimpl->entities.end(), e);
    if(it == myPimpl->entities.end())
        myPimpl->entities.push_back(e);
    return true;
}


bool SceneManager::remove(SceneEntity* e)
{
    if(!e)
        return false;

    auto it = std::find(myPimpl->entities.begin(), myPimpl->entities.end(), e);
    if(it == myPimpl->entities.end())
        return false;

    delete e;
    return true;
}
