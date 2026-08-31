#ifndef NUMGEOM_FRAMEWORK_COMMAND_H
#define NUMGEOM_FRAMEWORK_COMMAND_H

#include <list>
#include <string>

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/outcome.h"

class Gizmo;
class Phantom;
class Scene;
class SceneWidget;
class Workbench;

/**\brief Command
Команда -- это сущность, представляющая действие в приложении над данными.

Конкретная команда наследует базовый класс `Command` и реализует некоторое
действие. Интерфейс наследника не экспортируется, но регистрируется через макрос
`REGISTER_COMMAND`. Зарегистрированную команду приложение подхватывает
автоматически.
*/
class FRAMEWORK_EXPORT Command {
 public:
  Command(Workbench*);
  virtual ~Command();

  //! Краткое описание команды для отображения в пункте меню, заголовке окна.
  virtual std::string GetDisplayName() const = 0;

  //! Метод вызывается при выполнении команды.
  virtual Outcome Execute() = 0;

  //! Метод восстанавливает состояние команды, подготавливая ее к первому или
  //! следующему циклу выполнения. Базовая реализация метода вызывает
  //! метод `Gizmo::Reset` у всех зарегистрированных объектов `Gizmo`.
  virtual void Recover();

  //! Метод вызывается извне при изменении состояния `Gizmo`.
  virtual void Updated(Gizmo*) {}

  Scene* GetScene();
  Workbench* GetWorkbench();

  //! \name Служебные методы.
  //! \{
  void Setup();
  void Cleanup();
  void Register(Gizmo*);
  Iterator<Gizmo*> GetGizmos();
  //! \}

 protected:
  //! \name Методы, вызываемые в реализациях команд.
  //! \{
  Phantom* GetPhantom();
  void UpdateScreen();
  //! \}

 private:
  Workbench* workbench_;
  std::list<Gizmo*> gizmos_;
  Phantom* phantom_;
};
#endif // !NUMGEOM_FRAMEWORK_COMMAND_H
