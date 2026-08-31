#ifndef NUMGEOM_FRAMEWORK_WORKBENCH_H
#define NUMGEOM_FRAMEWORK_WORKBENCH_H

#include <filesystem>
#include <functional>
#include <map>
#include <string>

#include "glm/glm.hpp"

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"
#include "numgeom/outcome.h"

class Application;
class Command;
class Drawable;
class Scene;
class UserInputController;
class WorkbenchImpl;

class FRAMEWORK_EXPORT Workbench {
 public:
  static void Register(const std::string& workbench_type_name,
                       std::function<Workbench*()> create_function);

  //! Возвращает список идентификаторов зарегистрированных модулей.
  static Iterator<std::string> GetWorkbenchTypes();

  static Workbench* Create(const std::string& type_id);

  //! Принудительно загружает разделяемую библиотеку модуля в процесс.
  static bool LoadModule(const std::filesystem::path& library_path);

  static bool RegisterCommand(const std::string& type_id,
                              const std::string& command_id,
                              std::function<Command*(Workbench*)>);

  static Iterator<std::string> GetCommandIds(const std::string& workbench_type_id);

 public:
  Workbench();

  virtual ~Workbench();

  virtual std::string GetName() const = 0;

  virtual void Setup() = 0;

  virtual void Cleanup() = 0;

  Command* CreateCommand(const std::string&);

  Drawable* Pick(int x, int y, glm::vec3* picked_point = nullptr) const;

  Scene* GetScene();

  const Scene* GetScene() const;

  void UpdateScreen(bool force = false);

  void Remove();

  UserInputController* GetUserInputController();

  virtual void Clear();
  virtual void LoadWelcomeModel();
  virtual Outcome LoadFromFile(
      const std::filesystem::path& filepath,
      const glm::mat4x4& transform = glm::mat4(1.0f)) = 0;

 private:
  Workbench(const Workbench&) = delete;
  Workbench& operator=(const Workbench&) = delete;

 private:
  WorkbenchImpl* impl_;
};

#define REGISTER_WORKBENCH(Name, Type) \
class RegisterWorkbench_##Type { \
 public: \
  RegisterWorkbench_##Type() { \
    Workbench::Register(Name, [](){ return new Type(); }); \
  } \
}; \
static RegisterWorkbench_##Type g_RegisterWorkbech_##Type;

#define REGISTER_COMMAND(WorkbenchType, Cmd, CmdId) \
namespace { \
  class Register_##WorkbenchType_##Cmd { \
   public: \
    Register_##WorkbenchType_##Cmd() { \
      auto create_function = [](Workbench* wb) { return new Cmd(wb); }; \
      bool ret = Workbench::RegisterCommand(#WorkbenchType, CmdId, create_function); \
      if (!ret) std::abort(); \
    } \
  }; \
  Register_##WorkbenchType_##Cmd register_##WorkbenchType_##Cmd; \
}

#endif // !NUMGEOM_FRAMEWORK_WORKBENCH_H
