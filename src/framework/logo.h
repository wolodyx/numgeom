#ifndef NUMGEOM_FRAMEWORK_LOGO_H
#define NUMGEOM_FRAMEWORK_LOGO_H

#include "glm/glm.hpp"

class Logo {
 public:
  std::vector<unsigned char> pixels;
  int width;
  int height;
  int channels;
  glm::ivec2 position;

 public:
  bool operator!() const { return pixels.empty(); }
};

#endif // NUMGEOM_FRAMEWORK_LOGO_H
