#include "screentext.h"

#include <algorithm>
#include <fstream>
#include <format>

// stb_truetype single header library
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "boost/log/trivial.hpp"

ScreenText::ScreenText() 
    : position_(0, 0), 
      color_(1.0f, 1.0f, 1.0f, 1.0f),  // white
      font_size_(32),
      atlas_width_(0),
      atlas_height_(0) {
}

ScreenText::ScreenText(const std::string& text, const glm::ivec2& position)
    : text_(text), 
      position_(position),
      color_(1.0f, 1.0f, 1.0f, 1.0f),
      font_size_(32),
      atlas_width_(0),
      atlas_height_(0) {
}

ScreenText::ScreenText(const std::string& text, const glm::ivec2& position,
                       const glm::vec4& color, const std::string& font_path,
                       int font_size)
    : text_(text),
      position_(position),
      color_(color),
      font_path_(font_path),
      font_size_(font_size),
      atlas_width_(0),
      atlas_height_(0) {
  if (!font_path_.empty()) {
    if (!LoadFont(font_path_, font_size_)) {
      BOOST_LOG_TRIVIAL(trace) << "ScreenText: Font loading failed";
    }
  } else {
    BOOST_LOG_TRIVIAL(trace) << "ScreenText: No font path provided, using default";
  }
}

ScreenText::~ScreenText() {}

bool ScreenText::operator!() const {
  return text_.empty();
}

bool ScreenText::IsEmpty() const {
  return this->operator!();
}

bool ScreenText::LoadFont(const std::string& font_path, int font_size) {
  font_path_ = font_path;
  font_size_ = font_size;
  if (!LoadFontData(font_path))
    return false;
  CreateGlyphAtlas();
  return true;
}

void ScreenText::SetText(const std::string& text) {
  if (text_ != text) {
    text_ = text;
    // For simplicity, we'll regenerate the entire atlas
    // In a more advanced implementation, we'd only add new glyphs
    if (!font_data_.empty()) {
      CreateGlyphAtlas();
    }
  }
}

void ScreenText::SetPosition(const glm::ivec2& position) {
  position_ = position;
}

void ScreenText::SetColor(const glm::vec4& color) {
  color_ = color;
}

void ScreenText::RegenerateAtlas() {
  if (!font_data_.empty()) {
    CreateGlyphAtlas();
  }
}

const ScreenText::GlyphInfo* ScreenText::GetGlyphInfo(uint32_t codepoint) const {
  auto it = glyph_cache_.find(codepoint);
  if (it != glyph_cache_.end()) {
    return &it->second;
  }
  return nullptr;
}

glm::ivec2 ScreenText::GetTextDimensions() const {
  if (text_.empty()) {
    return glm::ivec2(0, 0);
  }
  
  int width = 0;
  int max_height = 0;
  
  for (char c : text_) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    auto it = glyph_cache_.find(codepoint);
    if (it != glyph_cache_.end()) {
      const GlyphInfo& glyph = it->second;
      width += glyph.advance / 64;  // advance is in 1/64 pixels
      max_height = std::max(max_height, glyph.size.y);
    }
  }
  
  return glm::ivec2(width, max_height);
}

bool ScreenText::LoadFontData(const std::string& font_path) {
  std::ifstream file(font_path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    BOOST_LOG_TRIVIAL(trace) << std::format("ScreenText: Failed to open font file: {}", font_path);
    return false;
  }
  
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  font_data_.resize(size);
  if (!file.read(reinterpret_cast<char*>(font_data_.data()), size)) {
    BOOST_LOG_TRIVIAL(trace) << std::format("ScreenText: Failed to read font file: {}", font_path);
    font_data_.clear();
    return false;
  }
  
  return true;
}

void ScreenText::CreateGlyphAtlas() {
  if (font_data_.empty()) {
    BOOST_LOG_TRIVIAL(trace) << "ScreenText::CreateGlyphAtlas: font_data_ is empty";
    return;
  }
  
  // Initialize stb_truetype font
  stbtt_fontinfo font;
  if (!stbtt_InitFont(&font, font_data_.data(), 0)) {
    BOOST_LOG_TRIVIAL(trace) << "ScreenText::CreateGlyphAtlas: stbtt_InitFont failed";
    return;
  }
  
  // Calculate scale for desired font size
  float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(font_size_));
  
  // Clear existing cache
  glyph_cache_.clear();
  
  // For simplicity, we'll create an atlas that contains only the ASCII characters in the text
  // First pass: calculate total dimensions needed
  int total_width = 0;
  int max_height = 0;
  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
  
  std::vector<std::pair<uint32_t, glm::ivec2>> glyph_sizes;
  
  for (char c : text_) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    if (glyph_cache_.find(codepoint) != glyph_cache_.end()) {
      continue;  // Already processed
    }
    
    int glyph_index = stbtt_FindGlyphIndex(&font, codepoint);
    if (glyph_index == 0) {
      continue;  // Glyph not found
    }
    
    int advance, lsb;
    stbtt_GetGlyphHMetrics(&font, glyph_index, &advance, &lsb);
    
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&font, glyph_index, scale, scale, &x0, &y0, &x1, &y1);
    
    int width = x1 - x0;
    int height = y1 - y0;
    
    if (width > 0 && height > 0) {
      glyph_sizes.emplace_back(codepoint, glm::ivec2(width, height));
      total_width += width + 1;  // +1 for padding
      max_height = std::max(max_height, height);
    }
    
    // Store glyph metrics
    GlyphInfo info;
    info.size = glm::ivec2(width, height);
    info.bearing = glm::ivec2(x0, -y0);  // y0 is negative for most glyphs
    info.advance = static_cast<int>(advance * scale * 64.0f);
    
    glyph_cache_[codepoint] = info;
  }
  
  if (glyph_sizes.empty()) {
    atlas_width_ = 0;
    atlas_height_ = 0;
    atlas_pixels_.clear();
    return;
  }
  
  // Simple atlas layout: horizontal strip
  atlas_width_ = total_width;
  atlas_height_ = max_height;
  atlas_pixels_.resize(atlas_width_ * atlas_height_, 0);
  
  // Second pass: rasterize glyphs into atlas
  int current_x = 0;
  for (const auto& [codepoint, size] : glyph_sizes) {
    int glyph_index = stbtt_FindGlyphIndex(&font, codepoint);
    
    // Rasterize glyph
    std::vector<unsigned char> bitmap(size.x * size.y);
    stbtt_MakeGlyphBitmap(&font, bitmap.data(), size.x, size.y, size.x,
                          scale, scale, glyph_index);
    
    // Copy to atlas
    for (int y = 0; y < size.y; ++y) {
      for (int x = 0; x < size.x; ++x) {
        int atlas_idx = (y * atlas_width_) + (current_x + x);
        int bitmap_idx = (y * size.x) + x;
        atlas_pixels_[atlas_idx] = bitmap[bitmap_idx];
      }
    }
    
    // Update UV coordinates in glyph cache
    GlyphInfo& info = glyph_cache_[codepoint];
    info.uv_min = glm::vec2(static_cast<float>(current_x) / atlas_width_,
                            0.0f);
    info.uv_max = glm::vec2(static_cast<float>(current_x + size.x) / atlas_width_,
                            static_cast<float>(size.y) / atlas_height_);
    
    current_x += size.x + 1;  // Move to next position with padding
  }
}

void ScreenText::RasterizeGlyph(uint32_t codepoint, int x, int y) {
  // This is now handled in CreateGlyphAtlas
}
