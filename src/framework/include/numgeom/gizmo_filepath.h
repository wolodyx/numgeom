#ifndef NUMGEOM_FRAMEWORK_GIZMO_FILEPATH_H
#define NUMGEOM_FRAMEWORK_GIZMO_FILEPATH_H

#include <filesystem>

#include "numgeom/gizmo.h"

class FRAMEWORK_EXPORT Gizmo_FilePath : public Gizmo {
 public:
  Gizmo_FilePath(Command*, const std::string& title);
  virtual ~Gizmo_FilePath();
  void Reset() override;

  std::string GetFilter() const;
  std::filesystem::path GetCurrentDir() const;
  bool IsOpenMode() const;
  bool IsSaveMode() const { return !this->IsOpenMode(); }
  std::filesystem::path GetFilePath() const;

  void SetOpenMode();
  void SetSaveMode();
  void SetCurrentDir(const std::filesystem::path&);
  void SetFilePath(const std::filesystem::path&);

 private:
  std::string filter_ = "All files (*.*)";
  std::filesystem::path current_dir_;
  std::filesystem::path file_path_;
  bool is_open_mode_ = true;
};
#endif // !NUMGEOM_FRAMEWORK_GIZMO_FILEPATH_H
