#ifndef NUMGEOM_FRAMEWORK_PIXELBUFFER_H
#define NUMGEOM_FRAMEWORK_PIXELBUFFER_H

#include <cstdint>
#include <vector>

class PixelBuffer {
 public:
  PixelBuffer();

 public:
  std::vector<unsigned char> pixels;
  uint32_t width = 0;
  uint32_t height = 0;
};

#endif // !NUMGEOM_FRAMEWORK_PIXELBUFFER_H
