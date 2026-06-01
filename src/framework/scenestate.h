#ifndef NUMGEOM_FRAMEWORK_SCENESTATE_H
#define NUMGEOM_FRAMEWORK_SCENESTATE_H

#include <list>
#include <map>

#include "numgeom/scene.h"

#include "camera.h"
#include "fgimage.h"

class ScreenText;

class Scene::State {
 public:
  ~State();

 public:
  std::string name_;
  std::map<const FgImage*,glm::ivec2> fg_image_positions;
  std::map<const ScreenText*,glm::ivec2> screen_text_positions;
  Camera camera_;
  std::list<SceneObject*> objects_;
  bool has_changes_ = true;
  Inner* inner_interface_;
};
#endif // !NUMGEOM_FRAMEWORK_SCENESTATE_H
