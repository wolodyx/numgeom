#ifndef NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
#define NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H

#include <map>

#include "numgeom/application.h"
#include "numgeom/scene.h"

/** \class Application::Impl
\brief Скрытое и разделяемое состояние класса `Application`.
*/
class Application::State {
 public:
  State();
  ~State();

 public:
  std::map<std::string,Scene*> scenes_;
  std::map<Scene*,Scene*> foreground2background_;
  Scene* active_scene_ = nullptr;
  VkSceneRenderer* renderer_ = nullptr;
  Inner* inner_interface_ = nullptr;
};
#endif  // !NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
