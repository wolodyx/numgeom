#ifndef NUMGEOM_FRAMEWORK_GIZMO_H
#define NUMGEOM_FRAMEWORK_GIZMO_H

#include <list>
#include <string>

#include "numgeom/framework_enums.h"
#include "numgeom/framework_export.h"
#include "numgeom/scenewidget.h"

class Command;
class GizmoObserver;

/** \class Gizmo
Гизмо -- это элемент графического и трехмерного интерфейса (виджет) ,
используемый для подготовки аргумента для команды.
*/
class FRAMEWORK_EXPORT Gizmo : public SceneWidget {
 public:
  Gizmo(Command*, const std::string& title);
  virtual ~Gizmo();

  //! Краткое описание виджета, фигурируемое в пользовательском интерфейсе.
  std::string GetTitle() const { return title_; }

  //! Метод вызывается при сбросе состояния виджета к первоначальному.
  //! Производный класс реализует данный метод для инициализации себя к
  //! первоначальному состоянию. Базовая реализация ничего не делает.
  virtual void Reset() {}

  void SetEnabled(bool enabled = true);
  bool IsEnabled() const;

  //! \name Методы управляют характеристическим размером -- размером объектов
  //!       сцены виджета.
  //! \{
  void SetCharacteristicSize(float);
  float GetCharacteristicSize() const;
  //! \}

  //! \name Методы для управления наблюдателями.
  //! \{
  void Attach(GizmoObserver*);
  void Detach(GizmoObserver*);
  //! \}

 protected:
  //! Метод вызывается производным классом для оповещения наблюдателей об
  //! изменении содержимого виджета.
  void NotifyAboutContentChanges();

  void NotifyAboutEnabling();

  virtual void UpdateSceneObject() {}

  void UpdateScene();

 private:
  Command* command_;
  std::string title_;
  bool enabled_ = true;
  std::list<GizmoObserver*> observers_;
  float characteristic_size_ = 1.0;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMO_H
