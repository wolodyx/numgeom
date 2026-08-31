#include "numgeom/gizmo_filepath.h"

namespace {
std::filesystem::path get_home_directory() {
#if defined(_WIN32)
  // Windows: Check 'USERPROFILE' first, fallback to 'HOMEDRIVE' + 'HOMEPATH'
  const char* userProfile = std::getenv("USERPROFILE");
  if (userProfile) {
    return std::filesystem::path(userProfile);
  }
  const char* homeDrive = std::getenv("HOMEDRIVE");
  const char* homePath = std::getenv("HOMEPATH");
  if (homeDrive && homePath) {
    return std::filesystem::path(homeDrive) / homePath;
  }
#else
  // Linux / macOS: Use 'HOME' environment variable
  const char* home = std::getenv("HOME");
  if (home) {
    return std::filesystem::path(home);
  }
#endif
  return std::filesystem::path();
}
}

Gizmo_FilePath::Gizmo_FilePath(Command* command, const std::string& title)
    : Gizmo(command, title) {
  current_dir_ = get_home_directory();
}

Gizmo_FilePath::~Gizmo_FilePath() {
}

std::filesystem::path Gizmo_FilePath::GetFilePath() const {
  return file_path_;
}

std::string Gizmo_FilePath::GetFilter() const {
  return filter_;
}

std::filesystem::path Gizmo_FilePath::GetCurrentDir() const {
  return current_dir_;
}

bool Gizmo_FilePath::IsOpenMode() const {
  return is_open_mode_;
}

void Gizmo_FilePath::SetCurrentDir(const std::filesystem::path& path) {
  current_dir_ = path;
}

void Gizmo_FilePath::SetFilePath(const std::filesystem::path& path) {
  file_path_ = path;
  this->NotifyAboutContentChanges();
}

void Gizmo_FilePath::SetOpenMode() {
  is_open_mode_ = true;
}

void Gizmo_FilePath::SetSaveMode() {
  is_open_mode_ = false;
}

void Gizmo_FilePath::Reset() {
  this->SetFilePath(std::filesystem::path());
  this->NotifyAboutContentChanges();
}
