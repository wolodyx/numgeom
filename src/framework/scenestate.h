#ifndef NUMGEOM_FRAMEWORK_SCENESTATE_H
#define NUMGEOM_FRAMEWORK_SCENESTATE_H

#include "numgeom/scene.h"

#include "camera.h"

class Scene::State {
 public:
  Camera camera_;
  std::list<SceneObject*> objects_;
  bool has_changes_ = true;
};
#endif // !NUMGEOM_FRAMEWORK_SCENESTATE_H
