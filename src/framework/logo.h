#ifndef NUMGEOM_FRAMEWORK_LOGO_H
#define NUMGEOM_FRAMEWORK_LOGO_H

#include <filesystem>

#include "glm/glm.hpp"

class Logo {
 public:
  Logo();

  Logo(const std::filesystem::path& image_path,
       const glm::ivec2& screen_position = glm::ivec2(0,0));

  Logo(const unsigned char* image_data, size_t image_data_size,
       const glm::ivec2& screen_position = glm::ivec2(0,0));

  bool operator!() const;
  bool IsEmpty() const { return this->operator!(); }

 public:
  std::vector<unsigned char> pixels;
  int width;
  int height;
  glm::ivec2 position;
};

#endif // NUMGEOM_FRAMEWORK_LOGO_H
