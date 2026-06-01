#ifndef NUMGEOM_FRAMEWORK_SCENEINNER_H
#define NUMGEOM_FRAMEWORK_SCENEINNER_H

#include "numgeom/iterator.h"
#include "numgeom/scene.h"

class FgImage;
class ScreenText;

/** \class Scene::Inner
\brief Дополнительный (скрытый) интерфейс класса `Scene`, необходимый для
внутреннего использования -- реализации бизнес-логики платформы.
*/
class Scene::Inner {
 public:
  Inner(State* pimpl);

  ~Inner();

  bool HasFgImages() const;
  Iterator<const FgImage*> GetFgImages() const;
  glm::ivec2 GetScreenPosition(const FgImage*) const;
  void AddFgImage(const FgImage*, const glm::ivec2&);

  bool HasScreenTexts() const;
  Iterator<const ScreenText*> GetScreenTextObjects() const;
  glm::ivec2 GetScreenPosition(const ScreenText*) const;

 private:
  State* impl_;
};
#endif  // !NUMGEOM_FRAMEWORK_SCENEINNER_H
