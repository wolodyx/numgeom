#include "numgeom/scenemanager.h"

#include <algorithm>
#include <list>

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
