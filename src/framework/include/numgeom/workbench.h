#ifndef NUMGEOM_FRAMEWORK_WORKBENCH_H
#define NUMGEOM_FRAMEWORK_WORKBENCH_H

#include <filesystem>
#include <functional>
#include <map>
#include <string>

#include "glm/glm.hpp"

#include "numgeom/framework_export.h"
#include "numgeom/iterator.h"

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

  static Iterator<std::string> GetWorkbenchTypes();

  static Workbench* Create(const std::string& type_name);

  static bool RegisterCommand(const std::string& type_id,
                              const std::string& command_id,
                              std::function<Command*(Workbench*)>);

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
  virtual bool LoadFromFile(const std::filesystem::path& filepath);

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

#endif // !NUMGEOM_FRAMEWORK_WORKBENCH_H
