#ifndef NUMGEOM_FRAMEWORK_APPLICATION_H
#define NUMGEOM_FRAMEWORK_APPLICATION_H

#include <functional>
#include <string>

#include "glm/glm.hpp"

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"

class AppContext;
class ApplicationImpl;
class Command;
class Drawable;
class FgObject;
class FgText;
class VkSceneRenderer;
class Workbench;
class WorkplaneAttributes;

/** \class Application
\brief Сущность, в которой содержится все состояние приложения.

Класс Application:
* связывает камеру с событиями от мыши;
* управляет выборкой рисуемых объектов;
* управляет модулями приложения;
* управляет моделями данных;

*/
class FRAMEWORK_EXPORT Application {
 public:
  static Application* Get();

 public:
  Application(int argc, char* argv[]);

  ~Application();

  FgObject* AddFgImage(const std::string& image_filename);
  FgObject* AddFgImage(const unsigned char* image_data, size_t image_data_size);
  FgText* AddFgText(const std::string& text);

  VkSceneRenderer* GetRenderer();
  AppContext* GetContext();

  void Sync();

  //! \{
  bool Add(Workbench*);
  bool Remove(Workbench*);
  Workbench* GetWorkbench(const std::string&) const;
  Iterator<Workbench*> GetWorkbenches() const;
  //! \}

  //! \{
  bool SetupCommand(const std::string&);
  bool ExecuteCommand();
  void StopCommand();
  Command* GetCommand() const;
  //! \}

  const WorkplaneAttributes* GetWorkplaneAttributes() const;

 private:
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

 private:
  ApplicationImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_APPLICATION_H
