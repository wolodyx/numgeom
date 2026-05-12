#ifndef NUMGEOM_FRAMEWORK_SCREENTEXTSTATE_H
#define NUMGEOM_FRAMEWORK_SCREENTEXTSTATE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"

#include "numgeom/screentext.h"

#include "screentextds.h"

class ScreenText::State {
 public:
  State();
  void CreateGlyphAtlas();

  std::string text_;
  glm::ivec2 position_ = glm::ivec2(0, 0);
  glm::vec4 color_ = glm::vec4(0.0, 0.0, 0.0, 1.0);
  int font_size_ = 32; //!< Font size in pixels
  std::vector<unsigned char> font_data_; //!< Content of `*.ttf` font file.
  GlyphAtlas glyph_atlas_;
  std::unordered_map<uint32_t, GlyphInfo> glyph_cache_;
  Inner* inner_interface_ = nullptr;
};
#endif // !NUMGEOM_FRAMEWORK_SCREENTEXTSTATE_H
