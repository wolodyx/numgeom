#ifndef NUMGEOM_FRAMEWORK_FGIMAGE_H
#define NUMGEOM_FRAMEWORK_FGIMAGE_H

#include <filesystem>

#include "glm/glm.hpp"

#include "numgeom/trackedobject.h"

class FgImage : public TrackedObject {
 public:
  FgImage(const std::filesystem::path& image_path);

  FgImage(const unsigned char* image_data, size_t image_data_size);

  bool operator!() const;
  bool IsEmpty() const { return this->operator!(); }

 public:
  std::vector<unsigned char> pixels;
  int width = 0;
  int height = 0;
};

#endif // !NUMGEOM_FRAMEWORK_FGIMAGE_H
