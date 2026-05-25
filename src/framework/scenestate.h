#ifndef NUMGEOM_FRAMEWORK_SCENESTATE_H
#define NUMGEOM_FRAMEWORK_SCENESTATE_H

#include <list>

#include "numgeom/scene.h"

#include "camera.h"
#include "foregroundimage.h"

class ScreenText;

class Scene::State {
 public:
  ~State();

 public:
  std::string name_;
  ForegroundImage fg_image_;
  std::list<ScreenText*> screen_text_objects_;
  Camera camera_;
  std::list<SceneObject*> objects_;
  bool has_changes_ = true;
  Inner* inner_interface_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENESTATE_H
