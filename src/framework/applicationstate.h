#ifndef NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
#define NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H

#include "numgeom/application.h"
#include "numgeom/scene.h"

#include "camera.h"
#include "logo.h"
#include "screentext.h"

/** \class Application::Impl
\brief Скрытое и разделяемое состояние класса `Application`.
*/
class Application::State {
 public:
  Logo logo;
  ScreenText screen_text;
  Camera camera;
  Scene scene;
  GpuManager* gpuManager;
  Inner* inner_interface_;

 public:
  ~State();
};
#endif  // !NUMGEOM_FRAMEWORK_APPLICATIONSTATE_H
