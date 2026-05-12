#ifndef NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
#define NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H

#include <list>

#include "numgeom/application.h"
#include "numgeom/scene.h"
#include "numgeom/screentext.h"

#include "camera.h"
#include "logo.h"

/** \class Application::Impl
\brief Скрытое и разделяемое состояние класса `Application`.
*/
class Application::State {
 public:
  Logo logo;
  std::list<ScreenText*> screen_text_objects_;
  Camera camera;
  Scene scene;
  GpuManager* gpuManager;
  Inner* inner_interface_;

 public:
  ~State();
};
#endif  // !NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
