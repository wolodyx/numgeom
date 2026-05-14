#ifndef NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
#define NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H

#include <list>

#include "numgeom/application.h"
#include "numgeom/scene.h"
#include "numgeom/screentext.h"

#include "logo.h"

/** \class Application::Impl
\brief Скрытое и разделяемое состояние класса `Application`.
*/
class Application::State {
 public:
  State();
  ~State();

 public:
  Logo logo;
  std::list<ScreenText*> screen_text_objects_;
  std::list<Scene*> scenes_;
  Scene* active_scene_ = nullptr;
  VkSceneRenderer* renderer = nullptr;
  Inner* inner_interface_ = nullptr;
};
#endif  // !NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
