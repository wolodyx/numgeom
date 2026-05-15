#ifndef NUMGEOM_FRAMEWORK_SCENEINNER_H
#define NUMGEOM_FRAMEWORK_SCENEINNER_H

#include "numgeom/iterator.h"
#include "numgeom/scene.h"

class Logo;
class ScreenText;

/** \class Scene::Inner
\brief Дополнительный (скрытый) интерфейс класса `Scene`, необходимый для
внутреннего использования -- реализации бизнес-логики платформы.
*/
class Scene::Inner {
 public:
  Inner(State* pimpl);

  ~Inner();

  bool HasLogo() const;

  const Logo& GetLogo() const;

  bool HasScreenTexts() const;

  Iterator<const ScreenText*> GetScreenTextObjects() const;

 private:
  State* impl_;
};
#endif  // !NUMGEOM_FRAMEWORK_SCENEINNER_H
