#ifndef NUMGEOM_FRAMEWORK_SCREENTEXTDS_H
#define NUMGEOM_FRAMEWORK_SCREENTEXTDS_H

#include "glm/glm.hpp"

//! Glyph information for rendering
struct GlyphInfo {
  glm::vec2 uv_min;      //!< UV coordinates in atlas (normalized)
  glm::vec2 uv_max;
  glm::ivec2 size;       //!< Glyph size in pixels
  glm::ivec2 bearing;    //!< Offset from baseline to top-left of glyph
  int advance;           //!< Horizontal advance to next glyph (in 1/64 pixels)
};

struct GlyphAtlas {
  glm::ivec2 size;       //!< Atlas size in pixels
  std::vector<unsigned char> pixels;
};
#endif // !NUMGEOM_FRAMEWORK_SCREENTEXTDS_H
