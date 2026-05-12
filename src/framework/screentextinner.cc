#include "screentextinner.h"

#include "screentextstate.h"

ScreenText::Inner::Inner(State* impl) {
  impl_ = impl;
}

const std::string& ScreenText::Inner::GetText() const {
  return impl_->text_;
}

glm::ivec2 ScreenText::Inner::GetPosition() const {
  return impl_->position_;
}

glm::vec4 ScreenText::Inner::GetColor() const {
  return impl_->color_;
}

int ScreenText::Inner::GetFontSize() const {
  return impl_->font_size_;
}

// Texture data for GPU upload
const std::vector<unsigned char>& ScreenText::Inner::GetAtlasPixels() const {
  return impl_->glyph_atlas_.pixels;
}

glm::ivec2 ScreenText::Inner::GetAtlasSize() const { return impl_->glyph_atlas_.size; }

const GlyphInfo* ScreenText::Inner::GetGlyphInfo(uint32_t codepoint) const {
  auto it = impl_->glyph_cache_.find(codepoint);
  if (it != impl_->glyph_cache_.end()) {
    return &it->second;
  }
  return nullptr;
}
