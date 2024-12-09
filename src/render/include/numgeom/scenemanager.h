#ifndef numgeom_render_scenemanager_h
#define numgeom_render_scenemanager_h

#include "numgeom/render_export.h"

class SceneEntity;


class RENDER_EXPORT SceneManager
{
public:

    SceneManager();
    ~SceneManager();

    void update();

    void draw();

    bool add(SceneEntity*);

    bool remove(SceneEntity*);

private:
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

private:
    class Internal;
    Internal* myPimpl;
};
#endif // !numgeom_render_scenemanager_h
