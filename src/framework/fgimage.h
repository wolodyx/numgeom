#ifndef NUMGEOM_FRAMEWORK_FGIMAGE_H
#define NUMGEOM_FRAMEWORK_FGIMAGE_H

#include <filesystem>

#include "glm/glm.hpp"

#include "numgeom/fgobject.h"

class FgImage : public FgObject {
 public:
  static FgImage* Make(const std::filesystem::path& image_path);
  static FgImage* Make(const unsigned char* image_data, size_t image_data_size);

 public:
  virtual ~FgImage() {}
  const PixelBuffer& GetPixelBuffer() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;

private:
  FgImage(PixelBuffer&&);

 private:
  PixelBuffer pixel_buffer_;
};
#endif // !NUMGEOM_FRAMEWORK_FGIMAGE_H
