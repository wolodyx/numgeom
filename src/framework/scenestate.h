#ifndef NUMGEOM_FRAMEWORK_SCENESTATE_H
#define NUMGEOM_FRAMEWORK_SCENESTATE_H

#include "numgeom/scene.h"

class Scene::State {
 public:
  std::list<SceneObject*> objects_;
  bool has_changes_ = true;
};
#endif // !NUMGEOM_FRAMEWORK_SCENESTATE_H
