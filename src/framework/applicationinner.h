#ifndef NUMGEOM_FRAMEWORK_APPLICATIONINNER_H
#define NUMGEOM_FRAMEWORK_APPLICATIONINNER_H

#include "numgeom/application.h"

class Logo;
class ScreenText;

/** \class Application::Inner
\brief Дополнительный (скрытый) интерфейс класса `Application`, необходимый для
внутреннего использования -- реализации бизнес-логики платформы.
*/
class Application::Inner {
 public:
  Inner(State* pimpl);

  ~Inner();

  bool HasLogo() const;

  const Logo& GetLogo() const;

  bool HasScreenText() const;

  const ScreenText& GetScreenText() const;

 private:
  State* pimpl_;
};
#endif  // !NUMGEOM_FRAMEWORK_APPLICATIONINNER_H
