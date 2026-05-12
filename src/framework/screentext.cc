#include "numgeom/screentext.h"

#include <algorithm>
#include <fstream>
#include <format>

#include "screentextinner.h"
#include "screentextstate.h"

#include "boost/log/trivial.hpp"

ScreenText::ScreenText(const std::string& text, const glm::ivec2& position) {
  impl_ = new State();
  impl_->text_ = text;
  impl_->position_ = position;
  impl_->CreateGlyphAtlas();
}

ScreenText::~ScreenText() {
  delete impl_;
}

void ScreenText::SetText(const std::string& text) {
  if (impl_->text_ != text) {
    impl_->text_ = text;
    impl_->CreateGlyphAtlas();
  }
}

void ScreenText::SetPosition(const glm::ivec2& position) {
  impl_->position_ = position;
}

void ScreenText::SetColor(const glm::vec3& color) {
  impl_->color_.x = color.x;
  impl_->color_.y = color.y;
  impl_->color_.z = color.z;
}

void ScreenText::SetOpacity(float opacity) {
  impl_->color_.a = opacity;
}

bool ScreenText::LoadFontData(const std::string& font_path) {
  std::ifstream file(font_path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    BOOST_LOG_TRIVIAL(trace)
        << std::format("ScreenText: Failed to open font file: {}", font_path);
    return false;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  impl_->font_data_.resize(size);
  if (!file.read(reinterpret_cast<char*>(impl_->font_data_.data()), size)) {
    BOOST_LOG_TRIVIAL(trace)
        << std::format("ScreenText: Failed to read font file: {}", font_path);
    impl_->font_data_.clear();
    return false;
  }

  return true;
}

ScreenText::Inner* ScreenText::GetInnerInterface() {
  if(impl_->inner_interface_ == nullptr)
    impl_->inner_interface_ = new Inner(impl_);
  return impl_->inner_interface_;
}

const ScreenText::Inner* ScreenText::GetInnerInterface() const {
  if(impl_->inner_interface_ == nullptr)
    impl_->inner_interface_ = new Inner(impl_);
  return impl_->inner_interface_;
}
