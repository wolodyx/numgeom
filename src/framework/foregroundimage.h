#ifndef NUMGEOM_FRAMEWORK_FOREGROUNDIMAGE_H
#define NUMGEOM_FRAMEWORK_FOREGROUNDIMAGE_H

#include <filesystem>

#include "glm/glm.hpp"

class ForegroundImage {
 public:
  ForegroundImage();

  ForegroundImage(const std::filesystem::path& image_path,
       const glm::ivec2& screen_position = glm::ivec2(0,0));

  ForegroundImage(const unsigned char* image_data, size_t image_data_size,
       const glm::ivec2& screen_position = glm::ivec2(0,0));

  bool operator!() const;
  bool IsEmpty() const { return this->operator!(); }

 public:
  std::vector<unsigned char> pixels;
  int width = 0;
  int height = 0;
  glm::ivec2 position{0, 0};
};

#endif // NUMGEOM_FRAMEWORK_FOREGROUNDIMAGE_H
